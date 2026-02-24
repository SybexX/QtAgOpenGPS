// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Scene graph-based field view renderer implementation

#include "fieldviewitem.h"
#include "cameraproperties.h"
#include "gridproperties.h"
#include "fieldsurfaceproperties.h"
#include "vehicleproperties.h"
#include "toolsproperties.h"
#include "boundariesproperties.h"
#include "layersproperties.h"
#include "recordedpathproperties.h"

#include "fieldsurfacenode.h"
#include "gridnode.h"
#include "boundariesnode.h"
#include "vehiclenode.h"
#include "toolsnode.h"
#include "tracksnode.h"
#include "recordedpathnode.h"
#include "layersnode.h"

#include "aogmaterial.h"
#include "aoggeometry.h"
#include "texturefactory.h"

#include "glm.h"

#include "cvehicle.h"
#include "cboundary.h"
#include "settingsmanager.h"
#include "boundaryinterface.h"

#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include <QSGTextureMaterial>
#include <QSGOpaqueTextureMaterial>
#include <QQuickWindow>
#include <QSize>
#include <QtMath>
#include <QDebug>
#include <QLoggingCategory>

// Logging category for this file - must end in ".qtagopengps" to not be suppressed
Q_LOGGING_CATEGORY(fieldviewitem_log, "fieldviewitem.qtagopengps")

// ============================================================================
// FieldViewNode Implementation
// ============================================================================

FieldViewNode::FieldViewNode()
{
    // Create child nodes (typed for refactored ones, generic for others)
    fieldSurfaceNode = new FieldSurfaceNode();
    gridNode = new GridNode();
    boundaryNode = new BoundariesNode();
    layersNode = new LayersNode();    // Coverage layers
    guidanceNode = new QSGNode();     // Not yet refactored
    vehicleNode = new VehicleNode();
    toolsNode = new ToolsNode();
    uiNode = new QSGNode();
    tracksNode = new TracksNode();
    recordedPathNode = new RecordedPathNode();

    // Add children in render order (back to front)
    appendChildNode(fieldSurfaceNode);  // Field surface first (furthest back)
    appendChildNode(gridNode);          // Grid lines
    appendChildNode(layersNode);        // Coverage layers (triangles)
    appendChildNode(boundaryNode);
    appendChildNode(tracksNode);
    appendChildNode(recordedPathNode);
    appendChildNode(guidanceNode);
    appendChildNode(toolsNode);
    appendChildNode(vehicleNode);
    appendChildNode(uiNode);
}

FieldViewNode::~FieldViewNode()
{
    // Child nodes are automatically deleted by QSGNode destructor
}

// ============================================================================
// FieldViewItem Implementation
// ============================================================================

