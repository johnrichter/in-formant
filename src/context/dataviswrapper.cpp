#include "dataviswrapper.h"

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

bool DataVisWrapper::fillPlot(int kind, const Gui::PlotRequest &request, Gui::PlotSnapshot &out)
{
    if (kind != Gui::PlotItem::Scope) return false;

    std::lock_guard<std::mutex> lock(mMutex);

    const size_t size = mRing.size();
    if (mFs <= 0 || size == 0) return false;

    // The number of samples in the view, and how far back the search for a trigger goes.
    const long long window = std::clamp<long long>(std::llround(request.scopeWindowMs * mFs / 1000.0), 32, (long long) size / 2);
    const long long maxBack = std::min<long long>(std::llround(0.025 * mFs), (long long) size - window - 1);
    const long long total = (long long) mTotal;
    if (total < window + maxBack + 8) return false;

    auto at = [&](long long index) { return mRing[(size_t) (index % (long long) size)]; };

    // The offset of the sound. The trigger and the view remove it.
    double dc = 0;
    for (long long i = total - window - maxBack; i < total; ++i) dc += at(i);
    dc /= (double) (window + maxBack);

    // The view starts at the last rising zero crossing before the newest window. A periodic sound
    // then has the same shape in each frame, like the trigger of an oscilloscope.
    double rough = 0;
    for (long long i = total - window - maxBack; i < total; ++i) rough = std::max(rough, std::fabs(at(i) - dc));
    const double threshold = 0.05 * rough;

    long long start = total - window;
    if (rough > 1e-6) {
        for (long long c = total - window; c > total - window - maxBack; --c) {
            if (at(c - 1) - dc < 0 && at(c) - dc >= 0 && at(c + 3) - dc > threshold) {
                start = c;
                break;
            }
        }
    }

    double peak = 1e-9;
    for (long long i = start; i < start + window; ++i) peak = std::max(peak, std::fabs(at(i) - dc));

    Gui::PlotSeries sound;
    sound.name = "Input sound";
    sound.colorIndex = 0;
    sound.points.reserve((int) window);
    for (long long k = 0; k < window; ++k) {
        sound.points.append(QPointF(k * 1000.0 / mFs, (at(start + k) - dc) / peak));
    }

    out.series.clear();
    out.series.push_back(std::move(sound));

    if (request.showGif && !mGif.empty() && mGifFs > 0) {
        // Place each sample of the estimate at its time in the view.
        const double n = (double) mGif.size();
        double gifPeak = 1e-9;
        std::vector<QPointF> points;
        for (size_t j = 0; j < mGif.size(); ++j) {
            const double index = (double) mGifEnd - (n - 1.0 - (double) j) * mFs / mGifFs;
            const double relative = index - (double) start;
            if (relative < 0 || relative > (double) (window - 1)) continue;
            points.emplace_back(relative * 1000.0 / mFs, mGif[j]);
            gifPeak = std::max(gifPeak, std::fabs(mGif[j]));
        }

        Gui::PlotSeries gif;
        gif.name = "Glottal flow estimate";
        gif.colorIndex = 1;
        for (const QPointF &p : points) gif.points.append(QPointF(p.x(), p.y() / gifPeak));
        out.series.push_back(std::move(gif));
    }

    out.title = "Oscilloscope";
    out.xUnit = "ms";
    out.yUnit = "";
    out.logX = false;
    out.xMin = 0;
    out.xMax = (double) (window - 1) * 1000.0 / mFs;
    out.yMin = -1.2;
    out.yMax = 1.2;
    out.version = (unsigned long long) total + (mGifVersion << 40);
    return true;
}
