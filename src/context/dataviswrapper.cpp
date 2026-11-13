#include "dataviswrapper.h"
#include "config.h"
#include "scopetrace.h"
#include "session.h"
#include "viewrange.h"

#include <algorithm>
#include <cmath>

using namespace Main;

DataVisWrapper::DataVisWrapper()
    : mTotal(0),
      mFs(0),
      mGifFs(0),
      mGifEnd(0),
      mGifVersion(0),
      mWatchers(0)
{
}

void DataVisWrapper::pushAudio(const double *data, int count, double fs)
{
    if (count <= 0 || !(fs > 0)) return;

    std::lock_guard<std::mutex> lock(mMutex);

    if (fs != mFs || mRing.empty()) {
        mFs = fs;
        mRing.assign((size_t) std::max(64.0, std::round(fs * 0.4)), 0.0);
        mTotal = 0;
        mGif.clear();
    }

    const size_t size = mRing.size();
    for (int i = 0; i < count; ++i) {
        mRing[mTotal % size] = data[i];
        ++mTotal;
    }
}

void DataVisWrapper::setGif(const rpm::vector<double>& signal, double fs)
{
    std::lock_guard<std::mutex> lock(mMutex);

    mGif.assign(signal.begin(), signal.end());
    mGifFs = fs;
    mGifEnd = mTotal;
    ++mGifVersion;
}

void DataVisWrapper::changeWatchers(int, int delta)
{
    mWatchers += delta;
}

void DataVisWrapper::setReview(Config *config, Session *session)
{
    mConfig = config;
    mSession = session;
}

namespace {

// The axes of the plot, with no line. The plot also shows a text.
void fillAxes(Gui::PlotSnapshot &out, double windowMs)
{
    out.title = "Oscilloscope";
    out.xUnit = "ms";
    out.yUnit = "";
    out.logX = false;
    out.xMin = 0;
    out.xMax = std::max(windowMs, 1.0);
    out.yMin = -1.2;
    out.yMax = 1.2;
}

}

bool DataVisWrapper::fillPlot(int kind, const Gui::PlotRequest &request, Gui::PlotSnapshot &out)
{
    if (kind != Gui::PlotItem::Scope) return false;

    // The live sound is on the plot while the capture of the input device runs. Otherwise the plot shows the
    // stored sound at the moment that the user picked, or at the end of the view.
    const bool capturing = mConfig == nullptr || (!mConfig->isPaused() && mConfig->getSessionMode() == 0);

    out.series.clear();
    out.message.clear();

    if (capturing && fillLive(request, out)) return true;
    if (!capturing && mSession != nullptr && fillStored(request, out)) return true;

    // No sound yet. Draw the axes and say what to do.
    fillAxes(out, request.scopeWindowMs);
    const bool paused = mConfig != nullptr && mConfig->isPaused();
    out.message = paused ? QStringLiteral("Press play to see the sound of the microphone.")
                         : QStringLiteral("Waiting for sound.");
    out.version = paused ? 0xFFFF0001ull : 0xFFFF0002ull;
    return true;
}

bool DataVisWrapper::fillStored(const Gui::PlotRequest &request, Gui::PlotSnapshot &out)
{
    const double duration = mSession->duration();
    if (!(duration > 0.0)) return false;

    // The moment of the picture: the picked moment, or the end of the view, or the end of the sound.
    double time = mConfig != nullptr ? mConfig->getCursorTime() : std::nan("");
    if (!std::isfinite(time)) {
        time = duration;
        if (mConfig != nullptr && !mConfig->getViewLive()) {
            time = pastViewEnd(mConfig->getSessionMode() != 0, mConfig->getViewEnd(), mConfig->getViewTimeSpan(), duration);
        }
    }
    time = std::clamp(time, 0.0, duration);

    const double fs = (double) Session::sampleRate;
    const ScopeSpan span = scopeSpan(request.scopeWindowMs, fs, (long long) std::llround(0.4 * fs));
    rpm::vector<float> samples;
    if (!mSession->read(time - (double) span.needed / fs, time, samples)) return false;

    std::vector<double> input(samples.begin(), samples.end());
    const ScopeTrace trace = computeScopeTrace(input, span);
    if (!trace.ok) return false;

    Gui::PlotSeries sound;
    sound.name = "Input sound";
    sound.colorIndex = 0;
    sound.points.reserve((int) trace.y.size());
    for (size_t k = 0; k < trace.y.size(); ++k) sound.points.append(QPointF((double) k * 1000.0 / fs, trace.y[k]));
    out.series.push_back(std::move(sound));

    fillAxes(out, (double) (span.window - 1) * 1000.0 / fs);
    out.title = "Oscilloscope: stored sound at " + QString::number(time, 'f', 2) + " s";

    // The picture changes when the sound, the moment, or the length of the view changes.
    unsigned long long version = mSession->version();
    version = version * 1000003ull + (unsigned long long) std::llround(time * 1000.0);
    version = version * 1000003ull + (unsigned long long) span.window;
    out.version = version | (1ull << 63);
    return true;
}

bool DataVisWrapper::fillLive(const Gui::PlotRequest &request, Gui::PlotSnapshot &out)
{
    std::lock_guard<std::mutex> lock(mMutex);

    const size_t size = mRing.size();
    if (mFs <= 0 || size == 0) return false;

    // The number of samples in the view, and how far back the search for a trigger goes.
    const ScopeSpan span = scopeSpan(request.scopeWindowMs, mFs, (long long) size);
    const long long total = (long long) mTotal;
    if (total < span.needed) return false;

    // The newest samples, in the order of time.
    std::vector<double> input((size_t) span.needed);
    for (long long i = 0; i < span.needed; ++i) {
        input[(size_t) i] = mRing[(size_t) ((total - span.needed + i) % (long long) size)];
    }
    const ScopeTrace trace = computeScopeTrace(input, span);
    if (!trace.ok) return false;

    // The first sample of the view, counted from the start of the capture.
    const long long start = total - span.needed + trace.start;

    Gui::PlotSeries sound;
    sound.name = "Input sound";
    sound.colorIndex = 0;
    sound.points.reserve((int) trace.y.size());
    for (size_t k = 0; k < trace.y.size(); ++k) sound.points.append(QPointF((double) k * 1000.0 / mFs, trace.y[k]));
    out.series.push_back(std::move(sound));

    if (request.showGif && !mGif.empty() && mGifFs > 0) {
        // Place each sample of the estimate at its time in the view.
        const double n = (double) mGif.size();
        double gifPeak = 1e-9;
        std::vector<QPointF> points;
        for (size_t j = 0; j < mGif.size(); ++j) {
            const double index = (double) mGifEnd - (n - 1.0 - (double) j) * mFs / mGifFs;
            const double relative = index - (double) start;
            if (relative < 0 || relative > (double) (span.window - 1)) continue;
            points.emplace_back(relative * 1000.0 / mFs, mGif[j]);
            gifPeak = std::max(gifPeak, std::fabs(mGif[j]));
        }

        Gui::PlotSeries gif;
        gif.name = "Glottal flow estimate";
        gif.colorIndex = 1;
        for (const QPointF &p : points) gif.points.append(QPointF(p.x(), p.y() / gifPeak));
        out.series.push_back(std::move(gif));
    }

    fillAxes(out, (double) (span.window - 1) * 1000.0 / mFs);
    out.version = (unsigned long long) total + (mGifVersion << 40);
    return true;
}
