#include "datastore.h"
#include <chrono>

using namespace Main;

DataStore::DataStore()
    : mTrackLength(0),
      mTime(0)
{
}

void DataStore::beginWrite()
{
    mLock.lockForWrite();
}

void DataStore::endWrite()
{
    mLock.unlock();
}

void DataStore::beginRead()
{
    mLock.lockForRead();
}

void DataStore::endRead()
{
    mLock.unlock();
}

double DataStore::getTime() const
{
    return mTime;
}

void DataStore::setTime(double t)
{
    mTime = t;
}

TimeTrack<SpectrogramCoefs>& DataStore::getSpectrogram()
{
    return mSpectrogram;
}

OptionalTimeTrack<double>& DataStore::getPitchTrack()
{
    return mPitchTrack;
}

OptionalTimeTrack<double>& DataStore::getFormantTrack(int i)
{
    return mFormantTracks.at(i);
}

int DataStore::getFormantTrackCount() const
{
    return (int) mFormantTracks.size();
}

void DataStore::setFormantTrackCount(int n)
{
    mFormantTracks.resize(n);
}

TimeTrack<rpm::vector<double>>& DataStore::getSoundTrack()
{
    return mSoundTrack;
}

TimeTrack<rpm::vector<double>>& DataStore::getGifTrack()
{
    return mGifTrack;
}




Session& DataStore::session()
{
    return mSession;
}

void DataStore::clearTracks()
{
    mSpectrogram.clear();
    mPitchTrack.clear();
    for (auto& track : mFormantTracks) track.clear();
    mSoundTrack.clear();
    mGifTrack.clear();
}
