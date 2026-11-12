#ifndef PIPELINE_PROCESSOR_OSCILLOSCOPE_H
#define PIPELINE_PROCESSOR_OSCILLOSCOPE_H

#include "rpcxx.h"

#include <memory>

#include "../../../audio/resampler/resampler.h"
#include "../../../../analysis/invglot/invglot.h"
#include "../../../../context/config.h"
#include "../../../../context/dataviswrapper.h"

#include "base.h"

namespace Module::App::Processors {

    class Oscilloscope : public BaseProcessor {
    public:
        Oscilloscope(Main::Config *config, Main::DataVisWrapper *dataVis,
            std::shared_ptr<Analysis::InvglotSolver>& invglotSolver);

        // The oscilloscope makes data only while its plot is on the screen.
        bool canProcess(double timeNow) const override;

        void processData(const rpm::vector<double>& data, double sampleRate) override;

    private:
        Main::Config *mConfig;
        Main::DataVisWrapper *mDataVis;
        std::shared_ptr<Analysis::InvglotSolver>& mInvglotSolver;

        Module::Audio::Resampler mResampler;
    };

}

#endif // PIPELINE_PROCESSOR_OSCILLOSCOPE_H