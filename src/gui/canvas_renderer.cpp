#include "canvas.h"
#include "../context/timings.h"
#include "../context/rendercontext.h"
#include "qpainterwrapper.h"
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <QMatrix4x4>
#include <QQuickWindow>
#include <QScreen>
#include <QVector2D>
#include <QVector4D>

#ifdef _WIN32
/**
 * Export these symbols to signal to use dedicated graphics card on laptops.
 */
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

using namespace Gui;

void CanvasRenderer::initialize(Main::RenderContext *renderContext)
{
    mRenderContext = renderContext;
    mSpectrogramStored = SpectrogramStored();
    initializeOpenGLFunctions();

#ifdef GL_DEBUG_OUTPUT
    const char *glDebugEnv = std::getenv("GL_DEBUG_OUTPUT");
    if (glDebugEnv != nullptr && strcmp(glDebugEnv, "1") == 0) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback([](GLenum source,
                                    GLenum type,
                                    GLuint id,
                                    GLenum severity,
                                    GLsizei length,
                                    const GLchar *message,
                                    const void *userParam) {
                                        std::cout << message << std::endl;
                                    }, nullptr);
    }
#endif

    mDevicePixelRatio = 1.0;
    mDpi = 96.0;

    mZoomScale = 1.0;
    mZoomScaleText = 1.0;

    initFonts();
    initShaders();

    // The spectrogram data. A value is a 32-bit float, because the power of a slice covers a wide range.
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &mSpecTex);
    glBindTexture(GL_TEXTURE_2D, mSpecTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, spectrogramColumns, spectrogramRows, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // The header of each column: the number of bins, and the sample rate.
    glGenTextures(1, &mHeaderTex);
    glBindTexture(GL_TEXTURE_2D, mHeaderTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, spectrogramColumns, 1, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void CanvasRenderer::setSpectrogramFilter(int quality, float blur)
{
    mSpecQuality = quality;
    mSpecBlur = blur;
}

void CanvasRenderer::cleanup()
{
    deleteFonts();
    deleteShaders();

    glDeleteTextures(1, &mSpecTex);
    glDeleteTextures(1, &mHeaderTex);
    glDeleteTextures(1, &mCmapTex);
}

void CanvasRenderer::synchronize(QQuickFramebufferObject *item)
{
    const double devicePixelRatio = item->window()->devicePixelRatio();
    const double dpi = item->window()->screen()->logicalDotsPerInch();

    mWidth = item->width() * devicePixelRatio;
    mHeight = item->height() * devicePixelRatio;

    // This function runs for every frame. Building the fonts creates hundreds of textures,
    // so build them again only when the glyph size changes.
    if (devicePixelRatio != mDevicePixelRatio || dpi != mDpi) {
        mDevicePixelRatio = devicePixelRatio;
        mDpi = dpi;

        deleteFonts();
        initFonts();
    }
}

void CanvasRenderer::render()
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
  
    glViewport(0, 0, mWidth, mHeight);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    setProjections();

    QPainterWrapper painter(this);
    mRenderContext->render(&painter);
}

void CanvasRenderer::setZoomScale(double zoomScale)
{
    if (mZoomScale != zoomScale) {
        mZoomScale = zoomScale;

        // Rescale max zoom from 0.5-5x to 0.75-2x for fonts.
        if (mZoomScale > 1) {
            mZoomScaleText = 1 + (mZoomScale - 1) * (2.0 / 5.0);
        }
        else {
            mZoomScaleText = 1 - (1 - mZoomScale) * (0.5 / 0.75);
        }

        deleteFonts();
        initFonts();
    }
}

void CanvasRenderer::drawTextNormal(float x, float y, const QColor &color, const std::string &text)
{
    drawText(mFontNormal, x, y, color, text);
}

void CanvasRenderer::drawTextNormalOutlined(float x, float y, const QColor &color, const std::string &text, const QColor &outlineColor)
{
    drawTextOutlined(mFontNormal, x, y, color, text, outlineColor);
}

QRect CanvasRenderer::textBoundsNormal(const std::string &text)
{
    return textBounds(mFontNormal, text);
}

void CanvasRenderer::drawTextSmall(float x, float y, const QColor &color, const std::string &text)
{
    drawText(mFontSmall, x, y, color, text);
}

void CanvasRenderer::drawTextSmallOutlined(float x, float y, const QColor &color, const std::string &text, const QColor &outlineColor)
{
    drawTextOutlined(mFontSmall, x, y, color, text, outlineColor);
}

QRect CanvasRenderer::textBoundsSmall(const std::string &text)
{
    return textBounds(mFontSmall, text);
}

void CanvasRenderer::drawTextSmaller(float x, float y, const QColor &color, const std::string &text)
{
    drawText(mFontSmaller, x, y, color, text);
}

void CanvasRenderer::drawTextSmallerOutlined(float x, float y, const QColor &color, const std::string &text, const QColor &outlineColor)
{
    drawTextOutlined(mFontSmaller, x, y, color, text, outlineColor);
}

QRect CanvasRenderer::textBoundsSmaller(const std::string &text)
{
    return textBounds(mFontSmaller, text);
}

void CanvasRenderer::drawScatterWithOutline(const rpm::vector<QPointF> &points, float radius, const QColor &fillColor, const QColor &outlineColor)
{
    // All outlines are below all fills.
    drawCircles(points, radius + 0.6667, outlineColor);
    drawCircles(points, radius, fillColor);
}

void CanvasRenderer::drawPoint(const QPointF &point, float radius, const QColor &color)
{
    rpm::vector<QPointF> points(1, point);
    drawCircles(points, radius, color);
}

void CanvasRenderer::drawCircles(const rpm::vector<QPointF> &points, float radius, const QColor &color)
{
    if (points.empty()) {
        return;
    }

    radius *= mZoomScale * (mDpi * mDevicePixelRatio) / 96;

    mCircleProgram->bind();
    mCircleProgram->setUniformValue(mCircleLoc.radius, radius);
    mCircleProgram->setUniformValue(mCircleLoc.fillColor, color.redF(), color.greenF(), color.blueF());

    glBindVertexArray(mCircleVao);

    // One draw call draws a limited number of circles. Draw the points in chunks.
    constexpr int maxCircles = Shaders::circleMaxCircles;
    static float centers[2 * maxCircles];

    for (size_t first = 0; first < points.size(); first += maxCircles) {
        const int count = (int) std::min<size_t>(maxCircles, points.size() - first);

        for (int i = 0; i < count; ++i) {
            centers[2 * i + 0] = (float) points[first + i].x();
            centers[2 * i + 1] = (float) points[first + i].y();
        }

        glUniform2fv(mCircleLoc.centers, count, centers);
        glDrawArrays(GL_TRIANGLES, 0, 6 * count);
    }

    glBindVertexArray(0);

    mCircleProgram->release();
}

void CanvasRenderer::drawLine(float x1, float y1, float x2, float y2, const QColor &color, float thickness)
{
    const float segment[4] = { x1, y1, x2, y2 };
    drawSegments(segment, 1, thickness, color);
}

void CanvasRenderer::drawSegments(const float *segments, int count, float width, const QColor &color)
{
    if (count <= 0) {
        return;
    }

    width *= mZoomScale * (mDpi * mDevicePixelRatio) / 96;

    mSegmentProgram->bind();
    mSegmentProgram->setUniformValue(mSegmentLoc.halfWidth, 0.5f * width);
    mSegmentProgram->setUniformValue(mSegmentLoc.color, color.redF(), color.greenF(), color.blueF(), color.alphaF());

    glBindVertexArray(mSegmentVao);

    constexpr int maxSegments = Shaders::segmentMaxSegments;
    for (int first = 0; first < count; first += maxSegments) {
        const int n = std::min(maxSegments, count - first);
        glUniform4fv(mSegmentLoc.segments, n, segments + 4 * first);
        glDrawArrays(GL_TRIANGLES, 0, 6 * n);
    }

    glBindVertexArray(0);
    mSegmentProgram->release();
}

void CanvasRenderer::drawPolyline(const rpm::vector<QPointF> &points, float width, const QColor &color, const QColor &outline)
{
    if (points.size() < 2) {
        return;
    }

    static std::vector<float> segments;
    segments.resize(4 * (points.size() - 1));
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        segments[4 * i + 0] = (float) points[i].x();
        segments[4 * i + 1] = (float) points[i].y();
        segments[4 * i + 2] = (float) points[i + 1].x();
        segments[4 * i + 3] = (float) points[i + 1].y();
    }

    // The outline is below the line, so the line has no dark marks at the joints.
    if (outline.alpha() > 0) {
        drawSegments(segments.data(), (int) (points.size() - 1), width + 1.6f, outline);
    }
    drawSegments(segments.data(), (int) (points.size() - 1), width, color);
}

