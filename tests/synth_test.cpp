#include <gtest/gtest.h>
#include "../src/context/playbackfeed.h"
#include "../src/context/vowelpresets.h"
#include <string>
#include <vector>

using namespace Main;
using Module::Audio::Queue;

namespace {

struct Counter {
    int blocks = 0;
    int samples = 0;
};

// The block that a sound maker writes is a constant, so a pulled sample shows which maker ran.
Queue makeQueue(double value)
{
    return Queue(20, 16000, [value](double *block, int length, void *userdata) {
        auto *counter = static_cast<Counter *>(userdata);
        ++counter->blocks;
        counter->samples += length;
        for (int i = 0; i < length; ++i) block[i] = value;
    });
}

std::vector<float> pullAll(Queue &queue, int length)
{
    std::vector<float> out((size_t) length, -1.0f);
    queue.pull(out.data(), length);
    return out;
}

}

// The stored sound wins. The synthesizer plays only when it is on.
TEST(Synth, TP_SYN_21_TheSourceFollowsTheSwitchOfTheSynthesizer)
{
    EXPECT_EQ(choosePlaybackSource(false, false), PlaybackSource::Silence);
    EXPECT_EQ(choosePlaybackSource(false, true), PlaybackSource::Synthesizer);
    EXPECT_EQ(choosePlaybackSource(true, false), PlaybackSource::Session);
    EXPECT_EQ(choosePlaybackSource(true, true), PlaybackSource::Session);
}

// With the synthesizer off, the playback stream gets silence only, and the synthesizer makes no sample.
TEST(Synth, TP_SYN_21_ASwitchedOffSynthesizerSendsNoSamplesToThePlaybackStream)
{
    Queue queue = makeQueue(0.5);
    queue.setOutSampleRate(48000);
    Counter synth, session;

    for (int i = 0; i < 20; ++i) {
        feedPlaybackQueue(queue, choosePlaybackSource(false, false), &synth, &session);
        const std::vector<float> out = pullAll(queue, 480);
        for (float x : out) EXPECT_EQ(x, 0.0f);
    }
    EXPECT_EQ(synth.blocks, 0);
    EXPECT_EQ(synth.samples, 0);
    EXPECT_EQ(session.blocks, 0);

    // Switch the synthesizer on. Now the synthesizer makes blocks, and the stream gets its sound.
    bool heard = false;
    for (int i = 0; i < 10; ++i) {
        feedPlaybackQueue(queue, choosePlaybackSource(false, true), &synth, &session);
        for (float x : pullAll(queue, 480)) heard = heard || x != 0.0f;
    }
    EXPECT_GT(synth.blocks, 0);
    EXPECT_EQ(session.blocks, 0);
    EXPECT_TRUE(heard);

    // Switch it off again. The synthesizer makes no more blocks.
    const int blocksWhileOn = synth.blocks;
    for (int i = 0; i < 20; ++i) feedPlaybackQueue(queue, PlaybackSource::Silence, &synth, &session);
    EXPECT_EQ(synth.blocks, blocksWhileOn);
}

// A session plays through the session maker, and the synthesizer stays quiet then.
TEST(Synth, TP_SYN_21_ASessionPlaysThroughTheSessionMakerOnly)
{
    Queue queue = makeQueue(0.25);
    queue.setOutSampleRate(48000);
    Counter synth, session;
    for (int i = 0; i < 5; ++i) {
        feedPlaybackQueue(queue, choosePlaybackSource(true, true), &synth, &session);
        pullAll(queue, 480);
    }
    EXPECT_GT(session.blocks, 0);
    EXPECT_EQ(synth.blocks, 0);
}

