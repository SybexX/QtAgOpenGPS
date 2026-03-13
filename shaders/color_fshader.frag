#version 440

// Fragment shader for simple colored primitives (QRhi version)

// Uniform buffer (must match vertex shader)
layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;
    vec4 color;
    float pointSize;
} ubuf;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = ubuf.color;
}