void CanvasRenderer::drawRoundedRect(float x, float y, float width, float height, float radius,
                                     const QColor &fill, const QColor &border, float borderWidth)
{
    mRectProgram->bind();

    mRectProgram->setUniformValue(mRectLoc.rect, x, y, width, height);
    mRectProgram->setUniformValue(mRectLoc.radius, radius);
    mRectProgram->setUniformValue(mRectLoc.fillColor, fill.redF(), fill.greenF(), fill.blueF(), fill.alphaF());
    mRectProgram->setUniformValue(mRectLoc.borderColor, border.redF(), border.greenF(), border.blueF(), border.alphaF());
    mRectProgram->setUniformValue(mRectLoc.borderWidth, borderWidth);

    glBindVertexArray(mRectVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    mRectProgram->release();
}

void CanvasRenderer::drawSpectrogram(
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
        float holdTimeEnd,
        float timeStart,
        float timeEnd)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mSpecTex);

    // Upload only the rows that hold data.
    if (chunkSize1 > 0) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset % spectrogramColumns, 0, chunkSize1, uploadRows, GL_RED, GL_FLOAT, chunkData1.data());
    }
    if (chunkSize2 > 0) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chunkSize2, uploadRows, GL_RED, GL_FLOAT, chunkData2.data());
    }

    // The header changes only when new slices arrive.
    if (chunkSize1 + chunkSize2 > 0) {
        static std::array<GLfloat, 2 * spectrogramColumns> headerData;
        for (int x = 0; x < spectrogramColumns; ++x) {
            headerData[2 * x + 0] = nffts[x];
            headerData[2 * x + 1] = sampleRates[x];
        }
        glBindTexture(GL_TEXTURE_2D, mHeaderTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spectrogramColumns, 1, GL_RG, GL_FLOAT, headerData.data());
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    // The colour map changes only when the colour table changes.
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mCmapTex);
    if (colorTable != mCmapCache && colorTable.size() == 256) {
        mCmapCache = colorTable;
        std::array<unsigned char, 256 * 4> pixels;
        for (int i = 0; i < 256; ++i) {
            pixels[4 * i + 0] = qRed(colorTable[i]);
            pixels[4 * i + 1] = qGreen(colorTable[i]);
            pixels[4 * i + 2] = qBlue(colorTable[i]);
            pixels[4 * i + 3] = 255;
        }
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    }
    glActiveTexture(GL_TEXTURE0);

    mSpecProgram->bind();

    // The picture fills the graph, which is the canvas without the margins of the axes. The
    // projection places the picture, and the viewport stays the whole canvas.
    const float w = mWidth - mPlotLeft;
    const float h = mHeight - mPlotBottom;
    glViewport(0, 0, mWidth, mHeight);

    QMatrix4x4 projection;
    projection.ortho(0, mWidth, 0, mHeight, -1, 1);
    mSpecProgram->setUniformValue(mSpecLoc.projection, projection);

    mSpecProgram->setUniformValue(mSpecLoc.frequencyScale, static_cast<int>(freqScale));
    mSpecProgram->setUniformValue(mSpecLoc.minFrequency, minFrequency);
    mSpecProgram->setUniformValue(mSpecLoc.maxFrequency, maxFrequency);
    // The shader works on the amplitude of the slice. A power of 10^(maxGain/20) gives the top of the colour map.
    mSpecProgram->setUniformValue(mSpecLoc.gainScale, float(7.0 / std::sqrt(std::pow(10.0, maxGain / 20.0))));
    mSpecProgram->setUniformValue(mSpecLoc.quality, mSpecQuality);
    mSpecProgram->setUniformValue(mSpecLoc.blur, mSpecBlur);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(mSpecVao);

    // The newest slice is centred on a time that is earlier than the end of the view. The picture
    // holds the newest slice up to holdTimeEnd, so that no black bar shows at the right edge.
    float x2 = mPlotLeft + w * (std::max(sliceTimeEnd, holdTimeEnd) - timeStart) / (timeEnd - timeStart);
    float x1 = mPlotLeft + w * (sliceTimeStart - timeStart) / (timeEnd - timeStart);
    float y2 = h;
    float y1 = 0;

    // The visible slices end at the column after the newest upload. Before this was added, the
    // right edge used the column where the new data starts, so the picture jumped back by the
    // number of new slices in each frame that brought new data.
    // The column numbers can grow very large, so the end column wraps. Large numbers lose the
    // precision of a float after some hours.
    const int xEnd = (xOffset + chunkSize1 + chunkSize2) % spectrogramColumns;
    float holdColumns = 0.0f;
    if (holdTimeEnd > sliceTimeEnd && sliceTimeEnd > sliceTimeStart) {
        holdColumns = float(totalSize) * (holdTimeEnd - sliceTimeEnd) / (sliceTimeEnd - sliceTimeStart);
    }
    float texX1 = float(xEnd - totalSize) / float(spectrogramColumns);
    float texX2 = (float(xEnd) + holdColumns) / float(spectrogramColumns);
    // The shader holds the position at the centre of the newest column.
    mSpecProgram->setUniformValue(mSpecLoc.lastPosition, float(xEnd) - 0.5f);
    float texY1 = 0.0f;
    float texY2 = 1.0f;

    const QVector4D quad[4] = {
        { x2, y1, texX2, texY2 },
        { x2, y2, texX2, texY1 },
        { x1, y2, texX1, texY1 },
        { x1, y1, texX1, texY2 },
    };

    mSpecProgram->setUniformValueArray(mSpecLoc.quad, quad, 4);

    glBindTexture(GL_TEXTURE_2D, mSpecTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mCmapTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mHeaderTex);
    glActiveTexture(GL_TEXTURE0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glViewport(0, 0, mWidth, mHeight);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    mSpecProgram->release();
}

