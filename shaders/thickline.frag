#version 440

// Thick line fragment shader

layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;     // projection * modelview
    vec4 color;
    vec2 viewportSize;  // viewport width and height in pixels
    float lineWidth;    // line width in pixels
} ubuf;

layout(location = 0) in float vSide;  // Side value from vertex shader (0 = degenerate)
layout(location = 0) out vec4 fragColor;

void main()
{
    // Discard fragments from degenerate triangles (marked with side = 0)
    if (abs(vSide) < 0.001) {
        discard;
    }

    fragColor = ubuf.color;
}