FieldViewItem::FieldViewItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setClip(true);  // Clip rendering to item bounds

    // Create camera settings object (owned by this)
    m_camera = new CameraProperties(this);
    m_grid = new GridProperties(this);
    m_fieldSurface = new FieldSurfaceProperties(this);
    m_vehicle = new VehicleProperties(this);
    m_tools = new ToolsProperties(this);
    m_boundaries = new BoundariesProperties(this);
    m_tracks = new TracksProperties(this);
    m_recordedPath = new RecordedPathProperties(this);
    // m_layers is null by default - set via QML: layers: LayerService.layers

    // Connect camera property changes to update()
    connect(m_camera, &CameraProperties::zoomChanged, this, &FieldViewItem::requestUpdate);
    connect(m_camera, &CameraProperties::xChanged, this, &FieldViewItem::requestUpdate);
    connect(m_camera, &CameraProperties::yChanged, this, &FieldViewItem::requestUpdate);
    connect(m_camera, &CameraProperties::rotationChanged, this, &FieldViewItem::requestUpdate);
    connect(m_camera, &CameraProperties::pitchChanged, this, &FieldViewItem::requestUpdate);
    connect(m_camera, &CameraProperties::fovChanged, this, &FieldViewItem::requestUpdate);

    // Track geometry depends on the view matrix (near-plane clipping and subdivision
    // produce view-dependent vertices), so rebuild whenever zoom, pitch, or rotation
    // changes.  requestUpdate() is already wired above; we just set the dirty flag.
    connect(m_camera, &CameraProperties::zoomChanged,    [this]() { m_tracksDirty = true; });
    connect(m_camera, &CameraProperties::pitchChanged,   [this]() { m_tracksDirty = true; });
    connect(m_camera, &CameraProperties::rotationChanged,[this]() { m_tracksDirty = true; });
    connect(m_camera, &CameraProperties::zoomChanged, [&]() {
        if (m_vehicle->svennArrow() && m_camera->zoom() > -1000 ||
            m_vehicle->firstHeadingSet() && m_camera->zoom() > -75) {

            updateVehicle();
        }
        updateTools();
    });

    //Connect grid property changes to update()
    connect(m_grid, &GridProperties::sizeChanged, this, &FieldViewItem::updateGrid);
    connect(m_grid, &GridProperties::colorChanged, this, &FieldViewItem::updateGrid);
    connect(m_grid, &GridProperties::visibleChanged, this, &FieldViewItem::updateGrid);

    //Connect field surface property changes to update()
    connect(m_fieldSurface, &FieldSurfaceProperties::visibleChanged, this, &FieldViewItem::requestUpdate);
    connect(m_fieldSurface, &FieldSurfaceProperties::colorChanged, this, &FieldViewItem::requestUpdate);
    connect(m_fieldSurface, &FieldSurfaceProperties::showTextureChanged, this, &FieldViewItem::requestUpdate);

    connect(m_vehicle, &VehicleProperties::visibleChanged, this, &FieldViewItem::requestUpdate);
    connect(m_vehicle, &VehicleProperties::colorChanged, this, &FieldViewItem::requestUpdate);
    connect(m_vehicle, &VehicleProperties::steerAngleChanged, this, &FieldViewItem::requestUpdate);
    // redraw vehicle geometry if any of these properties change:
    connect(m_vehicle, &VehicleProperties::vehicleChanged, this, &FieldViewItem::updateVehicle);

    connect(m_tools, &ToolsProperties::visibleChanged, this, &FieldViewItem::requestUpdate);
    connect(m_tools, &ToolsProperties::toolsChanged, this, &FieldViewItem::updateTools);

    // Connect other property changes to update()
    connect(this, &FieldViewItem::boundaryColorChanged, this, &FieldViewItem::requestUpdate);
    connect(this, &FieldViewItem::guidanceColorChanged, this, &FieldViewItem::requestUpdate);
    connect(this, &FieldViewItem::backgroundColorChanged, this, &FieldViewItem::requestUpdate);

    connect(SettingsManager::instance(), &SettingsManager::display_lineWidthChanged, [this]() {
        m_renderData.lineWidth = SettingsManager::instance()->display_lineWidth();
        requestUpdate();
    });
    m_renderData.lineWidth = SettingsManager::instance()->display_lineWidth();

    connect(BoundaryInterface::instance(), &BoundaryInterface::isOutOfBoundsChanged, [this]() {
        m_renderData.isOutOfBounds = BoundaryInterface::instance()->isOutOfBounds();
        requestUpdate();
    });
    m_renderData.isOutOfBounds = BoundaryInterface::instance()->isOutOfBounds();

    connect(m_tracks, &TracksProperties::tracksPropertiesChanged, [this]() {
        m_tracksDirty = true;
        requestUpdate();
    });

    connect(m_recordedPath, &RecordedPathProperties::recordedPathPropertiesChanged, [this]() {
        m_recordedPathDirty = true;
        requestUpdate();
    });

    // Schedule initial polish to sync singleton data before first render
    polish();
}

FieldViewItem::~FieldViewItem()
{
    // Note: m_textureFactory is cleaned up in releaseResources() while GL context is valid
}

