# Adding Shader Uniforms and Vertex Attributes to Materials

This document explains how to add new uniform variables and vertex attributes to shaders and their corresponding material classes in QtAgOpenGPS.

## Overview

The Qt Scene Graph uses a material/shader architecture where:
- **Vertex structure** (C++ struct) defines per-vertex data layout
- **AttributeSet** (C++) tells Qt how to interpret the vertex data
- **Material class** (C++) holds uniform data and provides it to the shader
- **Shader files** (.vert/.frag) declare inputs and uniforms
- **MaterialShader class** (C++) transfers uniform data to the GPU

## Part 1: Adding Vertex Attributes

Vertex attributes are per-vertex data (position, color, texture coords, etc.) passed to the vertex shader.

### 1.1 Define the Vertex Structure (aoggeometry.h)

Create a struct that defines the data layout for each vertex:

```cpp
// In aoggeometry.h

// Example: Vertex for dashed thick lines
struct DashedThickLineVertex {
    float ax, ay, az;    // Current vertex position (vec3) - 12 bytes
    float bx, by, bz;    // Neighbor vertex position (vec3) - 12 bytes
    float side;          // Which side of the line (-1 or +1) - 4 bytes
    float distance;      // Cumulative distance along line - 4 bytes
};                       // Total: 32 bytes per vertex
```

**Important**: The struct layout must match the shader's input layout exactly.

### 1.2 Create the AttributeSet Function (aoggeometry.cpp)

The AttributeSet tells Qt how to interpret the vertex buffer:

```cpp
// In aoggeometry.cpp

const QSGGeometry::AttributeSet &dashedThickLineAttributes()
{
    static QSGGeometry::Attribute attrs[] = {
        // create(location, tupleSize, type, isPosition)
        QSGGeometry::Attribute::create(0, 3, QSGGeometry::FloatType, true),   // pos (location 0)
        QSGGeometry::Attribute::create(1, 3, QSGGeometry::FloatType, false),  // nextPos (location 1)
        QSGGeometry::Attribute::create(2, 1, QSGGeometry::FloatType, false),  // side (location 2)
        QSGGeometry::Attribute::create(3, 1, QSGGeometry::FloatType, false),  // distance (location 3)
    };

    static QSGGeometry::AttributeSet attrSet = {
        4,                               // attribute count
        sizeof(DashedThickLineVertex),   // stride (bytes per vertex)
        attrs
    };

    return attrSet;
}
```

**Parameters for `Attribute::create()`**:
- `location`: Shader input location (matches `layout(location = N)`)
- `tupleSize`: Number of components (1 for float, 2 for vec2, 3 for vec3, 4 for vec4)
- `type`: Data type (`FloatType`, `UnsignedByteType`, etc.)
- `isPosition`: `true` for the primary position attribute (used by Qt for culling)

### 1.3 Declare in Header (aoggeometry.h)

```cpp
// In aoggeometry.h, inside namespace AOGGeometry

const QSGGeometry::AttributeSet &dashedThickLineAttributes();
```

### 1.4 Update the Vertex Shader Input Layout

The shader inputs must match the AttributeSet locations and sizes:

```glsl
// In your_shader.vert

layout(location = 0) in vec3 pos;       // matches attr 0, tupleSize 3
layout(location = 1) in vec3 nextPos;   // matches attr 1, tupleSize 3
layout(location = 2) in float side;     // matches attr 2, tupleSize 1
layout(location = 3) in float distance; // matches attr 3, tupleSize 1
```

### 1.5 Create Geometry Functions (aoggeometry.cpp)

Create functions to build geometry using your vertex structure:

```cpp
QSGGeometry *createDashedThickLineGeometry(const QVector<QVector3D> &points)
{
    if (points.size() < 2)
        return nullptr;

    int numSegments = points.size() - 1;
    int numVertices = numSegments * 4;  // 4 vertices per segment (quad)

    auto *geometry = new QSGGeometry(dashedThickLineAttributes(), numVertices);
    geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

    DashedThickLineVertex *data = static_cast<DashedThickLineVertex *>(geometry->vertexData());

    float cumulativeDistance = 0.0f;
    int idx = 0;

    for (int seg = 0; seg < numSegments; ++seg) {
        const QVector3D &a = points[seg];
        const QVector3D &b = points[seg + 1];
        float segmentLength = (b - a).length();

        // Fill in vertex data...
        data[idx].ax = a.x();
        data[idx].ay = a.y();
        data[idx].az = a.z();
        data[idx].bx = b.x();
        data[idx].by = b.y();
        data[idx].bz = b.z();
        data[idx].side = -1.0f;
        data[idx].distance = cumulativeDistance;
        idx++;

        // ... continue for other vertices
    }

    return geometry;
}
```

