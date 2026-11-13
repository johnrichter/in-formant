#include <gtest/gtest.h>
#include "../src/context/audiocontext.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Main;
using namespace Module;

namespace {

// A sound backend that has no device. It writes each call to a log, and it reports a call that comes at the wrong time.
class FakeBackend : public Audio::AbstractBase {
public:
    struct State {
        std::vector<std::string> log;
        bool initialized = false;
        bool captureOpen = false, captureRunning = false;
        bool playbackOpen = false, playbackRunning = false;
        bool failNextInitialize = false;
        int errors = 0;
        std::string firstError;
    };

    explicit FakeBackend(State &state) : s(state) {}

    void initialize() override
    {
        s.log.push_back("initialize");
        if (s.failNextInitialize) {
            s.failNextInitialize = false;
            throw std::runtime_error("no device");
        }
        s.initialized = true;
    }
    void terminate() override
    {
        s.log.push_back("terminate");
        // The system must not end while a stream is open or runs.
        if (s.captureOpen || s.captureRunning || s.playbackOpen || s.playbackRunning) fail("terminate with a stream open");
        s.initialized = false;
    }
    void refreshDevices() override { call("refresh"); }

    const rpm::vector<Audio::Device> &getCaptureDevices() const override { return captureDevices; }
    const rpm::vector<Audio::Device> &getPlaybackDevices() const override { return playbackDevices; }
    const Audio::Device &getDefaultCaptureDevice() const override { return defaultDevice; }
    const Audio::Device &getDefaultPlaybackDevice() const override { return defaultDevice; }

    void openCaptureStream(const Audio::Device *) override { call("openCapture"); s.captureOpen = true; }
    void startCaptureStream() override
    {
        call("startCapture");
        if (!s.captureOpen) fail("start of a capture stream that is not open");
        s.captureRunning = true;
    }
    void stopCaptureStream() override { call("stopCapture"); s.captureRunning = false; }
    void closeCaptureStream() override
    {
        call("closeCapture");
        if (s.captureRunning) fail("close of a capture stream that runs");
        s.captureOpen = false;
    }

    void openPlaybackStream(const Audio::Device *) override { call("openPlayback"); s.playbackOpen = true; }
    void startPlaybackStream() override
    {
        call("startPlayback");
        if (!s.playbackOpen) fail("start of a playback stream that is not open");
        s.playbackRunning = true;
    }
    void stopPlaybackStream() override { call("stopPlayback"); s.playbackRunning = false; }
    void closePlaybackStream() override
    {
        call("closePlayback");
        if (s.playbackRunning) fail("close of a playback stream that runs");
        s.playbackOpen = false;
    }

    bool needsTicking() override { return false; }

private:
    void call(const char *name)
    {
        s.log.push_back(name);
        if (!s.initialized) fail(std::string(name) + " while the audio system is not running");
    }
    void fail(const std::string &what)
    {
        if (s.errors++ == 0) s.firstError = what;
    }

    State &s;
    rpm::vector<Audio::Device> captureDevices, playbackDevices;
    Audio::Device defaultDevice;
};

size_t indexOf(const std::vector<std::string> &log, const std::string &name)
{
    for (size_t i = 0; i < log.size(); ++i) if (log[i] == name) return i;
    return log.size();
}

}

// A restart stops and closes each stream before the system ends, and it opens them again after the system starts.
TEST(AudioContext, TP_CAP_15_TheStreamsStopBeforeTheAudioSystemRestartsAndStartAfter)
{
    FakeBackend::State state;
    AudioContext context(std::make_unique<FakeBackend>(state), nullptr, nullptr);
    context.openCaptureStream(nullptr);
    context.startCaptureStream();
    context.openPlaybackStream(nullptr);
    context.startPlaybackStream();
    state.log.clear();

    context.reinitialize();

    const size_t terminate = indexOf(state.log, "terminate");
    const size_t initialize = indexOf(state.log, "initialize");
    ASSERT_LT(terminate, state.log.size());
    ASSERT_LT(initialize, state.log.size());
    EXPECT_LT(terminate, initialize);
    // Each stream stops, and then it closes, and both come before the end of the system.
    for (const char *name : { "stopPlayback", "closePlayback", "stopCapture", "closeCapture" }) {
        EXPECT_LT(indexOf(state.log, name), terminate) << name;
    }
    EXPECT_LT(indexOf(state.log, "stopPlayback"), indexOf(state.log, "closePlayback"));
    EXPECT_LT(indexOf(state.log, "stopCapture"), indexOf(state.log, "closeCapture"));
    // After the restart, no stream is open until the caller opens it again.
    EXPECT_FALSE(state.captureOpen);
    EXPECT_FALSE(state.playbackOpen);
    EXPECT_TRUE(context.ready());

    // The caller opens and starts the streams again, and the backend accepts each call.
    context.openCaptureStream(nullptr);
    context.openPlaybackStream(nullptr);
    context.startPlaybackStream();
    context.startCaptureStream();
    EXPECT_TRUE(state.captureRunning);
    EXPECT_TRUE(state.playbackRunning);
    EXPECT_EQ(state.errors, 0) << state.firstError;
}

// Many restarts in a row never touch a closed stream, and the destructor ends the system once.
TEST(AudioContext, TP_CAP_15_ManyRestartsNeverWriteToAClosedStream)
{
    FakeBackend::State state;
    {
        AudioContext context(std::make_unique<FakeBackend>(state), nullptr, nullptr);
        for (int i = 0; i < 25; ++i) {
            context.openCaptureStream(nullptr);
            if (i % 2 == 0) context.startCaptureStream();
            context.openPlaybackStream(nullptr);
            context.startPlaybackStream();
            context.reinitialize();
        }
        context.openPlaybackStream(nullptr);
        context.startPlaybackStream();
    }
    EXPECT_EQ(state.errors, 0) << state.firstError;
    EXPECT_FALSE(state.initialized);
    EXPECT_FALSE(state.playbackOpen);
    int terminates = 0, initializes = 0;
    for (const std::string &name : state.log) {
        terminates += name == "terminate";
        initializes += name == "initialize";
    }
    EXPECT_EQ(initializes, terminates) << "each start of the system has one end";
}

// A restart that fails leaves the context not ready. A call to a stream then throws, and the backend sees no call.
TEST(AudioContext, TP_CAP_15_AFailedRestartLeavesNoStreamCallToTheBackend)
{
    FakeBackend::State state;
    AudioContext context(std::make_unique<FakeBackend>(state), nullptr, nullptr);
    context.openCaptureStream(nullptr);
    context.startCaptureStream();

    state.failNextInitialize = true;
    EXPECT_THROW(context.reinitialize(), std::runtime_error);
    EXPECT_FALSE(context.ready());
    state.log.clear();

    EXPECT_THROW(context.openCaptureStream(nullptr), std::runtime_error);
    EXPECT_THROW(context.startCaptureStream(), std::runtime_error);
    EXPECT_THROW(context.openPlaybackStream(nullptr), std::runtime_error);
    EXPECT_THROW(context.startPlaybackStream(), std::runtime_error);
    EXPECT_THROW(context.refreshDevices(), std::runtime_error);
    EXPECT_TRUE(state.log.empty());
    EXPECT_EQ(state.errors, 0) << state.firstError;

    // A later restart works again.
    context.reinitialize();
    EXPECT_TRUE(context.ready());
    context.openCaptureStream(nullptr);
    context.startCaptureStream();
    EXPECT_TRUE(state.captureRunning);
    EXPECT_EQ(state.errors, 0) << state.firstError;
}
