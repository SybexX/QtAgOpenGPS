#version 440

// Fragment shader for round points with soft edges

layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;
    float pointSize;
    float softness;      // 0.0 = hard edge, 1.0 = very soft
} ubuf;

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 fragColor;

void main()
{
    // gl_PointCoord is 0-1, with (0,0) at top-left of point sprite
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord) * 2.0;  // 0 at center, 1 at edge

    // Soft edge using smoothstep
    // softness controls the blend range: 0 = hard, 1 = very soft
    float edgeStart = 1.0 - ubuf.softness;
    float alpha = 1.0 - smoothstep(edgeStart, 1.0, dist);

    if (alpha < 0.01)
        discard;

    fragColor = vec4(vColor.rgb, vColor.a * alpha);
}
