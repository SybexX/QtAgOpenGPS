#version 440

// Dashed thick line vertex shader
// Expands line segments to quads with constant screen-pixel width
// Passes distance along line to fragment shader for dash pattern

layout(location = 0) in vec3 pos;       // Current vertex position
layout(location = 1) in vec3 nextPos;   // Neighbor vertex position
layout(location = 2) in float side;     // -1.0 or 1.0 (left/right side of quad)
layout(location = 3) in float distance; // Cumulative distance along line (world units)

layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;     // full final matrix
    vec4 color;
    vec2 viewportSize;  // viewport width and height in pixels
    float lineWidth;    // line width in pixels
    float dashLength;   // dash length in pixels
    float gapLength;    // gap length in pixels
} ubuf;

layout(location = 0) out float vDistance;  // Pass distance to fragment shader

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main()
{
    // Step 1: Transform all the way to final clip space
    // This ensures the offset is perpendicular to the line as it appears on screen,
    // regardless of any rotations in ndcMatrix or windowMatrix
    vec4 currClip = ubuf.mvpMatrix * vec4(pos, 1.0);
    vec4 nextClip = ubuf.mvpMatrix * vec4(nextPos, 1.0);

    // Step 2: Calculate line direction in final clip space using homogeneous coordinates
    // This works correctly even when w is negative (vertex behind camera)
    vec2 lineVec = nextClip.xy * currClip.w - currClip.xy * nextClip.w;
    float len = length(lineVec);

    // Guard against degenerate lines (avoid NaN from normalize)
    vec2 dir = (len > 0.0001) ? (lineVec / len) : vec2(1.0, 0.0);
    vec2 normal = vec2(-dir.y, dir.x);

    // Step 3: Calculate offset in final clip space
    // In NDC, range is [-1, 1] = 2 units for viewport dimensions
    // 1 pixel = 2 / viewportSize NDC units
    // In clip space: offset = pixelOffset * (2 / viewportSize) * abs(w)
    float halfWidth = ubuf.lineWidth * 0.25;
    vec2 pixelToNDC = vec2(2.0 / ubuf.viewportSize.x, 2.0 / ubuf.viewportSize.y);
    vec2 ndcOffset = normal * halfWidth * pixelToNDC * side;
    vec2 clipOffset = ndcOffset * abs(currClip.w);

    // Step 4: Apply offset - position is already in final clip space, no more transforms
    currClip.xy += clipOffset;

    gl_Position = currClip;
    gl_PointSize = 1.0;

    // Pass distance to fragment shader for dash pattern calculation
    vDistance = distance;
}
