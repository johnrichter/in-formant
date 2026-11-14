#include "pipeline.h"
#include "../../../analysis/filter/filter.h"
#include "../../../synthesis/synthesis.h"
#include "../../../context/timings.h"
#include "../../../context/contextmanager.h"

#include "processors/spectrogram.h"
#include "processors/pitch.h"
#include "processors/formants.h"
#include "processors/oscilloscope.h"

#include <cctype>
#include <chrono>
#include <iostream>

using namespace Module::App;

Pipeline::Pipeline(Module::Audio::Buffer *captureBuffer,
                Main::DataStore *dataStore, Main::Config *config,
                Main::DataVisWrapper *dataVis,
                std::shared_ptr<Analysis::PitchSolver>& pitchSolver,
                std::shared_ptr<Analysis::LinpredSolver>& linpredSolver,
                std::shared_ptr<Analysis::FormantSolver>& formantSolver,
                std::shared_ptr<Analysis::InvglotSolver>& invglotSolver)
    : mCaptureBuffer(captureBuffer),
      mDataStore(dataStore),
      mConfig(config),
      mDataVis(dataVis),
      mTime(0),
      mThreadRunning(false),
      mStopThread(false),
      mBuffer(16000)
{
    mProcessors.push_back(std::make_unique<Processors::Spectrogram>(config, dataStore));
    mProcessors.push_back(std::make_unique<Processors::Pitch>(config, dataStore, pitchSolver));
    mProcessors.push_back(std::make_unique<Processors::Formants>(config, dataStore, linpredSolver, formantSolver));
    mProcessors.push_back(std::make_unique<Processors::Oscilloscope>(config, dataVis, invglotSolver));
}

Pipeline::~Pipeline()
{
    mThreadRunning = false;
    mStopThread = true;
    
    Module::Audio::Buffer::cancelPulls();

    if (mProcessingThread.joinable())
        mProcessingThread.join();
}

void Pipeline::callbackProcessing()
{
    rpm::vector<double> block;
    rpm::vector<double> slidingWindow;

    double time = 0;

    while (mThreadRunning && !mStopThread) {
        const double granularity = mConfig->getAnalysisGranularity() / 1000;

        block.resize(granularity * mSampleRate);
        mBuffer.pull(block.data(), (int) block.size());

        // A loaded file replaces the live input, so the sound of the input device is not used.
        if (mConfig->getSessionMode() != 0) {
            continue;
        }

        if (mRestart.exchange(false)) {
            time = 0;
            std::fill(slidingWindow.begin(), slidingWindow.end(), 0.0);
            for (auto& processor : mProcessors) processor->resetTime();
        }

        timer_guard timer(timings::update);

        double maxFrameLength = granularity;
        for (const auto& processor : mProcessors) {
            const double frameLength = processor->getFrameLength();
            if (frameLength > maxFrameLength)
                maxFrameLength = frameLength;
        }

        slidingWindow.resize(maxFrameLength * mSampleRate, 0.0);
        std::rotate(slidingWindow.begin(),
                std::next(slidingWindow.begin(), block.size()),
                slidingWindow.end());
        std::copy(block.begin(), block.end(), std::prev(slidingWindow.end(), block.size()));

        // The sliding window ends at the end of the new block. The time follows the samples of the block. The block
        // has a whole number of samples, so a sum of the granularity can differ from it, and the slices would then
        // run ahead of the stored sound.
        time += (double) block.size() / mSampleRate;

        mDataVis->pushAudio(block.data(), (int) block.size(), mSampleRate);

        mDataStore->session().append(block.data(), (int) block.size(), mSampleRate);

        for (auto& processor : mProcessors) {
            if (processor->canProcess(time)) {
                processor->process(slidingWindow, mSampleRate, time);
            }
        }
    }
}

void Pipeline::processAll()
{
    const double fs = (double) mCaptureBuffer->getSampleRate();

    static int blockSize = 512;
    rpm::vector<double> data(blockSize);
    mCaptureBuffer->pull(data.data(), (int) data.size());

    // The app is closing, so the block is incomplete and the processing thread must not start.
    if (Module::Audio::Buffer::pullsCancelled())
        return;

    mTime = mTime + blockSize / fs;

    mDataStore->setTime(mTime);
    mSampleRate = fs;

    rpm::vector<float> fdata(data.begin(), data.end());
    mBuffer.push(fdata.data(), (int) data.size());

    bool shouldNotBeRunning = false;
    if (mThreadRunning.compare_exchange_strong(shouldNotBeRunning, true)) {
        mProcessingThread = std::thread(std::mem_fn(&Pipeline::callbackProcessing), this);
    }

    // dynamically adjust blockSize to consume all the buffer.
    static int lastBufferLength = 0;
    int bufferLength = mCaptureBuffer->getLength();
    if (blockSize <= 16384 && lastBufferLength - bufferLength >= 8192) {
        blockSize += 128;
        std::cout << "Processing too slowly, "
                  << bufferLength << " samples remaining. "
                  << "Adjusted block size to "
                  << blockSize << " samples" << std::endl;
    }
    else if (blockSize >= 512 && lastBufferLength - bufferLength <= -1024) {
        blockSize -= 128;
        std::cout << "Processing fast enough, "
                  << "adjusted block size to "
                  << blockSize << " samples" << std::endl;
    }
    lastBufferLength = bufferLength;
} 
