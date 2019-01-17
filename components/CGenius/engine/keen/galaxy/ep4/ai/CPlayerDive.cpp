/*
 * CPlayerDive.cpp
 *
 *  Created on: 19.06.2011
 *      Author: gerstrong
 */

#include "CPlayerDive.h"
#include <base/CInput.h>
#include "CBubbles.h"


namespace galaxy {

const int A_KEENSWIM_MOVE = 0;


CPlayerDive::CPlayerDive(CMap *pmap,
		const Uint16 foeID,
		Uint32 x,
		Uint32 y,
		direction_t facedir,
        CInventory &l_Inventory,
        const int playerID,
        const int spriteVar) :
CPlayerBase(pmap, foeID, x, y,
		facedir,
		l_Inventory,        
        playerID, spriteVar),
m_swimupspeed(0),
m_breathtimer(0),
mDidSwimUp(false)
{
    mActionMap[A_KEENSWIM_MOVE] = static_cast<void (CPlayerBase::*)()>(&CPlayerDive::processDiving);

	setupGalaxyObjectOnMap(0x19EC, A_KEENSWIM_MOVE);
}

const int DIE_FALL_MAX_INERTIA = 150;

void CPlayerDive::kill(const bool force,
                       const bool )
{
	// Here were prepare Keen to die, setting the action to die
    if(!gBehaviorEngine.mCheatmode.god || force)
	{
		if(mp_processState == &CPlayerBase::processDying && yinertia < 0)
			return;

		gSound.playSound( SOUND_KEEN_DIE, SoundPlayMode::PLAY_NORESTART );
		setupGalaxyObjectOnMap(0x0D2E, (rand()%2));

        if(!mDying)
            m_Inventory.Item.m_lifes--;

		mDying = true;                
		yinertia = -DIE_FALL_MAX_INERTIA;
		solid = false;
		honorPriority = false;
		mp_processState = &CPlayerBase::processDying;
	}
}


const int MAXMOVESPEED = 20;
const int MOVESPEED = 30;
const int WATERFALLSPEED = 10;
const int BREATH_TIME = 60;

void CPlayerDive::processDiving()
{
	// In case no-clipping was triggered, make it solid, or remove it...
    if(gBehaviorEngine.mCheatmode.noclipping)
	{
		solid = !solid;
        gBehaviorEngine.mCheatmode.noclipping = false;
	}


    // If Released set to false
    if(!mPlaycontrol[PA_JUMP])
        mDidSwimUp = false;

	// If Player presses Jump button, make Keen swim faster
    if(mPlaycontrol[PA_JUMP] && !mDidSwimUp)
	{
		// Animate more Keen when pressing Jump button
		if(getActionNumber(A_KEENSWIM_MOVE))
			setActionForce(A_KEENSWIM_MOVE+1);
		else
			setActionForce(A_KEENSWIM_MOVE);

		if(m_swimupspeed<MAXMOVESPEED)
			m_swimupspeed = MAXMOVESPEED;

        mDidSwimUp = true;
	}

    const int pax = mPlaycontrol[PA_X];
    const int pay = mPlaycontrol[PA_Y];

    if( pax != 0 )
    {
        // but y movement, set direction to zero
        if(pay == 0)
            yDirection = 0;

        // Swimming - Left and Right
        if(pax < 0)
        {
            moveLeft(MOVESPEED+m_swimupspeed);
            xDirection = LEFT;
        }
        else if(pax > 0)
        {
            moveRight(MOVESPEED+m_swimupspeed);
            xDirection = RIGHT;
        }
    }


	// Up and down swimming
    if(mPlaycontrol[PA_Y] < 0)
	{
		moveUp(MOVESPEED+m_swimupspeed);
		yDirection = UP;
	}
	
    else if(mPlaycontrol[PA_Y] > 0)
	{
		moveDown(MOVESPEED+m_swimupspeed);
		yDirection = DOWN;
	}
	else
	{
		moveDown(WATERFALLSPEED+m_swimupspeed);
		yDirection = DOWN;
	}


	// Slow down the swim speed, by time
	if(m_swimupspeed>0)
	{
		m_swimupspeed--;
		if(m_swimupspeed<0)
			m_swimupspeed = 0;
	}

	if( m_breathtimer >= BREATH_TIME )
	{
		playSound(SOUND_BUBBLE);
		int dir_offset = (xDirection==RIGHT) ? +(1<<CSF) : -(1<<CSF) ;
		CBubbles *Bubble = new CBubbles(mpMap, 0, getXMidPos()+dir_offset, getYMidPos(), false);
		gEventManager.add( new EventSpawnObject( Bubble ) );
		m_breathtimer = 0;
	}
	else
		m_breathtimer++;

    processCamera();
}

void CPlayerDive::process()
{    
    processInput();

    if(!mDying && !mExitTouched)
    {
        processExiting();
    }

	(this->*mp_processState)();

	processLevelMiscFlagsCheck();

	if(!processActionRoutine())
			exists = false;
}

}
