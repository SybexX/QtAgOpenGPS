#ifndef CPATCHES_H
#define CPATCHES_H

#include <QSharedPointer>
#include <QVector>
#include <QVector3D>

typedef QVector<QVector3D> PatchTriangleList;

class CFieldData;

class CPatches
{
public:
    //torriem: we use a QVector of QVector3D so that it's
    //more efficient to draw on openGL back buffer.

    //list of patch data individual triangles
    QSharedPointer<PatchTriangleList> triangleList;

    //list of the list of patch data individual triangles for that entire section activity
    QVector<QSharedPointer<PatchTriangleList>> patchList;

    //mapping
    bool isDrawing = false;

    //points in world space that start and end of section are in
    QVector3D leftPoint, rightPoint;

    int numTriangles = 0;
    int currentStartSectionNum, currentEndSectionNum;
    int newStartSectionNum, newEndSectionNum;

    CPatches();

    void turnMappingOn(QVector3D newLeftPoint, QVector3D newRightPoint, QVector3D color);
    void turnMappingOff(QVector<QSharedPointer<PatchTriangleList>> &patchSaveList,
                        QVector3D newLeftPoint, QVector3D newRightPoint, QVector3D color,
                        CFieldData &fd);
    void addMappingPoint(QVector<QSharedPointer<PatchTriangleList>> &patchSaveList,
                         QVector3D newLeftPoint, QVector3D newRightPoint, QVector3D color,
                         CFieldData &fd);





};

#endif // CPATCHES_H