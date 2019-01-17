/* Keen Dreams Source Code
 * Copyright (C) 2014 Javier M. Chavez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// KD_PLAY.C

#include <base/CInput.h>

#include "engine/keen/dreams/dreamsengine.h"
#include "engine/keen/dreams/dreamsgameplay.h"
#include "engine/keen/dreams/dreamscontrolpanel.h"
#include "engine/keen/dreams/dreamsintro.h"

extern mapfiletype_modern  mapFile;

extern "C"
{


#include "kd_def.h"
//#pragma	hdrstop

/*
=============================================================================

						 LOCAL CONSTANTS

=============================================================================
*/

#define	INACTIVATEDIST	10

#define MAXMOVE	(TILEGLOBAL-17)

#define NUMLUMPS	22


#define CONTROLSLUMP	0
#define KEENLUMP		1
#define WORLDKEENLUMP	2
#define BROCCOLUMP		3
#define	TOMATLUMP       4
#define	CARROTLUMP		5
#define	ASPARLUMP		6
#define	GRAPELUMP		7
#define	TATERLUMP		8
#define	CARTLUMP		9
#define	FRENCHYLUMP		10
#define	MELONLUMP		11
#define	SQUASHLUMP		12
#define	APELLUMP		13
#define	PEALUMP			14
#define	BOOBUSLUMP		15

/*
=============================================================================

						 GLOBAL VARIABLES

=============================================================================
*/

exittype	playstate;
gametype	gamestate;

id0_boolean_t		button0held,button1held;
// (REFKEEN) new has been renamed newobj since new is a reserved C++ keyword
objtype		*newobj,*check,*player,*scoreobj;

id0_unsigned_t	originxtilemax,originytilemax;

ControlInfo	c;

objtype dummyobj;

// Set by the calle and changed, when a event forces to close the app.
extern int gDreamsForceClose;

#ifdef REFKEEN_VER_KDREAMS_CGA_ALL
const id0_char_t		*levelnames[21] =
{
"The Land of Tuberia",
"Horseradish Hill",
"The Melon Mines",
"Bridge Bottoms",
"Rhubarb Rapids",
"Parsnip Pass",
"Cheat Level 1",
"Spud City",
"Cheat Level 2",
"Apple Acres",
"Grape Grove",
"Cheat Level 3",
"Brussels Sprout Bay",LastScan
"Cheat Level 4",
"Squash Swamp",
"Boobus' Chamber",
"Castle Tuberia",
"",
"Title Page"
};
#elif defined REFKEEN_VER_KDREAMS_ANYEGA_ALL
const id0_char_t		*levelnames[21] =
{
"The Land of Tuberia",
"Horseradish Hill",
"The Melon Mines",
"Bridge Bottoms",
"Rhubarb Rapids",
"Parsnip Pass",
"Level 6",
"Spud City",
"Level 8",
"Apple Acres",
"Grape Grove",
"Level 11",
"Brussels Sprout Bay",
"Level 13",
"Squash Swamp",
"Boobus' Chamber",
"Castle Tuberia",
"",
"Title Page"
};
#endif


/*
=============================================================================

						 LOCAL VARIABLES

=============================================================================
*/

// for asm scaning of map planes (UPDATE (REFKEEN): Code ported to C)
id0_unsigned_t	mapx,mapy/*,mapxcount,mapycount*/,maptile/*,mapspot*/;

id0_int_t			plummet;

id0_int_t			objectcount;

objtype		objarray[MAXACTORS],*lastobj,*objfreelist;

id0_int_t			oldtileleft,oldtiletop,oldtileright,oldtilebottom,oldtilemidx;
id0_int_t			oldleft,oldtop,oldright,oldbottom,oldmidx;
id0_int_t			leftmoved,topmoved,rightmoved,bottommoved,midxmoved;

id0_int_t			topmove,bottommove,midxmove;

id0_int_t			inactivateleft,inactivateright,inactivatetop,inactivatebottom;

id0_int_t			fadecount;

id0_boolean_t		bombspresent;
id0_int_t           bombsleftinlevel = 0;

id0_boolean_t		openedStatusWindow;

id0_boolean_t		lumpneeded[NUMLUMPS];
id0_int_t			lumpstart[NUMLUMPS] =
{
CONTROLS_LUMP_START,
KEEN_LUMP_START,
WORLDKEEN_LUMP_START,
BROCCOLASH_LUMP_START,
TOMATO_LUMP_START,
CARROT_LUMP_START,
ASPAR_LUMP_START,
GRAPE_LUMP_START,
TATER_LUMP_START,
CANTA_LUMP_START,
FRENCHY_LUMP_START,
MELONLIPS_LUMP_START,
SQUASHER_LUMP_START,
APEL_LUMP_START,
PEAS_LUMP_START,
BOOBUS_LUMP_START,
};

id0_int_t			lumpend[NUMLUMPS] =
{
CONTROLS_LUMP_END,
KEEN_LUMP_END,
WORLDKEEN_LUMP_END,
BROCCOLASH_LUMP_END,
TOMATO_LUMP_END,
CARROT_LUMP_END,
ASPAR_LUMP_END,
GRAPE_LUMP_END,
TATER_LUMP_END,
CANTA_LUMP_END,
FRENCHY_LUMP_END,
MELONLIPS_LUMP_END,
SQUASHER_LUMP_END,
APEL_LUMP_END,
PEAS_LUMP_END,
BOOBUS_LUMP_END,
};


void	CheckKeys (void);
void	CalcInactivate (void);
void 	InitObjArray (void);
void 	GetNewObj (id0_boolean_t usedummy);
void	RemoveObj (objtype *gone);
void 	ScanInfoPlane (void);
void 	PatchWorldMap (void);
void 	MarkTileGraphics (void);
void 	FadeAndUnhook (void);
void 	SetupGameLevel (id0_boolean_t loadnow);
void 	ScrollScreen (void);
void 	MoveObjVert (objtype *ob, id0_int_t ymove);
void 	MoveObjHoriz (objtype *ob, id0_int_t xmove);
void 	GivePoints (id0_unsigned_t points);
void 	ClipToEnds (objtype *ob);
void 	ClipToEastWalls (objtype *ob);
void 	ClipToWestWalls (objtype *ob);
void 	ClipToWalls (objtype *ob);
void 	ClipToSpriteSide (objtype *push, objtype *solid);
void 	ClipToSprite (objtype *push, objtype *solid, id0_boolean_t squish);
id0_int_t 	DoActor (objtype *ob,id0_int_t tics);
void 	StateMachine (objtype *ob);
void 	NewState (objtype *ob,statetype *state);
void 	PlayLoop ();
void 	GameLoop (void);

