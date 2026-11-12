#include "spectrogram.h"
#include "../timings.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <qnamespace.h>

using namespace Main::View;

namespace {

using EntryVector = OptionalTimeTrack<double>::vector_type;

// The most entries of a track that the view draws and measures. A longer range goes through a
// reduction, so a view of many hours has the same cost as a short view.
constexpr size_t maxPoints = 6000;

struct Picked {
    OptionalTimeTrack<double>::const_iterator begin, end;

    // The number of entries and of entries with a value in the range, before the reduction.
    size_t total = 0;
    size_t valid = 0;
};

// The entries of a track from t0 to t1. A long range becomes groups of entries. A group has the
// median of its values, and it has no value when most of its entries have none. The storage keeps
// the groups, so it must stay alive while the caller uses the result.
Picked pick(OptionalTimeTrack<double> &track, double t0, double t1, EntryVector &storage)
{
    auto b = track.lower_bound(t0);
    auto e = track.upper_bound(t1);

    Picked r;
    r.begin = b;
    r.end = e;
    r.total = (size_t) (e - b);
    for (auto it = b; it != e; ++it) {
        if (it->second.has_value()) ++r.valid;
    }
    if (r.total <= maxPoints) return r;

    const size_t group = (r.total + maxPoints - 1) / maxPoints;
    storage.clear();
    std::vector<double> values;
    for (auto it = b; it != e;) {
        const auto stop = (size_t) (e - it) > group ? it + group : e;
        values.clear();
        for (auto j = it; j != stop; ++j) {
            if (j->second.has_value()) values.push_back(*j->second);
        }

        const double time = 0.5 * (it->first + std::prev(stop)->first);
        if (values.size() * 3 >= (size_t) (stop - it)) {
            auto middle = values.begin() + values.size() / 2;
            std::nth_element(values.begin(), middle, values.end());
            storage.emplace_back(time, *middle);
        }
        else {
            storage.emplace_back(time, std::nullopt);
        }
        it = stop;
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
        const double timeDelay = 50.0 / 1000.0;
        timeEnd = realTimeEnd - timeDelay;
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

    // The values of a track in the window of the statistics.
    const double statsWindow = config->optionDouble("statsWindow");
    const double statsStart = statsWindow > 0 ? timeEnd - statsWindow : timeStart;

    auto collect = [&](const char *name, const QColor &color, bool isPitch, OptionalTimeTrack<double> &track,
                       const Picked *visible) {
        EntryVector storage;
        Picked own;
        if (visible == nullptr || statsStart != timeStart) {
            own = pick(track, statsStart, timeEnd, storage);
            visible = &own;
        }

        Gui::HudTrack t;
        t.name = name;
        t.color = color;
        t.isPitch = isPitch;
        t.frames = visible->total;
        t.count = visible->valid;
        for (auto it = visible->begin; it != visible->end; ++it) {
            if (it->second.has_value()) {
                t.values.push_back(*it->second);
            }
        }
        hud.push_back(std::move(t));
    };

    if (config->getViewShowPitch()) {
        EntryVector storage;
        const Picked range = pick(pitchTrack, timeStart, timeEnd, storage);
        painter->drawFrequencyTrack(range.begin, range.end, options.theme.pitch);
        collect("Pitch", options.theme.pitch, true, pitchTrack, &range);
    }

    if (config->getViewShowFormants()) {
        const int formantCount = config->getViewFormantCount();

        for (int i = 0; i < formantCount; ++i) {
            auto& fi = dataStore->getFormantTrack(i);
            const QColor color = options.theme.formant(i);

            EntryVector storage;
            const Picked range = pick(fi, timeStart, timeEnd, storage);
            painter->drawFrequencyTrack(range.begin, range.end, color);
            collect(("F" + std::to_string(i + 1)).c_str(), color, false, fi, &range);
        }
    }

    painter->drawTimeAxis();
    painter->drawFrequencyScale();
    painter->drawHud(hud);

    dataStore->endRead();
}
