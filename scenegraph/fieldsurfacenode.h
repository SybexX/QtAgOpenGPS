// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Field surface node - renders textured or solid color field surface

#ifndef FIELDSURFACENODE_H
#define FIELDSURFACENODE_H

#include <QSGNode>
#include <QSGGeometryNode>
#include <QMatrix4x4>
#include <QColor>

class QSGTexture;

class FieldSurfaceNode : public QSGNode
{
public:
    FieldSurfaceNode();
    ~FieldSurfaceNode() override;

    // Update the field surface geometry and material
    void update(const QMatrix4x4 &mvp,
                const QColor &fieldColor,
                bool isTextureOn,
                QSGTexture *texture,
                double eastingMin, double eastingMax,
                double northingMin, double northingMax,
                int textureCount);

private:
    void updateTextured(const QMatrix4x4 &mvp, const QColor &fieldColor,
                        QSGTexture *texture,
                        double eastingMin, double eastingMax,
                        double northingMin, double northingMax,
                        int textureCount);

    void updateSolidColor(const QMatrix4x4 &mvp, const QColor &fieldColor,
                          double eastingMin, double eastingMax,
                          double northingMin, double northingMax);

    void clearChildren();

    QSGGeometryNode *m_geomNode = nullptr;
    bool m_init_children = false;
    bool m_isTextured = false;
};

#endif // FIELDSURFACENODE_H
