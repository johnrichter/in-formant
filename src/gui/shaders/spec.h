#ifndef GUI_SHADERS_SPEC_H
#define GUI_SHADERS_SPEC_H

namespace Gui::Shaders {

constexpr const char *specVertex = R"foo(
#version 150

out vec2 TexCoords;

uniform mat4 projection;

// The four corners of the quad, as <vec2 pos, vec2 tex>, for a triangle fan.
uniform vec4 quad[4];

void main()
{
    vec4 vertex = quad[gl_VertexID];
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)foo";

// The spectrogram texture has one column for each time slice and one row for each frequency bin.
// The texture is a ring, so the column index wraps. The header texture has one pixel for each
// column. It holds the number of bins and the sample rate of the slice in that column.
constexpr const char *specFragment = R"foo(
#version 150

#define COLUMNS 4096
#define ROWS    2560

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;
uniform sampler2D header;

// The color map is a texture of 256 by 1 pixels.
uniform sampler2D colorMap;

uniform int frequencyScale; // Linear, Log, Mel, ERB
uniform float minFrequency;
uniform float maxFrequency;

// Scale from the amplitude of the slice to the position in the color map.
uniform float gainScale;

// 0 is the bilinear filter of the graphics card. 1 is the cubic filter.
uniform int quality;

// The shape of the cubic filter. 0 is sharp, and 1 is the smoothest.
uniform float blur;

// The position of the center of the newest column. The picture holds this column for the
// time between the newest slice and the end of the view.
uniform float lastPosition;

#define INV_LOG_2   1.44269504089
#define INV_LOG_10  0.4342944819
#define ERB_A       21.33228113095401739888262

float transform(float f) {
    if (frequencyScale == 1)
        return log(f) * INV_LOG_2;
    else if (frequencyScale == 2)
        return 2595.0 * log(1.0 + f / 700.0) * INV_LOG_10;
    else if (frequencyScale == 3)
        return ERB_A * log(1.0 + 0.00437 * f) * INV_LOG_10;
    else
        return f;
}

float reverse(float v) {
    if (frequencyScale == 1)
        return pow(2.0, v);
    else if (frequencyScale == 2)
        return 700.0 * (pow(10.0, v / 2595.0) - 1.0);
    else if (frequencyScale == 3)
        return (pow(10.0, v / ERB_A) - 1.0) / 0.00437;
    else
        return v;
}

// The Mitchell-Netravali cubic filter. B = 0 gives the sharp Catmull-Rom filter.
// B = 1 gives the smooth cubic B-spline filter.
float mitchell(float x, float B, float C) {
    x = abs(x);
    if (x < 1.0)
        return ((12.0 - 9.0 * B - 6.0 * C) * x * x * x + (-18.0 + 12.0 * B + 6.0 * C) * x * x + (6.0 - 2.0 * B)) / 6.0;
    else if (x < 2.0)
        return ((-B - 6.0 * C) * x * x * x + (6.0 * B + 30.0 * C) * x * x + (-12.0 * B - 48.0 * C) * x + (8.0 * B + 24.0 * C)) / 6.0;
    return 0.0;
}

// The weights of the four taps at the positions -1, 0, 1, and 2. t is the distance from tap 0.
vec4 cubicWeights(float t) {
    float C = 0.5 * (1.0 - blur);
    return vec4(mitchell(1.0 + t, blur, C), mitchell(t, blur, C), mitchell(1.0 - t, blur, C), mitchell(2.0 - t, blur, C));
}

int wrapColumn(int c) {
    return ((c % COLUMNS) + COLUMNS) % COLUMNS;
}

// The value of one bin. The filter works on the square root of the power, because this makes the
// peaks of the harmonics smoother and gives no dark rings next to them.
float bin(int column, int row, int count) {
    row = clamp(row, 0, count - 1);
    return sqrt(max(texelFetch(tex, ivec2(column, row), 0).r, 0.0));
}

// The value of one column at a position in bins. footprint is the number of bins in one pixel.
float columnValue(int column, float position, float footprint, int count) {
    if (footprint > 1.0) {
        // Many bins fall in one pixel. Take the mean of some samples over these bins.
        int taps = int(min(ceil(footprint), 6.0));
        float sum = 0.0;
        for (int i = 0; i < taps; ++i) {
            float p = position + ((float(i) + 0.5) / float(taps) - 0.5) * footprint - 0.5;
            int i0 = int(floor(p));
            float t = p - float(i0);
            sum += mix(bin(column, i0, count), bin(column, i0 + 1, count), t);
        }
        return sum / float(taps);
    }

    float p = position - 0.5;
    int i0 = int(floor(p));
    vec4 w = cubicWeights(p - float(i0));
    return w.x * bin(column, i0 - 1, count) + w.y * bin(column, i0, count)
         + w.z * bin(column, i0 + 1, count) + w.w * bin(column, i0 + 2, count);
}

// The value of one column at one frequency. The result is negative when the column has no slice.
float sliceValue(int column, float freq, float dfreq) {
    vec2 h = texelFetch(header, ivec2(column, 0), 0).rg;
    int count = int(h.x);
    float nyquist = h.y * 0.5;

    if (count <= 1 || nyquist <= 0.0 || freq < 0.0 || freq > nyquist)
        return -1.0;

    // Bin k holds the frequency k / (count - 1) * nyquist.
    float scale = float(count - 1) / nyquist;
    return max(columnValue(column, freq * scale + 0.5, dfreq * scale, count), 0.0);
}

float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main()
{
    // TexCoords.x: the position in the ring of columns. TexCoords.y: [0,1] => [minFrequency,maxFrequency]
    // on the frequency scale of the view.
    float vmin = transform(minFrequency);
    float vmax = transform(maxFrequency);

    float freq = reverse(vmin + TexCoords.y * (vmax - vmin));
    float dfreq = abs(dFdy(freq));

    float position = TexCoords.x * float(COLUMNS);
    float dposition = abs(dFdx(position));
    position = min(position, lastPosition);

    float value;

    if (quality == 0) {
        // The filter of the graphics card. This is the fast path.
        int column = wrapColumn(int(floor(position)));
        vec2 h = texelFetch(header, ivec2(column, 0), 0).rg;
        int count = int(h.x);
        float nyquist = h.y * 0.5;

        if (count <= 1 || nyquist <= 0.0 || freq < 0.0 || freq > nyquist) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        float row = freq / nyquist * float(count - 1) + 0.5;
        value = sqrt(max(texture(tex, vec2(position / float(COLUMNS), row / float(ROWS))).r, 0.0));
    }
    else {
        float sum = 0.0;
        float weight = 0.0;

        if (dposition > 1.0) {
            // Many columns fall in one pixel. Take the mean of some of them.
            int taps = int(min(ceil(dposition), 4.0));
            for (int i = 0; i < taps; ++i) {
                float p = position + ((float(i) + 0.5) / float(taps) - 0.5) * dposition;
                float v = sliceValue(wrapColumn(int(floor(p))), freq, dfreq);
                if (v >= 0.0) {
                    sum += v;
                    weight += 1.0;
                }
            }
        }
        else {
            float p = position - 0.5;
            int c0 = int(floor(p));
            vec4 w = cubicWeights(p - float(c0));

            for (int i = 0; i < 4; ++i) {
                float v = sliceValue(wrapColumn(c0 - 1 + i), freq, dfreq);
                if (v >= 0.0) {
                    sum += w[i] * v;
                    weight += w[i];
                }
            }
        }

        // No slice was stored in these columns yet, or the frequency is above the range of the slice.
        if (weight <= 1e-3) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        value = max(sum / weight, 0.0);
    }

    // A little noise before the color lookup hides the steps between the 256 colors.
    float noise = (hash(gl_FragCoord.xy) + hash(gl_FragCoord.yx + 17.0) - 1.0) / 255.0;
    float adjusted = clamp(value * gainScale + noise, 0.0, 1.0);

    FragColor = vec4(texture(colorMap, vec2((adjusted * 255.0 + 0.5) / 256.0, 0.5)).rgb, 1.0);
}
)foo";

}

#endif // GUI_SHADERS_SPEC_H
