#include "contextmanager.h"
#include "micaccess.h"
#include "playbackfeed.h"
#include "timings.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QPermissions>
#include <QDir>
#include <QMetaObject>
#include <QStandardPaths>
#include <QTimer>

#include <chrono>
#include <iostream>

using namespace Main;
using namespace std::chrono_literals;

ContextManager::ContextManager(
                int captureSampleRate,
                const dur_ms &playbackBlockDuration,
                int playbackSampleRate
            )
    : mConfig(std::make_unique<Config>()),
      mPitchSolver(makePitchSolver(mConfig->getPitchAlgorithm())),
      mLinpredSolver(makeLinpredSolver(mConfig->getLinpredAlgorithm())),
      mFormantSolver(makeFormantSolver(mConfig->getFormantAlgorithm())),
      mInvglotSolver(makeInvglotSolver(mConfig->getInvglotAlgorithm())),
      mCaptureBuffer(std::make_unique<Audio::Buffer>(captureSampleRate)),
      mPlaybackQueue(std::make_unique<Audio::Queue>(
                  playbackBlockDuration.count(),
                  playbackSampleRate, [](auto...){})),
      mDataStore(std::make_unique<DataStore>()),
      mSessionPlayer(std::make_unique<SessionPlayer>(&mDataStore->session(), mPlaybackQueue.get())),
      mSessionWrapper(std::make_unique<SessionWrapper>(mConfig.get(), mDataStore.get(), mSessionPlayer.get(), captureSampleRate,
                  [this]() { mPipeline->restart(); mDataStore->resetRealTime(); })),
      mPipeline(std::make_unique<App::Pipeline>(
                  mCaptureBuffer.get(), mDataStore.get(), mConfig.get(),
                  &mDataVisWrapper,
                  mPitchSolver, mLinpredSolver,
                  mFormantSolver, mInvglotSolver)),
      mSynthesizer(std::make_unique<App::Synthesizer>(mPlaybackQueue.get())),
      mAudioContext(std::make_unique<AudioContext>(
                  mCaptureBuffer.get(),
                  mPlaybackQueue.get())),
      mRenderContext(std::make_unique<RenderContext>(mConfig.get(), mDataStore.get())),
      mGuiContext(std::make_unique<GuiContext>(mConfig.get(), mRenderContext.get(), &mSynthWrapper, &mDataVisWrapper, mSessionWrapper.get()))
{

    createViews();
    loadConfig();
    mDataVisWrapper.setReview(mConfig.get(), &mDataStore->session());

    // The loops of the synthesizer wait long when they have no work. These signals end the wait.
    QObject::connect(&mSynthWrapper, &SynthWrapper::enabledChanged, [this](bool) { wakeLoops(); });
    QObject::connect(mSessionWrapper.get(), &SessionWrapper::playingChanged, [this]() { wakeLoops(); });
    mDataStore->setFormantTrackCount(4);
    QObject::connect(mConfig.get(), &Config::pitchAlgorithmChanged,
            [this](int index) {
                mPitchSolver.reset(makePitchSolver(static_cast<PitchAlgorithm>(index)));
            });
    QObject::connect(mConfig.get(), &Config::linpredAlgorithmChanged,
            [this](int index) {
                mLinpredSolver.reset(makeLinpredSolver(static_cast<LinpredAlgorithm>(index)));
            });
    QObject::connect(mConfig.get(), &Config::formantAlgorithmChanged,
            [this](int index) {
                mFormantSolver.reset(makeFormantSolver(static_cast<FormantAlgorithm>(index)));
            });
    QObject::connect(mConfig.get(), &Config::invglotAlgorithmChanged,
            [this](int index) {
                mInvglotSolver.reset(makeInvglotSolver(static_cast<InvglotAlgorithm>(index)));
            });
    QObject::connect(mConfig.get(), &Config::audioCaptureDeviceChanged,
            [this](QString) {
                onAudioDevicesChanged();
            });
    QObject::connect(mConfig.get(), &Config::pausedChanged,
            [this](bool paused) {
                if (paused) {
                    mAudioContext->stopCaptureStream();
                    mDataStore->stopRealTime();
                }
                else {
                    requestMicrophoneThen([this]() {
                        // The system can change its default input device while the capture is off.
                        if (!mAudioReady || captureDeviceIsStale()) {
                            reinitializeAudio(false);
                        }
                        startCapture();
                    });
                }
            });

    // Follow the changes of the devices of the system. The function runs on a thread of the
    // system, so it posts the work to the Qt thread, and it waits a short time for the burst of
    // notifications that one change makes.
    mDeviceWatcher = std::make_unique<DeviceWatcher>([this]() {
        if (mDeviceChangePending.exchange(true)) return;
        QMetaObject::invokeMethod(mConfig.get(), [this]() {
            QTimer::singleShot(300, mConfig.get(), [this]() {
                mDeviceChangePending = false;
                onAudioDevicesChanged();
            });
        }, Qt::QueuedConnection);
    });
}

