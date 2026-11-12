#ifndef WITHOUT_SYNTH

#include "synthwrapper.h"
#include <algorithm>
#include <cfloat>
#include <cmath>

using namespace Main;

SynthWrapper::SynthWrapper()
    : mEnabled(false),
      mNoiseGain(0.3),
      mGlotGain(1.0),
      mGlotPitch(170),
      mGlotRd(1.7),
      mGlotTc(1.0),
      mFilterShift(1.0),
      mVoiced(true),
      mFollowPitch(false),
      mFollowFormants(true),
      mFilterResponseDbMin(0),
      mFilterResponseDbMax(0),
      mSourceSpectrumDbMin(DBL_MAX),
      mSourceSpectrumDbMax(-DBL_MAX),
      mWatchers(0),
      mVersion(0)
{
}

bool SynthWrapper::enabled() const
{
    return mEnabled;
}

double SynthWrapper::noiseGain() const
{
    return mNoiseGain;
}

double SynthWrapper::glotGain() const
{
    return mGlotGain;
}

double SynthWrapper::glotPitch() const
{
    return mGlotPitch;
}

double SynthWrapper::glotRd() const
{
    return mGlotRd;
}

double SynthWrapper::glotTc() const
{
    return mGlotTc;
}

double SynthWrapper::filterShift() const
{
    return mFilterShift;
}

bool SynthWrapper::voiced() const
{
    return mVoiced;
}

bool SynthWrapper::followPitch() const
{
    return mFollowPitch;
}

bool SynthWrapper::followFormants() const
{
    return mFollowFormants;
}

void SynthWrapper::setEnabled(bool b) 
{
    mEnabled = b;
    emit enabledChanged(b);
}

void SynthWrapper::setNoiseGain(double g)
{
    mNoiseGain = g;
    emit noiseGainChanged(g);
}

void SynthWrapper::setGlotGain(double g)
{
    mGlotGain = g;
    emit glotGainChanged(g);
}

void SynthWrapper::setGlotPitch(double pitch)
{
    mGlotPitch = pitch;
    emit glotPitchChanged(pitch);
}

void SynthWrapper::setGlotRd(double Rd)
{
    mGlotRd = Rd;
    emit glotRdChanged(Rd);
}

void SynthWrapper::setGlotTc(double Tc)
{
    mGlotTc = Tc;
    emit glotTcChanged(Tc);
}

void SynthWrapper::setFilterShift(double shf)
{
    mFilterShift = shf;
    emit filterShiftChanged(shf);
}

void SynthWrapper::setVoiced(bool v)
{
    mVoiced = v;
    emit voicedChanged(v);
}

void SynthWrapper::setFollowPitch(bool b)
{
    mFollowPitch = b;
    emit followPitchChanged(b);
}

void SynthWrapper::setFollowFormants(bool b)
{
    mFollowFormants = b;
    emit followFormantsChanged(b);
}

void SynthWrapper::setFilterResponse(const rpm::vector<double>& frequencies, const rpm::vector<double>& magnitudes)
{
    QMutexLocker lock(&mFilterResponseMutex);

    mFilterResponse.resize(frequencies.size());
    double dBmin = DBL_MAX;
    double dBmax = -DBL_MAX;
    for (int k = 0; k < frequencies.size(); ++k) {
        const double dB = std::max(-200.0, 20.0 * log10(std::max(magnitudes[k], 1e-12)));
        mFilterResponse[k] = QPointF(frequencies[k], dB);
        if (dB < dBmin)
            dBmin = dB;
        if (dB > dBmax)
            dBmax = dB;
    }
    mFilterResponseDbMin = std::max(dBmin, dBmax - 60.0);

    if (mFilterResponseDbMax == 0) {
        mFilterResponseDbMax = dBmax;
    }
    else {
        if (dBmax > mFilterResponseDbMax) {
            mFilterResponseDbMax = 0.2 * mFilterResponseDbMax + 0.8 * dBmax;
        }
        else {
            mFilterResponseDbMax = 0.95 * mFilterResponseDbMax + 0.05 * dBmax;
        }
    }

    ++mVersion;
}

void SynthWrapper::setSource(const rpm::vector<double>& signal, double fs)
{
    QMutexLocker lock(&mSourceMutex);
    
    mSource.resize(signal.size());
    for (int k = 0; k < signal.size(); ++k) {
        mSource[k] = QPointF((k * 1000) / fs, signal[k]);
    }
    ++mVersion;
}

void SynthWrapper::setSourceSpectrum(const rpm::vector<double>& frequencies, const rpm::vector<double>& magnitudes)
{
    QMutexLocker lock(&mSourceSpectrumMutex);

    mSourceSpectrum.resize(frequencies.size());
    double dBmin = DBL_MAX;
    double dBmax = -DBL_MAX;
    for (int k = 0; k < frequencies.size(); ++k) {
        const double dB = std::max(-200.0, 20.0 * log10(std::max(magnitudes[k], 1e-12)));
        mSourceSpectrum[k] = QPointF(frequencies[k], dB);
        if (dB < dBmin)
            dBmin = dB;
        if (dB > dBmax)
            dBmax = dB;
    }
    mSourceSpectrumDbMin = -60;
    mSourceSpectrumDbMax = std::max(mSourceSpectrumDbMax, dBmax);
    ++mVersion;
}

void SynthWrapper::changeWatchers(int, int delta)
{
    mWatchers += delta;
}

bool SynthWrapper::fillPlot(int kind, const Gui::PlotRequest &, Gui::PlotSnapshot &out)
{
    out.series.clear();
    out.version = mVersion.load();

    Gui::PlotSeries series;
    series.colorIndex = 0;

    if (kind == Gui::PlotItem::Filter) {
        QMutexLocker lock(&mFilterResponseMutex);
        if (mFilterResponse.isEmpty()) return false;

        series.points = mFilterResponse;
        out.title = "Synthesis filter";
        out.xUnit = "Hz";
        out.yUnit = "dB";
        out.logX = true;
        out.xMin = 100;
        out.xMax = mFilterResponse.back().x();
        out.yMin = std::floor(mFilterResponseDbMin / 10) * 10;
        out.yMax = std::max(mFilterResponseDbMax, out.yMin + 10);
    }
    else if (kind == Gui::PlotItem::Source) {
        QMutexLocker lock(&mSourceMutex);
        if (mSource.size() < 2) return false;

        series.points = mSource;
        out.title = "Synthesized source";
        out.xUnit = "ms";
        out.yUnit = "";
        out.logX = false;
        out.xMin = mSource.front().x();
        out.xMax = mSource.back().x();
        out.yMin = -1.5;
        out.yMax = 1.5;
    }
    else if (kind == Gui::PlotItem::Spectrum) {
        QMutexLocker lock(&mSourceSpectrumMutex);
        if (mSourceSpectrum.isEmpty()) return false;

        series.points = mSourceSpectrum;
        out.title = "Synthesized source spectrum";
        out.xUnit = "Hz";
        out.yUnit = "dB";
        out.logX = true;
        out.xMin = 100;
        out.xMax = mSourceSpectrum.back().x();
        out.yMin = std::floor(mSourceSpectrumDbMin / 10) * 10;
        out.yMax = std::max(std::ceil(mSourceSpectrumDbMax / 10) * 10, out.yMin + 10);
    }
    else {
        return false;
    }

    out.series.push_back(std::move(series));
    return true;
}

#endif // !WITHOUT_SYNTH
