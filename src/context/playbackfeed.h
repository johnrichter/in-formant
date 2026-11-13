#ifndef MAIN_PLAYBACK_FEED_H
#define MAIN_PLAYBACK_FEED_H

#include "../modules/audio/queue/queue.h"

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
