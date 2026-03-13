#version 440

// Vertex shader for simple colored primitives (QRhi version)

// Vertex input (location must match pipeline vertex input layout)
layout(location = 0) in vec3 vertex;

// Uniform buffer (std140 layout required for QRhi)
layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;
    vec4 color;
    float pointSize;
} ubuf;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main()
{
    gl_PointSize = ubuf.pointSize;
    gl_Position = ubuf.mvpMatrix * vec4(vertex, 1.0);
}
