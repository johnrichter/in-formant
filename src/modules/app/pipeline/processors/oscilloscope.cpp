#include "oscilloscope.h"

#include "../../../../analysis/analysis.h"

using namespace Module::App::Processors;

Oscilloscope::Oscilloscope(Main::Config *config, Main::DataVisWrapper *dataVis,
            std::shared_ptr<Analysis::InvglotSolver>& invglotSolver)
    : BaseProcessor(config->getAnalysisOscilloscopeSpacing(),
                    config->getAnalysisOscilloscopeWindow()),
      mConfig(config),
      mDataVis(dataVis),
      mInvglotSolver(invglotSolver)
{
}

bool Oscilloscope::canProcess(double timeNow) const
{
    return mDataVis->watched() && BaseProcessor::canProcess(timeNow);
}

void Oscilloscope::processData(const rpm::vector<double>& data, double sampleRate)
{
    constexpr double fsOsc = 8000;

    mResampler.setRate(sampleRate, fsOsc);

    auto out = mResampler.process(data);
    auto invglotResult = mInvglotSolver->solve(out.data(), (int) out.size(), fsOsc);

    mDataVis->setGif(invglotResult.glotSig, fsOsc);
}