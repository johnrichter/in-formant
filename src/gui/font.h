#ifndef GUI_FONT_H
#define GUI_FONT_H

#include <rpcxx.h>
#include <QOpenGLFunctions>
#include <QSize>
#include <array>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Gui {

    struct FontCharacter {
        // The position of the glyph in the atlas texture, as texture coordinates.
        // The top of the glyph has the coordinate vTop, and the bottom has vBottom.
        float        u0, u1;
        float        vTop, vBottom;
        unsigned int width;
        unsigned int height;
        int          bearingX;
        int          bearingY;
        long         advance;
    };

    class Font : public QOpenGLFunctions {
    public:
        Font(FT_Library ft, const QString &font, int pixelSize);
        Font(FT_Library ft, const QString &font, double pointSize, double dpi);
        ~Font();

        const FontCharacter &charFor(char c) const;

        // All the glyphs of the font are in one texture.
        GLuint atlas() const;

    private:
        void buildAtlas(FT_Face face);

        std::array<FontCharacter, 256> mCharacters;
        GLuint mAtlas;
    };

}

#include "shaders/text.h"

#endif // GUI_FONT_H
