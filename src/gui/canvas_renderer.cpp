#include "canvas.h"
#include "../context/timings.h"
#include "../context/rendercontext.h"
#include "qpainterwrapper.h"
#include <cstdlib>
#include <iostream>
#include <cmath>
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

    initTexture(mSpecTex, 2048, 4096+2);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mSpecTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 2048, 4096+2, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void CanvasRenderer::cleanup()
{
    deleteFonts();
    deleteShaders();

    glDeleteTextures(1, &mSpecTex);
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

    QPainterWrapper painter(this);
    mRenderContext->render(&painter);

    std::stringstream ss1;
    ss1 << "Render: " << timings::render;
    auto renderTimeString = ss1.str();

    std::stringstream ss2;
    ss2 << "Update: " << timings::update;
    auto updateTimeString = ss2.str();

    float y = 10;
    QRect textBox;

    textBox = textBoundsNormal(renderTimeString);
    y += textBox.height();
    drawTextNormalOutlined(10, y, Qt::white, renderTimeString);

    textBox = textBoundsNormal(updateTimeString);
    y += textBox.height() + 10;
    drawTextNormalOutlined(10, y, Qt::white, updateTimeString);
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
    for (const auto& point : points) {
        drawPoint(point, radius + 0.6667, outlineColor);
    }
    for (const auto& point : points) {
        drawPoint(point, radius, fillColor);
    }
}

void CanvasRenderer::drawPoint(const QPointF &point, float radius, const QColor &color)
{
    radius *= mZoomScale * (mDpi * mDevicePixelRatio) / 96;

    /*constexpr int N = 24;
    float vertices[2*(N+1)];
    for (int i = 0; i <= N; ++i) {
        float angle = 2 * M_PI * i / N;
        float x = cos(angle) * radius;
        float y = sin(angle) * radius;
        vertices[2*i] = point.x() + x;
        vertices[2*i+1] = mHeight - point.y() + y;
    }*/
    /*glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, N+1);*/

    mCircleProgram->bind();

    float w = mWidth;
    float h = mHeight;

    QMatrix4x4 projection;
    projection.ortho(0, w, 0, h, -1, 1);
    mCircleProgram->setUniformValue("projection", projection);

    mCircleProgram->setUniformValue("resolution", mWidth, mHeight);
    mCircleProgram->setUniformValue("center", point.x(), point.y());
    mCircleProgram->setUniformValue("radius", radius);
    mCircleProgram->setUniformValue("fillColor", color.redF(), color.greenF(), color.blueF());
    
    float x1 = point.x() - 1.5 * radius;
    float x2 = point.x() + 1.5 * radius;
    float y1 = point.y() - 1.5 * radius;
    float y2 = point.y() + 1.5 * radius;

    glBindVertexArray(mCircleVao);

    const QVector2D quad[4] = {
        { x2, y1 },
        { x2, y2 },
        { x1, y2 },
        { x1, y1 },
    };

    mCircleProgram->setUniformValueArray("quad", quad, 4);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);

    mCircleProgram->release();
}

void CanvasRenderer::drawLine(float x1, float y1, float x2, float y2, const QColor &color, float thickness)
{
    thickness *= mZoomScale * (mDpi * mDevicePixelRatio) / 96;

    float dx = x2 - x1;
    float dy = y2 - y1;
    float d = sqrt(dx * dx + dy * dy);

    float nx = -dy / d;
    float ny = dx / d;

    float qx1 = x1 - 0.5 * thickness * nx;
    float qx2 = x2 + 0.5 * thickness * nx;
    float qy1 = y1 - 0.5 * thickness * ny;
    float qy2 = y2 + 0.5 * thickness * ny;

    mCircleProgram->bind();

    float w = mWidth;
    float h = mHeight;

    QMatrix4x4 projection;
    projection.ortho(0, w, 0, h, -1, 1);
    mCircleProgram->setUniformValue("projection", projection);

    mCircleProgram->setUniformValue("resolution", mWidth, mHeight);
    mCircleProgram->setUniformValue("center", (qx1+qx2)/2, (qy1+qy2)/2);
    mCircleProgram->setUniformValue("radius", thickness);
    mCircleProgram->setUniformValue("fillColor", color.redF(), color.greenF(), color.blueF());

    glBindVertexArray(mCircleVao);

    const QVector2D quad[4] = {
        { qx1, qy2 },
        { qx1, qy1 },
        { qx2, qy1 },
        { qx2, qy2 },
    };

    mCircleProgram->setUniformValueArray("quad", quad, 4);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);

    mCircleProgram->release();
}

