/*
 * CVarPlatform.cpp
 *
 *  Created on: Sep 6, 2012
 *      Author: gerstrong
 */

#include "CVarPlatform.h"

#include <boost/property_tree/ptree.hpp>

namespace galaxy {


const int MOVE_SPEED = 20;

CVarPlatform::CVarPlatform(CMap *pmap, const Uint16 foeID, Uint32 x, Uint32 y,
                           const direction_t horidir,
                           const direction_t vertdir,
                           const int actionOffset, const int sprVar) :
    CGalaxySpriteObject(pmap, foeID, x, y, sprVar),
    CPlatform(pmap, foeID, x, y),
    CMoveTarget(m_Pos, xDirection, yDirection)
{
    m_ActionBaseOffset = actionOffset;

    setActionForce(A_PLATFORM_MOVE);
    setActionSprite();

    xDirection = horidir;
    yDirection = vertdir;

    solid = false; // I think they must be false, because some plats like those in Keen6
    // should only obey to the set markers

    calcBoundingBoxes();
}


void
CVarPlatform::
deserialize(CSaveGameController &savedGame)
{
    savedGame.decodeData(target.x);
    savedGame.decodeData(target.y);
}

void
CVarPlatform::
serialize(CSaveGameController &savedGame)
{
    savedGame.encodeData(target.x);
    savedGame.encodeData(target.y);
}

void
CVarPlatform::
serialize(boost::property_tree::ptree &node)
{
    auto &posNode = node.put("target", "");
    posNode.put("<xmlattr>.x", target.x);
    posNode.put("<xmlattr>.y", target.y);
}

void
CVarPlatform::
deserialize(boost::property_tree::ptree &node)
{
    auto &posNode = node.get_child("target");
    target.x = posNode.get<int>("<xmlattr>.x");
    target.y = posNode.get<int>("<xmlattr>.y");
}



void CVarPlatform::process()
{
    int xBlockPos = target.x - getXPosition();
    int yBlockPos = target.y - getYPosition();
    
    const int xBlockPosAbs = (xBlockPos<0) ? -xBlockPos : xBlockPos;
    const int yBlockPosAbs = (yBlockPos<0) ? -yBlockPos : yBlockPos;
    
    if( xBlockPosAbs < MOVE_SPEED && yBlockPosAbs < MOVE_SPEED )
    {
        const Uint16 object = mpMap->getPlaneDataAt(2, target.x, target.y);

        Vector2D<int> speed(xBlockPos, yBlockPos);
        movePlat(speed);

        readDirection(object, xDirection, yDirection );
        detectNextTarget(target, xDirection, yDirection);
    }
    
    if(yDirection == UP && blockedu)
        yDirection = DOWN;
    else if(yDirection == DOWN && blockedd)
        yDirection = UP;
    
    if(xDirection == RIGHT && blockedr)
        xDirection = LEFT;
    else if(xDirection == LEFT && blockedl)
        xDirection = RIGHT;
    
    Vector2D<int> speed;
    
    if(yDirection == UP)
    {
        speed.y = -MOVE_SPEED;
    }
    else if(yDirection == DOWN)
    {
        speed.y = MOVE_SPEED;
    }
    
    
    if(xDirection == RIGHT)
    {
        speed.x = MOVE_SPEED;
    }
    else if(xDirection == LEFT)
    {
        speed.x = -MOVE_SPEED;
    }
    
    movePlat(speed);
    
    CPlatform::process();
}

}

