#ifndef MAIN_CONTEXT_CONFIG_H
#define MAIN_CONTEXT_CONFIG_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include "../filesystem.hpp"
#include <atomic>
#include <fstream>
#include <toml++/toml.h>

#include "solvermakers.h"
#include "../gui/qpainterwrapper.h"
#include "../modules/audio/base/base.h"

namespace Main {

    fs::path getConfigPath();
    toml::table getConfigTable();

    class Config : public QObject {
        Q_OBJECT
        Q_PROPERTY(QVariantMap options      READ options                        NOTIFY optionsChanged)
        Q_PROPERTY(QVariantMap themeColors  READ themeColors                    NOTIFY optionsChanged)
        Q_PROPERTY(int pitchAlgorithm       READ getPitchAlgorithmNumeric       WRITE setPitchAlgorithm         NOTIFY pitchAlgorithmChanged)
        Q_PROPERTY(int linpredAlgorithm     READ getLinpredAlgorithmNumeric     WRITE setLinpredAlgorithm       NOTIFY linpredAlgorithmChanged)
        Q_PROPERTY(int formantAlgorithm     READ getFormantAlgorithmNumeric     WRITE setFormantAlgorithm       NOTIFY formantAlgorithmChanged)
        Q_PROPERTY(int invglotAlgorithm     READ getInvglotAlgorithmNumeric     WRITE setInvglotAlgorithm       NOTIFY invglotAlgorithmChanged)
        Q_PROPERTY(double viewZoom          READ getViewZoom                    WRITE setViewZoom               NOTIFY viewZoomChanged)
        Q_PROPERTY(int viewMinFrequency     READ getViewMinFrequency            WRITE setViewMinFrequency       NOTIFY viewMinFrequencyChanged)
        Q_PROPERTY(int viewMaxFrequency     READ getViewMaxFrequency            WRITE setViewMaxFrequency       NOTIFY viewMaxFrequencyChanged)
        Q_PROPERTY(int viewFFTSize          READ getViewFFTSize                 WRITE setViewFFTSize            NOTIFY viewFFTSizeChanged)
        Q_PROPERTY(int viewMaxGain          READ getViewMaxGain                 WRITE setViewMaxGain            NOTIFY viewMaxGainChanged)
        Q_PROPERTY(int viewSpectrogramQuality READ getViewSpectrogramQuality    WRITE setViewSpectrogramQuality NOTIFY viewSpectrogramQualityChanged)
        Q_PROPERTY(int viewSpectrogramBlur  READ getViewSpectrogramBlur         WRITE setViewSpectrogramBlur    NOTIFY viewSpectrogramBlurChanged)
        Q_PROPERTY(double viewTimeSpan      READ getViewTimeSpan                WRITE setViewTimeSpan           NOTIFY viewTimeSpanChanged)
        Q_PROPERTY(int viewFrequencyScale   READ getViewFrequencyScaleNumeric   WRITE setViewFrequencyScale     NOTIFY viewFrequencyScaleChanged)
        Q_PROPERTY(bool viewShowSpectrogram READ getViewShowSpectrogram         WRITE setViewShowSpectrogram    NOTIFY viewShowSpectrogramChanged)
        Q_PROPERTY(bool viewShowPitch       READ getViewShowPitch               WRITE setViewShowPitch          NOTIFY viewShowPitchChanged)
        Q_PROPERTY(bool viewShowFormants    READ getViewShowFormants            WRITE setViewShowFormants       NOTIFY viewShowFormantsChanged)
        Q_PROPERTY(bool viewLive            READ getViewLive                    WRITE setViewLive               NOTIFY viewLiveChanged)
        Q_PROPERTY(double viewEnd           READ getViewEnd                     WRITE setViewEnd                NOTIFY viewEndChanged)
        Q_PROPERTY(int sessionMode          READ getSessionMode                 WRITE setSessionMode            NOTIFY sessionModeChanged)
        Q_PROPERTY(bool paused              READ isPaused                       WRITE setPaused                 NOTIFY pausedChanged)
        Q_PROPERTY(QString audioCaptureDevice READ getAudioCaptureDevice        WRITE setAudioCaptureDevice     NOTIFY audioCaptureDeviceChanged)
        Q_PROPERTY(QStringList audioCaptureDevices READ getAudioCaptureDevices  NOTIFY audioCaptureDevicesChanged)
    
    signals:
        void optionsChanged();
        void pitchAlgorithmChanged(int);
        void linpredAlgorithmChanged(int);
        void formantAlgorithmChanged(int);
        void invglotAlgorithmChanged(int);
        void audioBackendChanged(int);
        void viewZoomChanged(double);
        void viewMinFrequencyChanged(int);
        void viewMaxFrequencyChanged(int);
        void viewFFTSizeChanged(int);
        void viewMaxGainChanged(int);
        void viewSpectrogramQualityChanged(int);
        void viewSpectrogramBlurChanged(int);
        void viewTimeSpanChanged(double);
        void viewFrequencyScaleChanged(int);
        void viewShowSpectrogramChanged(bool);
        void viewShowPitchChanged(bool);
        void viewShowFormantsChanged(bool);
        void pausedChanged(bool);
        void viewLiveChanged(bool);
        void viewEndChanged(double);
        void sessionModeChanged(int);
        void audioCaptureDeviceChanged(QString);
        void audioCaptureDevicesChanged();

    public:
        Config();
        virtual ~Config();

