#include "cboundary.h"
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include "vec2.h"
#include "vec3.h"
#include "glm.h"
#include "cvehicle.h"
#include "ctool.h"
#include "cpgn.h"

void CBoundary::SetHydPosition(SectionState::State autoBtnState, CPGN_EF &p_239, CVehicle &vehicle)
{
    if (CVehicle::instance()->isHydLiftOn() && CVehicle::instance()->avgSpeed() > 0.2 && autoBtnState == SectionState::Auto)
    {
        if (isToolInHeadland)
        {
            p_239.pgn[p_239.hydLift] = 2;
            //TODO: implement sounds
            emit soundHydLiftChange(isToolInHeadland);
            /*
            if (mf.sounds.isHydLiftChange != isToolInHeadland)
            {
                if (mf.sounds.isHydLiftSoundOn) mf.sounds.sndHydLiftUp.Play();
                mf.sounds.isHydLiftChange = isToolInHeadland;
            }
            */
        }
        else
        {
            p_239.pgn[p_239.hydLift] = 1;
            //TODO: implement sounds
            emit soundHydLiftChange(isToolInHeadland);
            /*
            if (mf.sounds.isHydLiftChange != isToolInHeadland)
            {
                if (mf.sounds.isHydLiftSoundOn) mf.sounds.sndHydLiftDn.Play();
                mf.sounds.isHydLiftChange = isToolInHeadland;
            }
            */
        }
    }
}

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
