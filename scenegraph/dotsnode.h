// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Dot node - scene graph node for rendering round points/dots

#ifndef DOTSNODE_H
#define DOTSNODE_H

#include <QSGNode>
#include <QSGGeometryNode>
#include <QMatrix4x4>
#include <QVector>
#include <QVector3D>
#include <QColor>
#include <QSize>
#include "aoggeometry.h"

class DotsNode : public QSGNode
{
public:
    DotsNode();

    // Add a single dot
    // position: 3D position of the dot
    // color: RGBA color of the dot
    // size: size of the dot in pixels
    void addDot(const QVector3D &position, const QColor &color, float size);

    // Add a single dot using ColorSizeVertexVectors
    void addDot(const ColorSizeVertexVectors &dot);

    // Add multiple dots
    void addDots(const QVector<ColorSizeVertexVectors> &dots);

    // Build or rebuild the geometry from accumulated dots
    // Call this after adding dots to create/update the geometry node
    void build();

    // Clear all children and accumulated dots
    void clearChildren();

    // Update the MVP matrix on all child nodes
    void updateUniforms(const QMatrix4x4 &mvp, const QSize &viewportSize);

private:
    QVector<ColorSizeVertexVectors> m_dots;
    QSGGeometryNode *m_geometryNode = nullptr;
};

#endif // DOTSNODE_H
