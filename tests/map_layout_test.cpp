#include "check.h"
#include "../src/gui/maplayout.h"

using namespace Gui;

int main()
{
    // A short session shows whole, and the map follows nothing.
    {
        MapState st;
        const MapView v = updateMapView(st, 40.0, 150.0, 15.0, 40.0, true);
        CHECK(v.start == 0.0);
        CHECK_NEAR(v.span, 40.0, 1e-9);
    }

    // A very short session has a map of one second at least.
    {
        MapState st;
        const MapView v = updateMapView(st, 0.2, 150.0, 15.0, 0.2, true);
        CHECK_NEAR(v.span, 1.0, 1e-9);
    }

    // A long live session shows the newest 150 s.
    {
        MapState st;
        const MapView v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
        CHECK_NEAR(v.start, 450.0, 1e-9);
        CHECK_NEAR(v.span, 150.0, 1e-9);
        const MapView w = updateMapView(st, 610.0, 150.0, 15.0, 610.0, true);
        CHECK_NEAR(w.start, 460.0, 1e-9);
    }

    // A person scrolls the map back. The map stays there while the capture continues.
    {
        MapState st;
        MapView v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
        scrollMap(st, 600.0, v, -200.0);
        CHECK(!st.follow);
        v = updateMapView(st, 620.0, 150.0, 15.0, 620.0, true);
        CHECK_NEAR(v.start, 250.0, 1e-9);
        // Scrolling to the end makes the map follow again.
        scrollMap(st, 620.0, v, 1000.0);
        CHECK(st.follow);
        v = updateMapView(st, 640.0, 150.0, 15.0, 640.0, true);
        CHECK_NEAR(v.start, 490.0, 1e-9);
    }

    // The live view returns after the person looked at the past. The map follows again.
    {
        MapState st;
        MapView v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
        scrollMap(st, 600.0, v, -300.0);
        v = updateMapView(st, 600.0, 150.0, 15.0, 100.0, false);
        v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
        CHECK(st.follow);
        CHECK_NEAR(v.start, 450.0, 1e-9);
    }

    // A frame that leaves the map brings the map along. A frame inside the map does not.
    {
        MapState st;
        MapView v = updateMapView(st, 600.0, 150.0, 15.0, 600.0, true);
        v = updateMapView(st, 600.0, 150.0, 15.0, 500.0, false);
        CHECK_NEAR(v.start, 450.0, 1e-9);
        v = updateMapView(st, 600.0, 150.0, 15.0, 100.0, false);
        CHECK(v.start <= 100.0 - 15.0);
        const double before = v.start;
        v = updateMapView(st, 600.0, 150.0, 15.0, before + 60.0, false);
        CHECK_NEAR(v.start, before, 1e-9);
    }

    // A frame that is longer than the window widens the map.
    {
        MapState st;
        const MapView v = updateMapView(st, 1000.0, 150.0, 400.0, 1000.0, true);
        CHECK_NEAR(v.span, 500.0, 1e-9);
        CHECK_NEAR(v.start, 500.0, 1e-9);
    }

    // The width of a bin is a round value that is not much wider than a pixel.
    {
        const long long a = mapBinSamples(150.0, 1500, 32000);
        CHECK(a >= 3200 && a <= (long long) (3200 * 1.34));
        // A small change of the span keeps the same bin width most of the time.
        CHECK(mapBinSamples(150.0, 1500, 32000) == mapBinSamples(151.0, 1500, 32000));
        CHECK(mapBinSamples(0.5, 1500, 32000) >= 1);
    }

    // The bins cover the map, and the first bin starts at a multiple of the bin width.
    {
        MapView v;
        v.start = 100.3;
        v.span = 50.0;
        const long long bin = mapBinSamples(v.span, 1000, 32000);
        const MapBins bins = mapBinRange(v, bin, 32000);
        const double perBin = (double) bin / 32000;
        CHECK(bins.first * perBin <= v.start);
        CHECK((bins.first + bins.count) * perBin >= v.start + v.span);
        // The same absolute bin has the same index after the map scrolls by whole bins.
        MapView scrolled = v;
        scrolled.start += 10 * perBin;
        const MapBins moved = mapBinRange(scrolled, bin, 32000);
        CHECK(moved.first == bins.first + 10);
    }

    // The span of the main view stays in its limits.
    {
        CHECK(clampViewSpan(0.1) == minViewSpan);
        CHECK(clampViewSpan(1e9) == maxViewSpan);
        CHECK(clampViewSpan(12.0) == 12.0);
    }

    // A drag of a frame edge sets a span with one decimal, and the span stays inside the session or the map time.
    {
        CHECK_NEAR(dragViewSpan(37.4999999, 600.0, 150.0), 37.5, 1e-9);
        CHECK_NEAR(dragViewSpan(7.252709999, 14.0, 150.0), 7.3, 1e-9);
        CHECK(dragViewSpan(1e9, 600.0, 150.0) == 600.0);
        CHECK(dragViewSpan(1e9, 20.0, 150.0) == 150.0);
        CHECK(dragViewSpan(-5.0, 600.0, 150.0) == minViewSpan);
        CHECK(dragViewSpan(1e9, 1e9, 1e9) == maxViewSpan);
    }

    return TEST_RESULT();
}