QRect CanvasRenderer::viewport() const
{
    return QRect(0, 0, mWidth, mHeight);
}

void CanvasRenderer::setClip(int x, int y, int width, int height)
{
    glEnable(GL_SCISSOR_TEST);
    // The canvas shows with its bottom row at the top, so y counts from the top of the picture and is the row of the framebuffer.
    glScissor(x, y, std::max(width, 0), std::max(height, 0));
}

void CanvasRenderer::clearClip()
{
    glDisable(GL_SCISSOR_TEST);
}

void CanvasRenderer::drawText(Font *font, float x, float y, const QColor &color, const std::string &text)
{
    drawGlyphs(font, x, y, 0.0f, color, text, nullptr);
}

void CanvasRenderer::drawTextOutlined(Font *font, float x, float y, const QColor &color, const std::string &text, const QColor &outlineColor)
{
    drawGlyphs(font, x, y, 0.0f, color, text, &outlineColor);
}

void CanvasRenderer::drawTextRotated(int size, float x, float y, float angleDegrees, const QColor &color,
                                     const std::string &text, const QColor &outline)
{
    Font *font = size == 0 ? mFontNormal : (size == 1 ? mFontSmall : mFontSmaller);
    drawGlyphs(font, x, y, angleDegrees, color, text, outline.alpha() > 0 ? &outline : nullptr);
}

