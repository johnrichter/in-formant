#include <gtest/gtest.h>
#include "../src/gui/maplayout.h"

using namespace Gui;

// A short session shows whole, and the map follows nothing.
TEST(MapLayout, AShortSessionShowsWholeAndTheMapFollowsNothing)
{
    MapState st;
    const MapView v = updateMapView(st, 40.0, 150.0, 15.0, 40.0, true);
    EXPECT_TRUE(v.start == 0.0);
    EXPECT_NEAR(v.span, 40.0, 1e-9);
}

// A very short session has a map of one second at least.
TEST(MapLayout, AVeryShortSessionHasAMapOfOneSecondAtLeast)
{
    MapState st;
    const MapView v = updateMapView(st, 0.2, 150.0, 15.0, 0.2, true);
    EXPECT_NEAR(v.span, 1.0, 1e-9);
}

// A long live session shows the newest 150 s.
TEST(MapLayout, ALongLiveSessionShowsTheNewest150S)
{
    MapState st;
    const MapView v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
    EXPECT_NEAR(v.start, 450.0, 1e-9);
    EXPECT_NEAR(v.span, 150.0, 1e-9);
    const MapView w = updateMapView(st, 610.0, 150.0, 15.0, 610.0, true);
    EXPECT_NEAR(w.start, 460.0, 1e-9);
}

// A person scrolls the map back. The map stays there while the capture continues.
TEST(MapLayout, APersonScrollsTheMapBackTheMapStaysThereWhileTheCapture)
{
    MapState st;
    MapView v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
    scrollMap(st, 600.0, v, -200.0);
    EXPECT_TRUE(!st.follow);
    v = updateMapView(st, 620.0, 150.0, 15.0, 620.0, true);
    EXPECT_NEAR(v.start, 250.0, 1e-9);
    // Scrolling to the end makes the map follow again.
    scrollMap(st, 620.0, v, 1000.0);
    EXPECT_TRUE(st.follow);
    v = updateMapView(st, 640.0, 150.0, 15.0, 640.0, true);
    EXPECT_NEAR(v.start, 490.0, 1e-9);
}

// The live view returns after the person looked at the past. The map follows again.
TEST(MapLayout, TheLiveViewReturnsAfterThePersonLookedAtThePastTheMapFollows)
{
    MapState st;
    MapView v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
    scrollMap(st, 600.0, v, -300.0);
    v = updateMapView(st, 600.0, 150.0, 15.0, 100.0, false);
    v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
    EXPECT_TRUE(st.follow);
    EXPECT_NEAR(v.start, 450.0, 1e-9);
}

// A frame that leaves the map brings the map along. A frame inside the map does not.
TEST(MapLayout, AFrameThatLeavesTheMapBringsTheMapAlongAFrameInsideTheMap)
{
    MapState st;
    MapView v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
    v = updateMapView(st, 600.0, 150.0, 15.0, 500.0, false);
    EXPECT_NEAR(v.start, 450.0, 1e-9);
    v = updateMapView(st, 600.0, 150.0, 15.0, 100.0, false);
    EXPECT_TRUE(v.start <= 100.0 - 15.0);
    const double before = v.start;
    v = updateMapView(st, 600.0, 150.0, 15.0, before + 60.0, false);
    EXPECT_NEAR(v.start, before, 1e-9);
}

// A frame that is longer than the window widens the map.
TEST(MapLayout, AFrameThatIsLongerThanTheWindowWidensTheMap)
{
    MapState st;
    const MapView v = updateMapView(st, 1000.0, 150.0, 400.0, 1000.0, true);
    EXPECT_NEAR(v.span, 500.0, 1e-9);
    EXPECT_NEAR(v.start, 500.0, 1e-9);
}

// The width of a bin is a round value that is not much wider than a pixel.
TEST(MapLayout, TheWidthOfABinIsARoundValueThatIsNotMuchWiderThanAPixel)
{
    const long long a = mapBinSamples(150.0, 1500, 32000);
    EXPECT_TRUE(a >= 3200 && a <= (long long) (3200 * 1.34));
    // A small change of the span keeps the same bin width most of the time.
    EXPECT_TRUE(mapBinSamples(150.0, 1500, 32000) == mapBinSamples(151.0, 1500, 32000));
    EXPECT_TRUE(mapBinSamples(0.5, 1500, 32000) >= 1);
}

