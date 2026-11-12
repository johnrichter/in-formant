#include "reviewspectrogram.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <samplerate.h>

#include "../analysis/analysis.h"

using namespace Main;

ReviewSpectrogram::ReviewSpectrogram(const Session *session)
    : mSession(session),
      mHasRequest(false),
      mStop(false),
      mNextId(1)
{
    mThread = std::thread(&ReviewSpectrogram::run, this);
}

ReviewSpectrogram::~ReviewSpectrogram()
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStop = true;
    }
    mWake.notify_all();
    if (mThread.joinable()) mThread.join();
}

void ReviewSpectrogram::request(const ReviewRequest &request)
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mRequest = request;
        mHasRequest = true;
    }
    mWake.notify_one();
}

std::shared_ptr<const ReviewResult> ReviewSpectrogram::latest() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mLatest;
}

void ReviewSpectrogram::run()
{
    for (;;) {
        ReviewRequest request;
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mWake.wait(lock, [this] { return mStop || mHasRequest; });
            if (mStop) return;
            request = mRequest;
            mHasRequest = false;
        }

        auto result = std::make_shared<ReviewResult>();
        result->request = request;
        compute(request, *result);

        std::lock_guard<std::mutex> lock(mMutex);
        result->id = mNextId++;
        mLatest = std::move(result);
    }
}

void ReviewSpectrogram::compute(const ReviewRequest &request, ReviewResult &result)
{
    const int fs = std::max(request.sampleRate, 1000);
    const int n = std::max(request.fftSamples, 64);
    const int columns = std::clamp(request.columns, 1, 4096);
    const double span = request.t1 - request.t0;
    if (!(span > 0)) return;

    const double hop = span / columns;
    const double windowSeconds = (double) n / fs;

    // The FFT keeps its plan from one request to the next.
    static std::unique_ptr<Analysis::RealFFT> fft;
    static rpm::map<int, rpm::vector<double>> windowCache;
    if (!fft || fft->getInputLength() != n) {
        fft = std::make_unique<Analysis::RealFFT>(n);
    }

    // Four more rows of zeros stop the filter of the texture from reading old values.
    const int rows = std::min(fft->getOutputLength() + 4, 2560);
    result.columns = columns;
    result.rows = rows;
    result.sessionEnd = mSession->duration();
    result.data.assign((size_t) rows * columns, 0.0f);
    result.holds.assign((size_t) columns, 1.0f);

    // A column keeps its value when the new span only moves against the old span by whole columns.
    // The columns near the end of the sound at the old time are not final, so the new result computes them again.
    int reuseFirst = 0, reuseLast = 0, shift = 0;
    const auto prev = latest();
    if (prev && prev->columns == columns && prev->rows == rows
        && prev->request.sampleRate == request.sampleRate && prev->request.fftSamples == request.fftSamples
        && std::abs((prev->request.t1 - prev->request.t0) - span) < 1e-6 * span) {
        const double moved = (request.t0 - prev->request.t0) / hop;
        const double whole = std::round(moved);
        if (std::abs(moved - whole) < 1e-3 && std::abs(whole) < columns) {
            shift = (int) whole;
            const int finalColumns = (int) std::floor((prev->sessionEnd - 0.5 * windowSeconds - prev->request.t0) / hop);
            const int oldFirst = std::max(0, shift);
            const int oldLast = std::min(std::min(columns, finalColumns), columns + shift);
            if (oldLast > oldFirst) {
                reuseFirst = oldFirst - shift;
                reuseLast = oldLast - shift;
                for (int r = 0; r < rows; ++r) {
                    std::memcpy(&result.data[(size_t) r * columns + reuseFirst], &prev->data[(size_t) r * columns + oldFirst],
                        (size_t) (reuseLast - reuseFirst) * sizeof(float));
                }
                for (int c = reuseFirst; c < reuseLast; ++c) {
                    result.sliceSizes[c] = prev->sliceSizes[c + shift];
                    result.sampleRates[c] = prev->sampleRates[c + shift];
                    result.holds[c] = prev->holds[c + shift];
                }
            }
        }
    }

    const auto highpass = Analysis::butterworthHighpass(8, 60.0, fs);

    // The decay of the level that normalizes the columns. The decay for 10 ms is the same as in
    // the live spectrogram.
    const double decay = std::pow(0.995, hop / 0.01);
    double hold = 1.0;

    // Windows that overlap come from one block of sound. Windows that do not overlap each have
    // their own short block, so a view of many hours does not read all of the sound.
    const bool contiguous = hop < windowSeconds;
    const double warmup = contiguous ? 0.25 : 0.1;

    // Make the samples of one span of time at the analysis rate. The result starts at the start time.
    // Each span has its own conversion, because a conversion keeps the state of the sound before.
    auto prepare = [&](double start, double stop) {
        rpm::vector<float> raw;
        mSession->read(start, stop, raw);

        std::vector<float> converted((size_t) ((double) raw.size() * fs / Session::sampleRate) + 32);
        SRC_DATA data{};
        data.data_in = raw.data();
        data.input_frames = (long) raw.size();
        data.data_out = converted.data();
        data.output_frames = (long) converted.size();
        data.src_ratio = (double) fs / Session::sampleRate;
        if (raw.empty() || src_simple(&data, SRC_SINC_FASTEST, 1) != 0) data.output_frames_gen = 0;

        rpm::vector<double> out(converted.begin(), converted.begin() + data.output_frames_gen);
        return Analysis::sosfilter(highpass, out);
    };

    auto transform = [&](const rpm::vector<double> &samples, double centerIndex, int column) {
        rpm::vector<double> window(n, 0.0);
        const long long first = std::llround(centerIndex) - n / 2;
        for (int i = 0; i < n; ++i) {
            const long long k = first + i;
            if (k >= 0 && k < (long long) samples.size()) window[i] = samples[(size_t) k];
        }

        auto bins = Analysis::fft_n(fft.get(), window, windowCache);

        double max = 0;
        for (double x : bins) max = std::max(max, x);
        hold = max = std::max(decay * hold + (1.0 - decay) * max, max);
        result.holds[column] = (float) hold;
        if (max <= 0) max = 1.0;

        const int size = (int) std::min<size_t>(bins.size(), (size_t) rows);
        for (int k = 0; k < size; ++k) result.data[(size_t) k * columns + column] = (float) (bins[k] / max);
        result.sliceSizes[column] = size;
        result.sampleRates[column] = (float) fs;
    };

    // Compute the columns from a to b, not including b.
    auto computeColumns = [&](int a, int b) {
        if (a >= b) return;
        hold = a > 0 ? result.holds[a - 1] : (prev && prev->columns == columns ? prev->holds[0] : 1.0);

        if (contiguous) {
            const double start = request.t0 + a * hop - 0.5 * windowSeconds - warmup;
            const double stop = request.t0 + b * hop + 0.5 * windowSeconds;
            const rpm::vector<double> samples = prepare(start, stop);

            for (int c = a; c < b; ++c) {
                const double time = request.t0 + (c + 0.5) * hop;
                transform(samples, (time - start) * fs, c);
            }
        }
        else {
            for (int c = a; c < b; ++c) {
                const double time = request.t0 + (c + 0.5) * hop;
                const double start = time - 0.5 * windowSeconds - warmup;
                const double stop = time + 0.5 * windowSeconds;
                const rpm::vector<double> samples = prepare(start, stop);
                transform(samples, (time - start) * fs, c);
            }
        }
    };

    if (reuseLast > reuseFirst) {
        computeColumns(0, reuseFirst);
        computeColumns(reuseLast, columns);
    }
    else {
        computeColumns(0, columns);
    }
}
