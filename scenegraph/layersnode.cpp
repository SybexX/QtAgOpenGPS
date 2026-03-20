// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Layers node implementation - immutable patch-based geometry

#include "layersnode.h"
#include "layersproperties.h"
#include "materials.h"
#include "aoggeometry.h"

#include <QSGGeometry>

LayersNode::LayersNode()
{
}

LayersNode::~LayersNode()
{
    // Child nodes are automatically deleted by QSGNode destructor
}

void LayersNode::clearChildren()
{
    // Remove and delete all child nodes
    while (childCount() > 0) {
        QSGNode *child = firstChild();
        removeChildNode(child);
        delete child;
    }

    m_layerPatches.clear();
}

void LayersNode::update(const QMatrix4x4 &mv,
                        const QMatrix4x4 &p,
                        const QMatrix4x4 &ndc,
                        const QSize &viewportSize,
                        const LayersProperties *properties)
{
    if (!properties || !properties->visible()) {
        return;
    }

    const QHash<int, CoverageLayer> &layers = properties->layers();

    // Remove layers that no longer exist
    removeOrphanedLayers(layers);

    QMatrix4x4 mvp = ndc * p * mv;

    // Process each layer
    for (auto it = layers.constBegin(); it != layers.constEnd(); ++it) {
        int layerId = it.key();
        const CoverageLayer &layer = it.value();

        // Skip invisible layers
        if (!layer.visible) {
            continue;
        }

        // Get or create layer patch tracking
        LayerPatches &lp = m_layerPatches[layerId];

        // Process any new triangles
        bool geometryChanged = processNewTriangles(lp, layer);

        // Update MVP matrix on all patches (filled + current)
        updateLayerMvp(lp, mvp, viewportSize);

        // Mark current patch dirty if geometry changed
        if (geometryChanged && lp.currentPatch) {
            lp.currentPatch->markDirty(QSGNode::DirtyGeometry);
        }
    }
}

QSGGeometryNode* LayersNode::createPatchNode()
{
    // Start with minimal allocation - will be resized as needed
    QSGGeometry *geometry = new QSGGeometry(AOGGeometry::colorVertexAttributes(), 0);
    geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(1.0f);
    geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);

    QSGGeometryNode *node = new QSGGeometryNode();
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);

    // Create material with per-vertex colors
    auto *material = new AOGVertexColorMaterial();
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);

    return node;
}

bool LayersNode::processNewTriangles(LayerPatches &lp, const CoverageLayer &layer)
{
    int totalTriangles = layer.triangles.count();
    int builtTriangles = lp.totalBuiltTriangles;

    if (totalTriangles <= builtTriangles) {
        // No new triangles
        return false;
    }

    // Calculate where the current patch starts in the source triangles
    int filledPatchCount = lp.filledPatches.count();
    int currentPatchStartIdx = filledPatchCount * COVERAGE_PATCH_SIZE;

    bool geometryChanged = false;

    while (lp.totalBuiltTriangles < totalTriangles) {
        // Ensure we have a current patch
        if (!lp.currentPatch) {
            lp.currentPatch = createPatchNode();
            lp.currentPatchTriangleCount = 0;
            appendChildNode(lp.currentPatch);
        }

        // Calculate how many triangles should be in current patch
        int trianglesAvailable = totalTriangles - currentPatchStartIdx;
        int trianglesForPatch = qMin(trianglesAvailable, COVERAGE_PATCH_SIZE);

        // Rebuild current patch with all its triangles
        fillPatchData(lp.currentPatch, layer.triangles, currentPatchStartIdx,
                      trianglesForPatch, layer.alpha);

        lp.currentPatchTriangleCount = trianglesForPatch;
        lp.totalBuiltTriangles = currentPatchStartIdx + trianglesForPatch;
        geometryChanged = true;

        // If current patch is full, finalize it
        if (lp.currentPatchTriangleCount >= COVERAGE_PATCH_SIZE) {
            lp.currentPatch->geometry()->setVertexDataPattern(QSGGeometry::StaticPattern);
            lp.filledPatches.append(lp.currentPatch);
            lp.currentPatch = nullptr;
            lp.currentPatchTriangleCount = 0;
            currentPatchStartIdx += COVERAGE_PATCH_SIZE;
        }
    }

    return geometryChanged;
}

