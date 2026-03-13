// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Geometry building utilities for scene graph rendering

#ifndef AOGGEOMETRY_H
#define AOGGEOMETRY_H

#include <QSGGeometry>
#include <QVector>
#include <QVector3D>
#include <QColor>

// ============================================================================
// Custom Vertex Structures
// ============================================================================

// Vertex with position and color (for vertex-colored geometry)
struct ColorVertex {
    float x, y, z;      // 3D position
    float r, g, b, a;   // RGBA color
};

struct ColorSizeVertex {
    float x, y, z;      // 3D position
    float r, g, b, a;   // RGBA color
    float s; //dot size
};

struct ColorSizeVertexVectors {
    QVector3D position;
    QColor color;
    float size;
};

struct PositionVertex {
    float x,y,z; // 3D position
};

// Vertex with position and texture coordinate
struct TexturedVertex {
    float x, y, z;      // 3D position
    float u, v;         // Texture coordinates
};

// Vertex for thick lines (screen-space width)
// Each vertex knows both endpoints so shader can compute screen-space direction
struct ThickLineVertex {
    float ax, ay, az;  // Endpoint A position
    float bx, by, bz;  // Endpoint B position
    float side;        // -1 or +1 (which side of the line)
};

struct ThickLineColorsVertex {
    float ax, ay, az;  // Endpoint A position
    float r, g, b, a;  // endpoint A color
    float bx, by, bz;  // Endpoint B position
    float side;        // -1 or +1 (which side of the line)
};

struct ColorVertexVectors {
    QVector3D vertex;
    QColor color;
};

// Vertex for dashed thick lines (screen-space width with dash pattern)
// Same as ThickLineVertex but with cumulative distance for dash calculation
struct DashedThickLineVertex {
    float ax, ay, az;  // Endpoint A position (pos)
    float bx, by, bz;  // Endpoint B position (nextPos)
    float side;        // -1 or +1 (which side of the line)
    float distance;    // Cumulative distance along the line (world units)
};

// ============================================================================
// Custom Attribute Sets
// ============================================================================

