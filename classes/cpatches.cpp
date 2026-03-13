#include "cpatches.h"
#include "classes/settingsmanager.h"
#include "qmlutil.h"
#include "backend.h"

inline QColor get_SectionColor(int section) {
    switch (section) {
    case 0:
        return SettingsManager::instance()->color_sec01();
    case 1:
        return SettingsManager::instance()->color_sec02();
    case 2:
        return SettingsManager::instance()->color_sec03();
    case 3:
        return SettingsManager::instance()->color_sec04();
    case 4:
        return SettingsManager::instance()->color_sec05();
    case 5:
        return SettingsManager::instance()->color_sec06();
    case 6:
        return SettingsManager::instance()->color_sec07();
    case 7:
        return SettingsManager::instance()->color_sec08();
    case 8:
        return SettingsManager::instance()->color_sec09();
    case 9:
        return SettingsManager::instance()->color_sec10();
    case 10:
        return SettingsManager::instance()->color_sec11();
    case 11:
        return SettingsManager::instance()->color_sec12();
    case 12:
        return SettingsManager::instance()->color_sec13();
    case 13:
        return SettingsManager::instance()->color_sec14();
    case 14:
        return SettingsManager::instance()->color_sec15();
    case 15:
        return SettingsManager::instance()->color_sec16();
    }
    return QColor();
}


CPatches::CPatches() {
    triangleList = QSharedPointer<PatchTriangleList>( new PatchTriangleList);
    triangleListBoundingBox = QSharedPointer<PatchBoundingBox>( new PatchBoundingBox);
}

/* torriem: modifications. Passing in left, right points, and color, rather than
 * accessing the CSection objects themselves.
 */
void CPatches::TurnMappingOn(QColor section_color,
                             Vec2 leftPoint, Vec2 rightPoint)
{
    QColor display_colorSectionsDay = SettingsManager::instance()->display_colorSectionsDay();

    QColor color_prop;
    numTriangles = 0;

    //do not tally square meters on inital point, that would be silly
    if (!isDrawing)   {
        //set the section bool to on
        isDrawing = true;

        //starting a new patch chunk so create a new triangle list
        triangleList = QSharedPointer<PatchTriangleList>( new PatchTriangleList);
        //create a new bounding box
        triangleListBoundingBox = QSharedPointer<PatchBoundingBox>( new PatchBoundingBox);

        patchList.append(triangleList);
        patchBoundingBoxList.append(triangleListBoundingBox);

        //Add Patch colour
        if (!(bool)SettingsManager::instance()->color_isMultiColorSections())
        {
            color_prop = display_colorSectionsDay;
        }
        else
        {
            if (SettingsManager::instance()->tool_isSectionsNotZones())
                color_prop = section_color;
            else
                color_prop = display_colorSectionsDay;
        }

        triangleList->append(QVector3D(color_prop.redF(), color_prop.greenF(), color_prop.blueF()));

        //leftPoint = tool.section[currentStartSectionNum].leftPoint;
        //rightPoint = tool.section[currentEndSectionNum].rightPoint;

        //left side of triangle
        triangleList->append(leftPoint);

        //Right side of triangle
        triangleList->append(rightPoint);
    }
}

void CPatches::TurnMappingOff(QColor section_color,
                              Vec2 leftPoint,
                              Vec2 rightPoint,
                              QVector<QSharedPointer<PatchTriangleList>> &patchSaveList
                              )
{
   AddMappingPoint(section_color, leftPoint, rightPoint, patchSaveList);

   isDrawing = false;
   numTriangles = 0;

   if (triangleList->count() > 4)
   {
       //save the triangle list in a patch list to add to saving file
       patchSaveList.append(triangleList);
   }
   else
   {
       //torriem: patch strip is too small to keep, so get rid of it
       triangleList->clear();
       triangleListBoundingBox.clear();
       if (patchList.count() > 0) {
           patchList.removeAt(patchList.count() - 1);
           patchBoundingBoxList.removeAt(patchList.count() - 1);
       }
   }
}