namespace {

// The folder for the sound that the user asks to keep (the setting keepRecordings).
QString keptRecordingsFolder()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Recordings";
}

}

void ContextManager::keepScratchRecording()
{
    // The sound of the capture goes to a scratch file. The file is deleted at the end of the run,
    // unless the user asks to keep it. A sound that comes from an audio file is not a recording.
    if (!mConfig->optionBool("keepRecordings") || mConfig->getSessionMode() != 0) return;

    const double length = mDataStore->session().duration();
    if (length < 1.0) return;

    const QString folder = keptRecordingsFolder();
    QDir().mkpath(folder);
    const QString path = folder + "/informant-session-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".wav";
    mDataStore->session().exportWav(path, 0.0, length);
}

int ContextManager::exec()
{
    // Remove the scratch files of runs that ended in a crash. Keep their sound when the user asks.
    Session::recoverOrphans(mConfig->optionBool("keepRecordings") ? keptRecordingsFolder() : QString());

    // With no input device, the audio system throws an error. The window then opens with a status, and the
    // device watcher restarts the audio when a device appears.
    try {
        openAndStartAudioStreams();
    }
    catch (const std::exception &e) {
        mAudioReady = false;
        std::cout << "ContextManager] The audio system did not start: " << e.what() << std::endl;
        mSessionWrapper->showStatus(QStringLiteral("No input device available"));
        mConfig->setPaused(true);
    }
    startAnalysisThread();
    startSynthesisThread();

    int retCode = mGuiContext->exec();

    stopAnalysisThread();
    stopSynthesisThread();

    keepScratchRecording();

    return retCode;
}

void ContextManager::createViews()
{
    mViews["spectrogram"] = std::make_unique<View::Spectrogram>();

    setView("spectrogram");
}

void ContextManager::loadConfig()
{
    mViewMinFrequency = mConfig->getViewMinFrequency();
    mViewMaxFrequency = mConfig->getViewMaxFrequency();
    mViewFftSize = mConfig->getViewFFTSize();

    mAnalysisMaxFrequency = mConfig->getAnalysisMaxFrequency();
    mAnalysisLpOffset = mConfig->getAnalysisLpOffset();
    mAnalysisLpOrder = std::round((double) mAnalysisMaxFrequency / 500.0) + mAnalysisLpOffset;
    mAnalysisPitchSampleRate = mConfig->getAnalysisPitchSampleRate();
}

const Audio::Device *ContextManager::findCaptureDevice()
{
    const std::string name = mConfig->getAudioCaptureDevice().toStdString();

    if (!name.empty()) {
        for (const auto &device : mAudioContext->getCaptureDevices()) {
            if (device.name == name) {
                return &device;
            }
        }
        std::cout << "ContextManager] Input device \"" << name << "\" not found, using the default device." << std::endl;
    }

    // A null pointer selects the default device of the system.
    return nullptr;
}

void ContextManager::publishCaptureDevices()
{
    QStringList names;
    for (const auto &device : mAudioContext->getCaptureDevices()) {
        names.push_back(QString::fromStdString(device.name));
    }

    QString defaultName;
    if (!mAudioContext->getCaptureDevices().empty()) {
        defaultName = QString::fromStdString(mAudioContext->getDefaultCaptureDevice().name);
    }

    std::cout << "ContextManager] Input devices: " << names.join(" | ").toStdString()
              << " (default: " << defaultName.toStdString() << ")" << std::endl;
    mConfig->setAudioCaptureDevices(names, defaultName);
}

void ContextManager::openAndStartAudioStreams()
{
    publishCaptureDevices();
    mAudioContext->openCaptureStream(findCaptureDevice());
    mOpenedDefaultInputId = systemDefaultInputDeviceId();

    // The application starts in the paused state. The capture starts when the user presses play.
    if (!mConfig->isPaused()) {
        mAudioContext->startCaptureStream();
    }
    mAudioContext->openPlaybackStream(nullptr);
    mAudioContext->startPlaybackStream();
}