void LayersNode::fillPatchData(QSGGeometryNode *patch, const QVector<CoverageTriangle> &triangles,
                                int startIdx, int count, float layerAlpha)
{
    if (count <= 0) {
        return;
    }

    QSGGeometry *geometry = patch->geometry();

    // Allocate 6 vertices per triangle (3 edges x 2 vertices each) for wireframe
    geometry->allocate(count * 6);

    ColorVertex *data = static_cast<ColorVertex*>(geometry->vertexData());

    for (int i = 0; i < count; ++i) {
        const CoverageTriangle &tri = triangles[startIdx + i];

        float a = tri.color.alphaF() * layerAlpha;
        float r = tri.color.redF() * a;
        float g = tri.color.greenF() * a;
        float b = tri.color.blueF() * a;

        // Edge 1: v0 -> v1
        data->x = tri.v0.x(); data->y = tri.v0.y(); data->z = tri.v0.z();
        data->r = r; data->g = g; data->b = b; data->a = a; data++;

        data->x = tri.v1.x(); data->y = tri.v1.y(); data->z = tri.v1.z();
        data->r = r; data->g = g; data->b = b; data->a = a; data++;

        // Edge 2: v1 -> v2
        data->x = tri.v1.x(); data->y = tri.v1.y(); data->z = tri.v1.z();
        data->r = r; data->g = g; data->b = b; data->a = a; data++;

        data->x = tri.v2.x(); data->y = tri.v2.y(); data->z = tri.v2.z();
        data->r = r; data->g = g; data->b = b; data->a = a; data++;

        // Edge 3: v2 -> v0
        data->x = tri.v2.x(); data->y = tri.v2.y(); data->z = tri.v2.z();
        data->r = r; data->g = g; data->b = b; data->a = a; data++;

        data->x = tri.v0.x(); data->y = tri.v0.y(); data->z = tri.v0.z();
        data->r = r; data->g = g; data->b = b; data->a = a; data++;
    }
}

void LayersNode::updateLayerMvp(LayerPatches &lp, const QMatrix4x4 &mvp, const QSize &viewportSize)
{
    // Update all filled patches
    for (QSGGeometryNode *patch : std::as_const(lp.filledPatches)) {
        updateNodeMvp(patch, mvp, viewportSize);
    }

    // Update current patch if it exists
    if (lp.currentPatch) {
        updateNodeMvp(lp.currentPatch, mvp, viewportSize);
    }
}

void LayersNode::updateNodeMvp(QSGGeometryNode *node, const QMatrix4x4 &mvp, const QSize &viewportSize)
{
    auto *material = static_cast<AOGVertexColorMaterial*>(node->material());
    if (material) {
        material->setMvpMatrix(mvp);
        material->setViewportSize(viewportSize);
    }
    node->markDirty(QSGNode::DirtyMaterial);
}

void LayersNode::removeOrphanedLayers(const QHash<int, CoverageLayer> &activeLayers)
{
    // Find layers that exist in our patch map but not in active layers
    QList<int> toRemove;
    for (auto it = m_layerPatches.constBegin(); it != m_layerPatches.constEnd(); ++it) {
        if (!activeLayers.contains(it.key())) {
            toRemove.append(it.key());
        }
    }

    // Remove orphaned layers
    for (int layerId : toRemove) {
        LayerPatches lp = m_layerPatches.take(layerId);

        // Remove and delete all patch nodes for this layer
        for (QSGGeometryNode *patch : std::as_const(lp.filledPatches)) {
            removeChildNode(patch);
            delete patch;
        }
        if (lp.currentPatch) {
            removeChildNode(lp.currentPatch);
            delete lp.currentPatch;
        }
    }
}
