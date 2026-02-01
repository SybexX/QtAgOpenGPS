#include "tracksnode.h"
#include "materials.h"
#include "aoggeometry.h"
#include "textnode.h"
#include "dotsnode.h"
#include "texturefactory.h"
#include "thicklinematerial.h"
#include "glm.h"

TracksNode::TracksNode() {}

TracksNode::~TracksNode() {}

void TracksNode::clearChildren()
{
       while (childCount() > 0) {
        QSGNode *child = firstChild();
        removeChildNode(child);
        delete child;
    }

    m_newTrack = nullptr;
    m_aRefFlag = nullptr;
    m_bRefFlag = nullptr;
    m_refDotsNode = nullptr;
}

void TracksNode::update(const QMatrix4x4 &mv,
                        const QMatrix4x4 &p,
                        const QMatrix4x4 &ndc,
                        const QSize &viewportSize,
                        float vehicleX,
                        float vehicleY,
                        float vehicleHeading,
                        bool isOutOfBounds,
                        int lineWidth,
                        float textSize,
                        TextureFactory *texture,
                        TracksProperties *properties)
{
    if (childCount() < 1) {
        //construct the lines that don't change often

    }
    //these things change frequently
    if (properties->newTrack().count()) {

        if (!m_newTrack) {
            auto *geometry = AOGGeometry::createThickLineGeometry(properties->newTrack());
            if (geometry) {
                m_newTrack = new QSGGeometryNode();
                m_newTrack->setGeometry(geometry);
                m_newTrack->setFlag(QSGNode::OwnsGeometry);

                // Create material with MVP matrix
                auto *material = new ThickLineMaterial();

                m_newTrack->setMaterial(material);
                m_newTrack->setFlag(QSGNode::OwnsMaterial);

                appendChildNode(m_newTrack);
            }
        } else {
            //update geometry
            int numVertices = properties->newTrack().count() * 4 + (properties->newTrack().count() - 1) * 2;
            auto geometry = m_newTrack->geometry();
            if (geometry) {
                geometry->allocate(numVertices);
                m_newTrack->markDirty(QSGNode::DirtyGeometry);
                ThickLineVertex *data = static_cast<ThickLineVertex *>(geometry->vertexData());
                AOGGeometry::updateThickLineGeometry(data, properties->newTrack());
            }
        }
    }

    if (!m_refDotsNode) {
        m_refDotsNode = new DotsNode();
        appendChildNode(m_refDotsNode);
    }

    if (properties->showRefFlags()) {
        if (!m_aRefFlag) {
            //create a dot

            m_aRefFlag = new TextNode(texture,"&A", textSize);
            appendChildNode(m_aRefFlag);
            m_refDotsNode->addDot(properties->aRefFlag(),QColor::fromRgbF(0.0f, 0.90f, 0.95f, 1.0), glm::dp(8.0f));
            m_refDotsNode->build();
        }

        if (!m_bRefFlag) {
            m_bRefFlag = new TextNode(texture,"&B", textSize);
            appendChildNode(m_bRefFlag);
            m_refDotsNode->addDot(properties->bRefFlag(),QColor::fromRgbF(0.95f, 0.0f, 0.0f, 1.0), glm::dp(8.0f));
            m_refDotsNode->build();
        }

        QMatrix4x4 flagMv = mv;
        flagMv.translate(properties->bRefFlag());
        m_bRefFlag->updateUniforms(ndc*p*flagMv,viewportSize,QColor::fromRgbF(0.0f, 0.90f, 0.95f, 1.0),true);

        flagMv = mv;
        flagMv.translate(properties->aRefFlag());
        m_aRefFlag->updateUniforms(ndc*p*flagMv,viewportSize,QColor::fromRgbF(0.95f, 0.0f, 0.0f, 1.0),true);

        //update the dots. Use field normal matrix
        m_refDotsNode->updateUniforms(ndc*p*mv, viewportSize);
    }

    //update matrices and uniforms

}