void ContextManager::reinitializeAudio(bool startCapture)
{
    if (mReinitializing) return;
    mReinitializing = true;

    try {
        mAudioContext->reinitialize();
        mAudioReady = true;

        publishCaptureDevices();
        mAudioContext->openCaptureStream(findCaptureDevice());
        mOpenedDefaultInputId = systemDefaultInputDeviceId();
        mAudioContext->openPlaybackStream(nullptr);
        mAudioContext->startPlaybackStream();
        if (startCapture) {
            mAudioContext->startCaptureStream();
        }
    }
    catch (const std::exception &e) {
        mAudioReady = false;
        std::cout << "ContextManager] The audio system did not start: " << e.what() << std::endl;
        mSessionWrapper->showStatus(QStringLiteral("No input device available"));
    }

    mReinitializing = false;
}

bool ContextManager::captureDeviceIsStale()
{
    // A named device does not change with the default. The watcher handles a change of the list.
    if (!mConfig->getAudioCaptureDevice().isEmpty()) return false;

    return mOpenedDefaultInputId != systemDefaultInputDeviceId();
}

void ContextManager::requestMicrophoneThen(std::function<void()> start)
{
    QCoreApplication *app = QCoreApplication::instance();
    const QMicrophonePermission permission;
    const Qt::PermissionStatus status = app->checkPermission(permission);
    const MicAccess access = status == Qt::PermissionStatus::Granted ? MicAccess::Granted
                           : status == Qt::PermissionStatus::Denied ? MicAccess::Denied
                                                                    : MicAccess::Undetermined;

    switch (micStep(access)) {
    case MicStep::Start:
        start();
        break;
    case MicStep::Ask:
        // The system shows its question. The handler runs on the Qt thread when the user answers.
        app->requestPermission(permission, app, [this, start](const QPermission &answer) {
            if (answer.status() == Qt::PermissionStatus::Granted) {
                start();
            }
            else {
                mSessionWrapper->showStatus(QString::fromUtf8(micDeniedMessage()));
                mConfig->setPaused(true);
            }
        });
        break;
    case MicStep::ShowDenied:
        mSessionWrapper->showStatus(QString::fromUtf8(micDeniedMessage()));
        mConfig->setPaused(true);
        break;
    }
}

void ContextManager::startCapture()
{
    try {
        mAudioContext->startCaptureStream();
        mDataStore->startRealTime();
    }
    catch (const std::exception &e) {
        std::cout << "ContextManager] The capture did not start: " << e.what() << std::endl;
        mSessionWrapper->showStatus(QStringLiteral("The input device did not start"));
        mConfig->setPaused(true);
    }
}

void ContextManager::onAudioDevicesChanged()
{
    const bool running = !mConfig->isPaused();

    // While the capture runs, its clock stops for the short gap of the restart.
    if (running) mDataStore->stopRealTime();
    reinitializeAudio(running);
    if (running && mAudioReady) mDataStore->startRealTime();
    else if (running) mConfig->setPaused(true);
}

void ContextManager::startAnalysisThread()
{
    mAnalysisRunning = true;
    mAnalysisThread = std::thread(std::mem_fn(&ContextManager::analysisThreadLoop), this);
    mDatavisThread = std::thread(std::mem_fn(&ContextManager::datavisThreadLoop), this);
}

void ContextManager::analysisThreadLoop()
{
    while (mAnalysisRunning) {
        mAudioContext->tickAudio();
        
        mPipeline->processAll();

        while (mConfig->isPaused() && mAnalysisRunning) {
            std::this_thread::sleep_for(0.5s);
        }
    }
}