// The vowel table is the table of Peterson and Barney (1952). The numbers below are from the paper.
TEST(Synth, TP_SYN_22_ThePresetTableIsTheTableOfPetersonAndBarney)
{
    struct Row {
        const char *name;
        int male[3];
        int female[3];
    };
    const Row paper[] = {
        { "ee, as in see",     { 270, 2290, 3010 }, { 310, 2790, 3310 } },
        { "ih, as in sit",     { 390, 1990, 2550 }, { 430, 2480, 3070 } },
        { "eh, as in bed",     { 530, 1840, 2480 }, { 610, 2330, 2990 } },
        { "ae, as in cat",     { 660, 1720, 2410 }, { 860, 2050, 2850 } },
        { "ah, as in father",  { 730, 1090, 2440 }, { 850, 1220, 2810 } },
        { "aw, as in thought", { 570, 840, 2410 },  { 590, 920, 2710 } },
        { "uh, as in book",    { 440, 1020, 2240 }, { 470, 1160, 2680 } },
        { "oo, as in food",    { 300, 870, 2240 },  { 370, 950, 2670 } },
        { "u, as in cup",      { 640, 1190, 2390 }, { 760, 1400, 2780 } },
        { "er, as in bird",    { 490, 1350, 1690 }, { 500, 1640, 1960 } },
    };
    ASSERT_EQ(vowelPresets.size(), sizeof(paper) / sizeof(paper[0]));
    for (size_t i = 0; i < vowelPresets.size(); ++i) {
        EXPECT_STREQ(vowelPresets[i].name, paper[i].name);
        for (size_t k = 0; k < 3; ++k) {
            EXPECT_EQ(vowelPresets[i].male[k], paper[i].male[k]) << paper[i].name << " F" << k + 1;
            EXPECT_EQ(vowelPresets[i].female[k], paper[i].female[k]) << paper[i].name << " F" << k + 1;
        }
        // The formants of a voice rise with their number.
        EXPECT_LT(vowelPresets[i].male[0], vowelPresets[i].male[1]);
        EXPECT_LT(vowelPresets[i].male[1], vowelPresets[i].male[2]);
        EXPECT_LT(vowelPresets[i].female[0], vowelPresets[i].female[1]);
        EXPECT_LT(vowelPresets[i].female[1], vowelPresets[i].female[2]);
    }

    // The voices: F4 is 3500 Hz for the lower voice and 4000 Hz for the higher voice.
    ASSERT_EQ(voicePresets.size(), 2u);
    EXPECT_STREQ(voicePresets[0].key, "male");
    EXPECT_EQ(voicePresets[0].f4, 3500);
    EXPECT_STREQ(voicePresets[1].key, "female");
    EXPECT_EQ(voicePresets[1].f4, 4000);
    EXPECT_LT(voicePresets[0].pitch, voicePresets[1].pitch);

    // F4 is above F3 of each vowel, and the bandwidths widen with the formant number.
    for (const VowelPreset &v : vowelPresets) {
        EXPECT_GT(voicePresets[0].f4, v.male[2]);
        EXPECT_GT(voicePresets[1].f4, v.female[2]);
    }
    for (size_t i = 1; i < presetBandwidths.size(); ++i) EXPECT_GT(presetBandwidths[i], presetBandwidths[i - 1]);
}

// The loops wait long when they have no sound to make. This saves 40 wake-ups in a second.
TEST(Synth, TP_SYN_31_TheLoopsWaitLongWhenTheyHaveNoWork)
{
    EXPECT_EQ(feedWait(PlaybackSource::Silence), feedIdleWait);
    EXPECT_EQ(feedWait(PlaybackSource::Synthesizer), feedBusyWait);
    EXPECT_EQ(feedWait(PlaybackSource::Session), feedBusyWait);
    EXPECT_GT(feedIdleWait, feedBusyWait);

    // The loop of the plot data has work when the synthesizer is on, a follow mode is on, or a plot is watched.
    EXPECT_FALSE(synthLoopHasWork(false, false, false));
    EXPECT_TRUE(synthLoopHasWork(true, false, false));
    EXPECT_TRUE(synthLoopHasWork(false, true, false));
    EXPECT_TRUE(synthLoopHasWork(false, false, true));
}

// An idle queue gets no feed on purpose. Its empty state is no underrun, so the blocks stay as long as they were.
TEST(Synth, TP_SYN_32_AnIdleQueueKeepsItsBlockLength)
{
    Queue queue = makeQueue(0.5);
    queue.setOutSampleRate(48000);
    const float start = queue.blockDuration();

    queue.setIdle(true);
    for (int i = 0; i < 200; ++i) pullAll(queue, 480);
    EXPECT_FLOAT_EQ(queue.blockDuration(), start);

    // The pull of an empty queue gives silence, also while it is idle.
    for (float x : pullAll(queue, 480)) EXPECT_EQ(x, 0.0f);

    // A queue that is not idle grows its blocks after an underrun.
    queue.setIdle(false);
    pullAll(queue, 480);
    EXPECT_GT(queue.blockDuration(), start);
}