QRect CanvasRenderer::textBoundsOfSize(int size, const std::string &text)
{
    Font *font = size == 0 ? mFontNormal : (size == 1 ? mFontSmall : mFontSmaller);
    return textBounds(font, text);
}

void CanvasRenderer::drawGlyphs(Font *font, float x, float y, float angleDegrees, const QColor &color,
                                const std::string &text, const QColor *outlineColor)
{
    if (text.empty()) {
        return;
    }

    // Start on a whole pixel. A glyph that starts between two pixels gets blurred by the texture filter.
    const float originX = std::round(x);
    const float originY = std::round(mHeight - y);

    // The glyph positions are relative to the origin.
    float cursor = 0;

    // One draw call draws a limited number of glyphs. Draw a long text in chunks.
    constexpr int maxGlyphs = Shaders::textMaxGlyphs;
    static float rects[4 * maxGlyphs];
    static float uvs[4 * maxGlyphs];

    mTextProgram->bind();

    const float radians = angleDegrees * float(M_PI) / 180.0f;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    const float rotation[4] = { cosine, sine, -sine, cosine };
    glUniformMatrix2fv(mTextLoc.rotation, 1, GL_FALSE, rotation);
    mTextProgram->setUniformValue(mTextLoc.origin, originX, originY);

    mTextProgram->setUniformValue(mTextLoc.textColor, color.redF(), color.greenF(), color.blueF(), color.alphaF());
    if (outlineColor != nullptr) {
        // The outline is the same text four times, moved by a little more than half a pixel
        // in each diagonal direction.
        const float delta = 0.66667 * mZoomScaleText;
        const float offsets[8] = { -delta, delta, -delta, -delta, delta, delta, delta, -delta };
        glUniform2fv(mTextLoc.offsets, 4, offsets);
        mTextProgram->setUniformValue(mTextLoc.outlineColor, outlineColor->redF(), outlineColor->greenF(), outlineColor->blueF(),
                                      outlineColor->alphaF() * color.alphaF());
        mTextProgram->setUniformValue(mTextLoc.passCount, 5);
    }
    else {
        mTextProgram->setUniformValue(mTextLoc.passCount, 1);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->atlas());
    glBindVertexArray(mTextVao);

    int count = 0;

    auto flush = [&]() {
        if (count > 0) {
            glUniform4fv(mTextLoc.rects, count, rects);
            glUniform4fv(mTextLoc.uvs, count, uvs);
            mTextProgram->setUniformValue(mTextLoc.glyphCount, count);
            glDrawArrays(GL_TRIANGLES, 0, 6 * count * (outlineColor != nullptr ? 5 : 1));
            count = 0;
        }
    };

    for (char c : text) {
        if (c == '\0') break;

        const FontCharacter &ch = font->charFor(c);

        // A glyph with no pixels, such as a space, has nothing to draw.
        if (ch.width > 0 && ch.height > 0) {
            rects[4 * count + 0] = cursor + ch.bearingX;
            rects[4 * count + 1] = -(float(ch.height) - ch.bearingY);
            rects[4 * count + 2] = ch.width;
            rects[4 * count + 3] = ch.height;

            uvs[4 * count + 0] = ch.u0;
            uvs[4 * count + 1] = ch.vBottom;
            uvs[4 * count + 2] = ch.u1;
            uvs[4 * count + 3] = ch.vTop;

            if (++count == maxGlyphs) {
                flush();
            }
        }

        cursor += (ch.advance >> 6);
    }

    flush();

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    mTextProgram->release();
}

