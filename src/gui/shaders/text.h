#ifndef GUI_SHADERS_TEXT_H
#define GUI_SHADERS_TEXT_H

namespace Gui::Shaders {

// The number of glyphs that one draw call can draw. The glyph data is in uniform arrays,
// because a buffer update for each draw call makes the Apple OpenGL driver submit the
// command buffer to the GPU every time.
constexpr int textMaxGlyphs = 64;

constexpr const char *textVertex = R"foo(
#version 150

#define MAX_GLYPHS 64

out vec2 TexCoords;
flat out vec4 Color;

uniform mat4 projection;

// For each glyph: <x, y, width, height> in pixels, and <u0, vBottom, u1, vTop> in the atlas.
uniform vec4 rects[MAX_GLYPHS];
uniform vec4 uvs[MAX_GLYPHS];
uniform int glyphCount;

// A draw call draws every glyph one time for each pass. The last pass uses the text
// colour. The passes before it use the outline colour, and each one has its own offset.
uniform int passCount;
uniform vec2 offsets[4];
uniform vec4 textColor;
uniform vec4 outlineColor;

// The glyph positions are relative to the origin of the text. The rotation turns the text
// around this origin.
uniform vec2 origin;
uniform mat2 rotation;

const vec2 corners[6] = vec2[6](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
    vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));

void main()
{
    int perPass = 6 * glyphCount;
    int pass = gl_VertexID / perPass;
    int local = gl_VertexID - pass * perPass;
    int glyph = local / 6;
    vec2 corner = corners[local - glyph * 6];

    bool isFill = (pass == passCount - 1);
    vec2 offset = isFill ? vec2(0.0) : offsets[pass];

    vec4 rect = rects[glyph];
    vec4 uv = uvs[glyph];

    vec2 position = origin + rotation * (rect.xy + corner * rect.zw + offset);
    gl_Position = projection * vec4(position, 0.0, 1.0);
    TexCoords = vec2(mix(uv.x, uv.z, corner.x), mix(uv.y, uv.w, corner.y));
    Color = isFill ? textColor : outlineColor;
}
)foo";

constexpr const char *textFragment = R"foo(
#version 150
in vec2 TexCoords;
flat in vec4 Color;
out vec4 FragColor;

uniform sampler2D text;

void main()
{
    FragColor = vec4(Color.rgb, Color.a * texture(text, TexCoords).r);
}
)foo";

}

#endif // GUI_SHADERS_TEXT_H
