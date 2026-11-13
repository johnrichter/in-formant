#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <set>

#include "../src/analysis/filterbanks/filterbanks.h"
#include "../src/gui/bands.h"
#include "../src/gui/gridlines.h"
#include "../src/gui/oklab.h"
#include "../src/gui/pitchaxis.h"
#include "../src/gui/ticks.h"

using namespace Gui;

namespace {

// The scales of the frequency axis. The program has the same list in the data store.
enum class Scale { Linear, Logarithmic, Mel, ERB };

// The build uses fast math, so a test makes a value that is not a number from its bits.
double bitsToDouble(std::uint64_t bits)
{
    double v;
    std::memcpy(&v, &bits, sizeof v);
    return v;
}
double notANumber() { return bitsToDouble(0x7ff8000000000000ull); }
double infinity() { return bitsToDouble(0x7ff0000000000000ull); }

double mantissaOf(double v)
{
    return v / std::pow(10.0, std::floor(std::log10(v) + 1e-9));
}

std::vector<double> labeledValues(const std::vector<Tick> &ticks)
{
    std::vector<double> values;
    for (const Tick &t : ticks) if (!t.label.empty()) values.push_back(t.value);
    return values;
}

bool isWhole(double v)
{
    return std::fabs(v - std::round(v)) < 1e-9;
}

// The position of a frequency on the axis for each scale, from the top of the axis (0) to the bottom (height).
ToPixel frequencyPixels(Scale scale, double lo, double hi, double height)
{
    return [=](double f) {
        auto transform = [&](double x) {
            switch (scale) {
            case Scale::Linear: return x;
            case Scale::Logarithmic: return hz2log(x);
            case Scale::Mel: return hz2mel(x);
            case Scale::ERB: return hz2erb(x);
            }
            return x;
        };
        return height * (1.0 - (transform(f) - transform(lo)) / (transform(hi) - transform(lo)));
    };
}

}

// No time tick falls between whole seconds, in the automatic mode and in the custom mode with a step of 0.5.
TEST(Axes, TP_AXT_02_NoTimeTickFallsBetweenWholeSeconds)
{
    for (double span : { 0.5, 1.0, 7.0, 15.0, 30.0, 60.0, 120.0, 300.0 }) {
        for (double start : { 0.0, 3.3, -span * 0.4, 1234.567 }) {
            auto toPixel = [&](double t) { return 1400.0 * (t - start) / span; };
            for (double step : { 0.0, 0.5, 0.25, 0.1 }) {
                const TimeTickMode mode = step == 0.0 ? TimeTickMode::Automatic : TimeTickMode::Step;
                const auto ticks = makeTimeTicks(mode, step, start, start + span, toPixel, 30, 10);
                for (const Tick &t : ticks) {
                    EXPECT_TRUE(isWhole(t.value)) << "span " << span << " start " << start << " step " << step << " tick " << t.value;
                }
            }
        }
    }
}

// A mantissa gets a label in all decades or in none, for each density and each scale of the log, mel, and ERB axes.
TEST(Axes, TP_AXT_04_AMantissaGetsALabelInAllDecadesOrInNone)
{
    struct Range { double lo, hi; };
    for (Scale scale : { Scale::Logarithmic, Scale::Mel, Scale::ERB }) {
        for (const Range range : { Range{ 50, 5000 }, Range{ 1, 5500 }, Range{ 20, 8000 } }) {
            for (double height : { 400.0, 700.0, 1000.0 }) {
                for (TickDensity density : { TickDensity::Sparse, TickDensity::Normal, TickDensity::Dense }) {
                    const ToPixel toPixel = frequencyPixels(scale, range.lo, range.hi, height);
                    const double labelSize = 14, labelGap = 8;
                    const auto ticks = makeFrequencyTicks(FrequencyTickMode::Automatic, 0, range.lo, range.hi, toPixel,
                                                          labelSize, labelGap, false, density);
                    const auto values = labeledValues(ticks);

                    std::set<double> mantissas;
                    for (double v : values) mantissas.insert(std::round(mantissaOf(v) * 100) / 100);

                    const double half = 0.5 * labelSize + 0.5 * labelGap;
                    for (double m : mantissas) {
                        // The decades where a label of this mantissa has room on the axis.
                        for (int k = -2; k <= 5; ++k) {
                            const double v = m * std::pow(10.0, k);
                            if (v < range.lo || v > range.hi) continue;
                            const double p = toPixel(v);
                            if (p < half || p > height - half) continue;
                            const bool got = std::any_of(values.begin(), values.end(), [&](double x) { return std::fabs(x - v) < 1e-6 * v; });
                            EXPECT_TRUE(got) << "scale " << (int) scale << " density " << (int) density << " range " << range.lo
                                             << "-" << range.hi << " height " << height << " missing " << v;
                        }
                    }
                }
            }
        }
    }
}

