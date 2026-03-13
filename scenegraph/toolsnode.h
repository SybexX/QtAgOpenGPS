// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Tools node - renders tools/implements attached to the vehicle

#ifndef TOOLSNODE_H
#define TOOLSNODE_H

#include <QSGNode>
#include <QSGGeometryNode>
#include <QMatrix4x4>
#include <QSize>
#include <QList>

class TextureFactory;
class ToolsProperties;

class ToolsNode : public QSGNode
{
public:
    ToolsNode();
    ~ToolsNode() override;

    void update(const QMatrix4x4 &mv,
                const QMatrix4x4 &p,
                const QMatrix4x4 &ndc,
                const QSize &viewportSize,
                TextureFactory *textureFactory,
                ToolsProperties *properties,
                double camSetDistance);

    void clearChildren();

private:
    // Each tool gets its own list of nodes
    QList<QList<QSGGeometryNode*>> m_toolNodes;
    QList<QList<QSGGeometryNode*>> m_sectionNodes;


    void updateNodeMvp(QSGGeometryNode *node,
                       const QMatrix4x4 mvp,
                       const QSize &viewportSize);

    void updateNodeColor(QSGGeometryNode *node,
                         const QColor &color);

};

#endif // TOOLSNODE_H
