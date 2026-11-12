#include "spectrogram.h"
#include "../timings.h"
#include <iostream>
#include <qnamespace.h>

using namespace Main::View;

Spectrogram::Spectrogram()
{
}

Spectrogram::~Spectrogram()
{
}

void Spectrogram::render(QPainterWrapper *painter, Config *config, DataStore *dataStore)
{
    const double realTimeEnd = dataStore->getRealTime();

    // Remove the old data about once for each second. Removing from the front of a vector moves
    // every element after it, and the write lock stops the analysis thread while it runs.
    static double lastTrimTime = -1e9;
    if (realTimeEnd - lastTrimTime >= 1.0 || realTimeEnd < lastTrimTime) {
        lastTrimTime = realTimeEnd;

        dataStore->beginWrite();
        constexpr double keepDuration = 50.0;
        const double keepTimeStart = realTimeEnd - keepDuration;
        dataStore->getSpectrogram().remove_before(keepTimeStart);
        dataStore->getSoundTrack().remove_before(keepTimeStart);
        dataStore->getGifTrack().remove_before(keepTimeStart);
        // The pitch and formant tracks grew without limit before.
        dataStore->getPitchTrack().remove_before(keepTimeStart);
        for (int i = 0; i < dataStore->getFormantTrackCount(); ++i) {
            dataStore->getFormantTrack(i).remove_before(keepTimeStart);
        }
        dataStore->endWrite();
    }

    dataStore->beginRead();

    auto& spectrogram = dataStore->getSpectrogram();
    auto& pitchTrack = dataStore->getPitchTrack();

    const double viewDuration = config->getViewTimeSpan();
    const double timeDelay = 50.0 / 1000.0;
    const double timeEnd = realTimeEnd - timeDelay;
    const double timeStart = timeEnd - viewDuration;

    painter->setTimeRange(timeStart, timeEnd);
  
    if (config->getViewShowSpectrogram()) {
        // Pass the range, not a copy. A copy of all visible slices is several megabytes for each frame.
        painter->drawSpectrogram(
            spectrogram.lower_bound(timeStart), spectrogram.upper_bound(timeEnd));
    }
    
    const Gui::RenderOptions &options = painter->options();
    std::vector<Gui::HudTrack> hud;

    // The values of a track in the window of the statistics.
    const double statsWindow = config->optionDouble("statsWindow");
    const double statsStart = statsWindow > 0 ? timeEnd - statsWindow : timeStart;

    auto collect = [&](const char *name, const QColor &color, bool isPitch, auto &track) {
        Gui::HudTrack t;
        t.name = name;
        t.color = color;
        t.isPitch = isPitch;
        for (auto it = track.lower_bound(statsStart); it != track.upper_bound(timeEnd); ++it) {
            ++t.frames;
            if (it->second.has_value()) {
                t.values.push_back(*it->second);
            }
        }
        hud.push_back(std::move(t));
    };

    if (config->getViewShowPitch()) {
        painter->drawFrequencyTrack(
                pitchTrack.lower_bound(timeStart),
                pitchTrack.upper_bound(timeEnd),
                options.theme.pitch);
        collect("Pitch", options.theme.pitch, true, pitchTrack);
    }

    if (config->getViewShowFormants()) {
        const int formantCount = config->getViewFormantCount();

        for (int i = 0; i < formantCount; ++i) {
            auto& fi = dataStore->getFormantTrack(i);
            const QColor color = options.theme.formant(i);

            painter->drawFrequencyTrack(
                    fi.lower_bound(timeStart),
                    fi.upper_bound(timeEnd),
                    color);
            collect(("F" + std::to_string(i + 1)).c_str(), color, false, fi);
        }
    }

    painter->drawTimeAxis();
    painter->drawFrequencyScale();
    painter->drawHud(hud);

    dataStore->endRead();
}
