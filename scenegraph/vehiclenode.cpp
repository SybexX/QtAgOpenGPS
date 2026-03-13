// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Vehicle node implementation

#include "vehiclenode.h"
#include "aogmaterial.h"
#include "aoggeometry.h"
#include "materials.h"
#include "thicklinematerial.h"
#include "roundpointsizematerial.h"
#include "roundpointmaterial.h"
#include "glm.h"

#include <QVector3D>

VehicleNode::VehicleNode()
{
}

VehicleNode::~VehicleNode()
{
}

void VehicleNode::clearChildren()
{
    while (childCount() > 0) {
        QSGNode *child = firstChild();
        removeChildNode(child);
        delete child;
    }
    this->m_nodes.clear();
}

void VehicleNode::update(const QMatrix4x4 &mv,
                         const QMatrix4x4 &p,
                         const QMatrix4x4 &ncd,
                         const QColor &vehicleColor,
                         const QSize &viewportSize,
                         TextureFactory *textureFactory,
                         double vehicleX, double vehicleY,
                         double vehicleHeading,
                         const VehicleProperties *properties, double camSetDistance)
{
    if (childCount() < 1 /*|| we need to change something */) {
        // construct geometry. either this is the initial display, or
        // something in the vehicle's geometry changed and we need to
        // update it.

        clearChildren();
        const float trackWidth = properties->trackWidth();   // 2m total width
        const float wheelBase = properties->wheelBase();


        QVector<QVector3D> hitches;

        //=====================================
        // draw front hitch
        //=====================================
        if (properties->frontHitchLength()) {
            //currently we don't draw a front hitch
        }

        if (properties->drawbarLength()) {

            hitches.append({ 0, static_cast<float>(properties->drawbarLength()), 0});
            hitches.append({ 0, 0, 0});
        }

        if (properties->threePtLength()) {
            hitches.append( {-0.35, -properties->threePtLength(),0} );
            hitches.append( {-0.35, 0 ,0} );
            hitches.append( {0.35, -properties->threePtLength(),0} );
            hitches.append( {0.35, 0 ,0} );
        }

        if (hitches.count()) {
            //shadow
            auto *geometry = AOGGeometry::createThickLinesGeometry(hitches);
            if (geometry) {
                auto geomNode = new QSGGeometryNode();
                geomNode->setGeometry(geometry);
                geomNode->setFlag(QSGNode::OwnsGeometry);

                auto *material = new ThickLineMaterial();
                material->setColor(QColor::fromRgbF(0,0,0,1));
                material->setLineWidth(3.0f);

                geomNode->setMaterial(material);
                geomNode->setFlag(QSGNode::OwnsMaterial);

                appendChildNode(geomNode);
                m_nodes[VehicleNodeType::HitchShadow] = geomNode;
            }

            //lines themselves
            geometry = AOGGeometry::createThickLinesGeometry(hitches);
            if (geometry) {
                auto geomNode = new QSGGeometryNode();
                geomNode->setGeometry(geometry);
                geomNode->setFlag(QSGNode::OwnsGeometry);

                //auto *material = new AOGFlatColorMaterial();
                auto *material = new ThickLineMaterial();
                material->setColor(QColor::fromRgbF(1.237f, 0.037f, 0.0397f));
                material->setLineWidth(1.0f);

                geomNode->setMaterial(material);
                geomNode->setFlag(QSGNode::OwnsMaterial);

                appendChildNode(geomNode);
                m_nodes[VehicleNodeType::HitchLine] = geomNode;
            }
        }

        if (textureFactory && properties->type() < 3) {
            //draw Question mark

            if (!properties->firstHeadingSet()) {

                auto *geometry = new QSGGeometry(AOGGeometry::texturedVertexAttributes(), 4);
                geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

                auto *geomNode = new QSGGeometryNode();
                geomNode->setGeometry(geometry);
                geomNode->setFlag(QSGNode::OwnsGeometry);

                auto *material = new AOGTextureMaterial();

                TexturedVertex *data = static_cast<TexturedVertex *>(geometry->vertexData());

                data[0] = { 5.0f ,5.0f ,0.0f , 1.0f ,0.0f  };
                data[1] = { 1.0f ,5.0f ,0.0f , 0.0f ,0.0f  };
                data[2] = { 5.0f ,1.0f ,0.0f , 1.0f ,1.0f  };
                data[3] = { 1.0f ,1.0f ,0.0f , 0.0f ,1.0f  };

                QSGTexture *texture = textureFactory->texture(TextureId::QuestionMark);
                if (texture) {
                    material->setTexture(texture);
                }

                material->setUseColor(false);
                geomNode->setMaterial(material);
                geomNode->setFlag(QSGNode::OwnsMaterial);

                appendChildNode(geomNode);
                m_nodes[VehicleNodeType::QuestionMark] = geomNode;
            }

            //Body of tractor or combine

            // Create textured quad for vehicle using local coordinates
            // Local coords: centered at origin, front is +Y, right is +X
            // Texture: bottom of image is back of vehicle, top is front

            auto *geometry = new QSGGeometry(AOGGeometry::texturedVertexAttributes(), 4);
            geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

            auto *geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            auto *material = new AOGTextureMaterial();

            TexturedVertex *data = static_cast<TexturedVertex *>(geometry->vertexData());

            // Triangle strip order: back-left, back-right, front-left, front-right
            // Back-left (texture bottom-left: u=0, v=1)

            //Rear or body part first

            if (properties->type() == 0) {
                // Regular tractor back

                data[0] = { -properties->trackWidth(), -properties->wheelBase() * 0.5f, 0.0f, 0.0f, 1.0f};
                data[1] = {  properties->trackWidth(), -properties->wheelBase() * 0.5f, 0.0f, 1.0f, 1.0f};
                data[2] = { -properties->trackWidth(),  properties->wheelBase() * 1.5f, 0.0f, 0.0f, 0.0f};
                data[3] = {  properties->trackWidth(),  properties->wheelBase() * 1.5f, 0.0f, 1.0f, 0.0f};

                QSGTexture *texture = textureFactory->texture(TextureId::Tractor);
                if (texture) {
                    material->setTexture(texture);
                }
                //standard matrix

            } else if (properties->type() == 1) {
                // Harvester body
                data[0] = { -properties->trackWidth(), -properties->wheelBase() * 1.5f, 0.0f, 0.0f, 1.0f };
                data[1] = {  properties->trackWidth(), -properties->wheelBase() * 1.5f, 0.0f, 1.0f, 1.0f };
                data[2] = { -properties->trackWidth(),  properties->wheelBase() * 1.5f, 0.0f, 0.0f, 0.0f };
                data[3] = {  properties->trackWidth(),  properties->wheelBase() * 1.5f, 0.0f, 1.0f, 0.0f };

                QSGTexture *texture = textureFactory->texture(TextureId::Harvester);
                if (texture) {
                    material->setTexture(texture);
                }

            } else if (properties->type() == 2) {
                // 4WD Tractor Rear
                data[0] = { -properties->trackWidth(), -properties->wheelBase() * 0.65f, 0.0f, 0.0f, 1.0f };
                data[1] = {  properties->trackWidth(), -properties->wheelBase() * 0.65f, 0.0f, 1.0f, 1.0f };
                data[2] = { -properties->trackWidth(),  properties->wheelBase() * 0.65f, 0.0f, 0.0f, 0.0f };
                data[3] = {  properties->trackWidth(),  properties->wheelBase() * 0.65f, 0.0f, 1.0f, 0.0f };

                QSGTexture *texture = textureFactory->texture(TextureId::Tractor4WDRear);
                if (texture) {
                    material->setTexture(texture);
                }


            }

            // Don't use color tinting - show texture as-is
            material->setUseColor(false);

            geomNode->setMaterial(material);
            geomNode->setFlag(QSGNode::OwnsMaterial);
            //geomNode->setFlag(QSGNode::DirtyGeometry); //

            appendChildNode(geomNode);
            m_nodes[VehicleNodeType::Body] = geomNode;

            // =============================================
            // Now draw left steering tire, or tractor front
            // =============================================

            geometry = new QSGGeometry(AOGGeometry::texturedVertexAttributes(), 4);
            geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

            geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            material = new AOGTextureMaterial();

            data = static_cast<TexturedVertex *>(geometry->vertexData());

            if (properties->type() == 0) {
                //front left wheel
                data[0] = { -properties->trackWidth() * 0.5f, -properties->wheelBase() * 0.75f, 0.0f, 0.0f, 1.0f };
                data[1] = {  properties->trackWidth() * 0.5f, -properties->wheelBase() * 0.75f, 0.0f, 1.0f, 1.0f };
                data[2] = { -properties->trackWidth() * 0.5f,  properties->wheelBase() * 0.75f, 0.0f, 0.0f, 0.0f };
                data[3] = {  properties->trackWidth() * 0.5f,  properties->wheelBase() * 0.75f, 0.0f, 1.0f, 0.0f };

                QSGTexture *texture = textureFactory->texture(TextureId::FrontWheels);
                if (texture) {
                    material->setTexture(texture);
                }

            } else if (properties->type() == 1) {
                //harvester rear left wheel
                data[0] = { -properties->trackWidth() * 0.25f, -properties->wheelBase() * 0.5f, 0.0f, 0.0f, 1.0f };
                data[1] = {  properties->trackWidth() * 0.25f, -properties->wheelBase() * 0.5f, 0.0f, 1.0f, 1.0f };
                data[2] = { -properties->trackWidth() * 0.25f,  properties->wheelBase() * 0.5f, 0.0f, 0.0f, 0.0f };
                data[3] = {  properties->trackWidth() * 0.25f,  properties->wheelBase() * 0.5f, 0.0f, 1.0f, 0.0f };

                QSGTexture *texture = textureFactory->texture(TextureId::FrontWheels);
                if (texture) {
                    material->setTexture(texture);
                }

            } else if (properties->type() == 2) {
                //4wd tractor front half
                data[0] = { -properties->trackWidth(), -properties->wheelBase() * 0.65f, 0.0f, 0.0f, 1.0f };
                data[1] = {  properties->trackWidth(), -properties->wheelBase() * 0.65f, 0.0f, 1.0f, 1.0f };
                data[2] = { -properties->trackWidth(),  properties->wheelBase() * 0.65f, 0.0f, 0.0f, 0.0f };
                data[3] = {  properties->trackWidth(),  properties->wheelBase() * 0.65f, 0.0f, 1.0f, 0.0f };

                QSGTexture *texture = textureFactory->texture(TextureId::Tractor4WDFront);
                if (texture) {
                    material->setTexture(texture);
                }

            }

            // Don't use color tinting - show texture as-is
            material->setUseColor(false);

            geomNode->setMaterial(material);
            geomNode->setFlag(QSGNode::OwnsMaterial);
            //geomNode->setFlag(QSGNode::DirtyGeometry); //

            appendChildNode(geomNode);
            m_nodes[VehicleNodeType::LeftWheel] = geomNode;

            // =============================================
            // Now draw right steering tire
            // =============================================

            geometry = new QSGGeometry(AOGGeometry::texturedVertexAttributes(), 4);
            geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

            geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            material = new AOGTextureMaterial();

            data = static_cast<TexturedVertex *>(geometry->vertexData());

            if (properties->type() == 0) {
                //front right wheel
                data[0] = { -properties->trackWidth() * 0.5f, -properties->wheelBase() * 0.75f, 0.0f, 0.0f, 1.0f };
                data[1] = {  properties->trackWidth() * 0.5f, -properties->wheelBase() * 0.75f, 0.0f, 1.0f, 1.0f };
                data[2] = { -properties->trackWidth() * 0.5f,  properties->wheelBase() * 0.75f, 0.0f, 0.0f, 0.0f };
                data[3] = {  properties->trackWidth() * 0.5f,  properties->wheelBase() * 0.75f, 0.0f, 1.0f, 0.0f };

                QSGTexture *texture = textureFactory->texture(TextureId::FrontWheels);
                if (texture) {
                    material->setTexture(texture);
                }

            } else if (properties->type() == 1) {
                //harvester rear right wheel
                data[0] = { -properties->trackWidth() * 0.25f, -properties->wheelBase() * 0.5f, 0.0f, 0.0f, 1.0f };
                data[1] = {  properties->trackWidth() * 0.25f, -properties->wheelBase() * 0.5f, 0.0f, 1.0f, 1.0f };
                data[2] = { -properties->trackWidth() * 0.25f,  properties->wheelBase() * 0.5f, 0.0f, 0.0f, 0.0f };
                data[3] = {  properties->trackWidth() * 0.25f,  properties->wheelBase() * 0.5f, 0.0f, 1.0f, 0.0f };

                QSGTexture *texture = textureFactory->texture(TextureId::FrontWheels);
                if (texture) {
                    material->setTexture(texture);
                }
            }

            // Don't use color tinting - show texture as-is
            material->setUseColor(false);

            geomNode->setMaterial(material);
            geomNode->setFlag(QSGNode::OwnsMaterial);
            //geomNode->setFlag(QSGNode::DirtyGeometry); //

            appendChildNode(geomNode);
            m_nodes[VehicleNodeType::RightWheel] = geomNode;

        } else {
            //just draw a triangle
            auto *geometry = new QSGGeometry(AOGGeometry::colorVertexAttributes(), 4);
            geometry->setDrawingMode(QSGGeometry::DrawTriangleFan);
            // Fill vertex data
            ColorVertex *v = static_cast<ColorVertex*>(geometry->vertexData());
            v[0] = { 0.0f, properties->antennaForward(), 0.0f,  0.0f, 1.20f, 1.22f, properties->opacity() };
            v[2] = { 0.0f, properties->wheelBase(), 0.0f,       1.22f, 0.0f, 1.2f,  properties->opacity() };
            v[1] = { -1.0f, 0.0f, 0.0f,                         1.22f, 0.0f, 1.2f,  properties->opacity() };
            v[3] = {  1.0f, 0.0f, 0.0f,                         1.22f, 0.0f, 1.2f,  properties->opacity() };

            // Create geometry node
            auto *geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            // Create and configure material
            auto *material = new AOGVertexColorMaterial();

            geomNode->setMaterial(material);
            geomNode->setFlag(QSGNode::OwnsMaterial);

            // Add to scene graph
            appendChildNode(geomNode);
            m_nodes[VehicleNodeType::Body] = geomNode;

            QVector<QVector3D> lines;
            lines.append( { -1.0f, 0.0f, 0.0f });
            lines.append( {  1.0f, 0.0f, 0.0f });
            lines.append( {  0.0f, properties->wheelBase(), 0.0f });

            geometry = AOGGeometry::createThickLineLoopGeometry(lines);
            geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            auto *lineMaterial = new ThickLineMaterial();
            lineMaterial->setColor(QColor::fromRgbF(0.12, 0.12, 0.12));
            lineMaterial->setLineWidth(1.0f);

            geomNode->setMaterial(lineMaterial);
            geomNode->setFlag(QSGNode::OwnsMaterial);

            appendChildNode(geomNode);
            m_nodes[VehicleNodeType::ArrowLineLoop] = geomNode;

        }

        // antenna dot
        if (camSetDistance > -75 && properties->firstHeadingSet()) {
            auto *geometry = new QSGGeometry(AOGGeometry::colorSizeVertexAttributes(), 2);
            geometry->setDrawingMode(QSGGeometry::DrawPoints);
            // Fill vertex data
            ColorSizeVertex *v = static_cast<ColorSizeVertex*>(geometry->vertexData());
            v[0] = { properties->antennaOffset(), properties->antennaForward(), 0.0f,  0.0f, 0.0f, 0.0f, 1.0f, 16.0f };
            v[1] = { properties->antennaOffset(), properties->antennaForward(), 0.0f,  0.2f, 0.98f, 0.98f, 1.0f, 10.0f };

            // Create geometry node
            auto *geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            // Create and configure material
            auto *material = new RoundPointSizeMaterial();
            material->setSoftness(0.2f);           // edge softness (0-1)

            geomNode->setMaterial(material);
            geomNode->setFlag(QSGNode::OwnsMaterial);

            // Add to scene graph
            appendChildNode(geomNode);
            m_nodes[VehicleNodeType::AntennaDot] = geomNode;
        }

        // boundary marking
        if (properties->markBoundary() != 0) {
            QVector<QVector3D> bndLine;

            bndLine.append( { 0.0f, 0.0f, 0.0f } );
            bndLine.append( { properties->markBoundary(), 0.0f, 0.0f } ) ;
            bndLine.append( { properties->markBoundary() * 0.75f, 0.25f, 0.0f } );

            auto *geometry = AOGGeometry::createThickLineGeometry(bndLine);
            auto geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            auto *material = new ThickLineMaterial();
            material->setColor(QColor::fromRgbF(1.270, 1.220, 0.20, 1.0));
            material->setLineWidth(2.0f);

            geomNode->setMaterial(material);
            geomNode->setFlag(QSGNode::OwnsMaterial);

            appendChildNode(geomNode);
            m_nodes[VehicleNodeType::Marker] = geomNode;
        }

        //Svenn Arow
        if (properties->svennArrow() && camSetDistance > -1000) {
            float svennDist = camSetDistance * -0.07;
            float svennWidth = svennDist * 0.22;
            QVector<QVector3D> svennLine;

            svennLine.append( { svennWidth, properties->wheelBase() + svennDist, 0.0f } );
            svennLine.append( { 0, properties->wheelBase() + svennWidth + 0.5f + svennDist, 0.0f} );
            svennLine.append( { -svennWidth, properties->wheelBase() + svennDist, 0.0f } );

            auto *geometry = AOGGeometry::createThickLineGeometry(svennLine);
            auto geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            auto *material = new ThickLineMaterial();
            material->setColor(QColor::fromRgbF(1.2, 1.25, 0.10));
            material->setLineWidth(1.0f);

            geomNode->setMaterial(material);
            geomNode->setFlag(QSGNode::OwnsMaterial);

            appendChildNode(geomNode);
            m_nodes[VehicleNodeType::SvennArrow] = geomNode;
        }
    }

    //set up the matrices for all nodes.  Geometry may not have changed so this is
    //very fast

    QMatrix4x4 ncdP = ncd * p;
    QMatrix4x4 vehicleMv = mv;
    QMatrix4x4 tempMvp;

    vehicleMv.translate(vehicleX, vehicleY, 0);
    vehicleMv.rotate(glm::toDegrees(-vehicleHeading),0.0,0.0,1.0);

    //hitches
    if (m_nodes.value(VehicleNodeType::HitchShadow, nullptr))
        updateNodeMvp(m_nodes[VehicleNodeType::HitchShadow], ncdP * vehicleMv, viewportSize);
    if (m_nodes.value(VehicleNodeType::HitchLine, nullptr))
        updateNodeMvp(m_nodes[VehicleNodeType::HitchLine], ncdP * vehicleMv, viewportSize);
    if (m_nodes.value(VehicleNodeType::QuestionMark, nullptr))
        updateNodeMvp(m_nodes[VehicleNodeType::QuestionMark], ncdP * vehicleMv, viewportSize);
    if (m_nodes.value(VehicleNodeType::AntennaDot, nullptr))
        updateNodeMvp(m_nodes[VehicleNodeType::AntennaDot], ncdP * vehicleMv, viewportSize);
    if (m_nodes.value(VehicleNodeType::Marker, nullptr))
        updateNodeMvp(m_nodes[VehicleNodeType::Marker], ncdP * vehicleMv, viewportSize);
    if (m_nodes.value(VehicleNodeType::SvennArrow, nullptr))
        updateNodeMvp(m_nodes[VehicleNodeType::SvennArrow], ncdP * vehicleMv, viewportSize);
    if (m_nodes.value(VehicleNodeType::ArrowLineLoop, nullptr))
        updateNodeMvp(m_nodes[VehicleNodeType::ArrowLineLoop], ncdP * vehicleMv, viewportSize);


    //Calculate steering angles for wheels
    double leftAckerman, rightAckerman;
    if (properties->steerAngle() < 2)
    {
        leftAckerman = 1.25 * -properties->steerAngle();
        rightAckerman = -properties->steerAngle();
    }
    else
    {
        leftAckerman = -properties->steerAngle();
        rightAckerman = 1.25 * -properties->steerAngle();
    }

    //tractor body
    if (m_nodes.value(VehicleNodeType::Body, nullptr)) {
        tempMvp = vehicleMv;

        if (properties->type() == 2) { //4WD
                 //4wd tractor needs special matrices
                tempMvp.translate(0, -properties->wheelBase() * 0.5);
                tempMvp.rotate(0.5 * properties->steerAngle(), 0, 0, 1);
        }
        //no special body transformation otherwise

        updateNodeMvp(m_nodes[VehicleNodeType::Body], ncdP * tempMvp, viewportSize);
    }

    //right steer tire
    if (m_nodes.value(VehicleNodeType::RightWheel, nullptr)) {
        tempMvp = vehicleMv;

        if (properties->type() == 0) { // tractor front right tire
            tempMvp.translate(properties->trackWidth() * 0.5, properties->wheelBase(), 0);
            tempMvp.rotate(rightAckerman, 0, 0, 1);
        } else { //harvester back right tire
            tempMvp.translate(properties->trackWidth() * 0.5, -properties->wheelBase(), 0);
            tempMvp.rotate(-rightAckerman, 0, 0, 1);
        }

        updateNodeMvp(m_nodes[VehicleNodeType::RightWheel], ncdP * tempMvp, viewportSize);
    }

    //left steer tire
    if (m_nodes.value(VehicleNodeType::LeftWheel, nullptr)) {
        tempMvp = vehicleMv;

        if (properties->type() == 2) { //4WD rear body
                tempMvp.translate(0, properties->wheelBase() * 0.5);
                tempMvp.rotate(-0.5 * properties->steerAngle(), 0, 0, 1);
        } else if (properties->type() == 0) { // tractor front left tire
            tempMvp.translate(-properties->trackWidth() * 0.5, properties->wheelBase(), 0);
            tempMvp.rotate(leftAckerman, 0, 0, 1);
        } else { //harvester back right tire
            tempMvp.translate(-properties->trackWidth() * 0.5, -properties->wheelBase(), 0);
            tempMvp.rotate(-leftAckerman, 0, 0, 1);
        }

        updateNodeMvp(m_nodes[VehicleNodeType::LeftWheel], ncdP * tempMvp, viewportSize);
    }
}

void VehicleNode::updateNodeMvp(QSGGeometryNode *node,
                                const QMatrix4x4 mvp,
                                const QSize &viewportSize)
{
    auto *material = static_cast<AOGMaterial *>(node->material());
    if (material) {
        material->setMvpMatrix(mvp);
        material->setViewportSize(viewportSize);
    }
}
