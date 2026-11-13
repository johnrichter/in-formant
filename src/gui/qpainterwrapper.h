#ifndef QPAINTER_WRAPPER_H
#define QPAINTER_WRAPPER_H

#include "rpcxx.h"
#include "canvas_renderer.h"
#include "renderoptions.h"
#include "readoutlayout.h"
#include "graphlayout.h"
#include "plotmap.h"
#include "ticks.h"
#include <map>
#include <string>
#include <vector>
#include "../timetrack.h"
#include "../context/datastore.h"
#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <QImage>

struct SpectrogramTextureData {
    double timeStart, timeEnd;
    double sampleRate;
    int nfft, segmentLen;
    rpm::vector<GLfloat> texture;
};

class QPainterWrapper {
public:
    QPainterWrapper(Gui::CanvasRenderer *p);
    
    QRect viewport() const;

    void setZoom(double scale);

    void setTimeRange(double start, double end);

    void setFrequencyScale(FrequencyScale scale);
    void setMinFrequency(double minFrequency);
    void setMaxFrequency(double maxFrequency);
    void setMaxGain(double maxGain);

    // quality 0 is the fast bilinear filter, and 1 is the cubic filter. blur is in the range 0 to 1.
    void setSpectrogramFilter(int quality, double blur);

    // Compute the margins for the axes and the place of the readout boxes from the options and the
    // fonts. The call comes before any drawing of a frame. trackNames has the names of the tracks in
    // the readouts. The graph is the canvas without the margins at the left and at the bottom.
    void updateLayout(const std::vector<std::string> &trackNames);

    // Limit the drawing to the graph, so that tracks do not cross into the margins of the axes.
    void beginPlot();
    void endPlot();

    void drawTimeAxis();
    void drawFrequencyScale();

    void drawTimeSeries(const rpm::vector<double> &y, double xstart, double xend, double ymin, double ymax); 

    // The options of the display. They set the style of the tracks and the readouts, and the colours.
    void setOptions(const Gui::RenderOptions &options);
    const Gui::RenderOptions &options() const { return mOptions; }

    // The colour map of the original application.
    static const QVector<QRgb>& defaultColorMap();

    // Draw a track as dots, as a line, or as both, as the options say.
    // The style is a mask of bits: 1 draws the dots, and 2 draws the line.
    void drawFrequencyTrack(const TimeTrack<double>::const_iterator& begin,
                            const TimeTrack<double>::const_iterator& end,
                            const QColor &color, int style);

    void drawFrequencyTrack(const OptionalTimeTrack<double>::const_iterator& begin,
                            const OptionalTimeTrack<double>::const_iterator& end,
                            const QColor &color, int style);

    // True when the pixel is inside the graph, not in a margin.
    bool plotContains(double x, double y) const;

    // The time at a pixel column of the graph.
    double timeAtX(double x) const;

    // The bar of the picked moment, and the values of the tracks at that moment. A label has a
    // dot on the bar at the value. The labels go to the side of the bar that has the room.
    void drawCursor(double time, const std::vector<Gui::CursorValue> &values);

    // The boxes of the legend and of the statistics, and the box of the frame timing.
    void drawHud(const std::vector<Gui::HudTrack> &tracks);

    double mapTimeToX(double time);
    // The pitch zones as bands behind the tracks. The bands follow the frequency scale.
    void drawFrequencyBands();
    // Horizontal lines across the graph at the ticks of the frequency axis. They are behind the tracks.
    void drawFrequencyGrid();
    // Fill the graph with the quietest colour of the theme. This is the first drawing of the graph.
    void fillPlotBackground();

    // The ticks of the frequency axis for the current options. The axis and the grid use the same ticks.
    std::vector<Gui::Tick> frequencyTicks();
    // The colour of a grid line, from the quiet colour of the theme. The scale changes the opacity.
    QColor gridColor(double alphaScale) const;
    // The size index of every label on the axes. One size keeps the axes in balance.
    static constexpr int axisLabelSize = 4;

    double mapFrequencyToY(double frequency);

    using SliceIterator = TimeTrack<Main::SpectrogramCoefs>::const_iterator;

    // Draw the slices in the range. The function stores only the slices that are new since the last call.
    void drawSpectrogram(SliceIterator begin, SliceIterator end);

    // Draw a spectrogram that is already in the layout of the texture. The call uploads the picture
    // when the number is new, and only draws it for the next calls with the same number.
    void drawSpectrogramImage(unsigned long long id, const rpm::vector<float> &data, int rows, int columns,
                              const std::array<int, Gui::spectrogramColumns> &sliceSizes,
                              const std::array<float, Gui::spectrogramColumns> &sampleRates,
                              double timeStart, double timeEnd);

    // Forget the slices in the texture. The next call of drawSpectrogram stores all of its slices.
    // The caller uses this when the slices come from another source or from another time.
    void invalidateSpectrogram();

    static double mapTimeToX(double time, int width, double startTime, double endTime);
    static double mapFrequencyToY(double frequency, int height, FrequencyScale scale, double minFrequency, double maxFrequency);
    static double mapYToFrequency(double y, int height, FrequencyScale scale, double minFrequency, double maxFrequency);

private:
    double transformFrequency(double frequency);
    double inverseFrequency(double value);
    
    Gui::CanvasRenderer *p;

    double mTimeStart;
    double mTimeEnd;

    // The margins of the axes, in pixels.
    int mPlotLeft = 0;
    int mPlotBottom = 0;
    // The space at the right of the graph. It balances the space at the left of the readout boxes.
    int mPlotRight = 0;
    // The height of the bar at the top of the canvas. The graph starts below it.
    int mPlotTop = 0;
    // The height of the row of readout boxes below the time axis. The row is part of the bottom margin.
    int mStripHeight = 0;

    // The layout of the readout boxes of the frame. updateLayout computes it and drawHud reads it.
    Gui::ReadoutLayout mReadout;
    // The graph and the time range, for the map between the time and the pixel.
    Gui::PlotFrame plotFrame() const { return { mPlotLeft, mPlotTop, plotWidth(), plotHeight(), mTimeStart, mTimeEnd, mOptions.timeFlipped }; }
    int plotWidth() const { return viewport().width() - mPlotLeft - mPlotRight; }
    int plotHeight() const { return viewport().height() - mPlotBottom - mPlotTop; }

    FrequencyScale mFrequencyScale;
    double mMinFrequency;
    double mMaxFrequency;
    double mMaxGain;
    Gui::RenderOptions mOptions;
    int mSpectrogramQuality;
    double mSpectrogramBlur;

    // The place of the next slice in the texture, the time of the newest stored slice, and the number
    // of the stored picture. The renderer owns them, because a painter lives for one frame only.
    int &mSpectrogramOffset;
    double &mSpectrogramLastTime;
    unsigned long long &mSpectrogramImageId;

    static double transformFrequency(double frequency, FrequencyScale scale);
    static double inverseFrequency(double value, FrequencyScale scale);

    static Eigen::SparseMatrix<double>& constructTransformY(int h, int vh, FrequencyScale freqScale, double freqMin, double freqMax, FrequencyScale sourceScale, double sourceMin, double sourceMax);

    static QVector<QRgb> cmap;
};

#endif // QPAINTER_WRAPPER_H