// The bins cover the map, and the first bin starts at a multiple of the bin width.
TEST(MapLayout, TheBinsCoverTheMapAndTheFirstBinStartsAtAMultipleOfTheBin)
{
    MapView v;
    v.start = 100.3;
    v.span = 50.0;
    const long long bin = mapBinSamples(v.span, 1000, 32000);
    const MapBins bins = mapBinRange(v, bin, 32000);
    const double perBin = (double) bin / 32000;
    EXPECT_TRUE(bins.first * perBin <= v.start);
    EXPECT_TRUE((bins.first + bins.count) * perBin >= v.start + v.span);
    // The same absolute bin has the same index after the map scrolls by whole bins.
    MapView scrolled = v;
    scrolled.start += 10 * perBin;
    const MapBins moved = mapBinRange(scrolled, bin, 32000);
    EXPECT_TRUE(moved.first == bins.first + 10);
}

// The span of the main view stays in its limits.
TEST(MapLayout, TheSpanOfTheMainViewStaysInItsLimits)
{
    EXPECT_TRUE(clampViewSpan(0.1) == minViewSpan);
    EXPECT_TRUE(clampViewSpan(1e9) == maxViewSpan);
    EXPECT_TRUE(clampViewSpan(12.0) == 12.0);
}

// A drag of a frame edge sets a span with one decimal, and the span stays inside the session or the map time.
TEST(MapLayout, ADragOfAFrameEdgeSetsASpanWithOneDecimalAndTheSpanStays)
{
    EXPECT_NEAR(dragViewSpan(37.4999999, 600.0, 150.0), 37.5, 1e-9);
    EXPECT_NEAR(dragViewSpan(7.252709999, 14.0, 150.0), 7.3, 1e-9);
    EXPECT_TRUE(dragViewSpan(1e9, 600.0, 150.0) == 600.0);
    EXPECT_TRUE(dragViewSpan(1e9, 20.0, 150.0) == 150.0);
    EXPECT_TRUE(dragViewSpan(-5.0, 600.0, 150.0) == minViewSpan);
    EXPECT_TRUE(dragViewSpan(1e9, 1e9, 1e9) == maxViewSpan);
}

// The size of the bin stays while the map grows from 10 to 20 seconds, and it changes rarely.
TEST(MapLayout, TP_MAP_21_BinSizeStaysStable)
{
    const int rate = 32000, width = 1600;
    long long size = mapStableBinSamples(0, 10.0, width, rate);
    EXPECT_TRUE(size >= 200 && size < 400);
    int changes = 0;
    for (double span = 10.0; span <= 150.0; span += 0.05) {
        const long long next = mapStableBinSamples(size, span, width, rate);
        if (next != size) ++changes;
        size = next;
        const double ideal = span * rate / width;
        EXPECT_TRUE((double) size >= ideal && (double) size <= 2.5 * ideal);
    }
    // A growth by a factor of 15 needs about four doublings.
    EXPECT_TRUE(changes >= 3 && changes <= 5);
    // A map that shrinks does not change the size until the bin is wider than two and a half pixels.
    EXPECT_TRUE(mapStableBinSamples(size, 140.0, width, rate) == size);
    EXPECT_TRUE(mapStableBinSamples(size, 20.0, width, rate) < size);
}

// The edges of the bins are multiples of the bin width in absolute time, so a bin keeps its samples when the map scrolls.
TEST(MapLayout, TP_MAP_02_TheBinEdgesAreMultiplesOfTheBinWidthInAbsoluteTime)
{
    constexpr int rate = 32000;
    for (double span : { 15.0, 150.0, 600.0, 3600.0, 86400.0 }) {
        const long long bin = mapBinSamples(span, 1500, rate);
        const double perBin = (double) bin / rate;
        long long previousFirst = -1;
        for (double start : { 0.0, 1.7, 33.3, 100.01, 5000.5 }) {
            const MapView view{ start, span };
            const MapBins bins = mapBinRange(view, bin, rate);

            // The first edge is at or before the start, and the last edge is at or after the end.
            EXPECT_LE(bins.first * perBin, start + 1e-9) << span << " " << start;
            EXPECT_GT((bins.first + 1) * perBin, start - 1e-9) << span << " " << start;
            EXPECT_GE((bins.first + bins.count) * perBin, start + span - 1e-9) << span << " " << start;
            // Each edge is a whole number of bins from time zero, so it is a multiple of the bin width in samples.
            for (int i = 0; i <= std::min(bins.count, 5); ++i) {
                EXPECT_EQ(((bins.first + i) * bin) % bin, 0) << span << " " << start;
                EXPECT_NEAR((double) ((bins.first + i) * bin) / rate, (bins.first + i) * perBin, 1e-6);
            }
            // The map at a later start has a first bin that is not before the first bin of the earlier start.
            EXPECT_GE(bins.first, previousFirst);
            previousFirst = bins.first;
        }
    }

    // The bin of the same sample is the same bin at every start. A move of 1 bin moves the first bin by 1.
    const long long bin = mapBinSamples(150.0, 1500, rate);
    const double perBin = (double) bin / rate;
    const MapBins a = mapBinRange({ 40.0, 150.0 }, bin, rate);
    const MapBins b = mapBinRange({ 40.0 + perBin, 150.0 }, bin, rate);
    EXPECT_EQ(b.first, a.first + 1);
    EXPECT_EQ(b.count, a.count);
}