### 1.6 Correspondence Table

| C++ Struct Field | AttributeSet | Shader Input |
|------------------|--------------|--------------|
| `float ax, ay, az` | `create(0, 3, FloatType, true)` | `layout(location = 0) in vec3 pos` |
| `float bx, by, bz` | `create(1, 3, FloatType, false)` | `layout(location = 1) in vec3 nextPos` |
| `float side` | `create(2, 1, FloatType, false)` | `layout(location = 2) in float side` |
| `float distance` | `create(3, 1, FloatType, false)` | `layout(location = 3) in float distance` |

---

## Part 2: Adding Uniforms

Uniforms are per-draw-call data (matrices, colors, settings) shared by all vertices.

### 2.1 Update the Shader Files

Add the new uniform to the uniform block in both vertex and fragment shaders (if needed by both).

**Example: Adding a `float opacity` uniform**

```glsl
// In your_shader.vert and your_shader.frag
layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;     // existing: 64 bytes at offset 0
    vec4 color;         // existing: 16 bytes at offset 64
    float lineWidth;    // existing: 4 bytes at offset 80
    float opacity;      // NEW: 4 bytes at offset 84
} ubuf;
```

### 2.2 Calculate the Uniform Buffer Offset

The uniform buffer uses **std140 layout rules**. Key alignment rules:

| Type | Size | Alignment |
|------|------|-----------|
| float | 4 bytes | 4 bytes |
| vec2 | 8 bytes | 8 bytes |
| vec3 | 12 bytes | 16 bytes (!) |
| vec4 | 16 bytes | 16 bytes |
| mat4 | 64 bytes | 16 bytes |
| int | 4 bytes | 4 bytes |

**Important**: `vec3` is aligned to 16 bytes, not 12! This often causes bugs.

Calculate your new uniform's offset by adding up the sizes of all preceding uniforms, respecting alignment:

```
mat4 mvpMatrix:  offset 0,  size 64  → next available: 64
vec4 color:      offset 64, size 16  → next available: 80
float lineWidth: offset 80, size 4   → next available: 84
float opacity:   offset 84, size 4   → next available: 88
```

### 2.3 Update the Material Header

Add a member variable, setter, and getter to the material class.

```cpp
// In yourmaterial.h
class YourMaterial : public QSGMaterial
{
public:
    // ... existing code ...

    // NEW: Add setter and getter
    void setOpacity(float opacity);
    float opacity() const { return m_opacity; }

private:
    // ... existing members ...
    float m_opacity = 1.0f;  // NEW: Add member with default value
};
```

### 2.4 Update the Material Implementation

Implement the setter and update the `compare()` method.

```cpp
// In yourmaterial.cpp

void YourMaterial::setOpacity(float opacity)
{
    m_opacity = opacity;
}

int YourMaterial::compare(const QSGMaterial *other) const
{
    const auto *o = static_cast<const YourMaterial *>(other);

    // ... existing comparisons ...

    // NEW: Add comparison for batching optimization
    if (m_opacity != o->m_opacity)
        return m_opacity < o->m_opacity ? -1 : 1;

    return 0;
}
```

### 2.5 Update the Shader's updateUniformData()

This is where you copy the C++ data into the GPU uniform buffer.

```cpp
// In yourmaterial.cpp

bool YourMaterialShader::updateUniformData(RenderState &state,
                                            QSGMaterial *newMaterial,
                                            QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    bool changed = false;
    QByteArray *buf = state.uniformData();
    auto *material = static_cast<YourMaterial *>(newMaterial);

    // Uniform buffer layout (std140):
    // offset 0:  mat4 mvpMatrix (64 bytes)
    // offset 64: vec4 color     (16 bytes)
    // offset 80: float lineWidth (4 bytes)
    // offset 84: float opacity   (4 bytes)  // NEW

    // ... existing uniform copies ...

    // NEW: Copy opacity at offset 84
    float opacity = material->opacity();
    memcpy(buf->data() + 84, &opacity, 4);

    changed = true;
    return changed;
}
```

