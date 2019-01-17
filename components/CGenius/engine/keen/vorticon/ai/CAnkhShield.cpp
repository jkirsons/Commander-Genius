/*
 * CAnkhShield.cpp
 *
 *  Created on: 16.07.2010
 *      Author: gerstrong
 */

#include "CAnkhShield.h"
#include "engine/core/spritedefines.h"

const unsigned int ANKH_FLICKER_DELAY = 3;
const unsigned int ANKH_SHIELD_FRAME = 61;

CAnkhShield::CAnkhShield(CPlayer &attachedPlayer) :
CVorticonSpriteObject(attachedPlayer.getMapPtr(),
		attachedPlayer.getXPosition(),
		attachedPlayer.getYPosition(),
        OBJ_ANKHSHIELD, 0),
m_attachedPlayer(attachedPlayer),
ankhtime(attachedPlayer.ankhtime)
{
	ankhtime=PLAY_ANKH_TIME;
	frame = 0;
	timer = 0;
	inhibitfall = true;
	canbezapped = false;
	state = ANKH_STATE_NOFLICKER;
	mSpriteIdx = ANKH_SHIELD_FRAME;
}

void CAnkhShield::process()
{
    if(m_attachedPlayer.ankhmode)
		ankhtime=PLAY_ANKH_TIME;

	if (!ankhtime)
	{
		exists = false;
		return;
	}
	else if (ankhtime < ANKH_STAGE3_TIME)
		state = ANKH_STATE_FLICKERSLOW;
	else if (ankhtime < ANKH_STAGE2_TIME)
		state = ANKH_STATE_FLICKERFAST;
	else
		state = ANKH_STATE_NOFLICKER;

	ankhtime--;

	moveToForce(m_attachedPlayer.getXPosition()-(8<<STC), m_attachedPlayer.getYPosition()-(8<<STC));

	switch(state)
	{
	case ANKH_STATE_NOFLICKER:
		mSpriteIdx = ANKH_SHIELD_FRAME + (frame&1);
		break;
	case ANKH_STATE_FLICKERFAST:
		if (frame&1)
			mSpriteIdx = BLANKSPRITE;
		else
		{
			if (frame&2)
				mSpriteIdx = ANKH_SHIELD_FRAME+1;
			else
				mSpriteIdx = ANKH_SHIELD_FRAME;
		}
		break;
	case ANKH_STATE_FLICKERSLOW:
		if (frame>4)
			mSpriteIdx = BLANKSPRITE;
		else
			mSpriteIdx = ANKH_SHIELD_FRAME;
		break;
	}

	if (timer > ANKH_FLICKER_DELAY)
	{
		frame++;
		if (frame>8) frame = 0;
		timer = 0;
	}
	else timer++;
}
