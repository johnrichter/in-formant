#ifndef APP_SYNTH_H
#define APP_SYNTH_H


#include "rpcxx.h"

#include "../../../analysis/analysis.h"
#include "../../../synthesis/synthesis.h"
#include "../../../synthesis/voice.h"
#include "../../audio/audio.h"

#include <atomic>
#include <mutex>
#include <QObject>

namespace Module::App
{
    class Synthesizer : public QObject {
        Q_OBJECT

    public:
        Synthesizer(Module::Audio::Queue *);

    public slots:
        void setMasterGain(double);
        void setNoiseGain(double);
        void setGlotGain(double);
        void setGlotPitch(double);
        void setGlotRd(double);
        void setGlotTc(double);
        void setJitter(double);
        void setFormants(const rpm::vector<Analysis::FormantData>&);
        void setFilterShift(double);
        void setVoiced(bool);

    public:
        double getMasterGain() const;
        double getNoiseGain() const;
        double getGlotGain() const;
        double getGlotPitch() const;
        double getGlotRd() const;
        double getGlotTc() const;
        double getJitter() const;
        double getFilterShift() const;
        bool isVoiced() const;
        
        rpm::vector<std::array<double, 6>> getFilterCopy(double *fs) const;
        rpm::vector<double> getSourceCopy(double fs, double durationInMs) const;
        
        void generateAudio(int requestedLength);

        static void audioCallback(double *, int, void *);

    private:
        Module::Audio::Queue *playbackQueue;

        // The parameters from the user and from the follow mode. Other threads write them, and
        // the audio thread reads them.
        std::atomic<double> masterGain;
        std::atomic<double> noiseGain;
        std::atomic<double> glotGain;
        std::atomic<double> glotPitch;
        std::atomic<double> glotRd;
        std::atomic<double> glotTc;
        std::atomic<double> jitter;
        std::atomic<double> filterShift;
        std::atomic<bool> voiced;

        rpm::vector<Analysis::FormantData> formants;
        mutable std::mutex formantsMutex;

        double realMasterGain;
        double realNoiseGain;
        double realGlotGain;
        rpm::vector<Analysis::FormantData> realFormants;
        rpm::vector<std::array<double, 6>> realFilter;
        double realFilterShift;
        mutable std::mutex realFilterMutex;

        // The voice makes the sound from the parameters. Its level follows the filter and not the
        // sound, so the loudness stays the same for the same parameters.
        Synthesis::Voice voice;

        rpm::vector<double> surplus;

        Module::Audio::Resampler resampler;
    };
}


#endif // APP_SYNTH_H
