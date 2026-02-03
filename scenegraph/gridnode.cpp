// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Grid node implementation - optimized to only rebuild when necessary

#include "gridnode.h"
#include "materials.h"
#include "aoggeometry.h"
// #include "thicklinematerial.h"  // Not used - thick lines disabled for grid

#include <QtMath>
#include <QVector3D>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(gridnode_log, "gridnode.qtagopengps")

GridNode::GridNode()
{
}

GridNode::~GridNode()
{
}

void GridNode::clearChildren()
{
    while (childCount() > 0) {
        QSGNode *child = firstChild();
        removeChildNode(child);
        delete child;
    }
    m_geomNode = nullptr;
    m_hasGeometry = false;
}

double GridNode::calculateGridSpacing(double camDistance) const
{
    if (camDistance <= 20000 && camDistance > 10000) return 2012;
    else if (camDistance <= 10000 && camDistance > 5000) return 1006;
    else if (camDistance <= 5000 && camDistance > 2000) return 503;
    else if (camDistance <= 2000 && camDistance > 1000) return 201.2;
    else if (camDistance <= 1000 && camDistance > 500) return 100.6;
    else if (camDistance <= 500 && camDistance > 250) return 50.3;
    else if (camDistance <= 250 && camDistance > 150) return 25.15;
    else if (camDistance <= 150 && camDistance > 50) return 10.06;
    else if (camDistance <= 50 && camDistance > 1) return 5.03;
    return 10.0;
}

bool GridNode::needsRebuild(double vehicleX, double vehicleY, double camDistance) const
{
    // Always rebuild if we don't have geometry yet
    if (!m_hasGeometry) {
        return true;
    }

    // Check if zoom level changed enough to affect grid spacing
    double newSpacing = calculateGridSpacing(camDistance);
    if (!qFuzzyCompare(newSpacing, m_gridSpacing)) {
        return true;
    }

    // Calculate grid half-size (distance from center to edge)
    double halfSize = m_gridSize;

    // Check if vehicle has moved halfway to any edge
    // The grid is centered at (m_lastVehicleX, m_lastVehicleY)
    double dx = qAbs(vehicleX - m_lastVehicleX);
    double dy = qAbs(vehicleY - m_lastVehicleY);

    // Rebuild when vehicle is halfway to the edge in either direction
    if (dx > halfSize / 2.0 || dy > halfSize / 2.0) {
        return true;
    }

    return false;
}

void GridNode::update(const QMatrix4x4 &mvMatrix,
                      const QMatrix4x4 &pMatrix,
                      const QMatrix4x4 &ndcMatrix,
                      const QSize &viewportSize,
                      double vehicleX,
                      double vehicleY,
                      double cameraZoom,
                      double gridSize,
                      const QColor &gridColor,
                      double lineWidth)
{
    double camDistance = qAbs(cameraZoom);
    m_gridSize = gridSize;

    // Check if we need to rebuild geometry
    if (needsRebuild(vehicleX, vehicleY, camDistance)) {
        // Calculate new grid bounds centered on vehicle
        m_gridSpacing = calculateGridSpacing(camDistance);
        m_lastVehicleX = vehicleX;
        m_lastVehicleY = vehicleY;
        m_lastCamDistance = camDistance;

        // Grid bounds: vehicle position +/- gridSize
        m_eastingMin = vehicleX - gridSize;
        m_eastingMax = vehicleX + gridSize;
        m_northingMin = vehicleY - gridSize;
        m_northingMax = vehicleY + gridSize;

        // Store material properties for potential future MVP-only updates
        m_gridColor = gridColor;
        m_lineWidth = lineWidth;

        // Rebuild the geometry
        rebuildGeometry(mvMatrix, pMatrix, ndcMatrix, viewportSize, gridColor, lineWidth);
    } else {
        // Just update the MVP matrix
        updateMvp(mvMatrix, pMatrix, ndcMatrix, viewportSize);
    }
}