void ContextManager::datavisThreadLoop()
{
    using Synth = Module::App::Synthesizer;

    // The pitch and the voicing go to the synthesizer from this loop, because it reads the pitch
    // estimates and the sliders in one place. A signal between threads would arrive late and
    // could replace a newer value.
    QObject::connect(&mSynthWrapper, &SynthWrapper::noiseGainChanged, mSynthesizer.get(), &Synth::setNoiseGain);
    QObject::connect(&mSynthWrapper, &SynthWrapper::glotGainChanged, mSynthesizer.get(), &Synth::setGlotGain);
    QObject::connect(&mSynthWrapper, &SynthWrapper::glotRdChanged, mSynthesizer.get(), &Synth::setGlotRd);
    QObject::connect(&mSynthWrapper, &SynthWrapper::glotTcChanged, mSynthesizer.get(), &Synth::setGlotTc);
    QObject::connect(&mSynthWrapper, &SynthWrapper::filterShiftChanged, mSynthesizer.get(), &Synth::setFilterShift);
    QObject::connect(&mSynthWrapper, &SynthWrapper::jitterChanged, mSynthesizer.get(), &Synth::setJitter);

    double maxFrequency = 0;
    rpm::vector<double> frequencies(1024);
    rpm::vector<std::complex<double>> wn(frequencies.size());

    double maxFrequencySource = 16000;
    rpm::map<int, rpm::vector<double>> fftWindowCache;
    Analysis::RealFFT fft(512);
    rpm::vector<double> fftFrequencies(fft.getOutputLength());
    for (int k = 0; k < fftFrequencies.size(); ++k) {
        fftFrequencies[k] = maxFrequencySource * (double) (k + 1) / (double) fftFrequencies.size();
    }

    // The state of the follow mode. The pitch is smooth on a logarithmic scale, and a short
    // gap in the estimates does not switch the voice off.
    double followLogPitch = 0;
    bool followActive = false;
    int unvoicedFrames = 0;

    // The follow formants mode keeps four stable slots. A slot holds its last value for a short
    // time. After that, the slot returns to the manual value. The count of formants stays at four,
    // so the filter does not jump.
    constexpr int slotCount = SynthWrapper::formantCount;
    constexpr auto slotHold = 300ms;
    double slotLogFrequency[slotCount] = {};
    std::chrono::steady_clock::time_point slotSeen[slotCount] = {};
    bool slotActive[slotCount] = {};

    auto lastUiUpdate = std::chrono::steady_clock::now();

    while (mAnalysisRunning && mSynthesisRunning) {
        const bool followPitch = mSynthWrapper.followPitch();
        const bool followFormants = mSynthWrapper.followFormants();

        std::optional<double> pitchEstimate;
        std::optional<double> formantEstimate[slotCount];

        if (followPitch || followFormants) {
            mDataStore->beginRead();

            if (followPitch && !mDataStore->getPitchTrack().empty()) {
                pitchEstimate = mDataStore->getPitchTrack().back();
            }

            if (followFormants) {
                for (int i = 0; i < slotCount; ++i) {
                    if (!mDataStore->getFormantTrack(i).empty()) {
                        formantEstimate[i] = mDataStore->getFormantTrack(i).back();
                    }
                }
            }

            mDataStore->endRead();
        }

        const auto now = std::chrono::steady_clock::now();

        // The manual values set the formants. In follow mode, an estimate replaces the manual
        // value of its slot, and the estimate is smooth on a logarithmic scale.
        {
            rpm::vector<Analysis::FormantData> formants;
            for (int i = 0; i < slotCount; ++i) {
                if (followFormants && formantEstimate[i].has_value() && *formantEstimate[i] > 0) {
                    const double target = std::log(*formantEstimate[i]);
                    slotLogFrequency[i] = slotActive[i] ? slotLogFrequency[i] + 0.4 * (target - slotLogFrequency[i]) : target;
                    slotActive[i] = true;
                    slotSeen[i] = now;
                }
                else if (!followFormants || now - slotSeen[i] > slotHold) {
                    slotActive[i] = false;
                }

                const double frequency = slotActive[i] ? std::exp(slotLogFrequency[i]) : mSynthWrapper.formantFrequency(i);
                formants.push_back({frequency, mSynthWrapper.formantBandwidth(i)});
            }
            mSynthesizer->setFormants(formants);
        }

        const bool updateUi = now - lastUiUpdate >= 100ms;
        if (updateUi) lastUiUpdate = now;

        if (followPitch) {
            if (pitchEstimate.has_value() && *pitchEstimate > 0) {
                const double target = std::log(*pitchEstimate);
                followLogPitch = followActive ? followLogPitch + 0.6 * (target - followLogPitch) : target;
                followActive = true;
                unvoicedFrames = 0;
                mSynthesizer->setGlotPitch(std::exp(followLogPitch));
                mSynthesizer->setVoiced(true);
            }
            else if (++unvoicedFrames >= 4) {
                followActive = false;
                mSynthesizer->setVoiced(false);
            }

            // The sliders show the values that the synthesizer uses.
            if (updateUi) {
                if (followActive) mSynthWrapper.setGlotPitch(std::exp(followLogPitch));
                mSynthWrapper.setVoiced(followActive);
            }
        }
        else {
            followActive = false;
            unvoicedFrames = 0;
            mSynthesizer->setGlotPitch(mSynthWrapper.glotPitch());
            mSynthesizer->setVoiced(mSynthWrapper.voiced());
        }

        if (mSynthWrapper.enabled()) {
            mSynthesizer->setMasterGain(1.0);
        }
        else {
            mSynthesizer->setMasterGain(0.0);
        }

        // Make the data of the plots only while a plot is on the screen.
        if (mSynthWrapper.watched()) {
            double synthSampleRate;
            auto filter = mSynthesizer->getFilterCopy(&synthSampleRate);

            if (maxFrequency != synthSampleRate / 2) {
                maxFrequency = synthSampleRate / 2;
                for (int k = 0; k < frequencies.size(); ++k) {
                    frequencies[k] = maxFrequency * (double) (k + 1) / (double) frequencies.size();
                    wn[k] = std::polar(1.0, 2.0 * M_PI * frequencies[k] / synthSampleRate);
                }
            }

            mSynthWrapper.setFilterResponse(frequencies, Analysis::sosfreqz(filter, wn));

            auto source = mSynthesizer->getSourceCopy(maxFrequencySource * 2, 25.0);
            mSynthWrapper.setSource(source, maxFrequencySource * 2);

            auto sourceSpectrum = Analysis::fft_n(&fft, source, fftWindowCache);
            mSynthWrapper.setSourceSpectrum(fftFrequencies, sourceSpectrum);
        }

        // Wait long when the synthesizer is off and no plot is on the screen. A switch ends the wait at once.
        const bool work = synthLoopHasWork(mSynthWrapper.enabled(), followPitch || followFormants, mSynthWrapper.watched());
        waitForWork(work ? feedBusyWait : feedIdleWait);
    }
}