//===========================================================================

/*
=====================
=
= CheckKeys
=
=====================
*/

void CheckKeys (void)
{
	if (screenfaded)			// don't do anything with a faded screen
		return;

    CInput &input = gInput;


//
// space for status screen
//
    if (Keyboard[sc_Space])
	{
        openedStatusWindow = true;
		IN_ClearKeysDown();
		RF_ForceRefresh();
		lasttimecount = SD_GetTimeCount();
	}

//
// pause key wierdness can't be checked as a scan code
//
	if (Paused)
	{
		VW_FixRefreshBuffer ();
		US_CenterWindow (8,3);
		US_PrintCentered ("PAUSED");
		VW_UpdateScreen ();
        //IN_Ack();
		RF_ForceRefresh ();
		Paused = false;
	}

//
// F1-F7/ESC to enter control panel
//    
    if ( (LastScan >= sc_F1 && LastScan <= sc_F7) || input.getPressedCommand(IC_BACK) )
	{
		VW_FixRefreshBuffer ();
		US_CenterWindow (20,8);
		US_CPrint ("Loading");
		VW_UpdateScreen ();

        gEventManager.add( new dreams::LaunchControlPanel );
        /*US_ControlPanel_Init();
        US_ControlPanel_Ponder();*/
        return;

        if(gDreamsForceClose)
            return;

		// REFKEEN - Alternative controllers support (maybe user has changed some keys which may currently have an effect)
        //BE_ST_AltControlScheme_PrepareInGameControls(KbdDefs[0].button0, KbdDefs[0].button1, KbdDefs[0].up, KbdDefs[0].down, KbdDefs[0].left, KbdDefs[0].right);
		//
		IN_ClearKeysDown();
		if (restartgame)
			playstate = resetgame;
		else if (!loadedgame)
			RF_ForceRefresh();		// don't refresh if loading a new game

		lasttimecount = SD_GetTimeCount();
	}

//
// F10-? debug keys
//
	if (Keyboard[sc_F10] && DebugKeys() )
	{
		RF_ForceRefresh();
		lasttimecount = SD_GetTimeCount();
	}

}

//===========================================================================


/*
=======================
=
= CalcInactivate
=
=======================
*/

void CalcInactivate (void)
{
	originxtilemax = originxtile+PORTTILESWIDE-1;
	originytilemax = originytile+PORTTILESHIGH-1;

	inactivateleft = originxtile-INACTIVATEDIST;
	if (inactivateleft < 0)
		inactivateleft = 0;
	inactivateright = originxtilemax+INACTIVATEDIST;
	inactivatetop = originytile-INACTIVATEDIST;
	if (inactivatetop < 0)
		inactivatetop = 0;
	inactivatebottom = originytilemax+INACTIVATEDIST;
}


//===========================================================================


/*
#############################################################################

				  The objarray data structure

#############################################################################

Objarray containt structures for every actor currently playing.  The structure
is accessed as a linked list starting at *player, ending when ob->next ==
NULL.  GetNewObj inserts a new object at the end of the list, meaning that
if an actor spawn another actor, the new one WILL get to think and react the
same frame.  RemoveObj unlinks the given object and returns it to the free
list, but does not damage the objects ->next pointer, so if the current object
removes itself, a linked list following loop can still safely get to the
next element.

<backwardly linked free list>

#############################################################################
*/


/*
=========================
=
= InitObjArray
=
= Call to clear out the entire object list, returning them all to the free
= list.  Allocates a special spot for the player.
=
=========================
*/

void InitObjArray (void)
{
	id0_int_t	i;

	for (i=0;i<MAXACTORS;i++)
	{
		objarray[i].prev = &objarray[i+1];
		objarray[i].next = NULL;
	}

	objarray[MAXACTORS-1].prev = NULL;

	objfreelist = &objarray[0];
	lastobj = NULL;

	objectcount = 0;

//
// give the player and score the first free spots
//
	GetNewObj (false);
	player = newobj;
	GetNewObj (false);
	scoreobj = newobj;
}

//===========================================================================

/*
=========================
=
= GetNewObj
=
= Sets the global variable newobj to point to a free spot in objarray.
= The free spot is inserted at the end of the liked list
=
= When the object list is full, the caller can either have it bomb out ot
= return a dummy object pointer that will never get used
=
=========================
*/

void GetNewObj (id0_boolean_t usedummy)
{
	if (!objfreelist)
	{
		if (usedummy)
		{
			newobj = &dummyobj;
			return;
		}
		Quit ("GetNewObj: No free spots in objarray!");
	}

	newobj = objfreelist;
	objfreelist = newobj->prev;
	memset (newobj,0,sizeof(*newobj));

	if (lastobj)
		lastobj->next = newobj;
	newobj->prev = lastobj;	// newobj->next is already NULL from memset

	newobj->active = yes;
	newobj->needtoclip = true;
	lastobj = newobj;

	objectcount++;
}

//===========================================================================

/*
=========================
=
= RemoveObj
=
= Add the given object back into the free list, and unlink it from it's
= neighbors
=
=========================
*/

void RemoveObj (objtype *gone)
{
	if (gone == player)
		Quit ("RemoveObj: Tried to remove the player!");

//
// erase it from the refresh manager
//
	RF_RemoveSprite (&gone->sprite);

//
// fix the next object's back link
//
	if (gone == lastobj)
		lastobj = (objtype *)gone->prev;
	else
		gone->next->prev = gone->prev;

//
// fix the previous object's forward link
//
	gone->prev->next = gone->next;

//
// add it back in to the free list
//
	gone->prev = objfreelist;
	objfreelist = gone;
}

//===========================================================================


