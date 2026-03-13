#version 440

// Vertex shader for round points

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) in float pointSize;

layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;
    float softness;      // 0.0 = hard edge, 1.0 = very soft
} ubuf;

layout(location = 0) out vec4 vColor;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main()
{
    gl_Position = ubuf.mvpMatrix * vec4(vertex, 1.0);
    gl_PointSize = pointSize;
    vColor = color;
}
