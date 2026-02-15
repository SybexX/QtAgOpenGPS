// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Coverage layer data types for scene graph rendering
// Used by LayersNode and LayerService

#ifndef LAYERTYPES_H
#define LAYERTYPES_H

#include <QVector3D>
#include <QColor>
#include <QRectF>
#include <QString>
#include <QVector>

// ============================================================================
// SectionPending - Pending vertices for triangle strip building per section
// ============================================================================

struct SectionPending {
    QVector3D prevLeft;      // Previous left vertex
    QVector3D prevRight;     // Previous right vertex
    QColor color;            // Section color
    bool hasPrevious = false; // Do we have previous vertices to form triangles with?

    // Reset to initial state
    void reset() {
        hasPrevious = false;
    }
};

// ============================================================================
// CoverageTriangle - Single triangle with vertices and color
// ============================================================================

struct CoverageTriangle {
    QVector3D v0, v1, v2;   // Three vertices of the triangle
    QColor color;            // Per-triangle color (flat shading)

    // Default constructor
    CoverageTriangle() = default;

    // Convenience constructor
    CoverageTriangle(const QVector3D &vertex0, const QVector3D &vertex1,
                     const QVector3D &vertex2, const QColor &c)
        : v0(vertex0), v1(vertex1), v2(vertex2), color(c) {}

    // Calculate bounding box for spatial queries
    QRectF boundingBox() const {
        float minX = qMin(qMin(v0.x(), v1.x()), v2.x());
        float maxX = qMax(qMax(v0.x(), v1.x()), v2.x());
        float minY = qMin(qMin(v0.y(), v1.y()), v2.y());
        float maxY = qMax(qMax(v0.y(), v1.y()), v2.y());
        return QRectF(minX, minY, maxX - minX, maxY - minY);
    }
};

// ============================================================================
// CoverageLayer - A collection of triangles with visibility and alpha
// ============================================================================

struct CoverageLayer {
    int id = -1;                        // Unique layer identifier
    QString name;                       // Human-readable layer name
    QVector<CoverageTriangle> triangles; // Committed triangle data
    QVector<SectionPending> pendingSections; // Per-section pending vertices
    float alpha = 1.0f;               // Layer-level alpha (AOG default)
    bool visible = true;                // Layer visibility
    QRectF boundingBox;                 // Cached bounding box for spatial queries

    // Default constructor
    CoverageLayer() = default;

    // Constructor with id and name
    CoverageLayer(int layerId, const QString &layerName)
        : id(layerId), name(layerName) {}

    // Configure for a given number of sections
    void configureSections(int count) {
        // Flush any pending vertices first (they become orphaned)
        flushPendingSections();
        // Resize and reset
        pendingSections.resize(count);
        for (SectionPending &section : pendingSections) {
            section.reset();
        }
    }

    // Flush all pending sections (reset hasPrevious flags)
    void flushPendingSections() {
        for (SectionPending &section : pendingSections) {
            section.reset();
        }
    }

    // Flush a single pending section (when one section turns off)
    void flushPendingSection(int sectionIndex) {
        if (sectionIndex >= 0 && sectionIndex < pendingSections.size()) {
            pendingSections[sectionIndex].reset();
        }
    }

    // Add two vertices for a section (like triangle strip)
    // Returns number of triangles added (0 or 2)
    int addSectionVertices(int sectionIndex, const QVector3D &left,
                           const QVector3D &right, const QColor &color) {
        if (sectionIndex < 0 || sectionIndex >= pendingSections.size()) {
            return 0;
        }

        SectionPending &section = pendingSections[sectionIndex];

        if (!section.hasPrevious) {
            // First pair of vertices - just store them
            section.prevLeft = left;
            section.prevRight = right;
            section.color = color;
            section.hasPrevious = true;
            return 0;
        }

        // We have previous vertices - create 2 triangles forming a quad
        // Quad: prevLeft -- left
        //       |       \   |
        //       |        \  |
        //       prevRight - right
        //
        // Triangle 1: prevLeft, prevRight, left
        // Triangle 2: left, prevRight, right

        CoverageTriangle tri1(section.prevLeft, section.prevRight, left, color);
        CoverageTriangle tri2(left, section.prevRight, right, color);

        triangles.append(tri1);
        triangles.append(tri2);

        // Update previous vertices for next strip segment
        section.prevLeft = left;
        section.prevRight = right;
        section.color = color;

        return 2;
    }

    // Update bounding box from all triangles
    void updateBoundingBox() {
        if (triangles.isEmpty()) {
            boundingBox = QRectF();
            return;
        }

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();

        for (const CoverageTriangle &tri : triangles) {
            minX = qMin(minX, qMin(qMin(tri.v0.x(), tri.v1.x()), tri.v2.x()));
            maxX = qMax(maxX, qMax(qMax(tri.v0.x(), tri.v1.x()), tri.v2.x()));
            minY = qMin(minY, qMin(qMin(tri.v0.y(), tri.v1.y()), tri.v2.y()));
            maxY = qMax(maxY, qMax(qMax(tri.v0.y(), tri.v1.y()), tri.v2.y()));
        }

        boundingBox = QRectF(minX, minY, maxX - minX, maxY - minY);
    }

    // Check if layer bounds intersect with given area
    bool intersects(const QRectF &area) const {
        return boundingBox.intersects(area);
    }

    // Get triangles within a given area (simple linear scan)
    QVector<CoverageTriangle> getTrianglesInArea(const QRectF &area) const {
        QVector<CoverageTriangle> result;
        for (const CoverageTriangle &tri : triangles) {
            if (tri.boundingBox().intersects(area)) {
                result.append(tri);
            }
        }
        return result;
    }
};

#endif // LAYERTYPES_H
