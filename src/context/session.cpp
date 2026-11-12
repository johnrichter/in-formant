#include "session.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <QDir>
#include <QFile>

using namespace Main;

namespace {

int16_t toInt16(double v)
{
    const double scaled = std::clamp(v, -1.0, 1.0) * 32767.0;
    return (int16_t) std::lround(scaled);
}

std::unique_ptr<QTemporaryFile> makeFile()
{
    auto file = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/informant-session-XXXXXX.raw");
    file->open();
    return file;
}

}

Session::Session()
    : mFile(makeFile()),
      mCount(0),
      mPartialLow(0),
      mPartialHigh(0),
      mPartialCount(0),
      mInputRate(0),
      mVersion(0)
{
}

Session::~Session() = default;

void Session::clear()
{
    std::lock_guard<std::mutex> lock(mMutex);

    mFile = makeFile();
    mCount = 0;
    mBlocks.clear();
    mPartialCount = 0;
    mInputRate = 0;
    ++mVersion;
}

double Session::duration() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return (double) mCount / sampleRate;
}

void Session::append(const double *samples, int count, double fs)
{
    if (count <= 0 || !(fs > 0)) return;

    std::vector<int16_t> out;

    std::lock_guard<std::mutex> lock(mMutex);

    if (std::lround(fs) == sampleRate) {
        out.reserve(count);
        for (int i = 0; i < count; ++i) out.push_back(toInt16(samples[i]));
    }
    else {
        if (mInputRate != (int) std::lround(fs)) {
            mInputRate = (int) std::lround(fs);
            mResampler.setRate(mInputRate, sampleRate);
        }

        rpm::vector<double> input(samples, samples + count);
        const rpm::vector<double> resampled = mResampler.process(input);
        out.reserve(resampled.size());
        for (double v : resampled) out.push_back(toInt16(v));
    }

    appendSamples(out);
}

void Session::appendStored(const float *samples, size_t count)
{
    std::vector<int16_t> out;
    out.reserve(count);
    for (size_t i = 0; i < count; ++i) out.push_back(toInt16(samples[i]));

    std::lock_guard<std::mutex> lock(mMutex);
    appendSamples(out);
}

void Session::appendSamples(const std::vector<int16_t> &samples)
{
    // The caller holds the lock.
    const long long limit = (long long) (maxDuration * sampleRate);
    size_t count = samples.size();
    if (mCount >= limit) return;
    if (mCount + (long long) count > limit) count = (size_t) (limit - mCount);
    if (count == 0 || !mFile || !mFile->isOpen()) return;

    mFile->seek(mCount * 2);
    mFile->write(reinterpret_cast<const char *>(samples.data()), (qint64) (count * 2));

    for (size_t i = 0; i < count; ++i) {
        const int16_t v = samples[i];
        if (mPartialCount == 0) {
            mPartialLow = mPartialHigh = v;
        }
        else {
            mPartialLow = std::min(mPartialLow, v);
            mPartialHigh = std::max(mPartialHigh, v);
        }

        if (++mPartialCount == blockSize) {
            mBlocks.push_back(mPartialLow);
            mBlocks.push_back(mPartialHigh);
            mPartialCount = 0;
        }
    }

    mCount += (long long) count;
    ++mVersion;
}

bool Session::readSamples(long long first, long long count, std::vector<int16_t> &out) const
{
    // The caller holds the lock.
    out.assign((size_t) count, 0);
    if (count <= 0) return true;
    if (!mFile || !mFile->isOpen()) return false;

    const long long begin = std::max(first, 0LL);
    const long long end = std::min(first + count, mCount);
    if (end <= begin) return true;

    if (!mFile->seek(begin * 2)) return false;
    const qint64 wanted = (end - begin) * 2;
    const qint64 got = mFile->read(reinterpret_cast<char *>(out.data() + (begin - first)), wanted);
    return got == wanted;
}

bool Session::read(double t0, double t1, rpm::vector<float> &out) const
{
    const long long first = std::llround(t0 * sampleRate);
    const long long count = std::max(0LL, std::llround(t1 * sampleRate) - first);

    std::vector<int16_t> raw;
    bool ok;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        ok = readSamples(first, count, raw);
    }

    out.resize(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) out[i] = (float) raw[i] / 32768.0f;
    return ok;
}