void CPatches::AddMappingPoint(QColor section_color,
                               Vec2 vleftPoint,
                               Vec2 vrightPoint,
                               QVector<QSharedPointer<PatchTriangleList>> &patchSaveList
                               )
{
    //Vec2 vleftPoint = tool.section[currentStartSectionNum].leftPoint;
    //Vec2 vrightPoint = tool.section[currentEndSectionNum].rightPoint;
    QVector3D leftPoint(vleftPoint.easting,vleftPoint.northing,0);
    QVector3D rightPoint(vrightPoint.easting,vrightPoint.northing,0);
    QVector3D color;
    QColor color_prop;
    QColor display_colorSectionsDay = SettingsManager::instance()->display_colorSectionsDay();


    //add two triangles for next step.
    //left side

    //add the point to List
    triangleList->append(leftPoint);

    //Right side
    triangleList->append(rightPoint);

    if (vleftPoint.easting < (*triangleListBoundingBox).minx) (*triangleListBoundingBox).minx = vleftPoint.easting;
    if (vleftPoint.easting > (*triangleListBoundingBox).maxx) (*triangleListBoundingBox).maxx = vleftPoint.easting;
    if (vleftPoint.northing < (*triangleListBoundingBox).maxy) (*triangleListBoundingBox).maxy = vleftPoint.northing;
    if (vleftPoint.northing > (*triangleListBoundingBox).maxy) (*triangleListBoundingBox).maxy = vleftPoint.northing;
    if (vrightPoint.easting < (*triangleListBoundingBox).minx) (*triangleListBoundingBox).minx = vrightPoint.easting;
    if (vrightPoint.easting > (*triangleListBoundingBox).maxx) (*triangleListBoundingBox).maxx = vrightPoint.easting;
    if (vrightPoint.northing < (*triangleListBoundingBox).maxy) (*triangleListBoundingBox).maxy = vrightPoint.northing;
    if (vrightPoint.northing > (*triangleListBoundingBox).maxy) (*triangleListBoundingBox).maxy = vrightPoint.northing;

    //count the triangle pairs
    numTriangles++;

    //quick count
    int c = triangleList->count() - 1;

    //when closing a job the triangle patches all are emptied but the section delay keeps going.
    //Prevented by quick check. 4 points plus colour

    //torriem: easting is .x(), northing is .y() when using QVector3D
    double temp = fabs(((*triangleList)[c].x() * ((*triangleList)[c - 1].y() - (*triangleList)[c - 2].y()))
                       + ((*triangleList)[c - 1].x() * ((*triangleList)[c - 2].y() - (*triangleList)[c].y()))
                       + ((*triangleList)[c - 2].x() * ((*triangleList)[c].y() - (*triangleList)[c - 1].y())));

    temp += fabs(((*triangleList)[c - 1].x() * ((*triangleList)[c - 2].y() - (*triangleList)[c - 3].y()))
                 + ((*triangleList)[c - 2].x() * ((*triangleList)[c - 3].y() - (*triangleList)[c - 1].y()))
                 + ((*triangleList)[c - 3].x() * ((*triangleList)[c - 1].y() - (*triangleList)[c - 2].y())));

    temp *= 0.5;

    // Update worked area using Q_PROPERTY
    Backend::instance()->currentField_addWorkedAreaTotal(temp);
    Backend::instance()->currentField_addWorkedAreaTotalUser(temp);

    if (numTriangles > 61)
    {
        numTriangles = 0;

        //save the cutoff patch to be saved later
        patchSaveList.append(triangleList);

        triangleList = QSharedPointer<PatchTriangleList>(new PatchTriangleList);
        triangleListBoundingBox = QSharedPointer<PatchBoundingBox>( new PatchBoundingBox);

        patchList.append(triangleList);
        patchBoundingBoxList.append(triangleListBoundingBox);

        //Add Patch colour
        if (!(bool)SettingsManager::instance()->color_isMultiColorSections())
            color_prop = display_colorSectionsDay;
        else
            color_prop = section_color;

        color = QVector3D(color_prop.redF(), color_prop.greenF(), color_prop.blueF());
        //add the points to List, yes its more points, but breaks up patches for culling
        triangleList->append(color);

        triangleList->append(leftPoint);
        triangleList->append(rightPoint);
    }
}
