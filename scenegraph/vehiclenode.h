// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Vehicle node - renders the vehicle representation

#ifndef VEHICLENODE_H
#define VEHICLENODE_H

#include <QSGNode>
#include <QSGGeometryNode>
#include <QSGTexture>
#include <QMatrix4x4>
#include <QColor>
#include <QHash>
#include "texturefactory.h"
#include "vehicleproperties.h"

class QQuickWindow;

// Node types for vehicle geometry components
enum class VehicleNodeType {
    HitchShadow,
    HitchLine,
    Body,
    ArrowLineLoop,
    RightWheel,
    LeftWheel,
    QuestionMark,
    AntennaDot,
    AntennaDotShadow,
    Marker,
    SvennArrow
};

class VehicleNode : public QSGNode
{
public:
    VehicleNode();
    ~VehicleNode() override;

    void update(const QMatrix4x4 &mv,
                const QMatrix4x4 &p,
                const QMatrix4x4 &ncd,
                const QColor &vehicleColor,
                const QSize &viewportSize,
                TextureFactory *textureFactory,
                double vehicleX, double vehicleY,
                double vehicleHeading,
                const VehicleProperties *properties,
                double camSetDistance);

    void clearChildren();

private:
    void updateNodeMvp(QSGGeometryNode *node,
                       const QMatrix4x4 mvp,
                       const QSize &viewportSize);


    QHash<VehicleNodeType, QSGGeometryNode*> m_nodes;
};

#endif // VEHICLENODE_H
