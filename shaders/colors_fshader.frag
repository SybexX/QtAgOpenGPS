#version 440

// Fragment shader for per-vertex colored primitives (QRhi version)

// Input from vertex shader
layout(location = 0) in vec4 vColor;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vColor;
}
