#ifndef SYNTH_WRAPPER_H
#define SYNTH_WRAPPER_H

#ifndef WITHOUT_SYNTH

#include <array>
#include <atomic>
#include <QMutex>
#include <QObject>
#include <QPointF>
#include <QVariantList>
#include <QVector>
#include "../gui/plotitem.h"
#include "../modules/app/synthesizer/synthesizer.h"
#include "vowelpresets.h"

namespace Main {
   
    class SynthWrapper : public QObject, public Gui::PlotProvider {
        Q_OBJECT
        Q_PROPERTY(bool enabled         READ enabled        WRITE setEnabled        NOTIFY enabledChanged)
        Q_PROPERTY(double noiseGain     READ noiseGain      WRITE setNoiseGain      NOTIFY noiseGainChanged)
        Q_PROPERTY(double glotGain      READ glotGain       WRITE setGlotGain       NOTIFY glotGainChanged)
        Q_PROPERTY(double glotPitch     READ glotPitch      WRITE setGlotPitch      NOTIFY glotPitchChanged)
        Q_PROPERTY(double glotRd        READ glotRd         WRITE setGlotRd         NOTIFY glotRdChanged)
        Q_PROPERTY(double glotTc        READ glotTc         WRITE setGlotTc         NOTIFY glotTcChanged)
        Q_PROPERTY(double filterShift   READ filterShift    WRITE setFilterShift    NOTIFY filterShiftChanged)
        Q_PROPERTY(bool voiced          READ voiced         WRITE setVoiced         NOTIFY voicedChanged)
        Q_PROPERTY(bool followPitch     READ followPitch    WRITE setFollowPitch    NOTIFY followPitchChanged)
        Q_PROPERTY(bool followFormants  READ followFormants WRITE setFollowFormants NOTIFY followFormantsChanged)
        Q_PROPERTY(double jitter        READ jitter         WRITE setJitter         NOTIFY jitterChanged)
        Q_PROPERTY(QVariantList formantFrequencies READ formantFrequencies NOTIFY formantsChanged)
        Q_PROPERTY(QVariantList formantBandwidths  READ formantBandwidths  NOTIFY formantsChanged)
        Q_PROPERTY(QVariantList vowelPresets READ vowelPresetList CONSTANT)
        Q_PROPERTY(QVariantList voicePresets READ voicePresetList CONSTANT)
        Q_PROPERTY(QVariantList presetBandwidths READ presetBandwidthList CONSTANT)

    signals:
        void enabledChanged(bool);
        void noiseGainChanged(double);
        void glotGainChanged(double);
        void glotPitchChanged(double);
        void glotRdChanged(double);
        void glotTcChanged(double);
        void filterShiftChanged(double);
        void voicedChanged(double);
        void followPitchChanged(bool);
        void followFormantsChanged(bool);
        void jitterChanged(double);
        void formantsChanged();

    public:
        SynthWrapper();

        bool enabled() const;
        double noiseGain() const;
        double glotGain() const;
        double glotPitch() const;
        double glotRd() const;
        double glotTc() const;
        double filterShift() const;
        bool voiced() const;
        bool followPitch() const;
        bool followFormants() const;
        double jitter() const;
        QVariantList formantFrequencies() const;
        QVariantList formantBandwidths() const;
        QVariantList vowelPresetList() const;
        QVariantList voicePresetList() const;
        QVariantList presetBandwidthList() const;

        // The manual formants. The synthesizer uses them while follow formants is off.
        static constexpr int formantCount = 4;
        double formantFrequency(int index) const;
        double formantBandwidth(int index) const;
        Q_INVOKABLE void setFormantFrequency(int index, double hz);
        Q_INVOKABLE void setFormantBandwidth(int index, double hz);

        void setEnabled(bool);
        void setNoiseGain(double);
        void setGlotGain(double);
        void setGlotPitch(double);
        void setGlotRd(double);
        void setGlotTc(double);
        void setFilterShift(double);
        void setVoiced(bool);
        void setFollowPitch(bool);
        void setFollowFormants(bool);
        void setJitter(double);

        void setFilterResponse(const rpm::vector<double>& frequencies, const rpm::vector<double>& magnitudes);
        void setSource(const rpm::vector<double>& signal, double fs);
        void setSourceSpectrum(const rpm::vector<double>& frequencies, const rpm::vector<double>& magnitudes);

        // True while a plot of the synthesizer is on the screen.
        bool watched() const { return mWatchers.load() > 0; }

        bool fillPlot(int kind, const Gui::PlotRequest &request, Gui::PlotSnapshot &out) override;
        void changeWatchers(int kind, int delta) override;

    private:
        void evaluateFilterResponse();

        bool mEnabled;
        double mNoiseGain;
        double mGlotGain;
        double mGlotPitch;
        double mGlotRd;
        double mGlotTc;
        double mFilterShift;
        bool mVoiced;
        bool mFollowPitch;
        bool mFollowFormants;
        double mJitter;
        std::array<std::atomic<double>, formantCount> mFormantFrequency;
        std::array<std::atomic<double>, formantCount> mFormantBandwidth;

        QVector<QPointF> mFilterResponse;
        double mFilterResponseDbMin;
        double mFilterResponseDbMax;
        QMutex mFilterResponseMutex;

        QVector<QPointF> mSource;
        QMutex mSourceMutex;

        QVector<QPointF> mSourceSpectrum;
        double mSourceSpectrumDbMin;
        double mSourceSpectrumDbMax;
        QMutex mSourceSpectrumMutex;

        std::atomic<int> mWatchers;
        std::atomic<unsigned long long> mVersion;
    };

}

#endif // !WITHOUT_SYNTH

#endif // SYNTH_WRAPPER_H