void /*near*/ HandleInfo (void)
{
	switch (maptile)
	{
	case 1:
		SpawnKeen(mapx,mapy,1);
		break;
	case 2:
		SpawnKeen(mapx,mapy,-1);
		break;
	case 19:
		SpawnWorldKeen(mapx,mapy);
		lumpneeded[WORLDKEENLUMP] = true;
		break;

	case 31:
        bombsleftinlevel = 3;
		bombspresent = true;
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 32:
		SpawnBonus(mapx,mapy,maptile-21);
		newobj->active = no/*false*/;
		break;
	case 33:
		SpawnDoor(mapx,mapy);
		newobj->active = no/*false*/;
		break;
	case 41:
		SpawnBrocco(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[BROCCOLUMP] = true;
		break;
	case 42:
		SpawnTomat(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[TOMATLUMP] = true;
		break;
	case 43:
		SpawnCarrot(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[CARROTLUMP] = true;
		break;
	case 45:
		SpawnAspar(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[ASPARLUMP] = true;
		break;
	case 46:
		SpawnGrape(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[GRAPELUMP] = true;
		break;
	case 47:
		SpawnTater(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[TATERLUMP] = true;
		break;
	case 48:
		SpawnCart(mapx,mapy);
		lumpneeded[CARTLUMP] = true;
		break;
	case 49:
		SpawnFrenchy(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[FRENCHYLUMP] = true;
		break;
	case 50:
	case 51:
	case 52:
		SpawnMelon(mapx,mapy,maptile-50);
		newobj->active = no/*false*/;
		lumpneeded[MELONLUMP] = true;
		break;
	case 57:
		SpawnSquasher(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[SQUASHLUMP] = true;
		break;
	case 58:
		SpawnApel(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[APELLUMP] = true;
		break;
	case 59:
		SpawnPeaPod(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[PEALUMP] = true;
		break;
	case 60:
		SpawnPeaBrain(mapx,mapy);
		newobj->active = no/*false*/;
		lumpneeded[PEALUMP] = true;
		break;
	case 61:
		SpawnBoobus(mapx,mapy);
		lumpneeded[BOOBUSLUMP] = true;
		break;
	}

	if (newobj->active != allways)
		newobj->active = no/*false*/;
}

/*
==========================
=
= ScanInfoPlane
=
= Spawn all actors and mark down special places
=
==========================
*/

void ScanInfoPlane (void)
{
	id0_unsigned_t	/*x,y,*/i,j;
	id0_int_t			tile;
	id0_unsigned_t	id0_far	*start;

	InitObjArray();			// start spawning things with a clean slate

	memset (lumpneeded,0,sizeof(lumpneeded));

	// Ported from ASM

	start = mapsegs[2];
	for (mapy/*y*/=0;mapy/*y*/<mapheight;mapy/*y*/++)
	{
		for (mapx/*x*/=0;mapx/*x*/<mapwidth;mapx/*x*/++)
		{
			tile = *(start++);
			if (tile)
			{
				maptile = tile;
				HandleInfo();
			}
		}
	}

	for (i=0;i<NUMLUMPS;i++)
		if (lumpneeded[i])
			for (j=lumpstart[i];j<=lumpend[i];j++)
				CA_MarkGrChunk(j);
}

//===========================================================================


/*
==========================
=
= PatchWorldMap
=
= Takes out blocking squares and puts in dones
=
==========================
*/

void PatchWorldMap (void)
{
	id0_unsigned_t	size,spot,info,foreground;

	size = mapwidth*mapheight;
	spot = 0;
	do
	{
		info = *(mapsegs[2] + spot);
		// finished a city here?
		if (info>=3 && info<=18 && gamestate.leveldone[info-2])
		{
            if(bombsleftinlevel <= 0)
            {
                *(mapsegs[2] + spot) = 0;

                foreground = *(mapsegs[1] + spot);
                if (foreground == 130)
                    *(mapsegs[1]+spot) = 0;	// not blocking now
                else if (foreground == 90)
                {
                    // plant done flag
                    *(mapsegs[1]+spot) = 133;
                    *(mapsegs[1]+(spot-mapwidth-1)) = 131;
                    *(mapsegs[1]+(spot-mapwidth)) = 132;
                }
            }
        }
		spot++;
	} while (spot<size);

    bombsleftinlevel = 0;
}

//===========================================================================

/*
==========================
=
= FadeAndUnhook
=
= Latch this onto the refresh so the screen only gets faded in after two
= refreshes.  This lets all actors draw themselves to both pages before
= fading the screen in.
=
==========================
*/

void FadeAndUnhook (void)
{
	if (++fadecount==2)
	{
#ifndef REFKEEN_VER_KDREAMS_CGA_ALL
		RF_ForceRefresh();
#endif
		VW_FadeIn ();
		RF_SetRefreshHook (NULL);
		lasttimecount = SD_GetTimeCount();	// don't adaptively time the fade
	}
}

//===========================================================================


/*
==========================
=
= SetupGameLevel
=
= Load in map mapon and cache everything needed for it
=
==========================
*/

void 	SetupGameLevel (id0_boolean_t loadnow)
{
	id0_long_t	orgx,orgy;

	bombspresent = false;    
//
// load the level header and three map planes
//
	CA_CacheMap (gamestate.mapon);

//
// let the refresh manager set up some variables
//
	RF_NewMap ();

//
// decide which graphics are needed and spawn actors
//
	CA_ClearMarks ();

	if (!mapon)
		PatchWorldMap ();

	if (mapon!=20)			// map 20 is the title screen
		ScanInfoPlane ();
	RF_MarkTileGraphics ();

//
// have the caching manager load and purge stuff to make sure all marks
// are in memory
//
	if (loadnow)
	{
		if (bombspresent)
		{
			VW_FixRefreshBuffer ();
			US_DrawWindow (10,1,20,2);
			US_PrintCentered ("Boobus Bombs Near!");
			VW_UpdateScreen ();
		}
#ifdef REFKEEN_VER_KDREAMS_CGA_ALL
		CA_CacheMarks (levelnames[mapon]);
#elif defined REFKEEN_VER_KDREAMS_ANYEGA_ALL
		CA_CacheMarks (levelnames[mapon], 0);
#endif
	}

#if 0
	VW_FixRefreshBuffer ();
	US_CenterWindow (20,8);
	US_Print ("\n\n\nObject count:");
	itoa (objectcount,str,10);
	US_Print (str);
	VW_UpdateScreen ();
	IN_Ack ();
#endif

	if (mapon!=20 && loadnow)			// map 20 is the title screen
	{
		VW_FadeOut ();
		fadecount = 0;
		RF_SetRefreshHook (&FadeAndUnhook);
		SpawnScore ();

//
// start the initial view position to center the player
//
		orgx = (id0_long_t)player->x - (150<<G_P_SHIFT);
		orgy = (id0_long_t)player->y-(84<<G_P_SHIFT);
		if (orgx<0)
			orgx=0;
		if (orgy<0)
			orgy=0;

		RF_NewPosition (orgx,orgy);
		CalcInactivate ();
	}


}

//==========================================================================

/*
===============
=
= ScrollScreen
=
= Scroll if Keen is nearing an edge
= Set playstate to levelcomplete
=
===============
*/

void ScrollScreen (void)
{
	id0_int_t	xscroll,yscroll;

//
// walked off edge of map?
//
	if (player->left < originxmin
	|| player->right > originxmax+20*TILEGLOBAL)
	{
		playstate = levelcomplete;
        gEventManager.add( new dreams::CompleteLevel );
		return;
	}

//
// fallen off bottom of world?
//
	if (!plummet && player->bottom > originymax+13*TILEGLOBAL)
	{
		godmode = 0;
		plummet = 1;
		KillKeen ();
		return;
	}

	if (player->x < originxglobal+SCROLLWEST)
		xscroll = player->x - (originxglobal+SCROLLWEST);
	else if (player->x > originxglobal+SCROLLEAST)
		xscroll = player->x - (originxglobal+SCROLLEAST);
	else
		xscroll = 0;

	if (player->y < originyglobal+SCROLLNORTH)
		yscroll = player->y - (originyglobal+SCROLLNORTH);
	else if (player->y > originyglobal+SCROLLSOUTH)
		yscroll = player->y - (originyglobal+SCROLLSOUTH);
	else yscroll = 0;

	if (xscroll || yscroll)
	{
		RF_Scroll (xscroll,yscroll);
		CalcInactivate ();
		scoreobj->needtoreact = true;
	}
}

//==========================================================================

/*
====================
=
= GivePoints
=
= Grants extra men at 20k,40k,80k,160k,320k
=
====================
*/

void GivePoints (id0_unsigned_t points)
{
	gamestate.score += points;
	if (gamestate.score >= gamestate.nextextra)
	{
		SD_PlaySound (EXTRAKEENSND);
		gamestate.lives++;
		gamestate.nextextra*=2;
	}
}


//==========================================================================

/*
====================
=
= MoveObjVert
=
====================
*/

void MoveObjVert (objtype *ob, id0_int_t ymove)
{
	ob->y += ymove;
	ob->top += ymove;
	ob->bottom += ymove;
	ob->tiletop = ob->top >> G_T_SHIFT;
	ob->tilebottom = ob->bottom >> G_T_SHIFT;
}


/*
====================
=
= MoveObjHoriz
=
====================
*/

void MoveObjHoriz (objtype *ob, id0_int_t xmove)
{
	ob->x += xmove;
	ob->left += xmove;
	ob->right += xmove;
	ob->tileleft = ob->left >> G_T_SHIFT;
	ob->tileright = ob->right >> G_T_SHIFT;
}


/*
=============================================================================

					Actor to tile clipping rouitnes

=============================================================================
*/

// walltype / x coordinate (0-15)

id0_int_t	wallclip[8][16] = {			// the height of a given point in a tile
{ 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256},
{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
{   0,0x08,0x10,0x18,0x20,0x28,0x30,0x38,0x40,0x48,0x50,0x58,0x60,0x68,0x70,0x78},
{0x80,0x88,0x90,0x98,0xa0,0xa8,0xb0,0xb8,0xc0,0xc8,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8},
{   0,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0},
{0x78,0x70,0x68,0x60,0x58,0x50,0x48,0x40,0x38,0x30,0x28,0x20,0x18,0x10,0x08,   0},
{0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x88,0x80},
{0xf0,0xe0,0xd0,0xc0,0xb0,0xa0,0x90,0x80,0x70,0x60,0x50,0x40,0x30,0x20,0x10,   0}
};

// assignment within ifs are used heavily here, so turn off the warning
//#pragma warn -pia

/*
===========================
=
= ClipToEnds
=
===========================
*/

void ClipToEnds (objtype *ob)
{
    id0_unsigned_t	id0_far *map/*,tile,facetile,info*/;
	//id0_int_t	leftpix,rightpix,midtiles,toppix,bottompix;
	id0_int_t	/*x,*/y,clip,move,totalmove,maxmove,midxpix;

	midxpix = (ob->midx&0xf0) >> 4;

	maxmove = -abs(midxmoved) - bottommoved - 16;
	map = (id0_unsigned_t id0_far *)mapsegs[1] +
		mapbwidthtable[oldtilebottom-1]/2 + ob->tilemidx;
	// (REFKEEN) Comparison is unsigned in vanilla Keen (has an effect on the explosion of King Boobus Tuber)
	for (y=oldtilebottom-1 ; (id0_unsigned_t)y<=ob->tilebottom ; y++,map+=mapwidth)
    {
        const auto wall = mapFile.tileinfo[NORTHWALL+*map];

        if (wall)
		{
			clip = wallclip[wall&7][midxpix];
			move = ( (y<<G_T_SHIFT)+clip - 1) - ob->bottom;
			if (move<0 && move>=maxmove)
			{
				ob->hitnorth = wall;
				MoveObjVert (ob,move);
				return;
			}
		}
	}

	maxmove = abs(midxmoved) - topmoved + 16;
	map = (id0_unsigned_t id0_far *)mapsegs[1] +
		mapbwidthtable[oldtiletop+1]/2 + ob->tilemidx;
	// (REFKEEN) Again comparison should be unsigned
	for (y=oldtiletop+1 ; (id0_unsigned_t)y>=ob->tiletop ; y--,map-=mapwidth)
	{
        const auto wall = mapFile.tileinfo[SOUTHWALL+*map];

        if (wall)
		{
			clip = wallclip[wall&7][midxpix];
			move = ( ((y+1)<<G_T_SHIFT)-clip ) - ob->top;
			if (move > 0 && move<=maxmove)
			{
				totalmove = ob->ymove + move;
				if (totalmove < TILEGLOBAL && totalmove > -TILEGLOBAL)
				{
					ob->hitsouth = wall;
					MoveObjVert (ob,move);
				}
			}
		}
	}
}


/*
===========================
=
= ClipToEastWalls / ClipToWestWalls
=
===========================
*/

void ClipToEastWalls (objtype *ob)
{
	id0_int_t			y,move,top,bottom;
	id0_unsigned_t	id0_far *map/*,tile,info,wall*/;

	// clip to east walls if moving west

	top = ob->tiletop;
	if (ob->hitsouth>1)
		top++;			// on a slope inside a tile
	bottom = ob->tilebottom;
	if (ob->hitnorth>1)
		bottom--;			// on a slope inside a tile

	for (y=top;y<=bottom;y++)
	{
		map = (id0_unsigned_t id0_far *)mapsegs[1] +
			mapbwidthtable[y]/2 + ob->tileleft;

        ob->hiteast = mapFile.tileinfo[EASTWALL+*map];

        if (ob->hiteast)
		{
			move = ( (ob->tileleft+1)<<G_T_SHIFT ) - ob->left;
			MoveObjHoriz (ob,move);
			return;
		}
	}
}


void ClipToWestWalls (objtype *ob)
{
	id0_int_t			y,move,top,bottom;
	id0_unsigned_t	id0_far *map/*,tile,info,wall*/;

	// check west walls if moving east

	top = ob->tiletop;
	if (ob->hitsouth>1)
		top++;			// on a slope inside a tile
	bottom = ob->tilebottom;
	if (ob->hitnorth>1)
		bottom--;			// on a slope inside a tile

	for (y=top;y<=bottom;y++)
	{
		map = (id0_unsigned_t id0_far *)mapsegs[1] +
			mapbwidthtable[y]/2 + ob->tileright;

        ob->hitwest = mapFile.tileinfo[WESTWALL+*map];

        if (ob->hitwest)
		{
			move = ( (ob->tileright<<G_T_SHIFT ) -1) - ob->right;
			MoveObjHoriz (ob,move);
			return;
		}
	}
}

// turn 'possibly incorrect assignment' warnings back on
//#pragma warn +pia


//==========================================================================

/*
================
=
= ClipToWalls
=
= Moves the current object xmove/ymove units, clipping to walls
=
================
*/

void ClipToWalls (objtype *ob)
{
	//id0_unsigned_t	x,y,tile;
	spritetabletype	id0_far *shape;
	//id0_boolean_t	endfirst;

//
// make sure it stays in contact with a 45 degree slope
//
	if (ob->state->pushtofloor)
	{
		if (ob->xmove > 0)
			ob->ymove = ob->xmove + 16;
		else
			ob->ymove = -ob->xmove + 16;
	}

//
// move the shape
//
	if (ob->xmove > MAXMOVE)
		ob->xmove = MAXMOVE;
	else if (ob->xmove < -MAXMOVE)
		ob->xmove = -MAXMOVE;

	if (ob->ymove > MAXMOVE+16)			// +16 for push to floor
		ob->ymove = MAXMOVE+16;
	else if (ob->ymove < -MAXMOVE)
		ob->ymove = -MAXMOVE;

	ob->x += ob->xmove;
	ob->y += ob->ymove;

	ob->needtoreact = true;

	if (!ob->shapenum)				// can't get a hit rect with no shape!
		return;

	shape = &spritetable[ob->shapenum-STARTSPRITES];

	oldtileright = ob->tileright;
	oldtiletop = ob->tiletop;
	oldtileleft = ob->tileleft;
	oldtilebottom = ob->tilebottom;
	oldtilemidx = ob->tilemidx;

	oldright = ob->right;
	oldtop = ob->top;
	oldleft = ob->left;
	oldbottom = ob->bottom;
	oldmidx = ob->midx;

	ob->left = ob->x + shape->xl;
	ob->right = ob->x + shape->xh;
	ob->top = ob->y + shape->yl;
	ob->bottom = ob->y + shape->yh;
	ob->midx = ob->left + (ob->right - ob->left)/2;

	ob->tileleft = ob->left >> G_T_SHIFT;
	ob->tileright = ob->right >> G_T_SHIFT;
	ob->tiletop = ob->top >> G_T_SHIFT;
	ob->tilebottom = ob->bottom >> G_T_SHIFT;
	ob->tilemidx = ob->midx >> G_T_SHIFT;

	ob->hitnorth = ob->hiteast = ob->hitsouth = ob->hitwest = 0;

	if (!ob->needtoclip)
		return;

	leftmoved = ob->left - oldleft;
	rightmoved = ob->right - oldright;
	topmoved = ob->top - oldtop;
	bottommoved = ob->bottom - oldbottom;
	midxmoved = ob->midx - oldmidx;

//
// clip it
//

	ClipToEnds(ob);

	if (leftmoved < 0 || ob == player)	// make sure player gets cliped
		ClipToEastWalls (ob);
	if (rightmoved > 0 || ob == player)
		ClipToWestWalls (ob);
}

//==========================================================================


/*
==================
=
= ClipToSpriteSide
=
= Clips push to solid
=
==================
*/

void ClipToSpriteSide (objtype *push, objtype *solid)
{
	id0_int_t xmove,leftinto,rightinto;

	//
	// amount the push shape can be pushed
	//
	xmove = solid->xmove - push->xmove;

	//
	// amount it is inside
	//
	leftinto = solid->right - push->left;
	rightinto = push->right - solid->left;

	if (leftinto>0 && leftinto<= xmove)
	{
		push->xmove = leftinto;
		if (push->state->pushtofloor)
			push->ymove = leftinto+16;
		ClipToWalls (push);
		push->hiteast = 1;
		return;
	}

	if (rightinto>0 && rightinto<= -xmove)
	{
		push->xmove = -rightinto;
		if (push->state->pushtofloor)
			push->ymove = rightinto+16;
		ClipToWalls (push);
		push->hitwest = 1;
		return;
	}

}

//==========================================================================


/*
==================
=
= ClipToSprite
=
= Clips push to solid
=
==================
*/

void ClipToSprite (objtype *push, objtype *solid, id0_boolean_t squish)
{
	id0_boolean_t temp;
	id0_int_t walltemp,xmove,leftinto,rightinto,topinto,bottominto;

	xmove = solid->xmove - push->xmove;

	push->xmove = push->ymove = 0;

	//
	// left / right
	//
	leftinto = solid->right - push->left;
	rightinto = push->right - solid->left;

	if (leftinto>0 && leftinto<=xmove)
	{
		push->xmove = leftinto;
		walltemp = push->hitnorth;
		ClipToWalls (push);
		if (!push->hitnorth)
			push->hitnorth = walltemp;
		if (squish && push->hitwest)
			KillKeen ();
		push->hiteast = 1;
		return;
	}
	else if (rightinto>0 && rightinto<=-xmove)
	{
		push->xmove = -rightinto;
		walltemp = push->hitnorth;
		ClipToWalls (push);
		if (!push->hitnorth)
			push->hitnorth = walltemp;
		if (squish && push->hiteast)
			KillKeen ();
		push->hitwest = 1;
		return;
	}

	//
	// top / bottom
	//
	topinto = solid->bottom - push->top;
	bottominto = push->bottom - solid->top;

	if (bottominto>0)
	{
		push->ymove = -bottominto+16;
		push->xmove = solid->xmove;
		temp = push->state->pushtofloor;
		push->state->pushtofloor = false;
		walltemp = push->hitnorth;
		ClipToWalls (push);
		if (!push->hitnorth)
			push->hitnorth = walltemp;
		push->state->pushtofloor = temp;
		push->hitnorth = 25;
	}
	else if (topinto>0)
	{
		push->ymove = topinto;
		ClipToWalls (push);
		push->hitsouth = 25;
	}
}

//==========================================================================


/*
==================
=
= DoActor
=
= Moves an actor in its current state by a given number of tics.
= If that time takes it into the next state, it changes the state
= and returns the number of excess tics after the state change
=
==================
*/

id0_int_t DoActor (objtype *ob,id0_int_t tics)
{
	id0_int_t	newtics,movetics,excesstics;
	statetype *state;

	state = ob->state;

	// REFKEEN: Workaround for vanilla Keen Dreams bug (state may be 0)
	// Based on CGA v1.05 but it may be the same in other versions
	if (state && state->progress == think)
	//if (state->progress == think)
	{
		if (state->thinkptr)
		{
			if (ob->nothink)
				ob->nothink--;
			else
//#pragma warn -pro
				state->thinkptr(ob);
//#pragma warn +pro
		}
		return 0;
	}

	newtics = ob->ticcount+tics;

	// REFKEEN: Workaround for vanilla Keen Dreams bug (state may be 0)
	// FIXME: Hopefully this always works - Magic number is CGA v1.05 specific,
	// but it may actually work with other original versions
	if ((!state && newtics < 0x2064) || (state && (newtics < state->tictime || state->tictime == 0)))
	//if (newtics < state->tictime || state->tictime == 0)
	{
		ob->ticcount = newtics;
		// REFKEEN: Workaround for vanilla Keen Dreams bug (state may be 0)
		if (!state)
		{
			return 0; // Both cases appear to be skipped as of CGA v1.05
		}
		if (state->progress == slide || state->progress == slidethink)
		{
			if (ob->xdir)
				ob->xmove += ob->xdir == 1 ? tics*state->xmove
				: -tics*state->xmove;
			if (ob->ydir)
				ob->ymove += ob->ydir == 1 ? tics*state->ymove
				: -tics*state->ymove;
		}
		if (state->progress == slidethink || state->progress == stepthink)
		{
			if (state->thinkptr)
			{
				if (ob->nothink)
					ob->nothink--;
				else
//#pragma warn -pro
					state->thinkptr(ob);
//#pragma warn +pro
			}
		}
		return 0;
	}
	else
	{
		movetics = state->tictime - ob->ticcount;
		excesstics = newtics - state->tictime;
		ob->ticcount = 0;
		if (state->progress == slide || state->progress == slidethink)
		{
			if (ob->xdir)
				ob->xmove += ob->xdir == 1 ? movetics*state->xmove
				: -movetics*state->xmove;
			if (ob->ydir)
				ob->ymove += ob->ydir == 1 ? movetics*state->ymove
				: -movetics*state->ymove;
		}
		else
		{
			if (ob->xdir)
				ob->xmove += ob->xdir == 1 ? state->xmove : -state->xmove;
			if (ob->ydir)
				ob->ymove += ob->ydir == 1 ? state->ymove : -state->ymove;
		}

		if (state->thinkptr)
		{
			if (ob->nothink)
				ob->nothink--;
			else
//#pragma warn -pro
				state->thinkptr(ob);
//#pragma warn +pro
		}

		if (ob->state == state)
			ob->state = state->nextstate;	// go to next state
		else if (!ob->state)
			return 0;			// object removed itself
		return excesstics;
	}
}

//==========================================================================


/*
====================
=
= StateMachine
=
= Change state and give directions
=
====================
*/

void StateMachine (objtype *ob)
{
	id0_int_t excesstics,oldshapenum;
	statetype *state;

	ob->xmove = ob->ymove = 0;
	oldshapenum = ob->shapenum;

	state = ob->state;

	excesstics = DoActor(ob,tics);
	if (ob->state != state)
	{
		ob->ticcount = 0;		// start the new state at 0, then use excess
		state = ob->state;
	}

	while (excesstics)
	{
	//
	// passed through to next state
	//
		// REFKEEN: Workaround for vanilla Keen Dreams CGA v1.05 bug (state may be 0)
		if (state && !state->skippable && excesstics >= state->tictime)
		//if (!state->skippable && excesstics >= state->tictime)
			excesstics = DoActor(ob,state->tictime-1);
		else
			excesstics = DoActor(ob,excesstics);
		if (ob->state != state)
		{
			ob->ticcount = 0;		// start the new state at 0, then use excess
			state = ob->state;
		}
	}

	if (!state)			// object removed itself
	{
		RemoveObj (ob);
		return;
	}


	//
	// if state->rightshapenum == NULL, the state does not have a standard
	// shape (the think routine should have set it)
	//
	if (state->rightshapenum)
	{
		if (ob->xdir>0)
			ob->shapenum = state->rightshapenum;
		else
			ob->shapenum = state->leftshapenum;
	}
	if (ob->shapenum == (id0_unsigned_t)-1)
		ob->shapenum = 0;		// make it invisable this time

	if (ob->xmove || ob->ymove || ob->shapenum != oldshapenum)
	{
	//
	// actor moved or changed shape
	// make sure the movement is within limits (one tile)
	//
		ClipToWalls (ob);
	}
}

//==========================================================================


/*
====================
=
= NewState
=
====================
*/

void NewState (objtype *ob,statetype *state)
{
	id0_boolean_t temp;

	ob->state = state;

	if (state->rightshapenum)
	{
		if (ob->xdir>0)
			ob->shapenum = state->rightshapenum;
		else
			ob->shapenum = state->leftshapenum;
	}

	temp = ob->needtoclip;

	ob->needtoclip = false;

	ClipToWalls (ob);					// just calculate values

	ob->needtoclip = temp;

	if (ob->needtoclip)
		ClipToWalls (ob);

}

//==========================================================================

}

/*
============================
=
= PlayLoop
=
============================
*/


void PlayLoopInit()
{
    // REFKEEN - Alternative controllers support
    /*BE_ST_AltControlScheme_Push();
    BE_ST_AltControlScheme_PrepareInGameControls(KbdDefs[0].button0, KbdDefs[0].button1, KbdDefs[0].up, KbdDefs[0].down, KbdDefs[0].left, KbdDefs[0].right);*/

    //objtype	*obj, *check;
    //id0_long_t	newtime;

    button0held = button1held = false;

    ingame = true;
    playstate = notdone/*0*/;
    plummet = 0;

    FixScoreBox ();					// draw bomb/flower

    // This redraws the whole playloop scene after closing the control panel
    RF_ForceRefresh();

    VW_UpdateScreen();
}


void PlayLoopRun()
{
    CalcSingleGravity ();
    IN_ReadControl(0,&c);		// get player input
    if (!c.button0)
        button0held = 0;
    if (!c.button1)
        button1held = 0;


    // Status screen code in which you have to press a key to close.
    // Also it will render correctly
    if(openedStatusWindow)
    {
       StatusWindow();

       if(c.button0 || c.button1 ||
          Keyboard[sc_Space] || gInput.getPressedAnyButtonCommand(0))
       {
           openedStatusWindow = false;
           RF_ForceRefresh();

           lasttimecount = SD_GetTimeCount();
           IN_ClearKeysDown();
           gInput.flushAll();
       }

       RF_Refresh(false);

       return;
    }



//
// go through state changes and propose movements
//
    objtype	*obj = player;
    do
    {
        if (!obj->active
        && obj->tileright >= originxtile
        && obj->tileleft <= originxtilemax
        && obj->tiletop <= originytilemax
        && obj->tilebottom >= originytile)
        {
            obj->needtoreact = true;
            obj->active = yes;
        }

        if (obj->active)
            StateMachine(obj);

        if ( (obj->active == true || obj->active == removable) &&
        (  obj->tileright < inactivateleft
        || obj->tileleft > inactivateright
        || obj->tiletop > inactivatebottom
        || obj->tilebottom < inactivatetop) )
        {
            if (obj->active == removable)
                RemoveObj (obj);				// temp thing (shots, etc)
            else
            {
                if (US_RndT()<tics)				// let them get a random dist
                {
                    RF_RemoveSprite (&obj->sprite);
                    obj->active = no;
                }
            }
        }

        obj = (objtype *)obj->next;
    } while (obj);

//
// check for and handle collisions between objects
//
    obj = player;
    do
    {
        if (obj->active)
        {
            check = (objtype *)obj->next;
            while (check)
            {
                if ( check->active
                && obj->right > check->left
                && obj->left < check->right
                && obj->top < check->bottom
                && obj->bottom > check->top)
                {
//#pragma warn -pro
                    if (obj->state->contactptr)
                        obj->state->contactptr(obj,check);
                    if (check->state->contactptr)
                        check->state->contactptr(check,obj);
//#pragma warn +pro
                    if (!obj->obclass)
                        break;				// contact removed object
                }
                check = (objtype *)check->next;
            }
        }
        obj = (objtype *)obj->next;
    } while (obj);


    ScrollScreen();

//
// react to whatever happened, and post sprites to the refresh manager
//
    obj = player;
    do
    {
        if (obj->needtoreact && obj->state->reactptr)
        {
            obj->needtoreact = false;
//#pragma warn -pro
            obj->state->reactptr(obj);
//#pragma warn +pro
        }
        obj = (objtype *)obj->next;
    } while (obj);


//
// update the screen and calculate the number of tics it took to execute
// this cycle of events (for adaptive timing of next cycle)
//

    //RF_Refresh(true, true);


    if(gDreamsForceClose)
        return;

//
// single step debug mode
//
    /*if (singlestep)
    {
        VW_WaitVBL(14);
        lasttimecount = SD_GetTimeCount();
    }*/


    CheckKeys();


    if(gDreamsForceClose)
        return;

}

void startLevel();


void HandleDeath_Init (void);

void HandleDeath_Loop (void);


//==========================================================================


void GamePlayStartLevel()
{
    startLevel();
}



void PlayLoop()
{

    if(!loadedgame && !playstate)
    {                
        PlayLoopRun();

        if(playstate == died)
        {
            HandleDeath_Init();
        }

    }
    else if(playstate == died)
    {
        HandleDeath_Loop();

        if(playstate == notdone)
        {
            GamePlayStartLevel();
        }
    }
    else
    {
        ingame = false;

        startLevel();
    }
}


void PlayLoopRender()
{
    if(!playstate)
    {
        RF_Refresh(true);
    }
}

//==========================================================================

/*
==========================
=
= GameFinale
=
==========================
*/

void GameFinale (void)
{
//struct date d;

	VW_FixRefreshBuffer ();

/* screen 1 of finale text (16 lines) */
	US_CenterWindow (30,21);
	PrintY += 4;
	US_CPrint (
"Yes!  Boobus Tuber's hash-brown-\n"
"like remains rained down from\n"
"the skies as Commander Keen\n"
"walked up to the Dream Machine.\n"
"He analyzed all the complex\n"
"controls and readouts on it, then\n"
"pulled down a huge red lever\n"
"marked \"On/Off Switch.\"  The\n"
"machine clanked and rattled,\n"
"then went silent. He had freed\n"
"all the children from their\n"
"vegetable-enforced slavery!\n"
"Everything around Keen wobbled\n"
"in a disconcerting manner, his\n"
"eyelids grew heavy, and he\n"
"fell asleep....\n"
	);
	VW_UpdateScreen();
	VW_WaitVBL(60);
	SD_WaitSoundDone ();
	IN_ClearKeysDown ();
	IN_Ack();

/* screen 2 of finale (15 lines) */
	US_CenterWindow (30,21);
	PrintY += 9;
	US_CPrint (
"Billy woke up, looking around the\n"
"room, the early morning sun\n"
"shining in his face.  Nothing.\n"
"No vegetables to be seen.  Was it\n"
"all just a dream?\n\n"
"Billy's mom entered the room.\n\n"
"\"Good morning, dear. I heard some\n"
"news on TV that you'd be\n"
"interested in,\" she said, sitting\n"
"by him on the bed.\n\n"
"\"What news?\" Billy asked,\n"
"still groggy.\n\n"
	);
	VW_UpdateScreen();
	VW_WaitVBL(60);
	IN_ClearKeysDown ();
	IN_Ack();

/* screen 3 of finale (12 lines)*/
	US_CenterWindow (30,21);
	PrintY += 23;
	US_CPrint (
"\"The President declared today\n"
"National 'I Hate Broccoli' Day.\n"
"He said kids are allowed to pick\n"
"one vegetable today, and they\n"
"don't have to eat it.\"\n\n"
"\"Aw, mom, I'm not afraid of any\n"
"stupid vegetables,\" Billy said.\n"
"\"But if it's okay with you, I'd\n"
"rather not have any french fries\n"
"for awhile.\"\n\n"
"THE END"
	);
	VW_UpdateScreen();
	VW_WaitVBL(60);
	IN_ClearKeysDown ();
	IN_Ack();
}

//==========================================================================

/*
==========================
=
= HandleDeath
=
==========================
*/

int mSelection = 0;
int mTop, mBottom;

void HandleDeath_Init (void)
{
    gamestate.keys = 0;
    gamestate.boobusbombs -= gamestate.bombsthislevel;
    gamestate.lives--;
    if (gamestate.lives < 0)
    {
        // Make it to be gameover
        gEventManager.add( new dreams::SwitchToIntro );
        gInput.flushAll();
        return;
    }

    VW_FixRefreshBuffer ();
    US_CenterWindow (20,8);
    PrintY += 4;
    US_CPrint ("You didn't make it past");
    US_CPrint (levelnames[mapon]);
    PrintY += 8;
    mTop = PrintY-2;
    US_CPrint ("Try Again");
    PrintY += 4;
    mBottom = PrintY-2;
    US_CPrint ("Exit to Tuberia");

    VW_UpdateScreen ();

    mSelection = 0;
}

void HandleDeath_Loop (void)
{

    id0_unsigned_t y, color;

	{
        if (mSelection)
            y = mBottom;
		else
            y = mTop;

// draw select bar
		if ( (SD_GetTimeCount() / 16)&1 )
			color = SECONDCOLOR;
		else
			color = FIRSTCOLOR;

		VWB_Hlin (WindowX+4,WindowX+WindowW-4,y,color);
		VWB_Hlin (WindowX+4,WindowX+WindowW-4,y+1,color);
		VWB_Hlin (WindowX+4,WindowX+WindowW-4,y+12,color);
		VWB_Hlin (WindowX+4,WindowX+WindowW-4,y+13,color);
		VWB_Vlin (y+1,y+11, WindowX+4,color);
		VWB_Vlin (y+1,y+11, WindowX+5,color);
		VWB_Vlin (y+1,y+11, WindowX+WindowW-4,color);
		VWB_Vlin (y+1,y+11, WindowX+WindowW-5,color);

		VW_UpdateScreen ();

// erase select bar
		VWB_Hlin (WindowX+4,WindowX+WindowW-4,y,WHITE);
		VWB_Hlin (WindowX+4,WindowX+WindowW-4,y+1,WHITE);
		VWB_Hlin (WindowX+4,WindowX+WindowW-4,y+12,WHITE);
		VWB_Hlin (WindowX+4,WindowX+WindowW-4,y+13,WHITE);
		VWB_Vlin (y+1,y+11, WindowX+4,WHITE);
		VWB_Vlin (y+1,y+11, WindowX+5,WHITE);
		VWB_Vlin (y+1,y+11, WindowX+WindowW-4,WHITE);
		VWB_Vlin (y+1,y+11, WindowX+WindowW-5,WHITE);

		if (LastScan == sc_Escape)
		{
			gamestate.mapon = 0;		// exit to tuberia
			IN_ClearKeysDown ();
            playstate = notdone;
			return;
		}

		IN_ReadControl(0,&c);		// get player input
        if (c.button0 || c.button1
            || LastScan == sc_Return
            || LastScan == sc_Space)
		{
            if (mSelection == 1)
            {
				gamestate.mapon = 0;		// exit to tuberia
            }

            playstate = notdone;
            IN_ClearKeysDown ();
            c.button0 = c.button1 = 0;
            gInput.flushAll();

			return;
		}

		if (c.yaxis == -1 || LastScan == sc_UpArrow)
            mSelection = 0;
		else if (c.yaxis == 1 || LastScan == sc_DownArrow)
            mSelection = 1;

    }
}


/*
============================
=
= GameLoop
=
= A game has just started (after the cinematic or load game)
=
============================
*/

bool mGamePlayRunning = false;

void GamePlayStart()
{
    VW_SetScreenMode (GRMODE);
    VW_ClearVideo (BLACK);

    US_SetLoadSaveHooks(LoadGame,SaveGame,ResetGame);
    restartgame = gd_Continue;

    mGamePlayRunning = true;

    // Set variables for set up a new game in case
    // no game was loaded expilicitely by the user
    if (!loadedgame)
    {
        NewGame();
    }
}


void startLevel()
{
    id0_long_t	orgx,orgy;

    if (loadedgame)
    {
        loadedgame = false;
        //
        // start the initial view position to center the player
        //
        orgx = (id0_long_t)player->x - (150<<G_P_SHIFT);
        orgy = (id0_long_t)player->y - (84<<G_P_SHIFT);
        if (orgx<0)
            orgx=0;
        if (orgy<0)
            orgy=0;

        VW_FadeOut ();
        fadecount = 0;
        RF_SetRefreshHook (&FadeAndUnhook);
        RF_NewPosition (orgx,orgy);
        CalcInactivate ();
    }
    else
    {
        VW_FixRefreshBuffer ();
        US_CenterWindow (20,8);
        US_CPrint ("Loading");
        VW_UpdateScreen ();
        gamestate.bombsthislevel = 0;
        SetupGameLevel (true);
        gEventManager.add( new dreams::GoIntoPlayLoop );
    }

}

void processLevelcomplete()
{
    if (mapon)
    {
        SD_PlaySound (LEVELDONESND);
    }

    gamestate.leveldone[mapon] = true;	// finished the level

    if (mapon != 0)
    {
        gamestate.mapon = 0;
    }

}


void GameLoopOpen()
{
    id0_unsigned_t	cities,i;
    //id0_long_t	orgx,orgy;

    gamestate.difficulty = restartgame;
    restartgame = gd_Continue;

    openedStatusWindow = false;


    if( gamestate.lives>-1 && playstate!=victorious )
    {

        /*VW_FixRefreshBuffer ();
        VW_UpdateScreen ();
        CA_ClearMarks ();
        RF_MarkTileGraphics();

        CA_CacheMarks (levelnames[mapon], 0);*/

        gEventManager.add( new dreams::GoIntoPlayLoop );

        return;


#if FRILLS
        if (tedlevel)
        {
            if (playstate == died)
                goto startlevel;
            else
                TEDDeath ();
        }
#endif

        //if (loadedgame)
        //{
        //    startLevel();
        //}

        switch (playstate)
        {
        case warptolevel:
            startLevel();

        /*case died:
            HandleDeath ();
            break;*/

/*        case levelcomplete:
            break;*/

        case resetgame:
            return;

        case victorious:
            GameFinale ();
            goto done;
        default: break;
        }


    }
    else
    {

    GameOver ();

done: // !!
    cities = 0;
    for (i= 1; i<=16; i++)
        if (gamestate.leveldone[i])
            cities++;
    US_CheckHighScore (gamestate.score,cities);
    VW_ClearVideo (FIRSTCOLOR);
    }
}

void GameLoop (void)
{
}