void FieldViewItem::releaseResources()
{
    // Called while GL/Vulkan context is still valid - safe to delete textures here
    delete m_textureFactory;
    m_textureFactory = nullptr;
}

// ============================================================================
// Property Groups Accessors
// ============================================================================

CameraProperties* FieldViewItem::camera() const { return m_camera; }

GridProperties* FieldViewItem::grid() const { return m_grid; }

FieldSurfaceProperties* FieldViewItem::fieldSurface() const { return m_fieldSurface; }

VehicleProperties* FieldViewItem::vehicle() const { return m_vehicle; }

void FieldViewItem::setVehicle(VehicleProperties *vehicle)
{
    if (m_vehicle == vehicle)
        return;

    // Disconnect all signals from old vehicle to us
    if (m_vehicle) {
        disconnect(m_vehicle, nullptr, this, nullptr);

        // Delete if we own it (created with us as parent)
        if (m_vehicle->parent() == this)
            delete m_vehicle;
    }

    m_vehicle = vehicle;

    // Connect signals from new vehicle
    if (m_vehicle) {
        connect(m_vehicle, &VehicleProperties::visibleChanged, this, &FieldViewItem::requestUpdate);
        connect(m_vehicle, &VehicleProperties::colorChanged, this, &FieldViewItem::requestUpdate);
        connect(m_vehicle, &VehicleProperties::steerAngleChanged, this, &FieldViewItem::requestUpdate);
        connect(m_vehicle, &VehicleProperties::vehicleChanged, this, &FieldViewItem::updateVehicle);
    }

    m_vehicleDirty = true;
    emit vehicleChanged();
    requestUpdate();
}

ToolsProperties* FieldViewItem::tools() const { return m_tools; }

BoundariesProperties* FieldViewItem::boundaries() const { return m_boundaries; }

TracksProperties *FieldViewItem::tracks() const { return m_tracks; }

RecordedPathProperties* FieldViewItem::recordedPath() const { return m_recordedPath; }

LayersProperties* FieldViewItem::layers() const { return m_layers; }

void FieldViewItem::setBoundaries(BoundariesProperties *boundaries)
{
    if (m_boundaries == boundaries)
        return;

    // Disconnect all signals from old boundaries to us
    if (m_boundaries) {
        disconnect(m_boundaries, nullptr, this, nullptr);

        // Delete if we own it (created with us as parent)
        if (m_boundaries->parent() == this)
            delete m_boundaries;
    }

    m_boundaries = boundaries;

    // Connect signals from new boundaries
    if (m_boundaries) {
        connect(m_boundaries, &BoundariesProperties::outerChanged, this, &FieldViewItem::updateBoundary);
        connect(m_boundaries, &BoundariesProperties::innerChanged, this, &FieldViewItem::updateBoundary);
        connect(m_boundaries, &BoundariesProperties::hdLineChanged, this, &FieldViewItem::updateBoundary);
    }

    m_boundaryDirty = true;
    emit boundariesChanged();
    requestUpdate();
}

void FieldViewItem::setTools(ToolsProperties *tools)
{
    if (m_tools == tools)
        return;

    // Disconnect all signals from old tools to us
    if (m_tools) {
        disconnect(m_tools, nullptr, this, nullptr);

        // Delete if we own it (created with us as parent)
        if (m_tools->parent() == this)
            delete m_tools;
    }

    m_tools = tools;

    // Connect signals from new tools
    if (m_tools) {
        connect(m_tools, &ToolsProperties::visibleChanged, this, &FieldViewItem::requestUpdate);
        connect(m_tools, &ToolsProperties::toolsChanged, this, &FieldViewItem::updateTools);
    }

    m_toolsDirty = true;
    emit toolsChanged();
    requestUpdate();
}

void FieldViewItem::setTracks(TracksProperties *tracks)
{
    if (m_tracks == tracks)
        return;

    if (m_tracks) {
        disconnect(m_tracks, nullptr, this, nullptr);

        if (m_tracks->parent() == this)
            delete m_tracks;
    }

    m_tracks = tracks;
    if (m_tracks) {
        connect (m_tracks, &TracksProperties::tracksPropertiesChanged, [this]() {
            m_tracksDirty = true;
        });
    }
}

