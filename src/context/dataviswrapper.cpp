#include "dataviswrapper.h"

#include <QSplineSeries>

using namespace Main;

DataVisWrapper::DataVisWrapper()
{
}

void DataVisWrapper::setSound(const rpm::vector<double>& signal, double fs)
{
    QMutexLocker lock(&mSoundMutex);
    QMutexLocker lock2(&mGifMutex);

    double absMax = 0;
    for (int k = 0; k < signal.size(); ++k) {
        if (std::abs(signal[k]) > absMax)
            absMax = std::abs(signal[k]);
    }

    int k1 = mGifStart * fs / 1000;
    int k2 = mGifEnd * fs / 1000;

    if (k1 < 0 || k2 < 0 || k1 > k2) return;

    mSound.resize(signal.size()); 
    for (int k = 0; k < signal.size(); ++k) {
        mSound[k] = QPointF((k * 1000) / fs, (absMax > 0) ? signal[k] / absMax : signal[k]);
    }
    emit soundChanged();
}

void DataVisWrapper::setGif(const rpm::vector<double>& signal, double fs)
{
    QMutexLocker lock(&mSoundMutex);
    QMutexLocker lock2(&mGifMutex);

    double absMax = 0;
    for (int k = 0; k < signal.size(); ++k) {
        if (std::abs(signal[k]) > absMax)
            absMax = std::abs(signal[k]);
    }

    int zcr = (int) signal.size() - 1;
    
    for (int i = zcr; i >= 1; --i) {
        if (signal[i - 1] * signal[i] < 0 && signal[i - 1] > 0) {
            zcr = i;
            break;
        }
    }

    mGifEnd = (double) zcr / fs * 1000;
    
    constexpr int periods = 7;
    int periodNum = 0;

    for (int i = zcr; i >= 1; --i) {
        if (signal[i - 1] * signal[i] < 0 && signal[i - 1] > 0) {
            zcr = i;
            if (++periodNum >= periods)
                break;
        }
    }

    mGifStart = (double) zcr / fs * 1000;

    int k1 = mGifStart * fs / 1000;
    int k2 = mGifEnd * fs / 1000;

    if (k1 < 0 || k2 < 0 || k1 > k2) return;

    mGif.resize(k2 - k1 + 1);
    for (int i = 0, k = k1; k <= k2; ++k, ++i) {
        mGif[i] = QPointF((k * 1000) / fs, (absMax > 0) ? signal[k] / absMax : signal[k]);
    }
    emit gifChanged();
}

void DataVisWrapper::updateSoundSeries(QXYSeries* series, QValueAxis* xAxis, QValueAxis* yAxis)
{
    // Copy the data under the locks, and update the chart after the locks are free. A chart
    // update can run other event handlers, and those handlers lock the same mutexes again.
    QVector<QPointF> sound;
    double gifStart, gifEnd;
    {
        QMutexLocker lock(&mSoundMutex);
        QMutexLocker lock2(&mGifMutex);
        sound = mSound;
        gifStart = mGifStart;
        gifEnd = mGifEnd;
    }

    series->replace(sound);
    xAxis->setRange(gifStart, gifEnd);
    yAxis->setRange(-1.5, 1.5);
}

void DataVisWrapper::updateGifSeries(QXYSeries* series, QValueAxis* xAxis, QValueAxis* yAxis)
{
    QVector<QPointF> gif;
    double gifStart, gifEnd;
    {
        QMutexLocker lock(&mSoundMutex);
        QMutexLocker lock2(&mGifMutex);
        gif = mGif;
        gifStart = mGifStart;
        gifEnd = mGifEnd;
    }

    series->replace(gif);
    xAxis->setRange(gifStart, gifEnd);
    yAxis->setRange(-1.5, 1.5);
}