QRect CanvasRenderer::textBounds(Font *font, const std::string &text)
{
    float x = 0;
    float y = 0;
    
    QRect rect;
    float xmax = 0;
    float ymax = 0;

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); ++c) {
        FontCharacter ch = font->charFor(*c);

        float xpos = x + ch.bearingX;
        float ypos = y - (ch.height - ch.bearingY);

        float w = ch.width;
        float h = ch.height;

        if (c == text.begin()) {
            rect.setX(xpos);
            rect.setY(ypos);
        }

        if (xpos + w > xmax) {
            xmax = xpos + w;
        }
        if (ypos + h > ymax) {
            ymax = ypos + h;
        }

        x += (ch.advance >> 6);
    }

    rect.setWidth(xmax - rect.x());
    rect.setHeight(ymax - rect.y());

    return rect;
}

void CanvasRenderer::initFonts()
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeType library" << std::endl;
        return;
    }

    mFontNormal = new Font(ft, ":/Roboto.ttf", std::min(14 * mDevicePixelRatio * mZoomScaleText, 24.0), mDpi);
    mFontSmall = new Font(ft, ":/Roboto.ttf", std::min(12 * mDevicePixelRatio * mZoomScaleText, 20.0), mDpi);
    mFontSmaller = new Font(ft, ":/Roboto.ttf", std::min(10 * mDevicePixelRatio * mZoomScaleText, 18.0), mDpi);

    FT_Done_FreeType(ft);
}