void FieldViewItem::setLayers(LayersProperties *layers)
{
    if (m_layers == layers)
        return;

    // Disconnect all signals from old layers to us
    if (m_layers) {
        disconnect(m_layers, nullptr, this, nullptr);

        // Delete if we own it (created with us as parent)
        if (m_layers->parent() == this)
            delete m_layers;
    }

    m_layers = layers;

    // Connect signals from new layers
    if (m_layers) {
        connect(m_layers, &LayersProperties::trianglesChanged, this, &FieldViewItem::markLayersDirty);
        connect(m_layers, &LayersProperties::layerAdded, this, &FieldViewItem::markLayersDirty);
        connect(m_layers, &LayersProperties::layerRemoved, this, &FieldViewItem::markLayersDirty);
        connect(m_layers, &LayersProperties::layerVisibilityChanged, this, &FieldViewItem::requestUpdate);
        connect(m_layers, &LayersProperties::layerAlphaChanged, this, &FieldViewItem::markLayersDirty);
    }

    m_layersDirty = true;
    emit layersChanged();
    requestUpdate();
}

void FieldViewItem::setRecordedPath(RecordedPathProperties *recordedPath)
{
    if (m_recordedPath == recordedPath)
        return;

    //Disconnect all signals from old layers to this
    if (m_recordedPath) {
        disconnect(m_recordedPath, nullptr, this, nullptr);

        //Delete if we own it (created with us as parent)
        if (m_recordedPath->parent() == this)
            delete m_recordedPath;
    }

    m_recordedPath = recordedPath;

    //connect signals from new recordedPath
    if (m_recordedPath) {
        connect(m_recordedPath, &RecordedPathProperties::recordedPathPropertiesChanged, this, &FieldViewItem::markRecordedPathDirty);
    }

    m_recordedPathDirty = true;
    emit recordedPathChanged();
    requestUpdate();
}

// ============================================================================
// Visibility Property Accessors
// ============================================================================

bool FieldViewItem::showCoverage() const { return m_showCoverage; }
void FieldViewItem::setShowCoverage(bool value) { m_showCoverage = value; }
QBindable<bool> FieldViewItem::bindableShowCoverage() { return &m_showCoverage; }

bool FieldViewItem::showGuidance() const { return m_showGuidance; }
void FieldViewItem::setShowGuidance(bool value) { m_showGuidance = value; }
QBindable<bool> FieldViewItem::bindableShowGuidance() { return &m_showGuidance; }

// ============================================================================
// Color Property Accessors
// ============================================================================

QColor FieldViewItem::guidanceColor() const { return m_guidanceColor; }
void FieldViewItem::setGuidanceColor(const QColor &color) { m_guidanceColor = color; }
QBindable<QColor> FieldViewItem::bindableGuidanceColor() { return &m_guidanceColor; }

QColor FieldViewItem::backgroundColor() const { return m_backgroundColor; }
void FieldViewItem::setBackgroundColor(const QColor &color) { m_backgroundColor = color; }
QBindable<QColor> FieldViewItem::bindableBackgroundColor() { return &m_backgroundColor; }

// ============================================================================
// Public Methods
// ============================================================================

void FieldViewItem::requestUpdate()
{
    polish();
    update();
}

void FieldViewItem::updateVehicle()
{
    m_vehicleDirty = true;
    polish();
    update();
}

void FieldViewItem::updateTools()
{
    m_toolsDirty = true;
    polish();
    update();
}

void FieldViewItem::updateGrid()
{
    m_gridDirty = true;
    polish();
    update();
}

void FieldViewItem::updateBoundary()
{
    m_boundaryDirty = true;
    polish();
    update();
}

void FieldViewItem::markCoverageDirty()
{
    m_coverageDirty = true;
    update();
}

