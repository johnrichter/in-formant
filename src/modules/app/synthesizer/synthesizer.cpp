#ifndef WITHOUT_SYNTH

#include "../../../analysis/analysis.h"
#include "../../../context/timings.h"
#include "synthesizer.h"
#include <random>
#include <iostream>

using namespace Module::App;

Synthesizer::Synthesizer(Module::Audio::Queue *playbackQueue)
    : playbackQueue(playbackQueue),
      resampler(48'000)
{
    playbackQueue->setCallback(App::Synthesizer::audioCallback);
    
    masterGain = 0.0;
    noiseGain = 0.3;
    glotGain = 1.0;
    glotPitch = 170.0;
    glotRd = 1.7;
    glotTc = 1.0;
    jitter = 0.0;
    formants = {{500, 60}, {1500, 90}, {2500, 120}, {3500, 150}};
    filterShift = 1.0;
    voiced = false;
    
    realMasterGain = 0.0;
    realNoiseGain = 0.0;
    realGlotGain = 0.0;
    realFilterShift = 1.0;
    realFilter.clear();

    resampler.setOutputRate(playbackQueue->getInSampleRate());
}

void Synthesizer::setMasterGain(double value)
{
    masterGain = value;
}

void Synthesizer::setNoiseGain(double value)
{
    noiseGain = value;
}

void Synthesizer::setGlotGain(double value)
{
    glotGain = value;
}

void Synthesizer::setGlotPitch(double value)
{
    glotPitch = value;
}

void Synthesizer::setGlotRd(double value)
{
    glotRd = value;
}

void Synthesizer::setGlotTc(double value)
{
    glotTc = value;
}

void Synthesizer::setJitter(double value)
{
    jitter = value;
}

void Synthesizer::setFormants(const rpm::vector<Analysis::FormantData>& value)
{
    std::lock_guard<std::mutex> lock(formantsMutex);
    formants = value;
}

void Synthesizer::setFilterShift(double value)
{
    filterShift = value;
}

void Synthesizer::setVoiced(bool value)
{
    voiced = value;
}

double Synthesizer::getMasterGain() const
{
    return realMasterGain;
}

double Synthesizer::getNoiseGain() const
{
    return realNoiseGain;
}

double Synthesizer::getGlotGain() const
{
    return realGlotGain;
}

double Synthesizer::getGlotPitch() const
{
    return glotPitch;
}

double Synthesizer::getGlotRd() const
{
    return glotRd;
}

double Synthesizer::getGlotTc() const
{
    return glotTc;
}

double Synthesizer::getJitter() const
{
    return jitter;
}

double Synthesizer::getFilterShift() const
{
    return filterShift;
}

bool Synthesizer::isVoiced() const
{
    return voiced;
}

rpm::vector<std::array<double, 6>> Synthesizer::getFilterCopy(double *fs) const
{
    *fs = resampler.getInputRate();

    // The plot shows the filter of the target values, so it also works while the audio is off.
    rpm::vector<Analysis::FormantData> targetFormants;
    {
        std::lock_guard<std::mutex> lock(formantsMutex);
        targetFormants = formants;
    }

    return Synthesis::frequencyShiftFilter(targetFormants, *fs, filterShift);
}

rpm::vector<double> Synthesizer::getSourceCopy(double fs, double durationInMs) const
{
    // The plot shows the source with no filter. It uses its own voice, so it never disturbs the sound.
    Synthesis::VoiceParams params;
    params.sampleRate = fs;
    params.pitch = glotPitch;
    params.rd = glotRd;
    params.tc = glotTc;
    params.jitter = jitter;
    params.noiseGain = noiseGain;
    params.glotGain = glotGain;
    params.voiced = true;

    Synthesis::Voice plotVoice;
    const int length = std::round((durationInMs * fs) / 1000);
    // The voice ramps its voicing up at the start, so the plot skips the first part.
    plotVoice.render(length, params);
    return plotVoice.render(length, params);
}

void Synthesizer::generateAudio(int requestedLength)
{
    const int inputLength = resampler.getRequiredInLength(requestedLength);

    rpm::vector<double> output(inputLength, 0.0);

    if (inputLength > 0 && realMasterGain > 1e-7) {
        Synthesis::VoiceParams params;
        params.sampleRate = resampler.getInputRate();
        params.pitch = glotPitch;
        params.rd = glotRd;
        params.tc = glotTc;
        params.jitter = jitter;
        params.noiseGain = realNoiseGain;
        params.glotGain = realGlotGain;
        params.voiced = voiced;
        {
            std::lock_guard<std::mutex> lock(realFilterMutex);
            params.filter = realFilter;
        }

        output = voice.render(inputLength, params);
        for (double &v : output) v *= 0.8 * realMasterGain;
    }

    output = resampler.process(output);

    surplus.insert(surplus.end(), output.begin(), output.end());
}

void Synthesizer::audioCallback(double *output, int length, void *userdata)
{
    timer_guard timer(timings::synth);

    auto self = static_cast<Synthesizer *>(userdata);

    // Update the real parameters.
    self->realMasterGain = 0.1 * self->realMasterGain + 0.9 * self->masterGain;
    self->realNoiseGain  = 0.1 * self->realNoiseGain  + 0.9 * self->noiseGain;
    self->realGlotGain   = 0.1 * self->realGlotGain   + 0.9 * self->glotGain;
    self->realFilterShift = 0.2 * self->realFilterShift + 0.8 * self->filterShift;

    rpm::vector<Analysis::FormantData> targetFormants;
    {
        std::lock_guard<std::mutex> lock(self->formantsMutex);
        targetFormants = self->formants;
    }

    // A change of the number of formants takes the target values at once. A formant that starts at
    // zero would sound like a pole at the lowest frequency. With the same number, each formant moves
    // to its target in small steps, so a new estimate does not make a click.
    if (self->realFormants.size() != targetFormants.size()) {
        self->realFormants = targetFormants;
    }
    else {
        for (size_t i = 0; i < targetFormants.size(); ++i) {
            self->realFormants[i].frequency = 0.9 * self->realFormants[i].frequency + 0.1 * targetFormants[i].frequency;
            self->realFormants[i].bandwidth = 0.8 * self->realFormants[i].bandwidth + 0.2 * targetFormants[i].bandwidth;
        }
    }

    self->realFilterMutex.lock();
    self->realFilter = Synthesis::frequencyShiftFilter(self->realFormants, self->resampler.getInputRate(), self->realFilterShift);
    self->realFilterMutex.unlock();

    // Generate the audio by chunks of fixed size.
    while (self->surplus.size() < length) {
        self->generateAudio(1024);
    }

    // Move the appropriate amount of samples from the surplus bank to the output buffer.
    if (length > 0) {
        std::copy(self->surplus.begin(), std::next(self->surplus.begin(), length), output);
        self->surplus.erase(self->surplus.begin(), std::next(self->surplus.begin(), length));
    }
}

#endif // !WITHOUT_SYNTH
