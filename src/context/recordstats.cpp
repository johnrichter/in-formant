#include "recordstats.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace Main;

namespace {

struct Summary {
    size_t frames = 0;
    std::vector<double> values;   // sorted

    bool empty() const { return values.empty(); }
    double mean() const {
        double sum = 0;
        for (double v : values) sum += v;
        return sum / (double) values.size();
    }
    // The sample standard deviation, with N - 1 in the divisor. One value has no spread.
    double sd() const {
        if (values.size() < 2) return 0.0;
        const double m = mean();
        double sum = 0;
        for (double v : values) sum += (v - m) * (v - m);
        return std::sqrt(sum / (double) (values.size() - 1));
    }

    // The value at a fraction of the sorted values, with a linear step between two values.
    double percentile(double fraction) const {
        const double position = fraction * (double) (values.size() - 1);
        const size_t low = (size_t) std::floor(position);
        const size_t high = std::min(low + 1, values.size() - 1);
        return values[low] + (position - (double) low) * (values[high] - values[low]);
    }
    double median() const { return percentile(0.5); }
    double share() const { return frames == 0 ? 0.0 : 100.0 * (double) values.size() / (double) frames; }
};

Summary summarize(OptionalTimeTrack<double> &track, double t0, double t1)
{
    Summary s;
    for (auto it = track.lower_bound(t0); it != track.upper_bound(t1); ++it) {
        ++s.frames;
        if (it->second.has_value()) s.values.push_back(*it->second);
    }
    std::sort(s.values.begin(), s.values.end());
    return s;
}

QString hz(double f) { return QString::number(f, 'f', f < 1000 ? 1 : 0); }

// The musical note of a frequency with the offset in cents, for example "A3 +12 cents".
QString note(double f)
{
    static const char *names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const double midi = 69.0 + 12.0 * std::log2(f / 440.0);
    const int nearest = (int) std::lround(midi);
    const int cents = (int) std::lround((midi - nearest) * 100.0);
    const int octave = nearest / 12 - 1;
    return QString("%1%2 %3%4 cents").arg(names[((nearest % 12) + 12) % 12]).arg(octave).arg(cents >= 0 ? "+" : "").arg(cents);
}

QString clock(double seconds)
{
    const long long s = (long long) std::llround(std::max(0.0, seconds));
    return QString("%1:%2:%3").arg(s / 3600, 2, 10, QChar('0')).arg((s / 60) % 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
}

}

QString Main::buildRecordingReport(Config &config, DataStore &dataStore, double t0, double t1,
                                   const QDateTime &started, const QStringList &files)
{
    const double length = t1 - t0;
    const int formantCount = std::min(dataStore.getFormantTrackCount(), 4);

    QString out;
    out += "# InFormant recording\n\n";
    out += "- Started: " + started.toString("yyyy-MM-dd HH:mm:ss") + "\n";
    out += "- Length: " + clock(length) + " (" + QString::number(length, 'f', 1) + " s)\n";
    for (const QString &file : files) out += "- File: " + file + "\n";
    out += QString("- Analysis: pitch every %1 ms, formants every %2 ms, up to %3 Hz\n")
        .arg(config.getAnalysisPitchSpacing()).arg(config.getAnalysisFormantSpacing()).arg(config.getAnalysisMaxFrequency());
    out += "\n";

    dataStore.beginRead();

    // The whole recording.
    Summary pitch = summarize(dataStore.getPitchTrack(), t0, t1);
    out += "## Pitch\n\n";
    if (pitch.empty()) {
        out += "The recording has no voiced frames.\n\n";
    }
    else {
        out += "The pitch is the fundamental frequency of the voiced frames.\n\n";
        out += "| Statistic | Value | Note |\n| --- | --- | --- |\n";
        auto row = [&](const QString &name, double f) { out += "| " + name + " | " + hz(f) + " Hz | " + note(f) + " |\n"; };
        row("Mean", pitch.mean());
        row("Median", pitch.median());
        row("Minimum", pitch.values.front());
        row("5th percentile", pitch.percentile(0.05));
        row("25th percentile", pitch.percentile(0.25));
        row("75th percentile", pitch.percentile(0.75));
        row("95th percentile", pitch.percentile(0.95));
        row("Maximum", pitch.values.back());
        out += "\n";
        const double rangeSemitones = 12.0 * std::log2(pitch.percentile(0.95) / pitch.percentile(0.05));
        out += "- Standard deviation: " + hz(pitch.sd()) + " Hz\n";
        out += "- Variation (standard deviation divided by mean): " + QString::number(100.0 * pitch.sd() / pitch.mean(), 'f', 1) + " %\n";
        out += "- Range from the 5th to the 95th percentile: " + QString::number(rangeSemitones, 'f', 1) + " semitones\n";
        out += "- Voiced frames: " + QString::number(pitch.values.size()) + " of " + QString::number(pitch.frames)
            + " (" + QString::number(pitch.share(), 'f', 1) + " %)\n\n";
    }

    std::vector<Summary> formants;
    for (int i = 0; i < formantCount; ++i) formants.push_back(summarize(dataStore.getFormantTrack(i), t0, t1));

    out += "## Formants\n\n";
    out += "| Formant | Frames with a value | Mean | Median | SD | 5th | 95th | Min | Max |\n| --- | --- | --- | --- | --- | --- | --- | --- | --- |\n";
    for (int i = 0; i < formantCount; ++i) {
        const Summary &f = formants[(size_t) i];
        if (f.empty()) {
            out += QString("| F%1 | 0 | | | | | | | |\n").arg(i + 1);
            continue;
        }
        out += QString("| F%1 | %2 (%3 %) | %4 | %5 | %6 | %7 | %8 | %9 | %10 |\n")
            .arg(i + 1).arg(f.values.size()).arg(f.share(), 0, 'f', 1)
            .arg(hz(f.mean()), hz(f.median()), hz(f.sd()), hz(f.percentile(0.05)), hz(f.percentile(0.95)))
            .arg(hz(f.values.front()), hz(f.values.back()));
    }
    out += "\nValues are in Hz.\n\n";

    // Each part of the recording. The parts have equal length, and there are at most 12.
    const double step = std::max(5.0, std::ceil(length / 12.0));
    out += "## Timeline\n\n";
    out += "Each row shows the median of the values in one part of the recording.\n\n";
    out += "| Time | Voiced | Pitch (Hz) |";
    for (int i = 0; i < std::min(formantCount, 3); ++i) out += QString(" F%1 (Hz) |").arg(i + 1);
    out += "\n| --- | --- | --- |";
    for (int i = 0; i < std::min(formantCount, 3); ++i) out += " --- |";
    out += "\n";
    for (double a = t0; a < t1; a += step) {
        const double b = std::min(a + step, t1);
        const Summary p = summarize(dataStore.getPitchTrack(), a, b);
        out += "| " + clock(a - t0).mid(3) + " to " + clock(b - t0).mid(3) + " | " + QString::number(p.share(), 'f', 0) + " % | "
            + (p.empty() ? QString("-") : hz(p.median())) + " |";
        for (int i = 0; i < std::min(formantCount, 3); ++i) {
            const Summary f = summarize(dataStore.getFormantTrack(i), a, b);
            out += " " + (f.empty() ? QString("-") : hz(f.median())) + " |";
        }
        out += "\n";
    }
    out += "\n";

    dataStore.endRead();
    return out;
}
