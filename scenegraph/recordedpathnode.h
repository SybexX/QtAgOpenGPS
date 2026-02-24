// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Recorded path node - renders recorded trail, Dubins approach path, and lookahead

#ifndef RECORDEDPATHNODE_H
#define RECORDEDPATHNODE_H

#include <QSGNode>

#include "recordedpathproperties.h"

class DotsNode;

class RecordedPathNode : public QSGNode
{
public:
    RecordedPathNode();
    ~RecordedPathNode() override;

    void update(const QMatrix4x4 &mv,
                const QMatrix4x4 &p,
                const QMatrix4x4 &ndc,
                const QSize &viewportSize,
                RecordedPathProperties *properties);

    void clearChildren();

private:
    QSGGeometryNode *m_trailLineNode = nullptr;
    DotsNode *m_dubinsDotsNode = nullptr;
    DotsNode *m_lookaheadDotNode = nullptr;
};

#endif // RECORDEDPATHNODE_H
