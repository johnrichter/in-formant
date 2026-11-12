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
    
    if (config->getViewShowPitch()) {
        painter->drawFrequencyTrack(
                pitchTrack.lower_bound(timeStart),
                pitchTrack.upper_bound(timeEnd),
                3, Qt::cyan);
    }

    if (config->getViewShowFormants()) {
        const int formantCount = config->getViewFormantCount();

        for (int i = 0; i < formantCount; ++i) {
            auto& fi = dataStore->getFormantTrack(i);
            const auto [r, g, b] = config->getViewFormantColor(i);
        
            painter->drawFrequencyTrack(
                    fi.lower_bound(timeStart),
                    fi.upper_bound(timeEnd),
                    3, QColor::fromRgbF(r, g, b));
        }
    }

    painter->drawTimeAxis();
    painter->drawFrequencyScale();

    dataStore->endRead();
}
