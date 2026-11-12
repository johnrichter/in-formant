#ifndef GUI_CANVAS_RENDERER_H
#define GUI_CANVAS_RENDERER_H

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QQuickFramebufferObject>
#include <QColor>
#include <cmath>
#include <string>

#include "font.h"
#include "shaders/spec.h"
#include "shaders/circle.h"
#include "shaders/shapes.h"
#include "theme.h"
#include "../context/datastore.h"

namespace Main {
    class RenderContext;
}

namespace Gui {

    // The spectrogram texture is a ring with one column for each time slice and one row for each
    // frequency bin. The size of the texture must match the constants in the spectrogram shader.
    constexpr int spectrogramColumns = 4096;
    constexpr int spectrogramRows = 2560;

    class CanvasRenderer : protected QOpenGLExtraFunctions {
    public:
        void initialize(Main::RenderContext *renderContext);
        void cleanup();
        void synchronize(QQuickFramebufferObject *item);
        
        void render();

        void setZoomScale(double zoomScale);

        void drawTextNormal(float x, float y, const QColor &color, const std::string &text);
        void drawTextNormalOutlined(float x, float y, const QColor &color, const std::string &text, const QColor &outlineColor = Qt::black);
        QRect textBoundsNormal(const std::string &text);

        void drawTextSmall(float x, float y, const QColor &color, const std::string &text);
        void drawTextSmallOutlined(float x, float y, const QColor &color, const std::string &text, const QColor &outlineColor = Qt::black);
        QRect textBoundsSmall(const std::string &text);

        void drawTextSmaller(float x, float y, const QColor &color, const std::string &text);
        void drawTextSmallerOutlined(float x, float y, const QColor &color, const std::string &text, const QColor &outlineColor = Qt::black);
        QRect textBoundsSmaller(const std::string &text);

        void drawScatterWithOutline(const rpm::vector<QPointF> &points, float radius, const QColor &fillColor, const QColor &outlineColor = Qt::black);
    
        void drawPoint(const QPointF &point, float radius, const QColor &color);

        void drawLine(float x1, float y1, float x2, float y2, const QColor &color, float thickness);

        // Many line segments of the same width and colour. segments has four floats for each
        // segment: x1, y1, x2, y2. The width is in pixels at 100 percent zoom.
        void drawSegments(const float *segments, int count, float width, const QColor &color);

        // A line through the points. The line has an outline. An outline colour with no alpha
        // draws no outline.
        void drawPolyline(const rpm::vector<QPointF> &points, float width, const QColor &color, const QColor &outline);

        // A rectangle with round corners. The sizes are in pixels. This function does not scale them.
        void drawRoundedRect(float x, float y, float width, float height, float radius,
                             const QColor &fill, const QColor &border = Qt::transparent, float borderWidth = 0.0f);

        // Text at an angle. size is 0 (normal), 1 (small), or 2 (smaller). The angle is in degrees
        // and the text turns counter-clockwise around its start.
        void drawTextRotated(int size, float x, float y, float angleDegrees, const QColor &color,
                             const std::string &text, const QColor &outline = Qt::black);

        // The size of the text in pixels, for a font size 0, 1, or 2.
        QRect textBoundsOfSize(int size, const std::string &text);

        // The factor between the pixels of the screen and the logical pixels of the interface.
        double devicePixelRatio() const { return mDevicePixelRatio; }

        void prepareSpectrogramDraw();

        // The filter of the spectrogram. quality 0 is the bilinear filter, and 1 is the cubic filter.
        // blur is in the range 0 to 1.
        void setSpectrogramFilter(int quality, float blur);
        void drawSpectrogram(
                int xOffset,
                int chunkSize1,
                int chunkSize2,
                int uploadRows,
                int totalSize,
                const std::array<GLint, spectrogramColumns>& nffts,
                const std::array<GLfloat, spectrogramColumns>& sampleRates,
                const rpm::vector<GLfloat>& chunkData1,
                const rpm::vector<GLfloat>& chunkData2,
                FrequencyScale freqScale,
                float minFrequency,
                float maxFrequency,
                float maxGain,
                const QVector<QRgb>& colorTable,
                float sliceTimeStart,
                float sliceTimeEnd,
                float timeStart,
                float timeEnd);

        QRect viewport() const;

        // Which slices the spectrogram texture holds. The texture lives as long as the renderer, so
        // this state lives as long as the renderer too. The painter of one frame reads and writes it.
        struct SpectrogramStored {
            int offset = 0;
            double lastTime = -HUGE_VAL;
            unsigned long long imageId = 0;
        };
        SpectrogramStored &spectrogramStored() { return mSpectrogramStored; }

    private:
        SpectrogramStored mSpectrogramStored;

        void initFonts();
        void initShaders();
        void deleteFonts();
        void deleteShaders();

        void initTexture(GLuint &texture, int width, int height);
        QOpenGLShaderProgram *createShaderProgram(const char *vertexSource, const char *fragmentSource);

        void drawText(Font *font, float x, float y, const QColor &color, const std::string &text);
        void drawGlyphs(Font *font, float x, float y, float angleDegrees, const QColor &color, const std::string &text, const QColor *outlineColor);
        void drawCircles(const rpm::vector<QPointF> &points, float radius, const QColor &color);
        void setProjections();
        void drawTextOutlined(Font *font, float x, float y, const QColor &color, const std::string &text, const QColor &outlineColor = Qt::black);
        QRect textBounds(Font *font, const std::string &text);

        Main::RenderContext *mRenderContext;

        double mDevicePixelRatio;
        int mWidth, mHeight;
        double mDpi;
        double mZoomScale, mZoomScaleText;

        Font *mFontNormal;
        Font *mFontSmall;
        Font *mFontSmaller;

        QOpenGLShaderProgram *mTextProgram;
        GLuint mTextVao;

        QOpenGLShaderProgram *mSpecProgram;
        GLuint mSpecVao;

        QOpenGLShaderProgram *mCircleProgram;
        GLuint mCircleVao;

        QOpenGLShaderProgram *mSegmentProgram;
        GLuint mSegmentVao;

        QOpenGLShaderProgram *mRectProgram;
        GLuint mRectVao;

        GLuint mSpecTex;
        GLuint mHeaderTex;

        int mSpecQuality = 1;
        float mSpecBlur = 0.33f;

        GLuint mCmapTex;
        QVector<QRgb> mCmapCache;

        // Uniform locations of the spectrogram program.
        struct {
            int projection, frequencyScale, minFrequency, maxFrequency, gainScale, quality, blur, quad;
        } mSpecLoc;

        // Uniform locations of the text program.
        struct {
            int projection, rects, uvs, glyphCount, passCount, offsets, textColor, outlineColor, origin, rotation;
        } mTextLoc;

        // Uniform locations of the circle program.
        struct {
            int projection, centers, radius, fillColor;
        } mCircleLoc;

        // Uniform locations of the segment program.
        struct {
            int projection, segments, halfWidth, color;
        } mSegmentLoc;

        // Uniform locations of the rectangle program.
        struct {
            int projection, rect, radius, fillColor, borderColor, borderWidth;
        } mRectLoc;
    };

}

#endif // GUI_CANVAS_RENDERER_H