void Session::envelope(double t0, double t1, int bins, std::vector<float> &low, std::vector<float> &high) const
{
    low.assign((size_t) std::max(bins, 0), 0.0f);
    high.assign((size_t) std::max(bins, 0), 0.0f);
    if (bins <= 0 || !(t1 > t0)) return;

    std::lock_guard<std::mutex> lock(mMutex);

    const double first = t0 * sampleRate;
    const double perBin = (t1 - t0) * sampleRate / bins;

    // A bin of less than two blocks reads the samples, because the blocks are too coarse.
    if (perBin < 2.0 * blockSize) {
        const long long begin = (long long) std::floor(first);
        const long long count = (long long) std::ceil(perBin * bins) + 2;
        std::vector<int16_t> raw;
        if (!readSamples(begin, count, raw)) return;

        for (int b = 0; b < bins; ++b) {
            const long long s0 = std::max(0LL, (long long) std::floor(first + b * perBin) - begin);
            const long long s1 = std::min((long long) raw.size(), std::max(s0 + 1, (long long) std::floor(first + (b + 1) * perBin) - begin));
            int16_t lo = 0, hi = 0;
            for (long long i = s0; i < s1; ++i) {
                lo = std::min(lo, raw[(size_t) i]);
                hi = std::max(hi, raw[(size_t) i]);
            }
            low[b] = (float) lo / 32768.0f;
            high[b] = (float) hi / 32768.0f;
        }
        return;
    }

    const long long blockCount = (long long) mBlocks.size() / 2;
    for (int b = 0; b < bins; ++b) {
        const long long s0 = (long long) std::floor(first + b * perBin);
        const long long s1 = (long long) std::floor(first + (b + 1) * perBin);
        if (s1 <= 0 || s0 >= mCount) continue;

        const long long b0 = std::max(0LL, s0 / blockSize);
        const long long b1 = std::min(blockCount - 1, s1 / blockSize);
        int16_t lo = 0, hi = 0;
        for (long long k = b0; k <= b1; ++k) {
            lo = std::min(lo, mBlocks[(size_t) (2 * k)]);
            hi = std::max(hi, mBlocks[(size_t) (2 * k + 1)]);
        }
        if (mPartialCount > 0 && s1 / blockSize >= blockCount) {
            lo = std::min(lo, mPartialLow);
            hi = std::max(hi, mPartialHigh);
        }
        low[b] = (float) lo / 32768.0f;
        high[b] = (float) hi / 32768.0f;
    }
}

bool Session::exportWav(const QString &path, double t0, double t1) const
{
    const double start = std::max(0.0, t0);
    const double stop = std::min(t1, duration());
    if (!(stop > start)) return false;

    const long long first = std::llround(start * sampleRate);
    const long long total = std::llround(stop * sampleRate) - first;
    if (total <= 0) return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

    auto put32 = [&](uint32_t v) { file.write(reinterpret_cast<const char *>(&v), 4); };
    auto put16 = [&](uint16_t v) { file.write(reinterpret_cast<const char *>(&v), 2); };

    // The header of a mono WAV file with 16-bit samples. The file uses the byte order of the
    // computer, and each supported computer has little-endian order.
    file.write("RIFF", 4);
    put32((uint32_t) (36 + total * 2));
    file.write("WAVEfmt ", 8);
    put32(16);
    put16(1);
    put16(1);
    put32(sampleRate);
    put32(sampleRate * 2);
    put16(2);
    put16(16);
    file.write("data", 4);
    put32((uint32_t) (total * 2));

    constexpr long long chunk = 1 << 20;
    std::vector<int16_t> raw;
    for (long long done = 0; done < total; done += chunk) {
        const long long n = std::min(chunk, total - done);
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (!readSamples(first + done, n, raw)) return false;
        }
        file.write(reinterpret_cast<const char *>(raw.data()), (qint64) (n * 2));
    }

    return file.error() == QFile::NoError;
}
