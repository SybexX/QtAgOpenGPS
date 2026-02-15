// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Grid node - renders the world grid lines
// Optimized to only regenerate geometry when vehicle moves near grid edge

#ifndef GRIDNODE_H
#define GRIDNODE_H

#include <QSGNode>
#include <QSGGeometryNode>
#include <QMatrix4x4>
#include <QSize>
#include <QColor>

class GridNode : public QSGNode
{
public:
    GridNode();
    ~GridNode() override;

    // Main update - checks if geometry rebuild is needed, always updates MVP
    void update(const QMatrix4x4 &mvMatrix,
                const QMatrix4x4 &pMatrix,
                const QMatrix4x4 &ndcMatrix,
                const QSize &viewportSize,
                double vehicleX,
                double vehicleY,
                double cameraZoom,
                double gridSize,
                const QColor &gridColor,
                double lineWidth);

private:
    void clearChildren();
    void rebuildGeometry(const QMatrix4x4 &mvMatrix,
                         const QMatrix4x4 &pMatrix,
                         const QMatrix4x4 &ndcMatrix,
                         const QSize &viewportSize,
                         const QColor &gridColor,
                         double lineWidth);
    void updateMvp(const QMatrix4x4 &mvMatrix,
                   const QMatrix4x4 &pMatrix,
                   const QMatrix4x4 &ndcMatrix,
                   const QSize &viewportSize);

    // Calculate grid spacing based on camera distance
    double calculateGridSpacing(double camDistance) const;

    // Check if vehicle has moved far enough to require rebuild
    bool needsRebuild(double vehicleX, double vehicleY, double camDistance) const;

    QSGGeometryNode *m_geomNode = nullptr;

    // Cached grid bounds
    double m_eastingMin = 0;
    double m_eastingMax = 0;
    double m_northingMin = 0;
    double m_northingMax = 0;
    double m_gridSpacing = 10.0;
    double m_gridSize = 6000;

    // Position when grid was last built
    double m_lastVehicleX = 0;
    double m_lastVehicleY = 0;
    double m_lastCamDistance = 0;

    // Track if we've ever built the grid
    bool m_hasGeometry = false;

    // Cached material properties for MVP updates
    QColor m_gridColor;
    double m_lineWidth = 1.0;
};

#endif // GRIDNODE_H
