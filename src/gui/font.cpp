#include "font.h"
#include "freetype/freetype.h"
#include <QFile>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace Gui;

Font::Font(FT_Library ft, const QString &font, int pixelSize)
    : mCharacters(), mAtlas(0)
{
    QFile fontFile(font);
    if (!fontFile.open(QIODevice::ReadOnly)) {
        std::cout << "ERROR::FREETYPE: Failed to read font file" << std::endl;
        return;
    }
    QByteArray buffer = fontFile.readAll();
    fontFile.close();

    FT_Face face;
    if (FT_New_Memory_Face(ft, (const FT_Byte *) buffer.constData(), buffer.size(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, pixelSize);
    buildAtlas(face);
    FT_Done_Face(face);
}

Font::Font(FT_Library ft, const QString &font, double pointSize, double dpi)
    : mCharacters(), mAtlas(0)
{
    QFile fontFile(font);
    if (!fontFile.open(QIODevice::ReadOnly)) {
        std::cout << "ERROR::FREETYPE: Failed to read font file" << std::endl;
        return;
    }
    QByteArray buffer = fontFile.readAll();
    fontFile.close();

    FT_Face face;
    if (FT_New_Memory_Face(ft, (const FT_Byte *) buffer.constData(), buffer.size(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    FT_Set_Char_Size(face, pointSize * 64, 0, dpi, 0);
    buildAtlas(face);
    FT_Done_Face(face);
}

Font::~Font()
{
    if (mAtlas != 0) {
        glDeleteTextures(1, &mAtlas);
    }
}

const FontCharacter &Font::charFor(char c) const
{
    return mCharacters[(unsigned char) c];
}

GLuint Font::atlas() const
{
    return mAtlas;
}

void Font::buildAtlas(FT_Face face)
{
    initializeOpenGLFunctions();

    struct Glyph {
        std::vector<unsigned char> pixels;
        int x = 0, y = 0;
    };
    std::array<Glyph, 255> glyphs;

    // Render each glyph, and keep the bitmap until the atlas has its place.
    for (unsigned int c = 0; c < 255; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYPE: Failed to load glyph" << std::endl;
            continue;
        }

        const FT_Bitmap &bitmap = face->glyph->bitmap;

        FontCharacter &ch = mCharacters[c];
        ch.width = bitmap.width;
        ch.height = bitmap.rows;
        ch.bearingX = face->glyph->bitmap_left;
        ch.bearingY = face->glyph->bitmap_top;
        ch.advance = face->glyph->advance.x;

        // The pitch of a bitmap can differ from its width, so copy it row by row.
        glyphs[c].pixels.resize((size_t) bitmap.width * bitmap.rows);
        for (unsigned int row = 0; row < bitmap.rows; ++row) {
            std::copy_n(bitmap.buffer + (size_t) row * bitmap.pitch, bitmap.width,
                        glyphs[c].pixels.begin() + (size_t) row * bitmap.width);
        }
    }

    // Place the glyphs in rows. A pixel of space around each glyph stops one glyph from
    // showing in the texture filter of another.
    constexpr int padding = 1;
    constexpr int atlasWidth = 1024;
    int x = padding, y = padding, rowHeight = 0;
    for (unsigned int c = 0; c < 255; ++c) {
        const int w = (int) mCharacters[c].width;
        const int h = (int) mCharacters[c].height;
        if (x + w + padding > atlasWidth) {
            x = padding;
            y += rowHeight + padding;
            rowHeight = 0;
        }
        glyphs[c].x = x;
        glyphs[c].y = y;
        x += w + padding;
        rowHeight = std::max(rowHeight, h);
    }
    int atlasHeight = 1;
    while (atlasHeight < y + rowHeight + padding) {
        atlasHeight *= 2;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &mAtlas);
    glBindTexture(GL_TEXTURE_2D, mAtlas);

    std::vector<unsigned char> empty((size_t) atlasWidth * atlasHeight, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, empty.data());

    for (unsigned int c = 0; c < 255; ++c) {
        FontCharacter &ch = mCharacters[c];
        if (ch.width > 0 && ch.height > 0) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, glyphs[c].x, glyphs[c].y, ch.width, ch.height,
                            GL_RED, GL_UNSIGNED_BYTE, glyphs[c].pixels.data());
        }
        ch.u0 = (float) glyphs[c].x / atlasWidth;
        ch.u1 = (float) (glyphs[c].x + (int) ch.width) / atlasWidth;
        ch.vTop = (float) glyphs[c].y / atlasHeight;
        ch.vBottom = (float) (glyphs[c].y + (int) ch.height) / atlasHeight;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}
