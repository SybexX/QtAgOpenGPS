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
    QSGGeometryNode *m_refLineNode = nullptr;
    QSGGeometryNode *m_currentLineNode = nullptr;
    QSGGeometryNode *m_newTrack = nullptr;
    TextNode *m_aRefFlag = nullptr;
    TextNode *m_bRefFlag = nullptr;
    DotsNode *m_refDotsNode = nullptr;

    QSGGeometryNode *m_shadowOutlineNode = nullptr;
    QSGGeometryNode *m_shadowFillNode = nullptr;
    QVector<QSGGeometryNode*> m_sideGuideNodes;
    QVector<QSGGeometryNode*> m_curveGuideNodes;
    DotsNode *m_lookaheadNode = nullptr;
    QSGGeometryNode *m_pursuitCircleNode = nullptr;
    QSGGeometryNode *m_smoothedCurveNode = nullptr;
    DotsNode *m_currentLineDotsNode = nullptr;
    DotsNode *m_youTurnDotsNode = nullptr;

    // Contour rendering
    QSGGeometryNode *m_contourLineNode = nullptr;       // Contour line (simple line)
    DotsNode *m_contourPointsNode = nullptr;           // Contour line points
    DotsNode *m_stripPointsNearbyNode = nullptr;       // Dense points near vehicle
    QVector<DotsNode*> m_stripPointsSparseNodes;       // Sparse points (chunked)
    QVector<QSGGeometryNode*> m_stripPointsSparseLineNodes;  // Thin line strips (chunked)
    DotsNode *m_contourCurrentPointNode = nullptr;    // Current position on strip
    DotsNode *m_contourGoalPointNode = nullptr;       // Goal point
    int m_lastContourLineCount = 0;
    int m_lastStripPointsNearbyCount = 0;
    int m_lastStripChunks = 0;

    void updateThickLineNode(QSGGeometryNode *node,
                             const QMatrix4x4 &mvp,
                             const QSize &viewportSize,
                             int lineWidth,
                             const QColor &color);
};

#endif // TRACKSNODE_H
