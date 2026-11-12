#include "qpainterwrapper.h"
#include "../context/timings.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <QMutex>

constexpr double mapToUnit(double v, double min, double max) {
    return (v - min) / (max - min);
}

double QPainterWrapper::transformFrequency(double frequency, FrequencyScale scale)
{
    switch (scale) {
    case FrequencyScale::Linear:
        return frequency;
    case FrequencyScale::Logarithmic:
        return hz2log(frequency);
    case FrequencyScale::Mel:
        return hz2mel(frequency);
    case FrequencyScale::ERB:
        return hz2erb(frequency);
    default:
        return 0;
    }
}

double QPainterWrapper::inverseFrequency(double value, FrequencyScale scale)
{
    switch (scale) {
    case FrequencyScale::Linear:
        return value;
    case FrequencyScale::Logarithmic:
        return log2hz(value);
    case FrequencyScale::Mel:
        return mel2hz(value);
    case FrequencyScale::ERB:
        return erb2hz(value);
    default:
        return 0;
    }
}

double QPainterWrapper::mapTimeToX(double time, int width, double startTime, double endTime)
{
    return width * (time - startTime) / (endTime - startTime);
}

double QPainterWrapper::mapFrequencyToY(double frequency, int height, FrequencyScale scale, double minFrequency, double maxFrequency)
{
    const double vmin = transformFrequency(minFrequency, scale);
    const double vmax = transformFrequency(maxFrequency, scale);
    const double value = transformFrequency(frequency, scale);

    return height * (1 - (value - vmin) / (vmax - vmin));
}

double QPainterWrapper::mapYToFrequency(double y, int height, FrequencyScale scale, double minFrequency, double maxFrequency)
{
    const double vmin = transformFrequency(minFrequency, scale);
    const double vmax = transformFrequency(maxFrequency, scale);

    const double value = vmin + (1 - y / (double) height) * (vmax - vmin);

    return inverseFrequency(value, scale);
}

