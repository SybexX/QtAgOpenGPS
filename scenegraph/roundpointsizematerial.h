// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Round point material - renders points as circles with optional soft edges

#ifndef ROUNDPOINTSIZEMATERIAL_H
#define ROUNDPOINTSIZEMATERIAL_H

#include "aogmaterial.h"
#include <QSGMaterialShader>

class RoundPointSizeMaterial : public AOGMaterial
{
public:
    RoundPointSizeMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;
    int compare(const QSGMaterial *other) const override;

    // Softness: 0.0 = hard edge, 1.0 = very soft/blurry edge
    void setSoftness(float softness) { m_softness = softness; }
    float softness() const { return m_softness; }

private:
    float m_softness = 0.2f;  // slight softness by default for anti-aliasing
};

class RoundPointSizeShader : public AOGMaterialShader
{
public:
    RoundPointSizeShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

#endif // ROUNDPOINTSIZEMATERIAL_H