// A loud burst sets the scale at once. A quieter sound lowers it slowly with the clock, and not with each drawing.
TEST(MapLayout, TP_MAP_03_TheLoudnessScaleFallsSlowlyAfterALoudBurst)
{
    // The scale rises at once.
    float peak = updateMapPeak(0.05f, 0.9f, 0.016);
    EXPECT_FLOAT_EQ(peak, 0.9f);

    // Quiet sound follows. A tenth of a second lowers the scale by 9.5 percent of the gap at most.
    const float quiet = 0.05f;
    const float after100ms = updateMapPeak(peak, quiet, 0.1);
    EXPECT_GT(after100ms, 0.9f - 0.1f * (0.9f - quiet) - 0.001f);
    EXPECT_LT(after100ms, 0.9f);

    // Drawings at once after each other (no time passes) change nothing.
    float still = peak;
    for (int i = 0; i < 100; ++i) still = updateMapPeak(still, quiet, 0.0);
    EXPECT_FLOAT_EQ(still, peak);

    // The result depends on the time that passed, and not on the number of drawings: 1000 drawings at 1 ms
    // and 10 drawings at 100 ms give the same scale after one second as one drawing after one second.
    float fine = peak, coarse = peak, once = peak;
    for (int i = 0; i < 1000; ++i) fine = updateMapPeak(fine, quiet, 0.001);
    for (int i = 0; i < 10; ++i) coarse = updateMapPeak(coarse, quiet, 0.1);
    once = updateMapPeak(once, quiet, 1.0);
    EXPECT_NEAR(fine, once, 0.002);
    EXPECT_NEAR(coarse, once, 0.002);
    // After one second, 63 percent of the gap is gone.
    EXPECT_NEAR(once, quiet + (0.9f - quiet) * std::exp(-1.0f), 0.002);

    // The scale falls each time and never goes below the target. It never goes below the minimum.
    float p = peak;
    float last = p;
    for (int i = 0; i < 40; ++i) {
        p = updateMapPeak(p, quiet, 0.25);
        EXPECT_LE(p, last);
        EXPECT_GE(p, quiet);
        last = p;
    }
    EXPECT_NEAR(p, quiet, 0.001);
    EXPECT_GE(updateMapPeak(0.5f, 0.0f, 100.0), mapMinimumPeak);
    // A long pause between two drawings counts as 2 seconds at most.
    EXPECT_NEAR(updateMapPeak(0.9f, 0.05f, 3600.0), updateMapPeak(0.9f, 0.05f, 2.0), 1e-6);
}

// A silence stays a gap in the pitch line at each zoom level. The rule uses the real width of a bin.
TEST(MapLayout, TP_MAP_04_ASilenceStaysAGapAtEachZoomLevel)
{
    constexpr int rate = 32000;
    for (double span : { 5.0, 15.0, 60.0, 150.0, 600.0, 3600.0, 86400.0 }) {
        const long long bin = mapBinSamples(span, 1500, rate);
        const double perBin = (double) bin / rate;
        const int maxGap = mapMaxGapBins(perBin);
        EXPECT_GE(maxGap, 1);
        // The line joins no bins that are more than 70 ms apart, or one bin when a bin is wider than that.
        EXPECT_LE((maxGap - 1) * perBin, 0.07 + 1e-9) << span;

        for (int silence = 0; silence <= maxGap + 3; ++silence) {
            const int after = 15 + silence;
            const int total = after + 10;
            std::vector<float> pitch((size_t) total, 0.0f);
            for (int i = 10; i < 15; ++i) pitch[(size_t) i] = 200.0f;
            for (int i = after; i < after + 5; ++i) pitch[(size_t) i] = 210.0f;

            const auto pieces = mapPitchPieces(pitch, 0, total, maxGap);
            bool bridged = false;
            for (const MapPitchPiece &p : pieces) bridged = bridged || (p.from == 14 && p.to == after);

            // A silence of 70 ms or more is a gap. A short silence is joined.
            const double silentSeconds = silence * perBin;
            if (silentSeconds > 0.07 + 1e-9) EXPECT_FALSE(bridged) << "span " << span << " silence " << silence << " bins";
            EXPECT_EQ(bridged, silence + 1 <= maxGap) << "span " << span << " silence " << silence << " bins";
        }
    }

    // A map that is zoomed out far has a bin wider than 70 ms. One empty bin is a gap.
    const long long coarse = mapBinSamples(3600.0, 1500, rate);
    EXPECT_GT((double) coarse / rate, 0.07);
    EXPECT_EQ(mapMaxGapBins((double) coarse / rate), 1);

    // Bins before the first drawn bin do not make a piece that starts at the left of the picture by mistake.
    std::vector<float> pitch(30, 0.0f);
    pitch[2] = 100.0f;
    pitch[3] = 100.0f;
    pitch[25] = 100.0f;
    pitch[26] = 100.0f;
    EXPECT_EQ(mapPitchPieces(pitch, 20, 30, 2).size(), 1u);
}

