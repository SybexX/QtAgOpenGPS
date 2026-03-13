# Round Points with RoundPointMaterial

This document shows how to render round dots/circles using `RoundPointMaterial` with the Qt Scene Graph.

## Overview

`RoundPointMaterial` renders GL_POINTS as circles with optional soft/anti-aliased edges. It uses `gl_PointCoord` in the fragment shader to discard pixels outside the circle radius, and `smoothstep` to create soft edges.

## Features

- Multiple points with a single geometry node
- Per-vertex colors using `ColorVertex`
- Arbitrary point sizes (in pixels)
- Configurable edge softness (anti-aliasing)

## Usage

```cpp
#include "roundpointmaterial.h"
#include "aoggeometry.h"

// Create geometry with ColorVertex (position + color per point)
int numPoints = 5;
auto *geometry = new QSGGeometry(AOGGeometry::colorVertexAttributes(), numPoints);
geometry->setDrawingMode(QSGGeometry::DrawPoints);

// Fill vertex data
ColorVertex *v = static_cast<ColorVertex*>(geometry->vertexData());
v[0] = { 10.0f, 20.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f };  // red point
v[1] = { 30.0f, 40.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f };  // green point
v[2] = { 50.0f, 60.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f };  // blue point
v[3] = { 70.0f, 80.0f, 0.0f,  1.0f, 1.0f, 0.0f, 1.0f };  // yellow point
v[4] = { 90.0f, 100.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.5f };  // magenta, 50% alpha

// Create geometry node
auto *node = new QSGGeometryNode();
node->setGeometry(geometry);
node->setFlag(QSGNode::OwnsGeometry);

// Create and configure material
auto *material = new RoundPointMaterial();
material->setMvpMatrix(mvp);           // your model-view-projection matrix
material->setPointSize(20.0f);         // diameter in pixels
material->setSoftness(0.2f);           // edge softness (0-1)

node->setMaterial(material);
node->setFlag(QSGNode::OwnsMaterial);

// Add to scene graph
parentNode->appendChildNode(node);
```

## Material Properties

### `setPointSize(float size)`
Sets the diameter of the points in pixels.

### `setSoftness(float softness)`
Controls edge anti-aliasing:
- `0.0` - Hard, crisp edge (no anti-aliasing)
- `0.2` - Slight softness (good default for anti-aliasing)
- `0.5` - Moderate blur
- `1.0` - Very soft/glowing effect

### `setMvpMatrix(const QMatrix4x4 &mvp)`
Sets the model-view-projection matrix for transforming point positions.

## ColorVertex Structure

```cpp
struct ColorVertex {
    float x, y, z;     // position
    float r, g, b, a;  // color (0.0 - 1.0)
};
```

## Updating Points

To update point positions or colors each frame:

```cpp
// If geometry already exists, get vertex data and modify
ColorVertex *v = static_cast<ColorVertex*>(geometry->vertexData());
v[0].x = newX;
v[0].y = newY;
// ... update other vertices ...

// Mark geometry as dirty
node->markDirty(QSGNode::DirtyGeometry);

// Update material if needed
auto *material = static_cast<RoundPointMaterial*>(node->material());
material->setMvpMatrix(newMvp);
```

## Shader Details

### Vertex Shader (`roundpoint.vert`)
- Passes vertex color to fragment shader
- Sets `gl_PointSize` from uniform

### Fragment Shader (`roundpoint.frag`)
- Uses `gl_PointCoord` (0-1 range within point sprite)
- Calculates distance from center
- Uses `smoothstep` for soft edge alpha blending
- Discards fragments outside the circle