namespace AOGGeometry {

// Position-only attribute (3 floats)
const QSGGeometry::AttributeSet &positionAttributes();

// Position (3 floats) + Color (4 floats)
const QSGGeometry::AttributeSet &colorVertexAttributes();

// Position (3 floats) + Color (4 floats) + Size (1 float)
const QSGGeometry::AttributeSet &colorSizeVertexAttributes();

// Position (3 floats) + TexCoord (2 floats)
const QSGGeometry::AttributeSet &texturedVertexAttributes();

// Thick line attributes (for screen-space width lines)
// posA (3) + posB (3) + side (1) = 7 floats per vertex
const QSGGeometry::AttributeSet &thickLineAttributes();

// Dashed thick line attributes (for screen-space width lines with dash pattern)
// posA (3) + posB (3) + side (1) + distance (1) = 8 floats per vertex
const QSGGeometry::AttributeSet &dashedThickLineAttributes();

// ============================================================================
// Geometry Creation Functions
// ============================================================================

// Create line strip geometry from 3D points
// Returns new QSGGeometry with GL_LINE_STRIP drawing mode
QSGGeometry *createLineStripGeometry(const QVector<QVector3D> &points);

// Create line loop geometry from 3D points (closed polygon outline)
// Returns new QSGGeometry with GL_LINE_LOOP drawing mode
QSGGeometry *createLineLoopGeometry(const QVector<QVector3D> &points);

// Create lines geometry (separate line segments)
// points must have even number of elements (pairs of start/end points)
QSGGeometry *createLinesGeometry(const QVector<QVector3D> &points);

// Create triangle strip geometry
QSGGeometry *createTriangleStripGeometry(const QVector<QVector3D> &vertices);

// Create triangle fan geometry (for filled polygons with center point)
QSGGeometry *createTriangleFanGeometry(const QVector<QVector3D> &vertices);

// Create triangles geometry (separate triangles)
// vertices.size() must be multiple of 3
QSGGeometry *createTrianglesGeometry(const QVector<QVector3D> &vertices);

// Create colored line strip (per-vertex colors)
QSGGeometry *createColoredLineStripGeometry(const QVector<QVector3D> &points,
                                            const QVector<QColor> &colors);

// Create colored triangles
QSGGeometry *createColoredTrianglesGeometry(const QVector<QVector3D> &vertices,
                                            const QVector<QColor> &colors);

// Create textured quad
QSGGeometry *createTexturedQuadGeometry(const QRectF &rect, const QRectF &texCoords,
                                        float z = 0.0f);

// Create thick line geometry for screen-space width lines
// Each line segment becomes a quad (4 vertices in triangle strip)
// Use with ThickLineMaterial which expands to screen-pixel width in shader
QSGGeometry *createLinesGeometry2(const QVector<QVector3D> &points);

// For CONNECTED polylines: points[0]→points[1]→points[2]→...
QSGGeometry *createThickLineGeometry(const QVector<QVector3D> &points);
void updateThickLineGeometry(ThickLineVertex *data, const QVector<QVector3D> &points);

// For DISCONNECTED line segments (like GL_LINES): points[0]→points[1], points[2]→points[3], ...
QSGGeometry *createThickLinesGeometry(const QVector<QVector3D> &points);

// Create thick line loop geometry (closed loop)
QSGGeometry *createThickLineLoopGeometry(const QVector<QVector3D> &points);

QSGGeometry *createThickLineColorsGeometry(const QVector<ColorVertexVectors> &points);
void updateThickLineColorsGeometry(ThickLineColorsVertex *data, const QVector<ColorVertexVectors> &points);

QSGGeometry *createThickLinesColorsGeometry(const QVector<ColorVertexVectors> &points);
QSGGeometry *createThickLineColorsLoopGeometry(const QVector<ColorVertexVectors> &points);

// Create dashed thick line geometry for screen-space width dashed lines
// For CONNECTED polylines: points[0]→points[1]→points[2]→...
// Use with DashedThickLineMaterial which expands to screen-pixel width and applies dash pattern
QSGGeometry *createDashedThickLineGeometry(const QVector<QVector3D> &points);
void updateDashedThickLineGeometry(DashedThickLineVertex *data, const QVector<QVector3D> &points);

// For DISCONNECTED line segments (like GL_LINES): points[0]→points[1], points[2]→points[3], ...
QSGGeometry *createDashedThickLinesGeometry(const QVector<QVector3D> &points);

// Create dashed thick line loop geometry (closed loop)
QSGGeometry *createDashedThickLineLoopGeometry(const QVector<QVector3D> &points);

// ============================================================================
// Geometry Update Functions (for retained mode - reuse existing QSGGeometry)
// ============================================================================

// Update existing line strip geometry with new points
// Returns true if geometry was resized, false if just data changed
bool updateLineStripGeometry(QSGGeometry *geometry, const QVector<QVector3D> &points);

// Update existing triangle geometry
bool updateTrianglesGeometry(QSGGeometry *geometry, const QVector<QVector3D> &vertices);

// Update colored triangles geometry
bool updateColoredTrianglesGeometry(QSGGeometry *geometry,
                                    const QVector<QVector3D> &vertices,
                                    const QVector<QColor> &colors);

// ============================================================================
// Utility Functions
// ============================================================================

// Convert Vec3 vector to QVector3D vector
QVector<QVector3D> toQVector3D(const void *vec3Array, int count);

// Triangulate a simple convex polygon (for boundary rendering)
// Input: polygon vertices in order
// Output: triangle indices (every 3 indices form a triangle)
QVector<int> triangulateConvex(int vertexCount);

// Triangulate using ear clipping for simple polygons
// Returns list of triangle vertices
QVector<QVector3D> triangulateEarClipping(const QVector<QVector3D> &polygon);

} // namespace AOGGeometry

#endif // AOGGEOMETRY_H
