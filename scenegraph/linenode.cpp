// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Line node implementation

#include "linenode.h"
#include "thicklinematerial.h"
#include "dashedthicklinematerial.h"
#include "aoggeometry.h"
#include <QSGGeometry>

LineNode::LineNode()
    : m_rebuildGeometry(false)
{
}

LineNode::~LineNode()
{
}

void LineNode::update(const QMatrix4x4 &mvp,
                      const QSize &viewportSize,
                      const LineProperties *properties)
{
    // Check if we have points to draw
    if (properties->points().isEmpty()) {
        // No points, so we don't need to render anything
        return;
    }

    // Check if geometry needs to be rebuilt
    bool geometryDirty = m_rebuildGeometry || !geometry();
    
    // Get the points to draw
    const QVector<QVector3D> &points = properties->points();

    // Only regenerate geometry if flagged or geometry doesn't exist
    if (geometryDirty) {
        // Clean up existing geometry if it exists
        if (geometry()) {
            delete geometry();
            setGeometry(nullptr);
        }

        // Create new geometry based on current properties
        QSGGeometry *newGeometry = nullptr;
        if (properties->loop()) {
            // Create loop geometry
            if (properties->dashed()) {
                newGeometry = AOGGeometry::createDashedThickLineLoopGeometry(points);
            } else {
                newGeometry = AOGGeometry::createThickLineLoopGeometry(points);
            }
        } else {
            // Create regular line geometry
            if (properties->dashed()) {
                newGeometry = AOGGeometry::createDashedThickLineGeometry(points);
            } else {
                newGeometry = AOGGeometry::createThickLineGeometry(points);
            }
        }
        
        if (newGeometry) {
            setGeometry(newGeometry);
            setFlag(QSGNode::OwnsGeometry);
        }
        
        // Reset the rebuild flag
        m_rebuildGeometry = false;
    }

    // Always update material properties (MVP matrix, color, width, etc.)
    QSGMaterial *mat = this->material();
    if (!mat) {
        // Create new material based on properties
        if (properties->dashed()) {
            auto *dashedMaterial = new DashedThickLineMaterial();
            dashedMaterial->setLineWidth(properties->width());
            dashedMaterial->setColor(properties->color());
            dashedMaterial->setViewportSize(viewportSize);
            dashedMaterial->setMvpMatrix(mvp);
            setMaterial(dashedMaterial);
            setFlag(QSGNode::OwnsMaterial);
        } else {
            auto *thickMaterial = new ThickLineMaterial();
            thickMaterial->setLineWidth(properties->width());
            thickMaterial->setColor(properties->color());
            thickMaterial->setViewportSize(viewportSize);
            thickMaterial->setMvpMatrix(mvp);
            setMaterial(thickMaterial);
            setFlag(QSGNode::OwnsMaterial);
        }
    } else {
        // Update existing material with current properties
        if (properties->dashed()) {
            auto *dashedMaterial = static_cast<DashedThickLineMaterial*>(mat);
            dashedMaterial->setLineWidth(properties->width());
            dashedMaterial->setColor(properties->color());
            dashedMaterial->setViewportSize(viewportSize);
            dashedMaterial->setMvpMatrix(mvp);
        } else {
            auto *thickMaterial = static_cast<ThickLineMaterial*>(mat);
            thickMaterial->setLineWidth(properties->width());
            thickMaterial->setColor(properties->color());
            thickMaterial->setViewportSize(viewportSize);
            thickMaterial->setMvpMatrix(mvp);
        }
        // We don't mark material as dirty here since we're updating all material properties
        // The scene graph will handle the material update
    }
}

void LineNode::rebuildLine()
{
    m_rebuildGeometry = true;
}