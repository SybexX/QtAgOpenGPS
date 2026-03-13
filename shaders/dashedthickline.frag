#version 440

// Dashed thick line fragment shader
// Uses fwidth() to convert world-space distance to screen pixels for dash pattern

layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;     // projection * modelview
    vec4 color;
    vec2 viewportSize;  // viewport width and height in pixels
    float lineWidth;    // line width in pixels
    float dashLength;   // dash length in pixels
    float gapLength;    // gap length in pixels
} ubuf;

layout(location = 0) in float vDistance;  // World-space distance along line
layout(location = 0) out vec4 fragColor;

void main()
{
    // Discard fragments from degenerate triangles (marked with negative distance)
    if (vDistance < 0.0) {
        discard;
    }

    // Convert world-space distance to screen pixels using derivatives
    // fwidth(vDistance) gives us the rate of change of distance across a pixel
    // This is approximately how many world units span one screen pixel
    float worldUnitsPerPixel = fwidth(vDistance);

    // Guard against division by zero (degenerate case)
    if (worldUnitsPerPixel < 0.0001) {
        fragColor = ubuf.color;
        return;
    }

    // Convert world-space distance to screen pixels
    float screenDistance = vDistance / worldUnitsPerPixel;

    // Apply dash pattern
    float patternLength = ubuf.dashLength + ubuf.gapLength;

    // Avoid division by zero if pattern is invalid
    if (patternLength < 0.001) {
        fragColor = ubuf.color;
        return;
    }

    float patternPos = mod(screenDistance, patternLength);

    // Discard fragments in the gap portion
    if (patternPos > ubuf.dashLength) {
        discard;
    }

    fragColor = ubuf.color;
}
