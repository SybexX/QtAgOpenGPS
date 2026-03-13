#ifndef CPATCHES_H
#define CPATCHES_H

#include <QSharedPointer>
#include <QVector>
#include <QVector3D>
#include <QObject>
#include "vec2.h"

typedef QVector<QVector3D> PatchTriangleList;

struct PatchBoundingBox {
    float minx;
    float miny;
    float maxx;
    float maxy;
};

class CPatches
{
public:
    //torriem: we use a QVector of QVector3D so that it's
    //more efficient to draw on openGL back buffer.

    //currently building list of patch data individual triangles
    QSharedPointer<PatchTriangleList> triangleList;
    QSharedPointer<PatchBoundingBox> triangleListBoundingBox;

    //list of the list of patch data individual triangles for that entire section activity
    QVector<QSharedPointer<PatchTriangleList>> patchList;
    QVector<QSharedPointer<PatchBoundingBox>> patchBoundingBoxList;

    //mapping
    bool isDrawing = false;

    //points in world space that start and end of section are in
    QVector3D leftPoint, rightPoint;

    int numTriangles = 0;
    int currentStartSectionNum, currentEndSectionNum;
    int newStartSectionNum, newEndSectionNum;

    CPatches();

    void TurnMappingOn(QColor section_color,
                       Vec2 rightPoint, Vec2 leftPoint);
    void TurnMappingOff(QColor section_color,
                        Vec2 leftPoint,
                        Vec2 rightPoint,
                        QVector<QSharedPointer<PatchTriangleList> > &patchSaveList);
    void AddMappingPoint(QColor section_color,
                         Vec2 rightPoint, Vec2 leftPoint,
                         QVector<QSharedPointer<PatchTriangleList> > &patchSaveList);





};

#endif // CPATCHES_H