void FieldViewItem::markGuidanceDirty()
{
    m_guidanceDirty = true;
    update();
}

void FieldViewItem::markLayersDirty()
{
    m_layersDirty = true;
    update();
}

void FieldViewItem::markRecordedPathDirty()
{
    m_recordedPathDirty = true;
    update();
}

void FieldViewItem::markAllDirty()
{
    m_boundaryDirty = true;
    m_coverageDirty = true;
    m_guidanceDirty = true;
    m_gridDirty = true;
    m_tracksDirty = true;
    m_recordedPathDirty = true;
    m_layersDirty = true;
    m_recordedPathDirty = true;
    update();
}

// ============================================================================
// Polish (GUI Thread) - Sync Data Before Rendering
// ============================================================================

void FieldViewItem::updatePolish()
{
    // This runs on the GUI thread before updatePaintNode() (render thread)
    // Safe to access singletons and GUI-thread-only data here
    // Copy data into m_renderData for thread-safe access in updatePaintNode()

    // Get vehicle data
    if (auto *vehicle = CVehicle::instance()) {
        m_renderData.vehicleX = vehicle->pivotAxlePos.easting;
        m_renderData.vehicleY = vehicle->pivotAxlePos.northing;
        m_renderData.vehicleHeading = vehicle->fixHeading();
        //qDebug(fieldviewitem_log) << glm::toDegrees(vehicle->fixHeading()) << m_renderData.vehicleX << m_renderData.vehicleY;

        // Note: steerAngle would need to come from another source
    }

    // Get boundary data if dirty
    if (m_boundaryDirty) {
        m_renderData.boundaries.clear();
        // Note: CBoundary needs to be accessed via FormGPS currently
        // This will be refactored when CBoundary becomes a singleton
        // For now, boundaries are updated via explicit calls
    }

    // Coverage and guidance would be synced similarly
}

// ============================================================================
// Core Scene Graph Method
// ============================================================================