// The scale of the pitch line fits data below 60 Hz and above 600 Hz, with a margin, and covers an octave.
TEST(MapLayout, TP_MAP_05_ThePitchScaleFitsDataBelow60HzAndAbove600Hz)
{
    const MapPitchScale start;
    // A low voice: the scale grows at once and reaches below the lowest pitch.
    const MapPitchScale low = updateMapPitchScale(start, 40.0, 300.0, 0.016);
    EXPECT_LT(low.low, 40.0);
    EXPECT_NEAR(low.low, 40.0 / 1.15, 1e-9);
    // The upper end shrinks slowly: after 16 ms it is still near the old value.
    EXPECT_LT(low.high, start.high);
    EXPECT_GT(low.high, 0.98 * start.high);

    // A high voice: the scale reaches above the highest pitch.
    const MapPitchScale high = updateMapPitchScale(start, 200.0, 900.0, 0.016);
    EXPECT_GT(high.high, 900.0);
    EXPECT_NEAR(high.high, 900.0 * 1.15, 1e-9);

    // The scale covers an octave at least, also for a flat pitch, and the pitch lies in the middle.
    MapPitchScale flat = { 60.0, 600.0 };
    for (int i = 0; i < 200; ++i) flat = updateMapPitchScale(flat, 200.0, 205.0, 0.25);
    EXPECT_GE(flat.high / flat.low, 2.0 - 1e-6);
    EXPECT_NEAR(std::sqrt(flat.low * flat.high), std::sqrt(200.0 / 1.15 * 205.0 * 1.15), 1.0);

    // The scale shrinks slowly: a tenth of a second moves it less than 10 percent of the way.
    const MapPitchScale wide = { 30.0, 1000.0 };
    const MapPitchScale shrunk = updateMapPitchScale(wide, 100.0, 200.0, 0.1);
    EXPECT_GT(shrunk.low, wide.low);
    EXPECT_LT(shrunk.low, wide.low + 0.1 * (100.0 / 1.15 - wide.low) + 1e-6);
    EXPECT_GT(shrunk.high, 200.0 * 1.15);
    EXPECT_LT(shrunk.high, wide.high);

    // A map with no pitch leaves the scale as it is.
    const MapPitchScale same = updateMapPitchScale(wide, 0.0, 0.0, 1.0);
    EXPECT_EQ(same.low, wide.low);
    EXPECT_EQ(same.high, wide.high);
    EXPECT_EQ(updateMapPitchScale(wide, -5.0, 100.0, 1.0).low, wide.low);
}

// A drag of an edge below 0.5 seconds or above 24 hours gives a span in the limits.
TEST(MapLayout, TP_MAP_08_ADragToASpanBelowHalfASecondOrAbove24HoursClamps)
{
    for (double span : { 0.49, 0.1, 0.0, -5.0, -1e9 }) {
        EXPECT_EQ(dragViewSpan(span, 600.0, 150.0), 0.5) << span;
    }
    // A span above the longer of the session and the map time is cut to that time.
    EXPECT_EQ(dragViewSpan(5000.0, 600.0, 150.0), 600.0);
    EXPECT_EQ(dragViewSpan(5000.0, 60.0, 150.0), 150.0);
    // A session of more than 24 hours still has a span of 24 hours at most.
    EXPECT_EQ(dragViewSpan(1e9, 1e7, 150.0), 86400.0);
    EXPECT_EQ(dragViewSpan(86400.0, 1e7, 150.0), 86400.0);
    EXPECT_EQ(dragViewSpan(86400.04, 1e7, 150.0), 86400.0);
    // A span in the limits has one decimal.
    EXPECT_EQ(dragViewSpan(12.34, 600.0, 150.0), 12.3);
    EXPECT_EQ(dragViewSpan(0.5, 600.0, 150.0), 0.5);
    // A session of 0 seconds and a window of 0 still give the minimum span.
    EXPECT_EQ(dragViewSpan(3.0, 0.0, 0.0), 0.5);
    EXPECT_EQ(clampViewSpan(0.1), 0.5);
    EXPECT_EQ(clampViewSpan(1e9), 86400.0);
}