        // The options are a store of named values for the display, the readouts, and the themes.
        // Each option has a default. The store saves the options in the [options] table of the
        // configuration file. QML reads them with config.options.<name>.
        QVariantMap options() const;
        Q_INVOKABLE QVariant option(const QString &name) const;
        Q_INVOKABLE void setOption(const QString &name, const QVariant &value);
        Q_INVOKABLE void resetOption(const QString &name);

        bool optionBool(const QString &name) const;
        int optionInt(const QString &name) const;
        double optionDouble(const QString &name) const;
        QString optionString(const QString &name) const;

        static QVariantMap defaultOptions();

        // The colours of the selected theme for the QML windows.
        QVariantMap themeColors() const;
        Q_INVOKABLE QStringList themeNames() const;
        Q_INVOKABLE QVariantList themePreview(const QString &name, int count) const;
        
        // The view follows the live sound when this is true. Otherwise the view ends at viewEnd
        // seconds of the session. These values are not stored in the configuration file.
        bool getViewLive() const { return mViewLive; }
        void setViewLive(bool live);
        double getViewEnd() const { return mViewEnd; }
        void setViewEnd(double seconds);

        // 0 is the live sound of the input device. 1 is a loaded audio file.
        int getSessionMode() const { return mSessionMode; }
        void setSessionMode(int mode);

        Module::Audio::Backend getAudioBackend();
        void setAudioBackend(Module::Audio::Backend b);

        // The name of the input device. An empty name selects the default device of the system.
        QString getAudioCaptureDevice();
        void setAudioCaptureDevice(const QString &name);

        // The names of the input devices that the audio backend found.
        QStringList getAudioCaptureDevices() const;
        void setAudioCaptureDevices(const QStringList &names);

        PitchAlgorithm getPitchAlgorithm();
        void setPitchAlgorithm(PitchAlgorithm alg);
        
        int getPitchAlgorithmNumeric();
        void setPitchAlgorithm(int alg);

        LinpredAlgorithm getLinpredAlgorithm();
        void setLinpredAlgorithm(LinpredAlgorithm alg);
        
        int getLinpredAlgorithmNumeric();
        void setLinpredAlgorithm(int alg);

        FormantAlgorithm getFormantAlgorithm(bool internal = false);
        void setFormantAlgorithm(FormantAlgorithm alg);
        
        int getFormantAlgorithmNumeric();
        void setFormantAlgorithm(int alg);

        InvglotAlgorithm getInvglotAlgorithm();
        void setInvglotAlgorithm(InvglotAlgorithm alg);
        
        int getInvglotAlgorithmNumeric();
        void setInvglotAlgorithm(int alg);

        double getViewZoom();
        void setViewZoom(double scale);

        int getViewMinFrequency();
        void setViewMinFrequency(int f);

        int getViewMaxFrequency();
        void setViewMaxFrequency(int f);

        int getViewFFTSize();
        void setViewFFTSize(int nfft);

        int getViewMaxGain();
        void setViewMaxGain(int g);

        // The filter of the spectrogram picture. Quality 0 is fast, and 1 is smooth.
        int getViewSpectrogramQuality();
        void setViewSpectrogramQuality(int quality);

        // The blur of the smooth filter in percent. 0 is sharp, and 100 is the softest.
        int getViewSpectrogramBlur();
        void setViewSpectrogramBlur(int percent);

        double getViewTimeSpan();
        void setViewTimeSpan(double dur);

        FrequencyScale getViewFrequencyScale();
        void setViewFrequencyScale(FrequencyScale scale);

        int getViewFrequencyScaleNumeric();
        void setViewFrequencyScale(int scale);

        int getViewFormantCount();
        std::tuple<double, double, double> getViewFormantColor(int i);

        bool getViewShowSpectrogram();
        void setViewShowSpectrogram(bool b);

        bool getViewShowPitch();
        void setViewShowPitch(bool b);

        bool getViewShowFormants();
        void setViewShowFormants(bool b);

        int getAnalysisMaxFrequency();
        int getAnalysisLpOffset();
        int getAnalysisPitchSampleRate();

        void setAnalysisGranularity(double ms); // default is 20ms
        double getAnalysisGranularity();

        void setAnalysisSpectrogramWindow(double ms); // default is 50ms
        double getAnalysisSpectrogramWindow();

        void setAnalysisPitchWindow(double ms); // default is 40ms
        double getAnalysisPitchWindow();

        void setAnalysisFormantWindow(double ms); // default is 20ms
        double getAnalysisFormantWindow();

        void setAnalysisOscilloscopeWindow(double ms); // default is 80ms
        double getAnalysisOscilloscopeWindow();

        void setAnalysisPitchSpacing(double ms); // default is 80ms
        double getAnalysisPitchSpacing();

        void setAnalysisFormantSpacing(double ms); // default is 80ms
        double getAnalysisFormantSpacing();

        void setAnalysisOscilloscopeSpacing(double ms); // default is 160ms
        double getAnalysisOscilloscopeSpacing();

        // WILL NOT BE SERIALIZED
        bool isPaused();
        void setPaused(bool p);

    private:
        toml::table mTbl;
        QVariantMap mOptions;
    
        // WILL NOT BE SERIALIZED
        bool mPaused;
        std::atomic<bool> mViewLive;
        std::atomic<double> mViewEnd;
        std::atomic<int> mSessionMode;
        QStringList mAudioCaptureDevices;
    };

}

#endif // MAIN_CONTEXT_CONFIG_H