QSGNode *FieldViewItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    //qCDebug(fieldviewitem_log) << "FieldViewItem::updatePaintNode called, oldNode:" << oldNode;

    // Data from singletons was already synced in updatePolish() (GUI thread)
    // m_renderData is now safe to use here on the render thread

    QSize viewportSize(static_cast<int>(width()), static_cast<int>(height()));

    // Get or create root node
    FieldViewNode *rootNode = static_cast<FieldViewNode *>(oldNode);
    if (!rootNode) {
        rootNode = new FieldViewNode();
        //qCDebug(fieldviewitem_log) << "FieldViewItem: Created new FieldViewNode";
    }

    // Build the MVP matrix once for all children
    m_currentMv = buildViewMatrix();
    m_currentP = buildProjectionMatrix();
    m_currentNcd = buildNdcMatrix();

    // Note: We don't set the matrix on the transform node because QSGTransformNode
    // is for 2D scene graph transforms, not 3D MVP matrices. Instead, we pass
    // the MVP matrix to each material's shader.

    // Initialize texture factory if needed (requires window())
    if (!m_textureFactory && window()) {
        m_textureFactory = new TextureFactory(window());
    }

    // Get WorldGrid data for field surface and grid
    //adjust zoom based on cam distance
    double _gridZoom = abs(m_camera->zoom());
    int count;

    if (_gridZoom> 100) count = 4;
    else if (_gridZoom> 80) count = 8;
    else if (_gridZoom> 50) count = 16;
    else if (_gridZoom> 20) count = 32;
    else if (_gridZoom> 10) count = 64;
    else count = 80;

    double n = glm::roundMidAwayFromZero(camera()->y() / (m_grid->size() / count * 2)) * (m_grid->size() / count * 2);
    double e = glm::roundMidAwayFromZero(camera()->x() / (m_grid->size() / count * 2)) * (m_grid->size() / count * 2);
    double eastingMin = e - m_grid->size();
    double eastingMax = e + m_grid->size();
    double northingMin = n - m_grid->size();
    double northingMax = n + m_grid->size();

    QMatrix4x4 currentMvp = m_currentNcd * m_currentP * m_currentMv;

    if (m_fieldSurface->visible()) {
        // Get floor texture from factory (created on demand)
        QSGTexture *floorTexture = nullptr;
        if (m_fieldSurface->showTexture() && m_textureFactory) {
            floorTexture = m_textureFactory->texture(TextureId::Floor);
        }

        // Always update field surface (it changes with camera position)
        rootNode->fieldSurfaceNode->update(
            currentMvp,
            m_fieldSurface->color(),
            m_fieldSurface->showTexture(),
            floorTexture,
            eastingMin, eastingMax,
            northingMin, northingMax,
            count
        );
    }

    double camDistance = abs(m_camera->zoom());

    // Update grid if visible
    if (m_grid->visible()) {
        rootNode->gridNode->update(
            m_currentMv,
            m_currentP,
            m_currentNcd,
            viewportSize,
            m_renderData.vehicleX,
            m_renderData.vehicleY,
            m_camera->zoom(),
            m_grid->size(),
            m_grid->color(),
            m_grid->thickness()
        );
        m_gridDirty = false;
    }


    float textSize;
    if (m_camera->pitch() < -45) {
        textSize = pow(camDistance, 0.8);
        textSize /= 300;
    } else {
        textSize = pow(camDistance, 0.85);
        textSize /=500;
    }

    if (m_tracksDirty) {
        m_tracksDirty = false;
        rootNode->tracksNode->clearChildren();
    }

    rootNode->tracksNode->update(m_currentMv,
                                 m_currentP,
                                 m_currentNcd,
                                 viewportSize,
                                 m_renderData.vehicleX,
                                 m_renderData.vehicleY,
                                 m_renderData.vehicleHeading,
                                 m_renderData.isOutOfBounds,
                                 m_renderData.lineWidth,
                                 textSize,
                                 m_textureFactory,
                                 m_tracks);

    // Update recorded path trail and Dubins approach
    if (m_recordedPathDirty) {
        m_recordedPathDirty = false;
        rootNode->recordedPathNode->clearChildren();
    }

    rootNode->recordedPathNode->update(m_currentMv,
                                       m_currentP,
                                       m_currentNcd,
                                       viewportSize,
                                       m_recordedPath);

    // Update boundary if visible and dirty
    if (m_boundaryDirty) {
        m_boundaryDirty = false;
        rootNode->boundaryNode->clearChildren();
    }

    //rebuild geometry if needed, otherwise update the uniforms
    //always try to show boundaries because individual lines
    //have their own visible state, and also if the lists are
    //empty, no geometry is created.
    rootNode->boundaryNode->update(
        m_currentMv,
        m_currentP,
        m_currentNcd,
        viewportSize,
        m_renderData.vehicleX,
        m_renderData.vehicleY,
        m_renderData.vehicleHeading,
        m_renderData.isOutOfBounds,
        m_renderData.lineWidth,
        m_boundaries
    );

    // Update coverage layers
    if (m_layersDirty) {
        m_layersDirty = false;
        rootNode->layersNode->clearChildren();
    }

    // Always update layers (updates MVP matrix and rebuilds geometry if needed)
    if (m_layers && m_layers->visible()) {
        rootNode->layersNode->update(
            m_currentMv,
            m_currentP,
            m_currentNcd,
            viewportSize,
            m_layers
        );
    }

    // Update coverage and guidance (not yet refactored)
    if (m_showCoverage && m_coverageDirty) {
        //updateCoverageNode(rootNode);
        m_coverageDirty = false;
    }

    if (m_showGuidance && m_guidanceDirty) {
        //updateGuidanceNode(rootNode);
        m_guidanceDirty = false;
    }

    if (m_toolsDirty) {
        m_toolsDirty = false;
        rootNode->toolsNode->clearChildren();
    }

    if (m_tools->visible()) {
        //set up mv foor the tools, based on the vehicle
        QMatrix4x4 toolMv = m_currentMv;


        rootNode->toolsNode->update(
            toolMv,
            m_currentP,
            m_currentNcd,
            viewportSize,
            m_textureFactory,
            m_tools,
            m_camera->zoom());

    }


    if (m_vehicleDirty) {
        //regenerate geometry
        m_vehicleDirty = false;
        rootNode->vehicleNode->clearChildren();
    }
    if (m_vehicle->visible()) {
        rootNode->vehicleNode->update(
            m_currentMv,
            m_currentP,
            m_currentNcd,
            m_vehicle->color(),
            viewportSize,
            m_textureFactory,
            m_renderData.vehicleX,
            m_renderData.vehicleY,
            m_renderData.vehicleHeading,
            m_vehicle,
            m_camera->zoom()
        );
    }

    return rootNode;
}