void CanvasRenderer::drawSpectrogram(
        int xOffset,
        int chunkSize1,
        int chunkSize2,
        int totalSize,
        const std::array<GLint, 2048>& nffts,
        const std::array<GLfloat, 2048>& sampleRates,
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
        float timeEnd)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mSpecTex);

    if (chunkSize1 > 0) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset % 2048, 0, chunkSize1, 4096, GL_RED, GL_FLOAT, chunkData1.data());
    }
    if (chunkSize2 > 0) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chunkSize2, 4096, GL_RED, GL_FLOAT, chunkData2.data());
    }

    std::array<GLfloat, 2048 * 2> extraData;
    for (int x = 0; x < 2048; ++x) {
        extraData[0 * 2048 + x] = nffts[x];
        extraData[1 * 2048 + x] = sampleRates[x];
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 4096, 2048, 2, GL_RED, GL_FLOAT, extraData.data());

    glBindTexture(GL_TEXTURE_2D, 0);

    mSpecProgram->bind();

    float w = mWidth;
    float h = mHeight;

    QMatrix4x4 projection;
    projection.ortho(0, w, 0, h, -1, 1);
    mSpecProgram->setUniformValue("projection", projection);

    std::array<QVector3D, 256> cmap;
    for (int i = 0; i < 256; ++i) {
        QColor color = QColor::fromRgb(colorTable[i]);
        cmap[i] = QVector3D(color.redF(), color.greenF(), color.blueF());
    }
    mSpecProgram->setUniformValueArray("colorMap", cmap.data(), 256);

    mSpecProgram->setUniformValue("frequencyScale", static_cast<int>(freqScale));
    mSpecProgram->setUniformValue("minFrequency", minFrequency);
    mSpecProgram->setUniformValue("maxFrequency", maxFrequency);
    mSpecProgram->setUniformValue("maxGain", maxGain);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(mSpecVao);

    float x2 = w * (sliceTimeEnd - timeStart) / (timeEnd - timeStart);
    float x1 = w * (sliceTimeStart - timeStart) / (timeEnd - timeStart);
    float y2 = h;
    float y1 = 0;

    // The visible slices end at the column after the newest upload. Before this was added, the
    // right edge used the column where the new data starts, so the picture jumped back by the
    // number of new slices in each frame that brought new data.
    const int xEnd = xOffset + chunkSize1 + chunkSize2;
    float texX1 = float(xEnd - totalSize) / 2048.0f;
    float texX2 = float(xEnd) / 2048.0f;
    float texY1 = 0.0f;
    float texY2 = 4095.0f / 4098.0f;

    const QVector4D quad[4] = {
        { x2, y1, texX2, texY2 },
        { x2, y2, texX2, texY1 },
        { x1, y2, texX1, texY1 },
        { x1, y1, texX1, texY2 },
    };

    mSpecProgram->setUniformValueArray("quad", quad, 4);

    glBindTexture(GL_TEXTURE_2D, mSpecTex);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    mSpecProgram->release();
}

QRect CanvasRenderer::viewport() const
{
    return QRect(0, 0, mWidth, mHeight);
}

void CanvasRenderer::drawText(Font *font, float x, float y, const QColor &color, const std::string &text)
{
    mTextProgram->bind();

    y = mHeight - y;

    QMatrix4x4 projection;
    projection.ortho(0, mWidth, mHeight, 0, -1, 1);
    mTextProgram->setUniformValue("projection", projection);
    mTextProgram->setUniformValue("textColor", color.redF(), color.greenF(), color.blueF());

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(mTextVao);

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); ++c) {
        if (*c == '\0') break;

        FontCharacter ch = font->charFor(*c);

        float xpos = x + ch.bearingX;
        float ypos = y - (ch.height - ch.bearingY);

        float w = ch.width;
        float h = ch.height;

        // A glyph with no pixels, such as a space, has an empty texture. Draw nothing for it.
        if (ch.width > 0 && ch.height > 0) {
            const QVector4D quad[4] = {
                { xpos + w, ypos,     1.0f, 1.0f },
                { xpos + w, ypos + h, 1.0f, 0.0f },
                { xpos,     ypos + h, 0.0f, 0.0f },
                { xpos,     ypos,     0.0f, 1.0f },
            };

            glBindTexture(GL_TEXTURE_2D, ch.texture);
            mTextProgram->setUniformValueArray("quad", quad, 4);

            // Render quad
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        x += (ch.advance >> 6);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    mTextProgram->release();
}

void CanvasRenderer::drawTextOutlined(Font *font, float x, float y, const QColor &color, const std::string &text, const QColor &outlineColor)
{
    const float delta = 0.66667 * mZoomScaleText;

    for (int dxi = -1; dxi <= 1; dxi += 2) {
        for (int dyi = -1; dyi <= 1; dyi += 2) {
            drawText(font, x + delta * dxi, y + delta * dyi, outlineColor, text);
        }
    }
    drawText(font, x, y, color, text);
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

    // The shaders take the corners of each quad from a uniform, so the vertex arrays have no buffers.
    // The core profile still requires a vertex array object for drawing.
    glGenVertexArrays(1, &mTextVao);

    mSpecProgram = createShaderProgram(Shaders::specVertex, Shaders::specFragment);

    glGenVertexArrays(1, &mSpecVao);

    mCircleProgram = createShaderProgram(Shaders::circleVertex, Shaders::circleFragment);

    glGenVertexArrays(1, &mCircleVao);
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

