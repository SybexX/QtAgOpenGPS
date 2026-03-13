#version 440

// Vertex shader for textured primitives (QRhi version)

// Vertex inputs
layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 texcoord_src;

// Uniform buffer
layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;
    vec4 color;
    int useColor;
} ubuf;

// Output to fragment shader
layout(location = 0) out vec2 vTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = ubuf.mvpMatrix * vec4(vertex, 1.0);
    vTexCoord = texcoord_src;
}