// The linear axis from 0 to 500 Hz and from 0 to 8000 Hz has equal steps with round values.
TEST(Axes, TP_AXT_06_TheLinearAxisHasEqualSteps)
{
    for (double hi : { 500.0, 8000.0 }) {
        for (double height : { 300.0, 600.0, 1000.0 }) {
            auto toPixel = [&](double f) { return height * (1.0 - f / hi); };
            const auto ticks = makeFrequencyTicks(FrequencyTickMode::Automatic, 0, 0, hi, toPixel, 14, 8, true, TickDensity::Normal);
            std::vector<double> values = labeledValues(ticks);
            ASSERT_GE(values.size(), 3u) << hi << " " << height;
            std::sort(values.begin(), values.end());

            const double step = values[1] - values[0];
            for (size_t i = 1; i < values.size(); ++i) {
                EXPECT_NEAR(values[i] - values[i - 1], step, 1e-6) << hi << " " << height;
            }
            // The step is 1, 2, 2.5, or 5 times a power of ten, and each label is a multiple of the step.
            const double m = mantissaOf(step);
            EXPECT_TRUE(std::fabs(m - 1) < 1e-9 || std::fabs(m - 2) < 1e-9 || std::fabs(m - 2.5) < 1e-9 || std::fabs(m - 5) < 1e-9)
                << "step " << step;
            for (double v : values) EXPECT_NEAR(std::fmod(v, step), 0.0, 1e-6) << v;
        }
    }
}

// A decreasing pixel mapping (the flip) gives the same labels as the increasing mapping, mirrored, with no overlap.
TEST(Axes, TP_FLIP_13_ADecreasingMappingGivesTheSameLabelsMirrored)
{
    const double width = 1400.0;
    for (double span : { 15.0, 60.0, 120.0 }) {
        for (double start : { 0.0, -6.0, 40.0 }) {
            auto increasing = [&](double t) { return width * (t - start) / span; };
            auto decreasing = [&](double t) { return width - increasing(t); };
            const double labelWidth = 34, gap = 12;

            const auto a = makeTimeTicks(TimeTickMode::Automatic, 0, start, start + span, increasing, labelWidth, gap);
            const auto b = makeTimeTicks(TimeTickMode::Automatic, 0, start, start + span, decreasing, labelWidth, gap);
            const auto la = labeledValues(a);
            const auto lb = labeledValues(b);
            EXPECT_EQ(la, lb) << span << " " << start;
            EXPECT_EQ(a.size(), b.size());

            // The positions are mirrored, and the labels keep the gap.
            std::vector<double> positions;
            for (double v : lb) {
                EXPECT_NEAR(decreasing(v), width - increasing(v), 1e-9);
                positions.push_back(decreasing(v));
            }
            std::sort(positions.begin(), positions.end());
            for (size_t i = 1; i < positions.size(); ++i) {
                EXPECT_GE(positions[i] - positions[i - 1], labelWidth + gap - 1e-9) << span << " " << start;
            }
        }
    }
}