// A view span that is longer than the map window widens the map, so the frame has room at both sides.
TEST(MapLayout, TP_MAP_13_AViewSpanLongerThanTheMapWindowWidensTheMap)
{
    // The map has 25 percent more than the view span.
    EXPECT_DOUBLE_EQ(mapSpanFor(1000.0, 150.0, 200.0), 250.0);
    EXPECT_DOUBLE_EQ(mapSpanFor(1000.0, 150.0, 100.0), 150.0);
    // The map never shows more than the session.
    EXPECT_DOUBLE_EQ(mapSpanFor(220.0, 150.0, 200.0), 220.0);

    // A live view: the frame ends at the end of the map, and its left edge is well inside the map.
    MapState live;
    const MapView a = updateMapView(live, 1000.0, 150.0, 200.0, 1000.0, true);
    EXPECT_DOUBLE_EQ(a.span, 250.0);
    EXPECT_DOUBLE_EQ(a.start, 750.0);
    const double leftEdge = (1000.0 - 200.0 - a.start) / a.span;
    EXPECT_GE(leftEdge, 0.1);
    EXPECT_LE(leftEdge, 0.3);

    // A past view: both edges of the frame are inside the map with room, and the handles can move.
    MapState past;
    for (double end : { 300.0, 500.0, 900.0 }) {
        const MapView v = updateMapView(past, 1000.0, 150.0, 200.0, end, false);
        EXPECT_DOUBLE_EQ(v.span, 250.0);
        EXPECT_GE(end - 200.0, v.start - 1e-9) << end;
        EXPECT_LE(end, v.start + v.span + 1e-9) << end;
    }
}

// The length of the map never goes back while the capture runs. After a pause or a reset it can go back.
TEST(MapLayout, TP_MAP_26_TheMapLengthNeverGoesBackWhileTheCaptureRuns)
{
    MapClock clock;
    // The capture starts. The sound arrives in blocks of 0.1 s every 0.1 s, and the clock is fake.
    EXPECT_DOUBLE_EQ(updateShownDuration(clock, 10.0, 100.0, true), 10.0);
    // Between two blocks, the length grows with the clock.
    EXPECT_NEAR(updateShownDuration(clock, 10.0, 100.05, true), 10.05, 1e-9);
    // The growth has a limit of 0.25 s.
    EXPECT_NEAR(updateShownDuration(clock, 10.0, 101.0, true), 10.25, 1e-9);
    // A block arrives that is shorter than the length that the map shows. The map does not go back.
    EXPECT_NEAR(updateShownDuration(clock, 10.2, 101.1, true), 10.25, 1e-9);
    // A longer block moves the map on.
    EXPECT_NEAR(updateShownDuration(clock, 10.4, 101.2, true), 10.4, 1e-9);

    // The shown length never decreases in a long run with irregular blocks.
    double previous = 0.0;
    double duration = 10.4;
    double now = 101.2;
    for (int i = 0; i < 2000; ++i) {
        now += (i % 7) * 0.013;
        if (i % 5 == 0) duration += 0.1 + (i % 3) * 0.02;
        const double shown = updateShownDuration(clock, duration, now, true);
        EXPECT_GE(shown, previous - 1e-12) << i;
        EXPECT_GE(shown, duration - 1e-12);
        EXPECT_LE(shown, duration + mapSmoothLimit + 1e-9);
        previous = shown;
    }

    // A pause: the capture does not run, and the shown length is the real length.
    const double paused = updateShownDuration(clock, duration, now + 5.0, false);
    EXPECT_DOUBLE_EQ(paused, duration);

    // A reset: the session starts again at a shorter length. The map goes back, with a minimum of 1 second.
    EXPECT_NEAR(updateShownDuration(clock, 0.0, now + 6.0, true), 1.0, 1e-9);
    EXPECT_NEAR(updateShownDuration(clock, 0.5, now + 6.1, true), 1.0, 1e-9);
    EXPECT_NEAR(updateShownDuration(clock, 3.0, now + 6.2, true), 3.0, 1e-9);

    // A restart at a length that is shorter but not 0 also goes back.
    MapClock other;
    updateShownDuration(other, 50.0, 0.0, true);
    EXPECT_NEAR(updateShownDuration(other, 20.0, 1.0, true), 20.0, 1e-9);
}
