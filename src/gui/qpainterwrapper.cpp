#include "qpainterwrapper.h"
#include <Eigen/src/Core/Array.h>
#include <QPainterPath>
#include <iostream>

QPainterWrapper::QPainterWrapper(Gui::CanvasRenderer *p)
    : p(p),
      mTimeStart(0.0),
      mTimeEnd(5.0),
      mFrequencyScale(FrequencyScale::Mel),
      mMinFrequency(60),
      mMaxFrequency(8000),
      mMaxGain(0),
      mSpectrogramQuality(1),
      mSpectrogramBlur(0.33)
{
}

QRect QPainterWrapper::viewport() const
{
    return p->viewport();
}

void QPainterWrapper::setZoom(double scale)
{
    p->setZoomScale(scale);
}

void QPainterWrapper::setTimeRange(double start, double end)
{
    mTimeStart = start;
    mTimeEnd = end;
}

void QPainterWrapper::setFrequencyScale(FrequencyScale scale)
{
    mFrequencyScale = scale;
}

void QPainterWrapper::setMinFrequency(double minFrequency)
{
    mMinFrequency = minFrequency;
}

void QPainterWrapper::setMaxFrequency(double maxFrequency)
{
    mMaxFrequency = maxFrequency;
}

void QPainterWrapper::setMaxGain(double maxGain)
{
    mMaxGain = maxGain;
}

void QPainterWrapper::setSpectrogramFilter(int quality, double blur)
{
    mSpectrogramQuality = quality;
    mSpectrogramBlur = blur;
}

double QPainterWrapper::transformFrequency(double frequency)
{
    return transformFrequency(frequency, mFrequencyScale);
}

double QPainterWrapper::inverseFrequency(double value)
{
    return inverseFrequency(value, mFrequencyScale);
}

double QPainterWrapper::mapTimeToX(double time)
{
    return mapTimeToX(time, p->viewport().width(), mTimeStart, mTimeEnd);
}

double QPainterWrapper::mapFrequencyToY(double frequency)
{
    return mapFrequencyToY(frequency, p->viewport().height(), mFrequencyScale, mMinFrequency, mMaxFrequency);
}

const QVector<QRgb>& QPainterWrapper::defaultColorMap()
{
    return cmap;
}

void QPainterWrapper::setOptions(const Gui::RenderOptions &options)
{
    mOptions = options;
}