// The grid has its major lines at the labeled ticks only, in the custom step, the note, and the octave mode.
TEST(Axes, TP_GRID_08_TheMajorLinesAreAtTheLabelledTicksOnly)
{
    const double lo = 50, hi = 2000, height = 800;
    const ToPixel toY = frequencyPixels(Scale::Logarithmic, lo, hi, height);

    for (FrequencyTickMode mode : { FrequencyTickMode::Step, FrequencyTickMode::Notes, FrequencyTickMode::Octaves }) {
        const auto ticks = makeFrequencyTicks(mode, 100.0, lo, hi, toY, 14, 8);
        const GridLines lines = makeGridLines(ticks, toY, height);

        std::set<long> labeledRows, unlabeledRows;
        for (const Tick &t : ticks) {
            (t.label.empty() ? unlabeledRows : labeledRows).insert(std::lround(toY(t.value)));
        }
        ASSERT_FALSE(lines.major.empty()) << (int) mode;
        for (double y : lines.major) {
            EXPECT_TRUE(labeledRows.count(std::lround(y))) << "mode " << (int) mode << " row " << y;
        }
        for (double y : lines.minor) {
            EXPECT_TRUE(unlabeledRows.count(std::lround(y))) << "mode " << (int) mode << " row " << y;
            EXPECT_FALSE(labeledRows.count(std::lround(y))) << "mode " << (int) mode << " row " << y;
        }
        // Each labeled tick that lies in the graph has a major line.
        EXPECT_EQ(lines.major.size(), labeledRows.size()) << (int) mode;
    }

    // The octave mode has fewer labeled ticks than the note mode.
    const auto notes = makeFrequencyTicks(FrequencyTickMode::Notes, 0, lo, hi, toY, 14, 8);
    const auto octaves = makeFrequencyTicks(FrequencyTickMode::Octaves, 0, lo, hi, toY, 14, 8);
    EXPECT_LT(labeledValues(octaves).size(), labeledValues(notes).size());
    EXPECT_GE(labeledValues(octaves).size(), 3u);
}

// A graph so short that the ticks are closer than 2 pixels has one line at most for each pixel row.
TEST(Axes, TP_GRID_09_AVeryShortGraphHasOneLineAtMostForEachPixelRow)
{
    for (double height : { 3.0, 8.0, 20.0, 40.0 }) {
        for (FrequencyTickMode mode : { FrequencyTickMode::Automatic, FrequencyTickMode::Step, FrequencyTickMode::Notes, FrequencyTickMode::Octaves }) {
            const ToPixel toY = frequencyPixels(Scale::Logarithmic, 50, 5000, height);
            // A label size of 1 pixel and a gap of 0 put the ticks as close as the algorithm allows.
            const auto ticks = makeFrequencyTicks(mode, 10.0, 50, 5000, toY, 1, 0);
            const GridLines lines = makeGridLines(ticks, toY, height);

            std::set<long> rows;
            for (double y : lines.major) EXPECT_TRUE(rows.insert(std::lround(y)).second) << "height " << height << " major row " << y;
            for (double y : lines.minor) EXPECT_TRUE(rows.insert(std::lround(y)).second) << "height " << height << " minor row " << y;
            EXPECT_LE(rows.size(), (size_t) height + 1);
        }
    }

    // Two ticks on the same row give a major line, and not a minor line on top of it.
    const std::vector<Tick> twoTicks = { { 100.0, 2, "" }, { 101.0, 0, "100" } };
    const GridLines lines = makeGridLines(twoTicks, [](double) { return 5.2; }, 20.0);
    EXPECT_EQ(lines.major.size(), 1u);
    EXPECT_EQ(lines.minor.size(), 0u);
    // A tick outside the graph has no line.
    const GridLines outside = makeGridLines({ { 100.0, 0, "100" } }, [](double) { return -3.0; }, 20.0);
    EXPECT_TRUE(outside.major.empty());
}

