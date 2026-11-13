#include "base.h"
#include "../../../../context/contextmanager.h"
#include <exception>

using namespace Module::App::Processors;

BaseProcessor::BaseProcessor(double frameSpace, double frameLength)
    : mFrameSpace(frameSpace / 1000.0),
      mFrameLength(frameLength / 1000.0),
      mTime(0),
      mFrameEnd(0)
{
}

bool BaseProcessor::canProcess(double timeNow) const
{
    return timeNow - mTime >= mFrameSpace;
}

void BaseProcessor::resetTime()
{
    mTime = 0;
    mFrameEnd = 0;
}

void BaseProcessor::process(const rpm::vector<double>& slidingWindow, double sampleRate, double timeNow)
{
    const int frameSamples = (int) std::round(mFrameLength * sampleRate);

    mData.resize(frameSamples);

    // timeNow is the time of the newest sample. The frame ends there.
    mFrameEnd = timeNow;

    std::copy(std::prev(slidingWindow.end(), frameSamples), slidingWindow.end(),
            mData.begin());

    processData(mData, sampleRate);

    mTime = timeNow;
}

double BaseProcessor::getFrameSpace() const
{
    return mFrameSpace;
}

double BaseProcessor::getFrameLength() const
{
    return mFrameLength;
}

double BaseProcessor::getFrameEndTime() const
{
    return mFrameEnd;
}

double BaseProcessor::getCenteredTime() const
{
    // The frame ends at its newest sample, so the center is half a frame length before that time.
    return mFrameEnd - mFrameLength / 2;
}