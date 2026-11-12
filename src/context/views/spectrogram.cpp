#include "spectrogram.h"
#include "../timings.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <qnamespace.h>

using namespace Main::View;

namespace {

using EntryVector = OptionalTimeTrack<double>::vector_type;

// The most entries of a track that the view draws. A longer range goes through a reduction, so a
// view of many hours has the same cost as a short view. A graph cannot show more points than it
// has pixels, so the limit follows the width of the graph.
constexpr size_t unlimited = (size_t) -1;

struct Picked {
    OptionalTimeTrack<double>::const_iterator begin, end;

    // The number of entries and of entries with a value in the range, before the reduction.
    size_t total = 0;
    size_t valid = 0;
};

// The entries of a track from t0 to t1. A long range becomes groups of entries. The size of a group
// is a power of two, and a group starts at a place that is a multiple of its size in the track. A
// group so keeps its entries while the range moves, and the picture does not shimmer. The value of
// a group is the median of up to eight entries of the group, and it is empty when most of these
// have no value. The reduction reads a fixed number of entries, not all of them. The storage keeps
// the groups, so it must stay alive while the caller uses the result. The count of the entries with
// a value reads all entries, so only the callers that need it ask for it.
Picked pick(OptionalTimeTrack<double> &track, double t0, double t1, EntryVector &storage, size_t maxPoints, bool count)
{
    auto b = track.lower_bound(t0);
    auto e = track.upper_bound(t1);

    Picked r;
    r.begin = b;
    r.end = e;
    r.total = (size_t) (e - b);
    if (count) {
        for (auto it = b; it != e; ++it) {
            if (it->second.has_value()) ++r.valid;
        }
    }
    if (r.total <= maxPoints) return r;

    const auto first = track.lower_bound(-1e300);
    const long long from = b - first;
    const long long to = e - first;

    long long group = 1;
    while ((to - from) / group > (long long) maxPoints) group *= 2;
    const long long stride = std::max(1LL, group / 8);

    storage.clear();
    std::vector<double> values;
    for (long long start = from / group * group; start < to; start += group) {
        const long long low = std::max(start, from);
        const long long high = std::min(start + group, to);

        values.clear();
        long long samples = 0;
        for (long long k = (low + stride - 1) / stride * stride; k < high; k += stride) {
            ++samples;
            const auto &entry = *(first + k);
            if (entry.second.has_value()) values.push_back(*entry.second);
        }

        const double time = (first + std::clamp(start + group / 2, low, high - 1))->first;
        if (samples > 0 && values.size() * 3 >= (size_t) samples) {
            auto middle = values.begin() + values.size() / 2;
            std::nth_element(values.begin(), middle, values.end());
            storage.emplace_back(time, *middle);
        }
        else {
            storage.emplace_back(time, std::nullopt);
        }
    }

    r.begin = storage.cbegin();
    r.end = storage.cend();
    return r;
}

}

Spectrogram::Spectrogram()
{
}

Spectrogram::~Spectrogram()
{
}

