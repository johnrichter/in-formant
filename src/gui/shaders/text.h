#ifndef GUI_SHADERS_TEXT_H
#define GUI_SHADERS_TEXT_H

namespace Gui::Shaders {

constexpr const char *textVertex = R"foo(
#version 150
out vec2 TexCoords;

uniform mat4 projection;

// The four corners of the quad, as <vec2 pos, vec2 tex>, for a triangle fan.
// The corners come from a uniform because a buffer update for each draw call makes
// the Apple OpenGL driver submit the command buffer to the GPU every time.
uniform vec4 quad[4];

void main()
{
    vec4 vertex = quad[gl_VertexID];
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)foo";

constexpr const char *textFragment = R"foo(
#version 150
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    FragColor = vec4(textColor, 1.0) * sampled;
}
)foo";

}

#endif // GUI_SHADERS_TEXT_H
