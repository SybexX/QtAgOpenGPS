#ifndef TRACKSNODE_H
#define TRACKSNODE_H

#include <QVector>
#include <QSGNode>

#include "tracksproperties.h"

class TextNode;
class TextureFactory;
class DotsNode;

class TracksNode : public QSGNode
{
public:
    TracksNode();
    ~TracksNode() override;

    void update(const QMatrix4x4 &mv,
                const QMatrix4x4 &p,
                const QMatrix4x4 &ndc,
                const QSize &viewportSize, float vehicleX, float vehicleY, float vehicleHeading,
                bool isOutOfBounds,
                int lineWidth, float textSize, TextureFactory *texture,
                TracksProperties *properties);

    void clearChildren();

private:
    QSGGeometryNode *m_newTrack = nullptr;
    TextNode *m_aRefFlag = nullptr;
    TextNode *m_bRefFlag = nullptr;
    DotsNode *m_refDotsNode = nullptr;
};

#endif // TRACKSNODE_H