void Spectrogram::render(QPainterWrapper *painter, Config *config, DataStore *dataStore)
{
    const double realTimeEnd = dataStore->getRealTime();
    const bool fileMode = config->getSessionMode() == 1;
    Session &session = dataStore->session();

    // Remove the old live slices about once for each second. Removing from the front of a vector
    // moves every element after it, and the write lock stops the analysis thread while it runs.
    // The pitch track and the formant tracks keep a whole session, because the overview and the
    // views of a past time use them.
    static double lastTrimTime = -1e9;
    if (!fileMode && (realTimeEnd - lastTrimTime >= 1.0 || realTimeEnd < lastTrimTime)) {
        lastTrimTime = realTimeEnd;

        dataStore->beginWrite();
        constexpr double keepSpectrogram = 50.0;
        dataStore->getSpectrogram().remove_before(realTimeEnd - keepSpectrogram);
        dataStore->getSoundTrack().remove_before(realTimeEnd - keepSpectrogram);
        dataStore->getGifTrack().remove_before(realTimeEnd - keepSpectrogram);

        const double keepTracks = realTimeEnd - Session::maxDuration;
        dataStore->getPitchTrack().remove_before(keepTracks);
        for (int i = 0; i < dataStore->getFormantTrackCount(); ++i) {
            dataStore->getFormantTrack(i).remove_before(keepTracks);
        }
        dataStore->endWrite();
    }

    const double viewDuration = config->getViewTimeSpan();
    const double liveStep = std::max(config->getAnalysisGranularity(), 1.0) / 1000.0;

    // The end of the view. The live view follows the newest sound with a short delay. A view of
    // a past time ends at the time that the user chose.
    const bool live = !fileMode && config->getViewLive();
    double timeEnd;
    if (live) {
        // The graph starts at time zero at the left, and it fills towards the right. After the
        // sound fills the width, the graph scrolls.
        const double timeDelay = 50.0 / 1000.0;
        timeEnd = std::max(realTimeEnd - timeDelay, viewDuration);
    }
    else {
        const double length = session.duration();
        timeEnd = fileMode ? std::clamp(config->getViewEnd(), viewDuration, std::max(length, viewDuration))
                           : std::clamp(config->getViewEnd(), std::min(viewDuration, length), length);
    }
    const double timeStart = timeEnd - viewDuration;

    // The texture of the live spectrogram holds 4096 slices. A longer view, and each view of a
    // past time, takes the spectrogram of the stored sound.
    const bool useReview = !live || viewDuration / liveStep > 3500.0;

    dataStore->beginRead();

    auto& spectrogram = dataStore->getSpectrogram();
    auto& pitchTrack = dataStore->getPitchTrack();

    painter->setTimeRange(timeStart, timeEnd);
    const int trackCount = (config->getViewShowPitch() ? 1 : 0)
                           + (config->getViewShowFormants() ? config->getViewFormantCount() : 0);
    painter->updateLayout(trackCount);

    // A click on the graph picks a moment. The click is a fraction of the item, and the layout gives its time.
    double clickX, clickY;
    if (config->takeCursorClick(clickX, clickY)) {
        const double pixelX = clickX * painter->viewport().width();
        const double pixelY = clickY * painter->viewport().height();
        if (painter->plotContains(pixelX, pixelY)) {
            config->setCursorTime(std::max(0.0, painter->timeAtX(pixelX)));
        }
    }

    painter->beginPlot();

    if (config->getViewShowSpectrogram()) {
        if (useReview) {
            if (!mReview) mReview = std::make_unique<ReviewSpectrogram>(&session);

            // The end of the span is a multiple of the column width. The columns then keep their
            // places when the live view moves, and the picture does not shimmer.
            ReviewRequest request;
            request.columns = std::clamp((int) std::ceil(viewDuration / liveStep), 1, 4096);
            const double columnWidth = viewDuration / request.columns;
            request.t1 = std::ceil(timeEnd / columnWidth) * columnWidth;
            request.t0 = request.t1 - viewDuration;
            request.sampleRate = (int) std::lround(2.0 * config->getViewMaxFrequency());
            request.fftSamples = config->getViewFFTSize();
            const double windowSeconds = (double) request.fftSamples / std::max(request.sampleRate, 1);
            if (request.t1 + 0.5 * windowSeconds >= session.duration()) request.version = session.version();

            // A change of the settings goes out at once. A move of the span goes out at a limited rate.
            const auto now = std::chrono::steady_clock::now();
            const bool sameSettings = mHasRequest
                && mLastRequest.columns == request.columns
                && mLastRequest.sampleRate == request.sampleRate
                && mLastRequest.fftSamples == request.fftSamples
                && mLastRequest.t1 - mLastRequest.t0 == request.t1 - request.t0;
            const auto interval = live ? std::chrono::milliseconds(400) : std::chrono::milliseconds(120);
            if (!mHasRequest || !(request == mLastRequest)) {
                if (!sameSettings || now - mLastRequestTime >= interval) {
                    mReview->request(request);
                    mLastRequest = request;
                    mHasRequest = true;
                    mLastRequestTime = now;
                }
            }

            // Draw the newest result. It stays in the texture until a new result comes.
            const auto result = mReview->latest();
            if (result && result->columns > 0) {
                painter->drawSpectrogramImage(result->id, result->data, result->rows, result->columns,
                    result->sliceSizes, result->sampleRates, result->request.t0, result->request.t1);
                mTextureSource = result->id;
            }
        }
        else {
            if (mTextureSource != 0) {
                painter->invalidateSpectrogram();
                mTextureSource = 0;
                mHasRequest = false;
            }

            // Pass the range, not a copy. A copy of all visible slices is several megabytes for each frame.
            painter->drawSpectrogram(
                spectrogram.lower_bound(timeStart), spectrogram.upper_bound(timeEnd));
        }
    }

    const Gui::RenderOptions &options = painter->options();
    std::vector<Gui::HudTrack> hud;
    const size_t drawLimit = (size_t) std::clamp(painter->viewport().width(), 400, 3000);

    // The values of a track in the window of the statistics.
    const double statsWindow = config->optionDouble("statsWindow");
    const double statsStart = statsWindow > 0 ? timeEnd - statsWindow : timeStart;

    // The values for the statistics come from all entries of the window, not from the reduced
    // entries of the drawing. A pass over all entries is costly for a long window, so the values
    // are computed about four times each second, and only while the table of the statistics is on.
    const auto now = std::chrono::steady_clock::now();
    const bool statsDue = options.showStats && (mHudCache.empty() || now - mHudTime >= std::chrono::milliseconds(250));
    auto collect = [&](const char *name, const QColor &color, bool isPitch, OptionalTimeTrack<double> &track) {
        if (options.showStats && !statsDue && hud.size() < mHudCache.size()) {
            hud.push_back(mHudCache[hud.size()]);
            return;
        }

        Gui::HudTrack t;
        t.name = name;
        t.color = color;
        t.isPitch = isPitch;
        if (options.showStats) {
            EntryVector storage;
            const Picked all = pick(track, statsStart, timeEnd, storage, unlimited, true);
            t.frames = all.total;
            t.count = all.valid;
            for (auto it = all.begin; it != all.end; ++it) {
                if (it->second.has_value()) {
                    t.values.push_back(*it->second);
                }
            }
        }
        hud.push_back(std::move(t));
    };

    if (config->getViewShowPitch()) {
        EntryVector storage;
        const Picked range = pick(pitchTrack, timeStart, timeEnd, storage, drawLimit, false);
        painter->drawFrequencyTrack(range.begin, range.end, options.theme.pitch);
        collect("Pitch", options.theme.pitch, true, pitchTrack);
    }

    if (config->getViewShowFormants()) {
        const int formantCount = config->getViewFormantCount();

        for (int i = 0; i < formantCount; ++i) {
            auto& fi = dataStore->getFormantTrack(i);
            const QColor color = options.theme.formant(i);

            EntryVector storage;
            const Picked range = pick(fi, timeStart, timeEnd, storage, drawLimit, false);
            painter->drawFrequencyTrack(range.begin, range.end, color);
            collect(("F" + std::to_string(i + 1)).c_str(), color, false, fi);
        }
    }

    if (statsDue) {
        mHudCache = hud;
        mHudTime = now;
    }

    // The values of the tracks at the picked moment. A value is the entry nearest to the moment. A
    // track with no entry near the moment has no label.
    std::vector<Gui::CursorValue> cursorValues;
    const double cursor = config->getCursorTime();
    const bool cursorShown = !std::isnan(cursor) && cursor >= timeStart && cursor <= timeEnd;
    if (cursorShown) {
        const double tolerance = 1.5 * liveStep + 0.02;
        auto sample = [&](const char *name, const QColor &color, bool isPitch, OptionalTimeTrack<double> &track) {
            const auto first = track.lower_bound(-1e300);
            const auto last = track.upper_bound(1e300);
            const auto after = track.lower_bound(cursor);

            const OptionalTimeTrack<double>::const_iterator candidates[2] = { after, after == first ? after : after - 1 };
            double best = tolerance;
            bool found = false;
            double frequency = 0.0;
            for (const auto &it : candidates) {
                if (it == last || !it->second.has_value()) continue;
                const double distance = std::fabs(it->first - cursor);
                if (distance <= best) {
                    best = distance;
                    frequency = *it->second;
                    found = true;
                }
            }
            if (found) {
                Gui::CursorValue v;
                v.name = name;
                v.color = color;
                v.isPitch = isPitch;
                v.frequency = frequency;
                cursorValues.push_back(std::move(v));
            }
        };

        if (config->getViewShowPitch()) sample("Pitch", options.theme.pitch, true, pitchTrack);
        if (config->getViewShowFormants()) {
            for (int i = 0; i < config->getViewFormantCount(); ++i) {
                sample(("F" + std::to_string(i + 1)).c_str(), options.theme.formant(i), false, dataStore->getFormantTrack(i));
            }
        }
    }

    painter->endPlot();
    painter->drawTimeAxis();
    painter->drawFrequencyScale();
    if (cursorShown) painter->drawCursor(cursor, cursorValues);
    painter->drawHud(hud);

    dataStore->endRead();
}
