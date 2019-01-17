/*
 * CBullet.cpp
 *
 *  Created on: 01.12.2010
 *      Author: gerstrong
 */

#include "CBullet.h"
#include "CPlayerLevel.h"
#include "sdl/audio/Audio.h"

size_t bulletActionMap[] =
{
    0x1738,
    0x1502,
    0x153E,
    0x15A4  // k6demo
};

namespace galaxy
{

//const Uint32 speed = 10;
//const Uint32 hittime = 100;

CBullet::CBullet(CMap *pmap, const Uint16 foeID, const Uint32 x, const Uint32 y,
                 const int xDir, const int yDir, const int sprVar) :
CGalaxySpriteObject(pmap, foeID, x, y, sprVar),
mReversed(false)
{
	xDirection = xDir;
	yDirection = yDir;

	const size_t offsetIndex = gBehaviorEngine.isDemo() ? 3 : gBehaviorEngine.getEpisode() - 4;

	setupGalaxyObjectOnMap(bulletActionMap[offsetIndex], A_KEENSHOT_MOVING);
	setActionSprite();
	calcBoundingBoxes();
    playSound( SOUND_KEEN_FIRE );
}


void CBullet::getTouchedBy(CSpriteObject& theObject)
{
    if( getActionNumber(A_KEENSHOT_IMPACT) )
        return;

    if( CPlayerLevel *player = dynamic_cast<CPlayerLevel*>(&theObject) )
    {
        if(mReversed) // Happens usually in Episode 6
        {
            player->stun();
            mIsDead = true;
            setAction(A_KEENSHOT_IMPACT);
        }

        // Friendly fire?
        const auto optFFire = gBehaviorEngine.mOptions[GameOption::ALLOWPKING];

        if(!optFFire.value)
        {
            return;
        }

        if( player->getSpriteVariantIdx() != getSpriteVariantIdx() )
        {
            player->kill(false);
        }
    }
}



bool CBullet::calcVisibility()
{
    return true;
}


void CBullet::process()
{    
	if( !getActionNumber(A_KEENSHOT_IMPACT) && (blockedd || blockedu || blockedl || blockedr || onslope) )
	{
		setAction(A_KEENSHOT_IMPACT);
		playSound( SOUND_SHOT_HIT );
		mIsDead = true;
	}

	if(!processActionRoutine())
    {
			exists = false;
    }
}

}