void ContextManager::stopAnalysisThread()
{
    Module::Audio::Buffer::cancelPulls();

    if (mAnalysisThread.joinable() || mDatavisThread.joinable()) {
        mAnalysisRunning = false;
        wakeLoops();
        mAnalysisThread.join();
        mDatavisThread.join();
    }
}


void ContextManager::startSynthesisThread()
{
    mSynthesisRunning = true;
    mSynthesisThread = std::thread(std::mem_fn(&ContextManager::synthesisThreadLoop), this);
}

void ContextManager::synthesisThreadLoop()
{ 
    // The queue plays either the synthesizer or the stored sound. Only this thread changes its source.
    bool playingSession = false;

    while (mSynthesisRunning) {
        const bool wantSession = mSessionPlayer->active();
        if (wantSession != playingSession) {
            playingSession = wantSession;
            mPlaybackQueue->setCallback(playingSession
                ? Module::Audio::QueueCallback(SessionPlayer::audioCallback)
                : Module::Audio::QueueCallback(App::Synthesizer::audioCallback));
        }

        // Generate sound only while the synthesizer is on. It ran all the time before, with the
        // volume at zero, and it used a few percent of a CPU core.
        const PlaybackSource source = choosePlaybackSource(playingSession, mSynthWrapper.enabled());
        mPlaybackQueue->setIdle(source == PlaybackSource::Silence);
        feedPlaybackQueue(*mPlaybackQueue, source, mSynthesizer.get(), mSessionPlayer.get());
        waitForWork(feedWait(source));
    }
}

void ContextManager::waitForWork(std::chrono::milliseconds limit)
{
    std::unique_lock<std::mutex> lock(mWakeMutex);
    mWake.wait_for(lock, limit);
}

void ContextManager::wakeLoops()
{
    mWake.notify_all();
}

void ContextManager::stopSynthesisThread()
{
    if (mSynthesisThread.joinable()) {
        mSynthesisRunning = false;
        wakeLoops();
        if (mSynthesisThread.joinable()) {
            mSynthesisThread.join();
        }
    }
}


void ContextManager::setView(const std::string &name)
{
    AbstractView *view;

    auto it = mViews.find(name);    
    if (it != mViews.end()) {
        view = it->second.get();
    }
    else {
        view = nullptr;
    }
    
    mGuiContext->setView(view);
    mRenderContext->setView(view);
}