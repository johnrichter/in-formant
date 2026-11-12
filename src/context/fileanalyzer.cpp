#include "fileanalyzer.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "solvermakers.h"
#include "../modules/audio/resampler/resampler.h"
#include "../modules/app/pipeline/processors/formants.h"
#include "../modules/app/pipeline/processors/pitch.h"

using namespace Main;

FileAnalyzer::FileAnalyzer(Config *config, DataStore *dataStore, int sampleRate)
    : mConfig(config),
      mDataStore(dataStore),
      mSampleRate(sampleRate),
      mRunning(false),
      mCancel(false),
      mProgress(0.0)
{
}

FileAnalyzer::~FileAnalyzer()
{
    cancel();
}

void FileAnalyzer::cancel()
{
    mCancel = true;
    if (mThread.joinable()) mThread.join();
    mRunning = false;
}

void FileAnalyzer::start(std::function<void(bool)> finished)
{
    cancel();

    mCancel = false;
    mProgress = 0.0;
    mRunning = true;

    const double duration = mDataStore->session().duration();
    mThread = std::thread(&FileAnalyzer::run, this, duration, std::move(finished));
}

void FileAnalyzer::run(double duration, std::function<void(bool)> finished)
{
    // The analysis has its own solvers, so it does not share the state of the live analysis.
    std::shared_ptr<Analysis::PitchSolver> pitchSolver(makePitchSolver(mConfig->getPitchAlgorithm()));
    std::shared_ptr<Analysis::LinpredSolver> linpredSolver(makeLinpredSolver(mConfig->getLinpredAlgorithm()));
    std::shared_ptr<Analysis::FormantSolver> formantSolver(makeFormantSolver(mConfig->getFormantAlgorithm()));

    std::vector<std::unique_ptr<Module::App::Processors::BaseProcessor>> processors;
    processors.push_back(std::make_unique<Module::App::Processors::Pitch>(mConfig, mDataStore, pitchSolver));
    processors.push_back(std::make_unique<Module::App::Processors::Formants>(mConfig, mDataStore, linpredSolver, formantSolver));

    const double fs = mSampleRate;
    const double granularity = std::max(mConfig->getAnalysisGranularity(), 1.0) / 1000.0;
    const size_t blockSize = (size_t) std::llround(granularity * fs);

    double maxFrameLength = granularity;
    for (const auto &processor : processors) maxFrameLength = std::max(maxFrameLength, processor->getFrameLength());
    rpm::vector<double> slidingWindow((size_t) std::llround(maxFrameLength * fs), 0.0);

    Module::Audio::Resampler resampler(Session::sampleRate, mSampleRate);

    std::vector<double> pending;
    size_t used = 0;
    double time = 0.0;
    bool completed = true;

    // The analysis reads one second of the stored sound at a time.
    constexpr double chunk = 1.0;
    for (double t = 0.0; t < duration; t += chunk) {
        if (mCancel) {
            completed = false;
            break;
        }

        rpm::vector<float> raw;
        mDataStore->session().read(t, std::min(t + chunk, duration), raw);
        rpm::vector<double> in(raw.begin(), raw.end());
        const rpm::vector<double> out = resampler.process(in);
        pending.erase(pending.begin(), pending.begin() + (long) used);
        used = 0;
        pending.insert(pending.end(), out.begin(), out.end());

        while (pending.size() - used >= blockSize) {
            std::rotate(slidingWindow.begin(), slidingWindow.begin() + (long) blockSize, slidingWindow.end());
            std::copy(pending.begin() + (long) used, pending.begin() + (long) (used + blockSize),
                      slidingWindow.end() - (long) blockSize);
            used += blockSize;
            time += granularity;

            for (auto &processor : processors) {
                if (processor->canProcess(time)) processor->process(slidingWindow, fs, time);
            }
        }

        mProgress = std::min(1.0, (t + chunk) / std::max(duration, 1e-9));
    }

    mRunning = false;
    if (finished) finished(completed);
}
