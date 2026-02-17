// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Scene graph-based field view renderer using native Qt scene graph nodes

#ifndef FIELDVIEWITEM_H
#define FIELDVIEWITEM_H

#include <QQuickItem>
#include <QMatrix4x4>
#include <QSGNode>
#include <QSGGeometryNode>
#include <QSGTransformNode>
#include <QtQml/qqmlregistration.h>
#include <QProperty>
#include <QBindable>
#include <QColor>

// Forward declarations for AOG classes
class CVehicle;
class CBoundary;
class CCamera;
class Backend;
class SettingsManager;

// Forward declarations for node classes
class FieldSurfaceNode;
class GridNode;
class BoundariesNode;
class VehicleNode;
class ToolsNode;
class TracksNode;
class LayersNode;

class CameraProperties;
class GridProperties;
class FieldSurfaceProperties;
class VehicleProperties;
class ToolsProperties;
class BoundariesProperties;
class TracksProperties;
class LayersProperties;
class TextureFactory;

Q_MOC_INCLUDE("cameraproperties.h")
Q_MOC_INCLUDE("gridproperties.h")
Q_MOC_INCLUDE("fieldsurfaceproperties.h")
Q_MOC_INCLUDE("vehicleproperties.h")
Q_MOC_INCLUDE("toolsproperties.h")
Q_MOC_INCLUDE("boundariesproperties.h")
Q_MOC_INCLUDE("tracksproperties.h")
Q_MOC_INCLUDE("layersproperties.h")

// ============================================================================
// FieldViewNode - Root node for the field view scene graph
// ============================================================================

class FieldViewNode : public QSGTransformNode
{
public:
    FieldViewNode();
    ~FieldViewNode();

    // Child nodes for different render layers (rendered back to front)
    FieldSurfaceNode *fieldSurfaceNode = nullptr;
    GridNode *gridNode = nullptr;
    TracksNode *tracksNode =nullptr;
    BoundariesNode *boundaryNode = nullptr;
    LayersNode *layersNode = nullptr;     // Coverage layers (triangles)
    QSGNode *guidanceNode = nullptr;     // Guidance lines (not yet refactored)
    VehicleNode *vehicleNode = nullptr;
    ToolsNode *toolsNode = nullptr;
    QSGNode *uiNode = nullptr;           // UI overlays (markers, flags)
};

// ============================================================================
// FieldViewItem - Main QQuickItem for field rendering
// ============================================================================

class FieldViewItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    // ===== Camera/View Properties - Grouped for QML access (camera.zoom, camera.x, etc.) =====
    Q_PROPERTY(CameraProperties* camera READ camera CONSTANT)
    Q_PROPERTY(GridProperties* grid READ grid CONSTANT)
    Q_PROPERTY(FieldSurfaceProperties* fieldSurface READ fieldSurface CONSTANT)
    Q_PROPERTY(VehicleProperties* vehicle READ vehicle WRITE setVehicle NOTIFY vehicleChanged)
    Q_PROPERTY(ToolsProperties* tools READ tools WRITE setTools NOTIFY toolsChanged)
    Q_PROPERTY(BoundariesProperties* boundaries READ boundaries WRITE setBoundaries NOTIFY boundariesChanged)
    Q_PROPERTY(TracksProperties* tracks READ tracks WRITE setTracks NOTIFY tracksChanged)
    Q_PROPERTY(LayersProperties* layers READ layers WRITE setLayers NOTIFY layersChanged)

    // ===== Rendering State Properties =====
    Q_PROPERTY(bool showCoverage READ showCoverage WRITE setShowCoverage NOTIFY showCoverageChanged BINDABLE bindableShowCoverage)
    Q_PROPERTY(bool showGuidance READ showGuidance WRITE setShowGuidance NOTIFY showGuidanceChanged BINDABLE bindableShowGuidance)

    // ===== Color Properties =====
    Q_PROPERTY(QColor guidanceColor READ guidanceColor WRITE setGuidanceColor NOTIFY guidanceColorChanged BINDABLE bindableGuidanceColor)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged BINDABLE bindableBackgroundColor)

