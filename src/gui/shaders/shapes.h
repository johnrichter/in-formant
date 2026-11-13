#ifndef GUI_SHADERS_SHAPES_H
#define GUI_SHADERS_SHAPES_H

namespace Gui::Shaders {

// The number of line segments that one draw call can draw.
constexpr int segmentMaxSegments = 128;

// Many line segments of the same width and color in one draw call. The ends of the segments are
// in a uniform array, because a buffer update for each draw call makes the Apple OpenGL driver
// submit the command buffer to the GPU every time. The fragment shader gives each segment round ends,
// so the joints of a polyline are round too.
constexpr const char *segmentVertex = R"foo(
#version 150

#define MAX_SEGMENTS 128

flat out vec4 Seg;

uniform mat4 projection;
uniform vec4 segments[MAX_SEGMENTS];
uniform float halfWidth;

const vec2 corners[6] = vec2[6](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
    vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));

void main()
{
    int s = gl_VertexID / 6;
    vec4 seg = segments[s];
    Seg = seg;

    vec2 a = seg.xy;
    vec2 b = seg.zw;
    vec2 d = b - a;
    float len = length(d);
    vec2 dir = len > 1e-4 ? d / len : vec2(1.0, 0.0);
    vec2 n = vec2(-dir.y, dir.x);

    float h = halfWidth + 1.5;
    vec2 c = corners[gl_VertexID - s * 6];
    vec2 base = mix(a - dir * h, b + dir * h, c.x);
    vec2 pos = base + n * h * (c.y * 2.0 - 1.0);

    gl_Position = projection * vec4(pos, 0.0, 1.0);
}
)foo";

constexpr const char *segmentFragment = R"foo(
#version 150

flat in vec4 Seg;
out vec4 FragColor;

uniform float halfWidth;
uniform vec4 color;
// The drawing starts this many pixels below the top of the canvas.
uniform float originY;

void main()
{
    vec2 p = gl_FragCoord.xy - vec2(0.0, originY);
    vec2 a = Seg.xy;
    vec2 ab = Seg.zw - a;
    float t = clamp(dot(p - a, ab) / max(dot(ab, ab), 1e-8), 0.0, 1.0);
    float d = length(p - (a + ab * t));
    float coverage = clamp(halfWidth + 0.5 - d, 0.0, 1.0);
    FragColor = vec4(color.rgb, color.a * coverage);
}
)foo";

// A rectangle with round corners and an optional border.
constexpr const char *rectVertex = R"foo(
#version 150

uniform mat4 projection;

// <x, y, width, height>
uniform vec4 rect;

const vec2 corners[6] = vec2[6](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
    vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));

void main()
{
    vec2 c = corners[gl_VertexID];
    vec2 pos = rect.xy - 1.0 + c * (rect.zw + 2.0);
    gl_Position = projection * vec4(pos, 0.0, 1.0);
}
)foo";

constexpr const char *rectFragment = R"foo(
#version 150

out vec4 FragColor;

uniform vec4 rect;
uniform float radius;
uniform vec4 fillColor;
uniform vec4 borderColor;
uniform float borderWidth;
// The drawing starts this many pixels below the top of the canvas.
uniform float originY;

void main()
{
    vec2 half_size = 0.5 * rect.zw;
    vec2 p = gl_FragCoord.xy - vec2(0.0, originY) - (rect.xy + half_size);
    vec2 q = abs(p) - (half_size - radius);
    float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;

    float outer = clamp(0.5 - d, 0.0, 1.0);
    float inner = borderWidth > 0.0 ? clamp(0.5 - (d + borderWidth), 0.0, 1.0) : 1.0;

    vec4 color = mix(borderColor, fillColor, inner);
    FragColor = vec4(color.rgb, color.a * outer);
}
)foo";

}

#endif // GUI_SHADERS_SHAPES_H