// The fixed axis of the pitch-only graph runs from 0 to 500 Hz, and the zones lie in it in full.
TEST(Axes, TP_PITCH_01_TheFixedAxisRunsFrom0To500AndTheZonesLieInItInFull)
{
    const PitchAxis axis = fixedPitchAxis(0.0, 500.0);
    EXPECT_EQ(axis.low, 0.0);
    EXPECT_EQ(axis.high, 500.0);

    // On a linear scale, each edge of the default zones lies inside the graph.
    const double height = 600.0;
    const ToPixel toY = frequencyPixels(Scale::Linear, axis.low, axis.high, height);
    double previous = height + 1.0;
    for (double edge : defaultBandEdges) {
        const double y = toY(edge);
        EXPECT_GT(y, 0.0);
        EXPECT_LT(y, height);
        EXPECT_LT(y, previous);   // a higher frequency is higher on the graph
        previous = y;
    }
    // Equal steps in Hz have equal height.
    EXPECT_NEAR(toY(100.0) - toY(200.0), toY(200.0) - toY(300.0), 1e-9);
}

// The automatic fit uses the 5th and the 95th percentile, a margin, and a minimum of 12 semitones.
TEST(Axes, TP_PITCH_02_TheAutomaticFitUsesThePercentilesAMarginAndAMinimumSpan)
{
    // The values 100 to 400 Hz in 301 steps. The percentiles are at the indexes 15 and 285 of 300.
    std::vector<double> wide;
    for (int i = 0; i <= 300; ++i) wide.push_back(100.0 + i);
    const double p5 = 100.0 + 15, p95 = 100.0 + 285;
    const PitchAxis a = fitPitchAxis(wide);
    const double span = semitonesBetween(p5, p95);
    ASSERT_GT(span, 12.0);
    const double margin = 0.15 * span / 12.0;   // octaves
    EXPECT_NEAR(a.low, p5 * std::exp2(-margin), 1e-6);
    EXPECT_NEAR(a.high, p95 * std::exp2(margin), 1e-6);

    // A narrow set of values gets the minimum span of 12 semitones plus the margin of 15 percent at each side.
    std::vector<double> narrow;
    for (int i = 0; i <= 100; ++i) narrow.push_back(200.0 + 0.02 * i);
    const PitchAxis n = fitPitchAxis(narrow);
    EXPECT_NEAR(semitonesBetween(n.low, n.high), 12.0 * 1.3, 1e-6);
    const double center = std::sqrt(n.low * n.high);
    EXPECT_NEAR(center, 201.0, 0.5);

    // Too few values give the fallback. A fallback that the caller gives is returned.
    EXPECT_EQ(fitPitchAxis({ 100, 110, 120, 130 }, { 70, 280 }).low, 70.0);
    EXPECT_EQ(fitPitchAxis({}, { 70, 280 }).high, 280.0);

    // The edges stay in the allowed range.
    const PitchAxis low = fitPitchAxis({ 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 });
    EXPECT_GE(low.low, 20.0);
    const PitchAxis high = fitPitchAxis({ 1900, 1900, 1900, 1900, 1900, 1900, 1900, 1900 });
    EXPECT_LE(high.high, 2000.0);
}

// A few octave jumps do not stretch the fit. Many octave jumps do.
TEST(Axes, TP_PITCH_03_AFewOctaveJumpsDoNotStretchTheFit)
{
    std::vector<double> clean;
    for (int i = 0; i < 100; ++i) clean.push_back(180.0 + (i % 21) * 3.0);   // 180 to 240 Hz
    const PitchAxis reference = fitPitchAxis(clean);

    // 4 percent of the values are an octave too low, and 4 percent are an octave too high.
    std::vector<double> jumpy = clean;
    jumpy.insert(jumpy.end(), { 90, 90, 90, 90, 480, 480, 480, 480 });
    const PitchAxis fit = fitPitchAxis(jumpy);
    EXPECT_NEAR(std::log2(fit.low / reference.low), 0.0, 0.1);
    EXPECT_NEAR(std::log2(fit.high / reference.high), 0.0, 0.1);

    // A value of 0, a negative value, and a value that is not a number do not count at all.
    std::vector<double> bad = clean;
    bad.insert(bad.end(), { 0.0, -50.0, notANumber(), infinity() });
    const PitchAxis fitBad = fitPitchAxis(bad);
    EXPECT_NEAR(fitBad.low, reference.low, reference.low * 0.02);
    EXPECT_NEAR(fitBad.high, reference.high, reference.high * 0.02);

    // 20 percent of the values are an octave too high: the fit shows them, because they are too many to be errors.
    std::vector<double> many = clean;
    for (int i = 0; i < 25; ++i) many.push_back(480.0);
    EXPECT_GT(fitPitchAxis(many).high, 1.5 * reference.high);
}

