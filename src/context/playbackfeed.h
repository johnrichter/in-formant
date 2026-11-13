#ifndef MAIN_PLAYBACK_FEED_H
#define MAIN_PLAYBACK_FEED_H

#include "../modules/audio/queue/queue.h"

#include <chrono>

namespace Main {

    // The sound that goes to the playback stream.
    enum class PlaybackSource {
        Silence,
        Synthesizer,
        Session,
    };

    // The stored sound of a session wins. Otherwise the synthesizer plays only while it is on.
    inline PlaybackSource choosePlaybackSource(bool sessionActive, bool synthesizerOn)
    {
        if (sessionActive) return PlaybackSource::Session;
        return synthesizerOn ? PlaybackSource::Synthesizer : PlaybackSource::Silence;
    }

    // The time that a loop waits between two steps. A loop that has no sound to make waits long, and a signal
    // from the switch of the synthesizer or from the start of the playback ends the wait at once. The loops
    // woke 50 times in a second for nothing before.
    constexpr std::chrono::milliseconds feedBusyWait {20};
    constexpr std::chrono::milliseconds feedIdleWait {200};

    inline std::chrono::milliseconds feedWait(PlaybackSource source)
    {
        return source == PlaybackSource::Silence ? feedIdleWait : feedBusyWait;
    }

    // The loop of the plot data and of the synthesizer settings has work only when the synthesizer is on, follows
    // the voice, or draws a plot that is on the screen. Else it needs no step.
    inline bool synthLoopHasWork(bool synthesizerOn, bool followsVoice, bool plotsWatched)
    {
        return synthesizerOn || followsVoice || plotsWatched;
    }

    // One step of the feed. The synthesizer and the session player make a block only when they are the source,
    // so a synthesizer that is off costs no work. The silence needs no block from a sound maker.
    inline void feedPlaybackQueue(Module::Audio::Queue &queue, PlaybackSource source, void *synthesizer, void *session)
    {
        switch (source) {
        case PlaybackSource::Session:
            queue.pushIfNeeded(session);
            break;
        case PlaybackSource::Synthesizer:
            queue.pushIfNeeded(synthesizer);
            break;
        case PlaybackSource::Silence:
            queue.pushSilenceIfNeeded();
            break;
        }
    }

}

#endif // MAIN_PLAYBACK_FEED_H
