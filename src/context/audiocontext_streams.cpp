// The part of the audio context that does not depend on a sound backend. The tests build it with a stand-in backend.
#include "audiocontext.h"
#include <iostream>
#include <stdexcept>

using namespace Main;

void AudioContext::requireReady() const
{
    if (!mInitialized) {
        throw std::runtime_error("AudioContext] The audio system is not running.");
    }
}

AudioContext::AudioContext(std::unique_ptr<Audio::AbstractBase> backend, Audio::Buffer *captureBuffer, Audio::Queue *playbackQueue)
    : mAudio(std::move(backend)),
      mInitialized(false),
      mCaptureStreamOpened(false),
      mCaptureStreamStarted(false),
      mPlaybackStreamOpened(false),
      mPlaybackStreamStarted(false)
{
    if (!mAudio) {
        throw std::runtime_error("AudioContext] Backend could not be initialized.");
    }

    mAudio->setCaptureBuffer(captureBuffer);
    mAudio->setPlaybackQueue(playbackQueue);
    mAudio->initialize();
    mInitialized = true;
    mAudio->refreshDevices();
}

AudioContext::~AudioContext()
{
    try {
        stopCaptureStream();
        closeCaptureStream();
        stopPlaybackStream();
        closePlaybackStream();
        if (mInitialized) {
            mAudio->terminate();
        }
    }
    catch (const std::exception &e) {
        std::cout << "AudioContext] " << e.what() << std::endl;
    }
}

void AudioContext::refreshDevices()
{
    requireReady();
    mAudio->refreshDevices();
}

void AudioContext::reinitialize()
{
    stopCaptureStream();
    closeCaptureStream();
    stopPlaybackStream();
    closePlaybackStream();

    if (mInitialized) {
        mInitialized = false;
        mAudio->terminate();
    }

    mAudio->initialize();
    mInitialized = true;
    mAudio->refreshDevices();
}

const rpm::vector<Audio::Device>& AudioContext::getCaptureDevices() const
{
    return mAudio->getCaptureDevices();
}

const rpm::vector<Audio::Device>& AudioContext::getPlaybackDevices() const
{
    return mAudio->getPlaybackDevices();
}
 
const Audio::Device& AudioContext::getDefaultCaptureDevice() const
{
    return mAudio->getDefaultCaptureDevice();
}

const Audio::Device& AudioContext::getDefaultPlaybackDevice() const
{
    return mAudio->getDefaultPlaybackDevice();
}

void AudioContext::openCaptureStream(const Audio::Device *pDevice)
{
    requireReady();
    if (!mCaptureStreamOpened) {
        mAudio->openCaptureStream(pDevice);
        mCaptureStreamOpened = true;
    }
}

void AudioContext::startCaptureStream()
{
    requireReady();
    if (!mCaptureStreamStarted) {
        mAudio->startCaptureStream();
        mCaptureStreamStarted = true;
    }
}

void AudioContext::stopCaptureStream()
{
    if (mCaptureStreamStarted) {
        mAudio->stopCaptureStream();
        mCaptureStreamStarted = false;
    }
}

void AudioContext::closeCaptureStream()
{
    if (mCaptureStreamOpened) {
        mAudio->closeCaptureStream();
        mCaptureStreamOpened = false;
    }
}

void AudioContext::openPlaybackStream(const Audio::Device *pDevice)
{
    requireReady();
    if (!mPlaybackStreamOpened) {
        mAudio->openPlaybackStream(pDevice);
        mPlaybackStreamOpened = true;
    }
}

void AudioContext::startPlaybackStream()
{
    requireReady();
    if (!mPlaybackStreamStarted) {
        mAudio->startPlaybackStream();
        mPlaybackStreamStarted = true;
    }
}

void AudioContext::stopPlaybackStream()
{
    if (mPlaybackStreamStarted) {
        mAudio->stopPlaybackStream();
        mPlaybackStreamStarted = false;
    }
}

void AudioContext::closePlaybackStream()
{
    if (mPlaybackStreamOpened) {
        mAudio->closePlaybackStream();
        mPlaybackStreamOpened = false;
    }
}

void AudioContext::tickAudio()
{
    if (mAudio->needsTicking()) {
        mAudio->tickAudio();
    }
}

