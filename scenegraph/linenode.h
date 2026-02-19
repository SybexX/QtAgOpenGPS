// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Line node - renders a line using LineProperties

#ifndef LINENODE_H
#define LINENODE_H

#include <QSGGeometryNode>
#include <QMatrix4x4>
#include <QColor>
#include <QSize>
#include <QVector3D>
#include "lineproperties.h"

class LineNode : public QSGGeometryNode
{
public:
    LineNode();
    ~LineNode() override;

    void update(const QMatrix4x4 &mvp,
                const QSize &viewportSize,
                const LineProperties *properties);

    void rebuildLine();

private:
    void updateThickLineNode(QSGGeometryNode *node,
                            const QMatrix4x4 mvp,
                            const QSize &viewportSize,
                            float lineWidth,
                            const QColor &color,
                            bool dashed = false);
    
    bool m_rebuildGeometry;
};

#endif // LINENODE_H