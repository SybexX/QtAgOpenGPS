// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Layers node - scene graph node for rendering coverage triangles
// Uses immutable patch-based geometry for efficient append-only updates

#ifndef LAYERSNODE_H
#define LAYERSNODE_H

#include <QSGNode>
#include <QSGGeometryNode>
#include <QMatrix4x4>
#include <QHash>
#include <QVector>
#include <QSize>

#include "layertypes.h"

class LayersProperties;

// Number of triangles per patch - adjust if needed for performance tuning
// Larger = fewer draw calls, more memory waste for partial patches
// Smaller = more draw calls, less memory waste
#define COVERAGE_PATCH_SIZE 4096

// ============================================================================
// LayersNode - Renders coverage triangles using immutable patch nodes
// ============================================================================
//
// Optimized for the coverage use case where triangles are only added, never
// modified or removed. Each layer maintains:
// - A list of "filled" patch nodes (immutable, never updated after creation)
// - One "current" patch node that receives new triangles
// - When current patch fills up, it becomes immutable and a new one is created
//
// This gives O(1) amortized cost for adding triangles - only the current
// patch ever gets geometry updates.
//
// Uses AOGVertexColorMaterial for per-vertex colors (flat shading achieved
// by giving all 3 vertices of each triangle the same color).

class LayersNode : public QSGNode
{
public:
    LayersNode();
    ~LayersNode() override;

    // Update the node with current layer data
    // Only the current patch gets geometry updates
    void update(const QMatrix4x4 &mv,
                const QMatrix4x4 &p,
                const QMatrix4x4 &ndc,
                const QSize &viewportSize,
                const LayersProperties *properties);

    // Clear all child nodes and reset state
    // Call this when layer data is invalidated (e.g., field closed)
    void clearChildren();

private:
    // Per-layer patch tracking
    struct LayerPatches {
        QVector<QSGGeometryNode*> filledPatches;  // Immutable patches
        QSGGeometryNode* currentPatch = nullptr;   // Active patch receiving triangles
        int currentPatchTriangleCount = 0;         // Triangles in current patch
        int totalBuiltTriangles = 0;               // Total triangles across all patches
    };

    QHash<int, LayerPatches> m_layerPatches;

    // Create a new patch geometry node
    QSGGeometryNode* createPatchNode();

    // Process new triangles for a layer, creating patches as needed
    // Returns true if any geometry was modified
    bool processNewTriangles(LayerPatches &lp, const CoverageLayer &layer);

    // Fill vertex data for triangles into a patch
    void fillPatchData(QSGGeometryNode *patch, const QVector<CoverageTriangle> &triangles,
                       int startIdx, int count, float layerAlpha);

    // Update MVP matrix on all patches for a layer
    void updateLayerMvp(LayerPatches &lp, const QMatrix4x4 &mvp, const QSize &viewportSize);

    // Update MVP matrix on a single geometry node
    void updateNodeMvp(QSGGeometryNode *node, const QMatrix4x4 &mvp, const QSize &viewportSize);

    // Remove layers that no longer exist in properties
    void removeOrphanedLayers(const QHash<int, CoverageLayer> &activeLayers);
};

#endif // LAYERSNODE_H
