// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Recorded path node - renders recorded trail, Dubins approach path, and lookahead

#include "recordedpathnode.h"
#include "materials.h"
#include "aoggeometry.h"
#include "dotsnode.h"
#include "glm.h"

RecordedPathNode::RecordedPathNode() {}

RecordedPathNode::~RecordedPathNode() {}

void RecordedPathNode::clearChildren()
{
    while (childCount() > 0) {
        QSGNode *child = firstChild();
        removeChildNode(child);
        delete child;
    }

    m_trailLineNode = nullptr;
    m_dubinsDotsNode = nullptr;
    m_lookaheadDotNode = nullptr;
}

void RecordedPathNode::update(const QMatrix4x4 &mv,
                              const QMatrix4x4 &p,
                              const QMatrix4x4 &ndc,
                              const QSize &viewportSize,
                              RecordedPathProperties *properties)
{
    const QMatrix4x4 mvp = ndc * p * mv;

    if (childCount() < 1) {
        // Phase 1: Build geometry from properties

        // Recorded trail line (yellow, thin line strip)
        const QVector<QVector3D> &trail = properties->recordedLine();
        if (trail.count() >= 2) {
            auto *geo = AOGGeometry::createLineStripGeometry(trail);
            if (geo) {
                m_trailLineNode = new QSGGeometryNode();
                m_trailLineNode->setGeometry(geo);
                m_trailLineNode->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new AOGFlatColorMaterial();
                m_trailLineNode->setMaterial(mat);
                m_trailLineNode->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(m_trailLineNode);
            }
        }

        // Dubins approach path (green dots, size 2)
        const QVector<QVector3D> &dubins = properties->dubinsPath();
        if (dubins.count() >= 1) {
            m_dubinsDotsNode = new DotsNode();
            for (const QVector3D &pt : dubins)
                m_dubinsDotsNode->addDot(pt, QColor::fromRgbF(0.298f, 0.96f, 0.296f), glm::dp(2.0f));
            m_dubinsDotsNode->build();
            appendChildNode(m_dubinsDotsNode);
        }

        // Lookahead point (magenta dot, size 16, only during playback)
        if (properties->showLookahead()) {
            m_lookaheadDotNode = new DotsNode();
            m_lookaheadDotNode->addDot(properties->lookaheadPoint(),
                                       QColor::fromRgbF(1.0f, 0.5f, 0.95f), glm::dp(16.0f));
            m_lookaheadDotNode->build();
            appendChildNode(m_lookaheadDotNode);
        }
    }

    // Phase 2: ALWAYS update MVP matrices for all child nodes

    if (m_trailLineNode) {
        auto *mat = static_cast<AOGFlatColorMaterial *>(m_trailLineNode->material());
        mat->setMvpMatrix(mvp);
        mat->setViewportSize(viewportSize);
        mat->setColor(QColor::fromRgbF(0.98f, 0.92f, 0.46f));
    }

    if (m_dubinsDotsNode)
        m_dubinsDotsNode->updateUniforms(mvp, viewportSize);

    if (m_lookaheadDotNode)
        m_lookaheadDotNode->updateUniforms(mvp, viewportSize);
}
