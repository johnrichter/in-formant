#ifndef AUDIO_BASE_H
#define AUDIO_BASE_H

#include "rpcxx.h"
#include "../buffer/buffer.h"
#include "../queue/queue.h"
#include <initializer_list>
#include <string>
#include <cstring>

#include <portaudio.h>

namespace Module::Audio {

    // A sound device that PortAudio reports. The latencies are in seconds, and the sample rate is in Hz.
    struct Device {
        std::string name;
        PaDeviceIndex index = paNoDevice;
        PaTime inputLatency = 0;
        PaTime outputLatency = 0;
        double sampleRate = 0;
    };

    class AbstractBase {
    public:
        AbstractBase();
        virtual ~AbstractBase();

        virtual void initialize() = 0;
        virtual void terminate() = 0;
   
        virtual void refreshDevices() = 0;

        virtual const rpm::vector<Device>& getCaptureDevices() const = 0;
        virtual const rpm::vector<Device>& getPlaybackDevices() const = 0;
        
        virtual const Device& getDefaultCaptureDevice() const = 0;
        virtual const Device& getDefaultPlaybackDevice() const = 0;

        virtual void openCaptureStream(const Device *pDevice) = 0;
        virtual void startCaptureStream() = 0;
        virtual void stopCaptureStream() = 0;
        virtual void closeCaptureStream() = 0;

        virtual void openPlaybackStream(const Device *pDevice) = 0;
        virtual void startPlaybackStream() = 0;
        virtual void stopPlaybackStream() = 0;
        virtual void closePlaybackStream() = 0;

        virtual void tickAudio();
        virtual bool needsTicking() = 0;

        void setCaptureBuffer(Buffer *buffer);
        void setPlaybackQueue(Queue *queue);

    protected:
        void setCaptureBufferSampleRate(int sampleRate);
        void pushToCaptureBuffer(const float *data, int length);

        Buffer *mCaptureBuffer;
        Queue *mPlaybackQueue;
    };

}

#endif // AUDIO_BASE_H
