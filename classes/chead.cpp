#include "cboundary.h"
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include "vec2.h"
#include "vec3.h"
#include "glm.h"
#include "cvehicle.h"
#include "ctool.h"
#include "cpgn.h"

bool CBoundary::IsPointInsideHeadArea(Vec2 pt) const
{
    //if inside outer boundary, then potentially add
    if (glm::IsPointInPolygon(bndList[0].hdLine, pt))
    {
        for (int i = 1; i < bndList.count(); i++)
        {
            if (glm::IsPointInPolygon(bndList[i].hdLine,pt))
            {
                //point is in an inner turn area but inside outer
                return false;
            }
        }
        return true;
    }
    return false;
}