void CanvasRenderer::initShaders()
{
    mTextProgram = createShaderProgram(Shaders::textVertex, Shaders::textFragment);

    mTextLoc.projection = mTextProgram->uniformLocation("projection");
    mTextLoc.rects = mTextProgram->uniformLocation("rects");
    mTextLoc.uvs = mTextProgram->uniformLocation("uvs");
    mTextLoc.glyphCount = mTextProgram->uniformLocation("glyphCount");
    mTextLoc.passCount = mTextProgram->uniformLocation("passCount");
    mTextLoc.offsets = mTextProgram->uniformLocation("offsets");
    mTextLoc.textColor = mTextProgram->uniformLocation("textColor");
    mTextLoc.outlineColor = mTextProgram->uniformLocation("outlineColor");
    mTextLoc.origin = mTextProgram->uniformLocation("origin");
    mTextLoc.rotation = mTextProgram->uniformLocation("rotation");

    mTextProgram->bind();
    mTextProgram->setUniformValue("text", 0);
    mTextProgram->release();

    // The shaders take the corners of each quad from a uniform, so the vertex arrays have no buffers.
    // The core profile still requires a vertex array object for drawing.
    glGenVertexArrays(1, &mTextVao);

    mSpecProgram = createShaderProgram(Shaders::specVertex, Shaders::specFragment);

    mSpecLoc.projection = mSpecProgram->uniformLocation("projection");
    mSpecLoc.frequencyScale = mSpecProgram->uniformLocation("frequencyScale");
    mSpecLoc.minFrequency = mSpecProgram->uniformLocation("minFrequency");
    mSpecLoc.maxFrequency = mSpecProgram->uniformLocation("maxFrequency");
    mSpecLoc.gainScale = mSpecProgram->uniformLocation("gainScale");
    mSpecLoc.quality = mSpecProgram->uniformLocation("quality");
    mSpecLoc.blur = mSpecProgram->uniformLocation("blur");
    mSpecLoc.quad = mSpecProgram->uniformLocation("quad");
    mSpecLoc.lastPosition = mSpecProgram->uniformLocation("lastPosition");

    // The spectrogram texture is on unit 0, and the colour map is on unit 1.
    mSpecProgram->bind();
    mSpecProgram->setUniformValue("tex", 0);
    mSpecProgram->setUniformValue("colorMap", 1);
    mSpecProgram->setUniformValue("header", 2);
    mSpecProgram->release();

    glGenTextures(1, &mCmapTex);
    glBindTexture(GL_TEXTURE_2D, mCmapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // The colour map has a linear filter, so the colour changes smoothly between two entries.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenVertexArrays(1, &mSpecVao);

    mCircleProgram = createShaderProgram(Shaders::circleVertex, Shaders::circleFragment);

    mCircleLoc.projection = mCircleProgram->uniformLocation("projection");
    mCircleLoc.centers = mCircleProgram->uniformLocation("centers");
    mCircleLoc.radius = mCircleProgram->uniformLocation("radius");
    mCircleLoc.fillColor = mCircleProgram->uniformLocation("fillColor");

    glGenVertexArrays(1, &mCircleVao);

    mSegmentProgram = createShaderProgram(Shaders::segmentVertex, Shaders::segmentFragment);

    mSegmentLoc.projection = mSegmentProgram->uniformLocation("projection");
    mSegmentLoc.segments = mSegmentProgram->uniformLocation("segments");
    mSegmentLoc.halfWidth = mSegmentProgram->uniformLocation("halfWidth");
    mSegmentLoc.color = mSegmentProgram->uniformLocation("color");

    glGenVertexArrays(1, &mSegmentVao);

    mRectProgram = createShaderProgram(Shaders::rectVertex, Shaders::rectFragment);

    mRectLoc.projection = mRectProgram->uniformLocation("projection");
    mRectLoc.rect = mRectProgram->uniformLocation("rect");
    mRectLoc.radius = mRectProgram->uniformLocation("radius");
    mRectLoc.fillColor = mRectProgram->uniformLocation("fillColor");
    mRectLoc.borderColor = mRectProgram->uniformLocation("borderColor");
    mRectLoc.borderWidth = mRectProgram->uniformLocation("borderWidth");

    glGenVertexArrays(1, &mRectVao);
}

void CanvasRenderer::setProjections()
{
    // The text program draws with the y axis pointing down. The other programs use the y axis pointing up.
    QMatrix4x4 up;
    up.ortho(0, mWidth, 0, mHeight, -1, 1);
    QMatrix4x4 down;
    down.ortho(0, mWidth, mHeight, 0, -1, 1);

    mTextProgram->bind();
    mTextProgram->setUniformValue(mTextLoc.projection, down);

    mCircleProgram->bind();
    mCircleProgram->setUniformValue(mCircleLoc.projection, up);

    mSegmentProgram->bind();
    mSegmentProgram->setUniformValue(mSegmentLoc.projection, up);

    mRectProgram->bind();
    mRectProgram->setUniformValue(mRectLoc.projection, up);
    mRectProgram->release();
}

void CanvasRenderer::deleteFonts()
{
    delete mFontNormal;
    delete mFontSmall;
    delete mFontSmaller;
}

void CanvasRenderer::deleteShaders()
{
    delete mTextProgram;
    glDeleteVertexArrays(1, &mTextVao);

    delete mSpecProgram; 
    glDeleteVertexArrays(1, &mSpecVao);

    delete mCircleProgram;
    glDeleteVertexArrays(1, &mCircleVao);

    delete mSegmentProgram;
    glDeleteVertexArrays(1, &mSegmentVao);

    delete mRectProgram;
    glDeleteVertexArrays(1, &mRectVao);
}

void CanvasRenderer::initTexture(GLuint &texture, int width, int height)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

QOpenGLShaderProgram *CanvasRenderer::createShaderProgram(
        const char *vertexSource, const char *fragmentSource)
{
    auto program = new QOpenGLShaderProgram();
    program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource);
    program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource);
    program->link();
    return program;
}

