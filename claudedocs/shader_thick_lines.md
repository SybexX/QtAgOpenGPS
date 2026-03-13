# Drawing 2-pixel lines in OpenGL with shaders

**human** (Dec 6, 2025, 07:44 PM)

Using shaders with OpenGL, how can I draw a line that is 2 pixels wide on the screen, regardless of world coordinates and transformations?

**assistant** (Dec 6, 2025, 07:45 PM)

 This is a classic challenge because OpenGL's built-in `glLineWidth` is poorly supported (many drivers cap it at 1 pixel), so the robust solution is to generate geometry in a shader.

## The Approach: Expand Lines to Screen-Space Quads

The key insight is to transform your line endpoints to **clip space**, then expand them perpendicular to the line direction in **screen space** (pixel units), then transform back.

### Vertex Shader

```glsl
#version 330 core

layout(location = 0) in vec3 aPos;      // Line endpoint position
layout(location = 1) in vec3 aOtherPos; // The other endpoint of the line
layout(location = 2) in float aSide;    // -1 or +1 (which side of the line)

uniform mat4 uMVP;
uniform vec2 uResolution; // Viewport size in pixels
uniform float uLineWidth; // Desired width in pixels (e.g., 2.0)

void main() {
    // Transform both endpoints to clip space
    vec4 clipPos = uMVP * vec4(aPos, 1.0);
    vec4 clipOther = uMVP * vec4(aOtherPos, 1.0);
    
    // Convert to NDC (normalized device coordinates)
    vec2 ndcPos = clipPos.xy / clipPos.w;
    vec2 ndcOther = clipOther.xy / clipOther.w;
    
    // Convert to screen space
    vec2 screenPos = ndcPos * uResolution * 0.5;
    vec2 screenOther = ndcOther * uResolution * 0.5;
    
    // Line direction and perpendicular in screen space
    vec2 dir = normalize(screenPos - screenOther);
    vec2 perp = vec2(-dir.y, dir.x);
    
    // Offset by half the line width (in pixels)
    vec2 offset = perp * (uLineWidth * 0.5) * aSide;
    
    // Convert offset back to NDC
    vec2 ndcOffset = offset / (uResolution * 0.5);
    
    // Apply offset in clip space
    gl_Position = vec4((ndcPos + ndcOffset) * clipPos.w, clipPos.z, clipPos.w);
}
```

### Fragment Shader

```glsl
#version 330 core

uniform vec4 uColor;
out vec4 FragColor;

void main() {
    FragColor = uColor;
}
```

## Setting Up the Geometry

For each line segment, you need **4 vertices** (a quad) or **6 if using triangles**:

```cpp
struct LineVertex {
    glm::vec3 pos;
    glm::vec3 otherPos;
    float side;
};

// For a line from A to B, create 4 vertices:
// Two triangles: (0,1,2) and (2,1,3)
std::vector<LineVertex> vertices = {
    {A, B, -1.0f},  // vertex 0
    {A, B, +1.0f},  // vertex 1
    {B, A, -1.0f},  // vertex 2
    {B, A, +1.0f},  // vertex 3
};

std::vector<unsigned int> indices = {0, 1, 2, 2, 1, 3};
```

## Alternative: Geometry Shader

If you prefer, a geometry shader can generate the quad from just 2 input points:

```glsl
#version 330 core
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 uResolution;
uniform float uLineWidth;

void main() {
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    
    vec2 ndc0 = p0.xy / p0.w;
    vec2 ndc1 = p1.xy / p1.w;
    
    vec2 dir = normalize((ndc1 - ndc0) * uResolution);
    vec2 perp = vec2(-dir.y, dir.x) * uLineWidth / uResolution;
    
    gl_Position = vec4((ndc0 + perp) * p0.w, p0.zw); EmitVertex();
    gl_Position = vec4((ndc0 - perp) * p0.w, p0.zw); EmitVertex();
    gl_Position = vec4((ndc1 + perp) * p1.w, p1.zw); EmitVertex();
    gl_Position = vec4((ndc1 - perp) * p1.w, p1.zw); EmitVertex();
    EndPrimitive();
}
```

The geometry shader approach is cleaner but can be slower for many lines. The vertex-only approach is more efficient for batching large numbers of line segments.

**human** (Dec 6, 2025, 07:46 PM)

can you give me the vertex shader version, but in a form that is compatible with Qt's qsb compiler?

**assistant** (Dec 6, 2025, 07:47 PM)

 Qt's `qsb` compiler uses a Vulkan-style GLSL dialect with explicit bindings and slightly different syntax. Here's the adapted version:

```glsl
#version 440

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aOtherPos;
layout(location = 2) in float aSide;

layout(std140, binding = 0) uniform buf {
    mat4 uMVP;
    vec2 uResolution;
    float uLineWidth;
};

void main()
{
    // Transform both endpoints to clip space
    vec4 clipPos = uMVP * vec4(aPos, 1.0);
    vec4 clipOther = uMVP * vec4(aOtherPos, 1.0);
    
    // Convert to NDC
    vec2 ndcPos = clipPos.xy / clipPos.w;
    vec2 ndcOther = clipOther.xy / clipOther.w;
    
    // Convert to screen space (pixels)
    vec2 screenPos = ndcPos * uResolution * 0.5;
    vec2 screenOther = ndcOther * uResolution * 0.5;
    
    // Line direction and perpendicular in screen space
    vec2 dir = normalize(screenPos - screenOther);
    vec2 perp = vec2(-dir.y, dir.x);
    
    // Offset by half the line width in pixels
    vec2 offset = perp * (uLineWidth * 0.5) * aSide;
    
    // Convert offset back to NDC
    vec2 ndcOffset = offset / (uResolution * 0.5);
    
    // Apply offset in clip space
    gl_Position = vec4((ndcPos + ndcOffset) * clipPos.w, clipPos.z, clipPos.w);
}
```

And the corresponding fragment shader:

```glsl
#version 440

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 uMVP;
    vec2 uResolution;
    float uLineWidth;
    vec4 uColor;
};

void main()
{
    fragColor = uColor;
}
```

## Key Changes for Qt/qsb

1. **Version 440** — `qsb` prefers `#version 440` or `450` for broad compatibility
2. **Uniform blocks** — Use `layout(std140, binding = 0) uniform buf { ... }` instead of individual uniforms
3. **Explicit output location** — Fragment outputs need `layout(location = 0)`
4. **No `precision` qualifiers** — Those are for GLSL ES only

## Compiling with qsb

```bash
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -o line.vert.qsb line.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -o line.frag.qsb line.frag
```

## Uniform Buffer Layout in C++/Qt

Make sure your C++ struct matches the `std140` layout (watch out for padding):

```cpp
struct LineUniforms {
    QMatrix4x4 mvp;      // 64 bytes
    float resolution[2]; // 8 bytes
    float lineWidth;     // 4 bytes
    float _pad1;         // 4 bytes (padding to align vec4)
    float color[4];      // 16 bytes
};
```

Then upload via `QRhiBuffer` with `UniformBuffer` usage and bind it at binding point 0.