void QPainterWrapper::drawSpectrogram(SliceIterator begin, SliceIterator end)
{
    if (begin == end) {
        return;
    }

    // The texture holds 4096 slices. Show the newest slices if the view has more.
    if (end - begin > Gui::spectrogramColumns) {
        begin = end - Gui::spectrogramColumns;
    }

    constexpr int texWidth = Gui::spectrogramColumns;
    constexpr int texHeight = Gui::spectrogramRows;

    int &xOffset = mSpectrogramOffset;

    static std::array<GLint, texWidth> nffts;
    static std::array<GLfloat, texWidth> sampleRates;

    const int totalSlices = (int) (end - begin);
    const double firstTime = begin->first;
    const double lastTime = std::prev(end)->first;

    // Each slice covers the time around its stamp. Extend the first and the last slice by half the
    // slice spacing, so that the picture and the tracks use the same time scale.
    const double sliceSpacing = totalSlices > 1
        ? (lastTime - firstTime) / (double) (totalSlices - 1)
        : 0.0;
    const double sliceTimeStart = firstTime - 0.5 * sliceSpacing;
    const double sliceTimeEnd = lastTime + 0.5 * sliceSpacing;

    // Only store the slices that are not in the texture yet.
    double &lastTimeEnd = mSpectrogramLastTime;

    const auto firstNew = std::upper_bound(begin, end, lastTimeEnd,
            [](double t, const auto& slice) { return t < slice.first; });

    lastTimeEnd = lastTime;

    const int sliceCount = (int) (end - firstNew);

    if (sliceCount > 0) {
        // Split it into two chunks if it overlaps the texture width.
        int sliceCount1, sliceCount2;
        if ((xOffset % texWidth) + sliceCount > texWidth) {
            sliceCount1 = texWidth - (xOffset % texWidth);
            sliceCount2 = sliceCount - sliceCount1;
        }
        else {
            sliceCount1 = sliceCount;
            sliceCount2 = 0;
        }

        // The upload needs only as many rows as the longest new slice has values. A few more rows
        // of zeros stop old values from an earlier slice size from showing in the filter.
        int uploadRows = 1;
        for (auto it = firstNew; it != end; ++it) {
            uploadRows = std::max(uploadRows, (int) it->second.magnitudes.size());
        }
        uploadRows = std::min(uploadRows + 4, texHeight);

        static rpm::vector<GLfloat> data1, data2;

        data1.assign((size_t) sliceCount1 * uploadRows, 0.0f);
        data2.assign((size_t) sliceCount2 * uploadRows, 0.0f);

        int ioff = 0;
        for (auto it = firstNew; it != end; ++it, ++ioff) {
            const auto& slice = it->second;
            const auto& fftData = slice.magnitudes;
            const int nfft = std::min((int) fftData.size(), texHeight);
            const double sampleRate = slice.sampleRate;

            int index;
            if (ioff < sliceCount1) {
                index = (xOffset % texWidth) + ioff;
            }
            else {
                index = ioff - sliceCount1;
            }

            nffts[index] = nfft;
            sampleRates[index] = sampleRate;

            // Each chunk is a separate texture upload, so each chunk has its own row length.
            if (ioff < sliceCount1) {
                for (int k = 0; k < nfft; ++k) {
                    data1[(size_t) k * sliceCount1 + ioff] = fftData[k];
                }
            }
            else {
                for (int k = 0; k < nfft; ++k) {
                    data2[(size_t) k * sliceCount2 + (ioff - sliceCount1)] = fftData[k];
                }
            }
        }

        p->setSpectrogramFilter(mSpectrogramQuality, (float) mSpectrogramBlur);
        p->drawSpectrogram(
            xOffset,
            sliceCount1,
            sliceCount2,
            uploadRows,
            totalSlices,
            nffts,
            sampleRates,
            data1,
            data2,
            mFrequencyScale,
            mMinFrequency, mMaxFrequency,
            mMaxGain,
            mOptions.theme.colormap,
            sliceTimeStart,
            sliceTimeEnd,
            mTimeStart,
            mTimeEnd);
        
        xOffset += sliceCount;
    }
    else {
        static rpm::vector<GLfloat> emptyData(0);

        p->setSpectrogramFilter(mSpectrogramQuality, (float) mSpectrogramBlur);
        p->drawSpectrogram(
            xOffset,
            0,
            0,
            0,
            totalSlices,
            nffts,
            sampleRates,
            emptyData,
            emptyData,
            mFrequencyScale,
            mMinFrequency, mMaxFrequency,
            mMaxGain,
            mOptions.theme.colormap,
            sliceTimeStart,
            sliceTimeEnd,
            mTimeStart,
            mTimeEnd);
    }
}

void QPainterWrapper::drawSpectrogramImage(unsigned long long id, const rpm::vector<float> &data, int rows, int columns,
                                           const std::array<int, Gui::spectrogramColumns> &sliceSizes,
                                           const std::array<float, Gui::spectrogramColumns> &sampleRates,
                                           double timeStart, double timeEnd)
{
    if (columns <= 0 || rows <= 0) {
        return;
    }

    static rpm::vector<GLfloat> emptyData(0);
    const bool upload = id != mSpectrogramImageId;

    if (upload) {
        mSpectrogramImageId = id;
        mSpectrogramOffset = 0;
        mSpectrogramLastTime = -HUGE_VAL;
    }

    p->setSpectrogramFilter(mSpectrogramQuality, (float) mSpectrogramBlur);
    p->drawSpectrogram(
        mSpectrogramOffset,
        upload ? columns : 0,
        0,
        upload ? rows : 0,
        columns,
        sliceSizes,
        sampleRates,
        upload ? data : emptyData,
        emptyData,
        mFrequencyScale,
        mMinFrequency, mMaxFrequency,
        mMaxGain,
        mOptions.theme.colormap,
        timeStart,
        timeEnd,
        mTimeStart,
        mTimeEnd);

    if (upload) {
        mSpectrogramOffset = columns;
    }
}
