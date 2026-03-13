// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Dashed thick line material - renders dashed lines with constant screen-pixel width

#ifndef DASHEDTHICKLINEMATERIAL_H
#define DASHEDTHICKLINEMATERIAL_H

#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QMatrix4x4>
#include <QColor>
#include <QSize>
#include <QSGGeometry>
#include "aogmaterial.h"
#include "aoggeometry.h"  // For DashedThickLineVertex struct

class DashedThickLineMaterial : public AOGMaterial
{
public:
    DashedThickLineMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    int compare(const QSGMaterial *other) const override;

    void setColor(const QColor &color);
    QColor color() const { return m_color; }

    void setLineWidth(float width);
    float lineWidth() const { return m_lineWidth; }

    // Dash pattern (in screen pixels)
    void setDashLength(float length);
    float dashLength() const { return m_dashLength; }

    void setGapLength(float length);
    float gapLength() const { return m_gapLength; }

    // Geometry attribute set for dashed thick lines
    static const QSGGeometry::AttributeSet &attributes();

private:
    QColor m_color = Qt::white;
    float m_lineWidth = 2.0f;    // Width in screen pixels
    float m_dashLength = 10.0f;  // Dash length in screen pixels
    float m_gapLength = 5.0f;    // Gap length in screen pixels
};

class DashedThickLineMaterialShader : public AOGMaterialShader
{
public:
    DashedThickLineMaterialShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

#endif // DASHEDTHICKLINEMATERIAL_H