public:
    explicit FieldViewItem(QQuickItem *parent = nullptr);
    ~FieldViewItem() override;

    // ===== Property Groups Accessors =====
    CameraProperties* camera() const;
    GridProperties * grid() const;
    FieldSurfaceProperties* fieldSurface() const;
    VehicleProperties* vehicle() const;
    void setVehicle(VehicleProperties *vehicle);
    ToolsProperties* tools() const;
    void setTools(ToolsProperties *tools);
    BoundariesProperties* boundaries() const;
    void setBoundaries(BoundariesProperties *boundaries);
    TracksProperties* tracks() const;
    void setTracks(TracksProperties *tracks);
    LayersProperties* layers() const;
    void setLayers(LayersProperties *layers);


    // ===== Visibility Property Accessors =====
    bool showCoverage() const;
    void setShowCoverage(bool value);
    QBindable<bool> bindableShowCoverage();

    bool showGuidance() const;
    void setShowGuidance(bool value);
    QBindable<bool> bindableShowGuidance();

    // ===== Color Property Accessors =====
    QColor guidanceColor() const;
    void setGuidanceColor(const QColor &color);
    QBindable<QColor> bindableGuidanceColor();

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor &color);
    QBindable<QColor> bindableBackgroundColor();

    // ===== Public Methods =====
    Q_INVOKABLE void requestUpdate();  // Call this instead of update() to also sync singleton data
    Q_INVOKABLE void updateVehicle();
    Q_INVOKABLE void updateTools();
    Q_INVOKABLE void updateGrid();
    Q_INVOKABLE void updateBoundary();
    Q_INVOKABLE void markCoverageDirty();
    Q_INVOKABLE void markGuidanceDirty();
    Q_INVOKABLE void markLayersDirty();
    Q_INVOKABLE void markAllDirty();

signals:
    // Property group signals
    void vehicleChanged();
    void toolsChanged();
    void boundariesChanged();
    void tracksChanged();
    void layersChanged();

    // Visibility signals
    void showCoverageChanged();
    void showGuidanceChanged();

    // Color signals
    void boundaryColorChanged();
    void guidanceColorChanged();
    void backgroundColorChanged();

protected:
    // ===== Core Scene Graph Methods =====
    void updatePolish() override;  // Runs on GUI thread before rendering
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void releaseResources() override;  // Called while GL context still valid

private:
    // ===== Node Update Methods (for nodes not yet refactored) =====
    void updateCoverageNode(FieldViewNode *rootNode);
    void updateGuidanceNode(FieldViewNode *rootNode);

    // ===== Matrix Building =====
    QMatrix4x4 buildNdcMatrix() const;
    QMatrix4x4 buildProjectionMatrix() const;
    QMatrix4x4 buildViewMatrix() const;

    // ===== Dirty Tracking =====
    bool m_fieldSurfaceDirty = true;
    bool m_vehicleDirty = true;
    bool m_toolsDirty = true;
    bool m_boundaryDirty = true;
    bool m_coverageDirty = true;
    bool m_guidanceDirty = true;
    bool m_gridDirty = true;
    bool m_tracksDirty = true;
    bool m_layersDirty = true;

    // ===== Current MVP Matrix (set each frame for materials) =====
    QMatrix4x4 m_currentMv;
    QMatrix4x4 m_currentP;
    QMatrix4x4 m_currentNcd;

    // ===== Texture Factory =====
    TextureFactory *m_textureFactory = nullptr;

    // ===== Singleton Access (cached for thread safety) =====
    // These are populated in updatePaintNode from the main thread
    struct RenderData {
        // Vehicle position
        double vehicleX = 0;
        double vehicleY = 0;
        double vehicleHeading = 0;
        double steerAngle = 0;
        bool isOutOfBounds = false;
        int lineWidth = 1;

        // Boundary data
        QVector<QVector<QVector3D>> boundaries;

        // Coverage data
        QVector<QVector3D> coverageVertices;
        QVector<QColor> coverageColors;

        // Guidance data
        QVector<QVector3D> guidanceLine;
        bool hasGuidance = false;
    };
    RenderData m_renderData;

    // ===== Grouped Property Objects =====
    CameraProperties *m_camera = nullptr;
    GridProperties *m_grid = nullptr;
    FieldSurfaceProperties *m_fieldSurface = nullptr;
    VehicleProperties *m_vehicle = nullptr;
    ToolsProperties *m_tools = nullptr;
    BoundariesProperties *m_boundaries = nullptr;
    TracksProperties *m_tracks = nullptr;
    LayersProperties *m_layers = nullptr;

    // ===== Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY Members =====
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FieldViewItem, bool, m_showCoverage, true, &FieldViewItem::showCoverageChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FieldViewItem, bool, m_showGuidance, true, &FieldViewItem::showGuidanceChanged)

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FieldViewItem, QColor, m_guidanceColor, QColor(0, 255, 0), &FieldViewItem::guidanceColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FieldViewItem, QColor, m_backgroundColor, QColor(69, 102, 179), &FieldViewItem::backgroundColorChanged)  // Day sky blue
};

#endif // FIELDVIEWITEM_H