// Edge values that are not in order, equal, zero, negative, or not numbers give the default edges.
TEST(Axes, TP_ZONE_02_BadEdgeValuesGiveTheDefaultEdges)
{
    const BandEdges good = { 80, 160, 200, 320 };
    EXPECT_EQ(parseBandEdges("80,160,200,320"), good);

    for (const char *text : { "300,180,150,60", "60,150,150,300", "100,100,100,100", "60,150,180,180",
                              "0,150,180,300", "-60,150,180,300", "60,150,180,-300", "10,150,180,300", "60,150,180,2500",
                              "nan,150,180,300", "60,nan,180,300", "60,150,180,nan", "inf,150,180,300", "60,150,180,inf",
                              "-inf,150,180,300", "1e999,150,180,300", "60,150,180", "60,150,180,300,400", "", "   ", ",,,",
                              "a,b,c,d", "60;150;;" }) {
        EXPECT_EQ(parseBandEdges(text), defaultBandEdges) << "text: " << text;
    }
    // The edge values at the limits stay.
    EXPECT_EQ(parseBandEdges("20,100,500,2000"), (BandEdges{ 20, 100, 500, 2000 }));
}

// The three zone colors differ for a person with red-green color blindness.
TEST(Axes, TP_ZONE_06_TheZoneColorsDifferForRedGreenColorBlindness)
{
    // The matrices of Machado, Oliveira, and Fernandes (2009) for a severity of 1, on linear RGB.
    const double protan[3][3] = { { 0.152286, 1.052583, -0.204868 }, { 0.114503, 0.786281, 0.099216 }, { -0.003882, -0.048116, 1.051998 } };
    const double deutan[3][3] = { { 0.367322, 0.860646, -0.227968 }, { 0.280085, 0.672501, 0.047413 }, { -0.011820, 0.042940, 0.968881 } };

    auto simulate = [](const ZoneColor &c, const double (&m)[3][3]) {
        const double in[3] = { srgbToLinear(c.r / 255.0), srgbToLinear(c.g / 255.0), srgbToLinear(c.b / 255.0) };
        double out[3];
        for (int i = 0; i < 3; ++i) {
            out[i] = std::clamp(m[i][0] * in[0] + m[i][1] * in[1] + m[i][2] * in[2], 0.0, 1.0);
        }
        const unsigned rgb = ((unsigned) std::lround(linearToSrgb(out[0]) * 255.0) << 16)
                           | ((unsigned) std::lround(linearToSrgb(out[1]) * 255.0) << 8)
                           | (unsigned) std::lround(linearToSrgb(out[2]) * 255.0);
        return rgbToLab(rgb);
    };
    auto normal = [](const ZoneColor &c) {
        return rgbToLab(((unsigned) c.r << 16) | ((unsigned) c.g << 8) | (unsigned) c.b);
    };

    // A distance of 0.08 in OKLab is far above the smallest difference that a person sees (about 0.02).
    constexpr double minimum = 0.08;
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            EXPECT_GT(labDistance(normal(zoneColors[i]), normal(zoneColors[j])), minimum) << i << " " << j << " normal";
            EXPECT_GT(labDistance(simulate(zoneColors[i], protan), simulate(zoneColors[j], protan)), minimum)
                << zoneColors[i].name << " and " << zoneColors[j].name << " with protanopia";
            EXPECT_GT(labDistance(simulate(zoneColors[i], deutan), simulate(zoneColors[j], deutan)), minimum)
                << zoneColors[i].name << " and " << zoneColors[j].name << " with deuteranopia";
        }
    }
}