// ============================================================================
// Matrix Building
// ============================================================================

QMatrix4x4 FieldViewItem::buildNdcMatrix() const
{
    // Standard 3D MVP: world coords -> NDC (-1 to 1)
    //QMatrix4x4 mvp3d = buildProjectionMatrix() * buildViewMatrix();

    // Transform from NDC to item-local coordinates
    // NDC: x=-1 to 1, y=-1 to 1
    // Item: x=0 to width, y=0 to height (Qt Y is down, so we flip)
    //
    // Mapping:
    //   x_item = (x_ndc + 1) / 2 * width  = x_ndc * (width/2) + (width/2)
    //   y_item = (1 - y_ndc) / 2 * height = y_ndc * (-height/2) + (height/2)
    //
    // As a matrix (applied after MVP):
    //   scale(width/2, -height/2, 1) then translate(width/2, height/2, 0)
    // But since we're pre-multiplying, we do it in reverse order

    float w = static_cast<float>(width());
    float h = static_cast<float>(height());

    QMatrix4x4 ndcToItem;

    if (w > 0 && h > 0) {
        // First translate NDC origin to item center, then scale
        ndcToItem.translate(w / 2.0f, h / 2.0f, 0.0f);
        ndcToItem.scale(w / 2.0f, -h / 2.0f, 1.0f);  // Negative Y to flip (Qt Y is down)
    }

    return ndcToItem;
}

QMatrix4x4 FieldViewItem::buildProjectionMatrix() const
{
    QMatrix4x4 projection;

    // Set up perspective projection
    float aspect = static_cast<float>(width()) / static_cast<float>(height());
    if (aspect <= 0)
        aspect = 1.0f;

    // FOV is set via camera.fov property (default 40.1 degrees matches OpenGL's fovy = 0.7 radians)
    // Far plane uses camDistanceFactor * camSetDistance where camDistanceFactor = -2
    // Since zoom is positive (abs of camSetDistance), far = 2 * zoom
    float fov = static_cast<float>(m_camera->fov());
    float farPlane = static_cast<float>(2.0 * abs(m_camera->zoom()));
    if (farPlane < 100.0f) farPlane = 100.0f;  // Minimum far plane

    projection.perspective(fov, aspect, 1.0f, farPlane);

    return projection;
}

QMatrix4x4 FieldViewItem::buildViewMatrix() const
{
    QMatrix4x4 view;

    // Match OpenGL version: camera distance = camSetDistance * 0.5
    // zoom is positive (abs of camSetDistance), so we use -zoom * 0.5
    view.translate(0, 0, static_cast<float>(m_camera->zoom() * 0.5));

    // Apply pitch (tilt)
    view.rotate(static_cast<float>(m_camera->pitch()), 1.0f, 0.0f, 0.0f);

    // Apply rotation (yaw) - camera follows vehicle heading
    view.rotate(static_cast<float>(-m_camera->rotation()), 0.0f, 0.0f, 1.0f);

    // Translate to camera position (center on vehicle/target)
    view.translate(static_cast<float>(-m_camera->x()), static_cast<float>(-m_camera->y()), 0.0f);

    return view;
}