void GridNode::rebuildGeometry(const QMatrix4x4 &mvMatrix,
                                const QMatrix4x4 &pMatrix,
                                const QMatrix4x4 &ndcMatrix,
                                const QSize &viewportSize,
                                const QColor &gridColor,
                                double lineWidth)
{
    Q_UNUSED(viewportSize)
    Q_UNUSED(lineWidth)

    // Clear existing geometry
    clearChildren();

    // Helper function to round mid away from zero
    auto roundMidAwayFromZero = [](double val) -> double {
        return (val >= 0) ? qFloor(val + 0.5) : qCeil(val - 0.5);
    };

    // Create grid lines - no clipping needed for native GL_LINES (GPU clips automatically)
    QVector<QVector3D> gridLines;

    // Create vertical lines (constant easting, varying northing)
    for (double num = roundMidAwayFromZero(m_eastingMin / m_gridSpacing) * m_gridSpacing;
         num < m_eastingMax; num += m_gridSpacing) {
        if (num < m_eastingMin) continue;

        gridLines.append(QVector3D(static_cast<float>(num), static_cast<float>(m_northingMax), 0.1f));
        gridLines.append(QVector3D(static_cast<float>(num), static_cast<float>(m_northingMin), 0.1f));
    }

    // Create horizontal lines (constant northing, varying easting)
    for (double num2 = roundMidAwayFromZero(m_northingMin / m_gridSpacing) * m_gridSpacing;
         num2 < m_northingMax; num2 += m_gridSpacing) {
        if (num2 < m_northingMin) continue;

        gridLines.append(QVector3D(static_cast<float>(m_eastingMax), static_cast<float>(num2), 0.1f));
        gridLines.append(QVector3D(static_cast<float>(m_eastingMin), static_cast<float>(num2), 0.1f));
    }

    if (gridLines.isEmpty())
        return;

    QMatrix4x4 mvp = ndcMatrix * pMatrix * mvMatrix;

    // Always use native 1-pixel lines for grid
    // Thick lines require per-frame clipping which defeats the optimization
    // TODO: Consider geometry shader approach for thick grid lines in the future
    //if (lineWidth <= 0.2) {
        // Use normal lines
        auto *geometry = AOGGeometry::createLinesGeometry(gridLines);
        if (!geometry)
            return;

        m_geomNode = new QSGGeometryNode();
        m_geomNode->setGeometry(geometry);
        m_geomNode->setFlag(QSGNode::OwnsGeometry);

        auto *material = new AOGFlatColorMaterial();
        material->setColor(gridColor);
        material->setMvpMatrix(mvp);

        m_geomNode->setMaterial(material);
        m_geomNode->setFlag(QSGNode::OwnsMaterial);

    //} else {
    //    // Use thick lines
    //    auto *geometry = AOGGeometry::createThickLinesGeometry(gridLines);
    //    if (!geometry)
    //        return;
    //
    //    m_geomNode = new QSGGeometryNode();
    //    m_geomNode->setGeometry(geometry);
    //    m_geomNode->setFlag(QSGNode::OwnsGeometry);
    //
    //    auto *material = new ThickLineMaterial();
    //    material->setColor(gridColor);
    //    material->setLineWidth(lineWidth);
    //    material->setMvpMatrix(mvp);
    //    material->setViewportSize(viewportSize);
    //
    //    m_geomNode->setMaterial(material);
    //    m_geomNode->setFlag(QSGNode::OwnsMaterial);
    //}

    appendChildNode(m_geomNode);
    m_hasGeometry = true;
}

void GridNode::updateMvp(const QMatrix4x4 &mvMatrix,
                         const QMatrix4x4 &pMatrix,
                         const QMatrix4x4 &ndcMatrix,
                         const QSize &viewportSize)
{
    Q_UNUSED(viewportSize)

    if (!m_geomNode)
        return;

    QMatrix4x4 mvp = ndcMatrix * pMatrix * mvMatrix;

    // Update the material's MVP matrix (always flat color material now)
    auto *material = static_cast<AOGFlatColorMaterial*>(m_geomNode->material());
    if (material) {
        material->setMvpMatrix(mvp);
    }

    // Thick lines disabled - would need per-frame clipping
    //if (m_lineWidth <= 0.2) {
    //    ...
    //} else {
    //    auto *material = static_cast<ThickLineMaterial*>(m_geomNode->material());
    //    if (material) {
    //        material->setMvpMatrix(mvp);
    //        material->setViewportSize(viewportSize);
    //    }
    //}

    m_geomNode->markDirty(QSGNode::DirtyMaterial);
}
