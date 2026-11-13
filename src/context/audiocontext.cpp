#include "audiocontext.h"

using namespace Main;

AudioContext::AudioContext(Audio::Buffer *captureBuffer, Audio::Queue *playbackQueue)
    : AudioContext(std::make_unique<Audio::PortAudio>(), captureBuffer, playbackQueue)
{
}
