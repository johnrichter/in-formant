#ifndef GUI_RENDER_OPTIONS_H
#define GUI_RENDER_OPTIONS_H

#include "theme.h"
#include <string>
#include <vector>

namespace Gui {

    // The bits of the mask of the statistics. Each bit shows one value in the table of statistics.
    enum StatBit {
        StatCurrent  = 0x1,
        StatMin      = 0x2,
        StatP5       = 0x4,
        StatMean     = 0x8,
        StatMedian   = 0x10,
        StatStdDev   = 0x20,
        StatP95      = 0x40,
        StatMax      = 0x80,
        StatRange    = 0x100,
        StatVoiced   = 0x200,
        StatNote     = 0x400,
        StatCV       = 0x800,
        StatCount    = 0x1000,
    };

    struct RenderOptions {
        bool showDebug = false;
        bool showAxisLabels = true;
        bool showAxisTitles = true;
        bool showLegend = true;
        bool showStats = false;
        // Hide the frame timing box. The recorder grabs the whole window, so the box would be in the video.
        bool hideDebug = false;

        // The pitch zones in the graph, and their four edges in Hz.
        bool showBands = false;
        double bandEdges[4] = { 60.0, 150.0, 180.0, 300.0 };

        // The readout boxes: the keys in their order, the side of the graph, and the box of each track.
        // Index 0 is the pitch. Indexes 1 to 4 are the formants F1 to F4.
        std::string readoutOrder = "legend,pitch,f1,f2,f3,f4";
        bool readoutLeft = false;
        bool trackBoxShown[5] = { true, true, true, true, true };

        int freqTickMode = 0;
        double freqTickStep = 100.0;
        int timeTickMode = 0;
        double timeTickStep = 1.0;

        // The style of each track, as bits: 1 dots, 2 line. Index 0 is the pitch. Indexes 1 to 4 are
        // the formants F1 to F4, and the formants after F4 use the style of F4.
        int trackStyle[5] = { 3, 3, 3, 3, 3 };
        // 0 to 1.
        double lineSmoothness = 0.35;
        double lineWidth = 2.0;
        double dotRadius = 2.0;

        int statsMask = StatCurrent | StatMin | StatMean | StatP95 | StatMax;

        Theme theme;
    };

    // The values of one track for the legend and for the table of statistics.
    // The value of a track at the moment that the user picked.
    struct CursorValue {
        std::string name;
        QColor color;
        bool isPitch = false;
        double frequency = 0.0;
    };

    struct HudTrack {
        std::string name;
        QColor color;
        bool isPitch = false;

        // The values in the window of the statistics. A frame with no value is not in this list.
        std::vector<double> values;

        // The number of frames in the window, with and without a value.
        size_t frames = 0;

        // The number of frames with a value. It is larger than the list of values when a long
        // window goes through a reduction. A value of zero means that the list is complete.
        size_t count = 0;
    };

}

#endif // GUI_RENDER_OPTIONS_H