## Common Patterns

### Copying Different Types

```cpp
// float (4 bytes)
float value = material->value();
memcpy(buf->data() + offset, &value, 4);

// vec2 (8 bytes)
QSize size = material->viewportSize();
float sizeData[2] = { float(size.width()), float(size.height()) };
memcpy(buf->data() + offset, sizeData, 8);

// vec4 / color (16 bytes)
QColor c = material->color();
float colorData[4] = {
    float(c.redF()), float(c.greenF()),
    float(c.blueF()), float(c.alphaF())
};
memcpy(buf->data() + offset, colorData, 16);

// mat4 (64 bytes)
QMatrix4x4 matrix = material->mvpMatrix();
memcpy(buf->data() + offset, matrix.constData(), 64);

// int/bool (4 bytes)
int flag = material->useFeature() ? 1 : 0;
memcpy(buf->data() + offset, &flag, 4);
```

### Using state.combinedMatrix()

When you need the final MVP matrix that includes Qt's scene graph transforms:

```cpp
// Combines your material's MVP with Qt's item positioning
QMatrix4x4 combined = state.combinedMatrix() * material->mvpMatrix();
memcpy(buf->data() + 0, combined.constData(), 64);
```

## Debugging Tips

1. **Shader won't compile**: Check that uniform block matches exactly between .vert and .frag
2. **Wrong values in shader**: Double-check std140 alignment and offsets
3. **vec3 issues**: Remember vec3 aligns to 16 bytes; consider using vec4 instead
4. **Visual glitches**: Verify `compare()` includes new uniforms for proper batching

## Example: Complete Uniform Block

Here's a well-documented example from `DashedThickLineMaterial`:

```cpp
// Uniform buffer layout (std140):
// offset 0:   mat4 mvpMatrix    (64 bytes)
// offset 64:  vec4 color        (16 bytes)
// offset 80:  vec2 viewportSize (8 bytes)
// offset 88:  float lineWidth   (4 bytes)
// offset 92:  float dashLength  (4 bytes)
// offset 96:  float gapLength   (4 bytes)
// Total: ~100 bytes
```

Corresponding shader declaration:

```glsl
layout(std140, binding = 0) uniform buf {
    mat4 mvpMatrix;
    vec4 color;
    vec2 viewportSize;
    float lineWidth;
    float dashLength;
    float gapLength;
} ubuf;
```

---

## Part 3: Complete Workflow Summary

When creating a new material with custom vertex attributes and uniforms:

### Files to Modify/Create

| File | What to Add |
|------|-------------|
| `scenegraph/aoggeometry.h` | Vertex struct definition |
| `scenegraph/aoggeometry.h` | AttributeSet function declaration |
| `scenegraph/aoggeometry.cpp` | AttributeSet function implementation |
| `scenegraph/aoggeometry.cpp` | Geometry creation functions |
| `scenegraph/yourmaterial.h` | Material class with uniform getters/setters |
| `scenegraph/yourmaterial.cpp` | Material and MaterialShader implementation |
| `shaders/yourshader.vert` | Vertex shader with matching inputs and uniforms |
| `shaders/yourshader.frag` | Fragment shader with matching uniforms |
| `CMakeLists.txt` | Register shaders in `qt_add_shaders()` |
| `CMakeLists.txt` | Add material source files to `SOURCES` |

### Checklist

1. [ ] Vertex struct fields match shader `in` variables (order, types, sizes)
2. [ ] AttributeSet locations match shader `layout(location = N)`
3. [ ] AttributeSet tuple sizes match shader types (3 for vec3, etc.)
4. [ ] AttributeSet stride equals `sizeof(YourVertex)`
5. [ ] Uniform block identical in .vert and .frag (if both use it)
6. [ ] Uniform offsets in `updateUniformData()` follow std140 rules
7. [ ] Material `compare()` includes all uniforms that affect rendering
8. [ ] Shaders added to `qt_add_shaders()` in CMakeLists.txt

## File Locations

- Vertex structures: `scenegraph/aoggeometry.h`
- AttributeSets & geometry functions: `scenegraph/aoggeometry.cpp`
- Material headers: `scenegraph/*.h`
- Material implementations: `scenegraph/*.cpp`
- Shaders: `shaders/*.vert`, `shaders/*.frag`
- Shader registration: `CMakeLists.txt` (look for `qt_add_shaders`)
