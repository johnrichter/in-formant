#ifndef GUI_SHADERS_CIRCLE_H
#define GUI_SHADERS_CIRCLE_H

namespace Gui::Shaders {

// The number of circles that one draw call can draw.
constexpr int circleMaxCircles = 128;

// Many circles of the same radius and color in one draw call. The centers are in a uniform
// array, because a buffer update for each draw call makes the Apple OpenGL driver submit
// the command buffer to the GPU every time.
constexpr const char *circleVertex = R"foo(
#version 150

#define MAX_CIRCLES 128

flat out vec2 Center;

uniform mat4 projection;
uniform vec2 centers[MAX_CIRCLES];
uniform float radius;

const vec2 corners[6] = vec2[6](
    vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0),
    vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0));

void main()
{
    int circle = gl_VertexID / 6;
    vec2 center = centers[circle];
    Center = center;
    gl_Position = projection * vec4(center + corners[gl_VertexID - circle * 6] * (1.5 * radius), 0.0, 1.0);
}
)foo";

constexpr const char *circleFragment = R"foo(
#version 150
flat in vec2 Center;
out vec4 FragColor;

uniform float radius;
uniform vec3 fillColor;
// The drawing starts this many pixels below the top of the canvas.
uniform float originY;

void main()
{
    float d = length(Center - (gl_FragCoord.xy - vec2(0.0, originY))) - radius;
    float t = clamp(d, 0.0, 1.0);
    FragColor = vec4(fillColor, 1.0 - t);
}
)foo";

}

#endif // GUI_SHADERS_CIRCLE_H
