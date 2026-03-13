#version 440

// Vertex shader for per-vertex colored primitives (QRhi version)

// Vertex inputs
layout(location = 0) in vec3 vertex;
layout(location = 1) in vec4 color;

// Uniform buffer
layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;
    float pointSize;
} ubuf;

// Output to fragment shader
layout(location = 0) out vec4 vColor;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main()
{
    gl_PointSize = ubuf.pointSize;
    gl_Position = ubuf.mvpMatrix * vec4(vertex, 1.0);
    vColor = color;
}
