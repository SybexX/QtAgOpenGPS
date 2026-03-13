#version 440

// Fragment shader for textured primitives (QRhi version)

// Uniform buffer
layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;
    vec4 color;
    int useColor;
} ubuf;

// Texture sampler (separate binding)
layout(binding = 1) uniform sampler2D textureSampler;

// Input from vertex shader
layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec4 texColor = texture(textureSampler, vTexCoord);

    if (ubuf.useColor != 0) {
        // Multiply texture with color (colorize the texture)
        fragColor = vec4(ubuf.color.rgb * texColor.rgb, texColor.a * ubuf.color.a);
    } else {
        // Just draw texture without colorization
        fragColor = texColor;
    }
}
