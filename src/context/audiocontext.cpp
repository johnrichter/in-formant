#include "audiocontext.h"
#include <array>
#include <iostream>
#include <stdexcept>

using namespace Main;

static std::array supportedAudioBackends {
// Disable Dummy backend regardless because of locking.
/*#ifdef AUDIO_USE_DUMMY
    Audio::Backend::Dummy,
#endif*/
#ifdef AUDIO_USE_ALSA
    Audio::Backend::ALSA,
#endif
// Disable PulseAudio backend regardless because of latency issues.
/*#ifdef AUDIO_USE_PULSE
    Audio::Backend::Pulse,
#endif*/
#ifdef AUDIO_USE_PORTAUDIO
    Audio::Backend::PortAudio,
#endif
#ifdef AUDIO_USE_OBOE
    Audio::Backend::Oboe,
#endif
#ifdef AUDIO_USE_WEBAUDIO
    Audio::Backend::WebAudio,
#endif
};

Audio::Backend Main::getDefaultAudioBackend()
{
#if defined(_WIN32) || defined(__APPLE__)
    return Audio::Backend::PortAudio;
#elif defined(ANDROID) || defined(__ANDROID__)
    return Audio::Backend::Oboe;
#elif defined(__linux)
    return Audio::Backend::PortAudio; // FIXME: alsa causes really high CPU usage
#elif defined(__EMSCRIPTEN__)
    return Audio::Backend::WebAudio;
#else
#  error "No default backend for this platform."
#endif
}

rpm::vector<Audio::Backend> Main::getSupportedAudioBackends()
{
    return rpm::vector<Audio::Backend>(
            supportedAudioBackends.begin(),
            supportedAudioBackends.end());
}

std::string Main::getAudioBackendName(Audio::Backend type)
{
    switch (type) {
    case Audio::Backend::Dummy:
        return "Dummy";
    case Audio::Backend::ALSA:
        return "ALSA";
    case Audio::Backend::Pulse:
        return "PulseAudio";
    case Audio::Backend::PortAudio:
        return "PortAudio";
    case Audio::Backend::Oboe:
        return "Oboe";
    case Audio::Backend::WebAudio:
        return "WebAudio";
    default:
        return "Unknown";
    }
}

static std::unique_ptr<Audio::AbstractBase> makeAudioBackend(Audio::Backend type)
{
    std::unique_ptr<Audio::AbstractBase> backend;
    switch (type) {
#ifdef AUDIO_USE_DUMMY
    case Audio::Backend::Dummy:
        backend = std::make_unique<Audio::Dummy>();
        break;
#endif
#ifdef AUDIO_USE_ALSA
    case Audio::Backend::ALSA:
        backend = std::make_unique<Audio::Alsa>();
        break;
#endif
#ifdef AUDIO_USE_PULSE
    case Audio::Backend::Pulse:
        backend = std::make_unique<Audio::Pulse>();
        break;
#endif
#ifdef AUDIO_USE_PORTAUDIO
    case Audio::Backend::PortAudio:
        backend = std::make_unique<Audio::PortAudio>();
        break;
#endif
#ifdef AUDIO_USE_OBOE
    case Audio::Backend::Oboe:
        backend = std::make_unique<Audio::Oboe>();
        break;
#endif
#ifdef AUDIO_USE_WEBAUDIO
    case Audio::Backend::WebAudio:
        backend = std::make_unique<Audio::WebAudio>();
        break;
#endif
    default:
        throw std::runtime_error(std::string("AudioContext] Unknown backend: ") + std::to_string(static_cast<int>(type)));
    }

    return backend;
}

AudioContext::AudioContext(Audio::Backend type, Audio::Buffer *captureBuffer, Audio::Queue *playbackQueue)
    : AudioContext(makeAudioBackend(type), captureBuffer, playbackQueue)
{
}

