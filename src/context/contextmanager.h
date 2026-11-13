#ifndef MAIN_CONTEXT_MANAGER_H
#define MAIN_CONTEXT_MANAGER_H

#include "../analysis/analysis.h"
#include "../modules/app/app.h"
#include "audiocontext.h"
#include "devicewatcher.h"
#include "rendercontext.h"
#include "guicontext.h"
#include "datastore.h"
#include "views/views.h"
#include "config.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>


#include "synthwrapper.h"
#include "dataviswrapper.h"
#include "sessionplayer.h"
#include "sessionwrapper.h"

#define STR_(arg) #arg
#define STR(arg) STR_(arg)
#define INFORMANT_VERSION_STR STR(INFORMANT_VERSION)


namespace Main {

    using namespace Module;

    using dur_ms = std::chrono::milliseconds;

    extern int argc;
    extern char **argv;

    class ContextManager;
    extern std::unique_ptr<ContextManager> contextManager;

    class ContextManager {
    public:
        ContextManager(
                int captureSampleRate,
                const dur_ms &playbackBlockDuration,
                int playbackSampleRate);

        int exec();

    private:
        void createViews();
        void loadConfig();
        
        void openAndStartAudioStreams();
        const Audio::Device *findCaptureDevice();
        void publishCaptureDevices();

        // Rebuild the state of the audio system, and open the streams again. Only the Qt thread
        // calls these functions. The capture starts when startCapture is true.
        void reinitializeAudio(bool startCapture);
        void openCaptureStreamIfAllowed();
        bool captureDeviceIsStale();
        // Starts the capture when the user allows the microphone. The first call shows the question of the system.
        void requestMicrophoneThen(std::function<void()> start);
        void startCapture();
        void onAudioDevicesChanged();

        // Save the sound of the run as a WAV file, when the user asks for it.
        void keepScratchRecording();

        void startAnalysisThread();
        void analysisThreadLoop();
        void stopAnalysisThread();

        void startSynthesisThread();
        void synthesisThreadLoop();
        void stopSynthesisThread();
        
        void datavisThreadLoop();

        void setView(const std::string &name);

        std::unique_ptr<Config> mConfig;

        std::shared_ptr<Analysis::PitchSolver> mPitchSolver;
        std::shared_ptr<Analysis::LinpredSolver> mLinpredSolver;
        std::shared_ptr<Analysis::FormantSolver> mFormantSolver;
        std::shared_ptr<Analysis::InvglotSolver> mInvglotSolver;

        
        std::unique_ptr<Audio::Buffer> mCaptureBuffer;
        std::unique_ptr<Audio::Queue> mPlaybackQueue;

        std::unique_ptr<DataStore> mDataStore;
        std::unique_ptr<SessionPlayer> mSessionPlayer;
        std::unique_ptr<SessionWrapper> mSessionWrapper;

        DataVisWrapper mDataVisWrapper;
        
        std::unique_ptr<App::Pipeline> mPipeline;
       
        std::unique_ptr<App::Synthesizer> mSynthesizer;
        SynthWrapper mSynthWrapper;

        std::unique_ptr<AudioContext> mAudioContext;
        std::unique_ptr<RenderContext> mRenderContext;
        std::unique_ptr<GuiContext> mGuiContext;

        rpm::map<std::string, std::unique_ptr<AbstractView>> mViews;
        
        std::thread mAnalysisThread;
        std::atomic_bool mAnalysisRunning;

        std::thread mSynthesisThread;
        std::atomic_bool mSynthesisRunning;

        // The synthesis loop and the loop of the plot data wait on this signal. A switch of the synthesizer, the
        // start of a playback, and the end of the loops send it, so a long wait ends at once.
        std::mutex mWakeMutex;
        std::condition_variable mWake;
        void waitForWork(std::chrono::milliseconds limit);
        void wakeLoops();

        std::thread mDatavisThread;

        int mViewMinFrequency;
        int mViewMaxFrequency;
        int mViewFftSize;

        int mAnalysisMaxFrequency;
        int mAnalysisLpOffset;
        int mAnalysisLpOrder;
        int mAnalysisPitchSampleRate;

        // False when the audio system could not start, for example with no input device.
        bool mAudioReady = true;
        // The capture stream opens only after the user allowed the microphone. Opening it earlier makes the system
        // show its question at launch, and the whole program waits for the answer.
        bool mCaptureOpen = false;
        // True while the question of the system for the microphone is open.
        bool mMicRequestPending = false;
        bool mReinitializing = false;
        std::atomic_bool mDeviceChangePending{false};

        // The default input device of the system at the time the capture stream opened.
        uint32_t mOpenedDefaultInputId = 0;

        // This member is the last one, so the watcher ends before the objects that its function uses.
        std::unique_ptr<DeviceWatcher> mDeviceWatcher;
    };

}

#endif // MAIN_CONTEXT_MANAGER_H
