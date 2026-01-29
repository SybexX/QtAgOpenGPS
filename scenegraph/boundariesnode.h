// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Boundary node - renders field boundaries

#ifndef BOUNDARIESNODE_H
#define BOUNDARIESNODE_H

#include <QSGNode>
#include <QSGGeometryNode>
#include <QMatrix4x4>
#include <QColor>
#include <QVector>
#include <QVector3D>
#include "boundariesproperties.h"

class BoundariesNode : public QSGNode
{
public:
    BoundariesNode();
    ~BoundariesNode() override;

    void update(const QMatrix4x4 &mv,
                const QMatrix4x4 &p,
                const QMatrix4x4 &ndc,
                const QSize &viewportSize, float vehicleX, float vehicleY, float vehicleHeading,
                bool isOutOfBounds,
                int lineWidth,
                const BoundariesProperties *properties);

    void clearChildren();

private:
    QList<QSGGeometryNode *> m_outerNodes;
    QList<QSGGeometryNode *> m_innerNodes;
    QSGGeometryNode *m_lastPointToPivotNode;
    QSGGeometryNode *m_bndBeingMadeNode;
    QSGGeometryNode *m_beingMadeDotsNode;

    void updateThickLineNode(QSGGeometryNode *node,
                        const QMatrix4x4 mvp,
                        const QSize &viewportSize,
                        int lineWidth, const QColor &color);

    void updateDashedLineNode(QSGGeometryNode *node,
                              const QMatrix4x4 mvp,
                              const QSize &viewportSize,
                              int lineWidth,
                              const QColor &color,
                              int dashLength = 10,
                              int dashGap = 10);
};

#endif // BOUNDARIESNODE_H
