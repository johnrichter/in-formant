#ifndef PIPELINE_PROCESSOR_BASE_H
#define PIPELINE_PROCESSOR_BASE_H

#include "rpcxx.h"

namespace Module::App::Processors {

    class BaseProcessor {
    public:
        BaseProcessor(double frameSpace, double frameLength);

        // The pipeline deletes each processor through a pointer to this class.
        virtual ~BaseProcessor() = default;

        bool canProcess(double timeNow) const;

        void process(const rpm::vector<double>& slidingWindow, double sampleRate, double timeNow);

        virtual void processData(const rpm::vector<double>& data, double sampleRate) = 0;

        double getFrameSpace() const;
        double getFrameLength() const;

    protected:
        // The time of the newest sample in the frame, in seconds.
        double getFrameEndTime() const;

        // The time of the center of the frame, in seconds.
        double getCenteredTime() const;

    private:
        double mFrameSpace;
        double mFrameLength;

        double mTime;
        double mFrameEnd;

        rpm::vector<double> mData;
    };
}

#endif // PIPELINE_PROCESSOR_BASE_H