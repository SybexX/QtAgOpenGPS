// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Round point material - renders points as circles with optional soft edges

#ifndef ROUNDPOINTMATERIAL_H
#define ROUNDPOINTMATERIAL_H

#include "aogmaterial.h"
#include <QSGMaterialShader>

class RoundPointMaterial : public AOGMaterial
{
public:
    RoundPointMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;
    int compare(const QSGMaterial *other) const override;

    void setPointSize(float size) { m_pointSize = size; }
    float pointSize() const { return m_pointSize; }

    // Softness: 0.0 = hard edge, 1.0 = very soft/blurry edge
    void setSoftness(float softness) { m_softness = softness; }
    float softness() const { return m_softness; }

private:
    float m_pointSize = 10.0f;
    float m_softness = 0.2f;  // slight softness by default for anti-aliasing
};

class RoundPointShader : public AOGMaterialShader
{
public:
    RoundPointShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

#endif // ROUNDPOINTMATERIAL_H
