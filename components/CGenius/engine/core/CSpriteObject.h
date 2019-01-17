/*
 * CSpriteObject.h
 *
 *  Created on: 17.05.2009
 *      Author: gerstrong
 *
 *  This class handles the objects like sprites that are draw on the screen
 *  It also handles the AI if any.
 */


#ifndef CSpriteObject_H
#define CSpriteObject_H

#include <base/GsEvent.h>

#include "ActionFormat.h"
#include "direction.h"
#include "CBehaviorEngine.h"

// structures for each AI module's data
#include "CMap.h"
#include "graphics/GsGraphics.h"
#include "options.h"
#include "sdl/audio/Audio.h"

// Enumerations are here
#include "objenums.h"

#include <base/utils/FindFile.h>

#include "fileio/KeenFiles.h"

#include <base/GsPython.h>


const int COLISION_RES = (1<<STC);

// The bouncing box used by the object which is used to determine the collisions
struct BoundingBox
{
    int x1, x2, y1, y2;
    BoundingBox(int l_x1 = 0, int l_x2 = 0,
		int l_y1 = 0, int l_y2 = 0 ) :
      x1(l_x1), x2(l_x2),
      y1(l_y1), y2(l_y2) {}
    
    void operator()(const int l_x1,
		    const int l_y1,
		    const int l_x2,
		    const int l_y2 )
    {
      x1 = l_x1; x2 = l_x2;
      y1 = l_y1; y2 = l_y2;
    }
    
    
    unsigned int Width()
    {	return (x2-x1);		}
    
    unsigned int Height()
    {	return (y2-y1);		}
};


class CSpriteObject;

// Event that will be used to move the objects in the game
struct ObjMove
{
    Vector2D<int> m_Vec;
    
    ObjMove(const Vector2D<int>& Vector) : m_Vec(Vector) {}
    ObjMove(const int offx, const int offy) : m_Vec(offx, offy) {}
    
    virtual ~ObjMove() {}
};

// Event that will be used to move the objects in the game together with another object.
// This is applied for example whenever keen is being moved on the platform
struct ObjMoveCouple : ObjMove
{
    CSpriteObject &mSecond;
    ObjMoveCouple(const Vector2D<int>& vec,
		  CSpriteObject &second) :
      ObjMove(vec), mSecond(second)  {}
    
    ObjMoveCouple(const int offx, const int offy,
		  CSpriteObject &second) :
      ObjMove(offx, offy), mSecond(second) {}
};

// Same as above but for multiple couples

struct ObjMoveCouples : ObjMove
{
    std::vector<CSpriteObject*> mCarriedObjVec;
    
    ObjMoveCouples(const Vector2D<int>& Vector,
		   std::vector<CSpriteObject*> &carriedObjVec) :
      ObjMove(Vector), mCarriedObjVec(carriedObjVec)  {}
    
    ObjMoveCouples(const int offx, const int offy,
		   std::vector<CSpriteObject*> &carriedObjVec) :
      ObjMove(offx, offy), mCarriedObjVec(carriedObjVec) {}
};


// Small special routine for spawning objects. Might be called by other objects and the level manager
void spawnObj(const CSpriteObject *obj);

class CSpriteObject
{
  public:
    CSpriteObject(CMap *pmap, Uint32 x, Uint32 y, const int spriteVar);
    
    unsigned int m_index;        	// Like an ID for some objects that need this implementation
    
    
    
    unsigned int mHealthPoints;              // episode 1 style four-shots-to-kill
    bool mTurnAroundOnCliff = false;    // Can enemy turn around if there is a cliff
    bool mEndGameOnDefeat = false;    // End game if enemy is defeated. Useful for the last boss in some mods
    bool exists;
    bool onscreen;    				// true=(scrx,scry) position is visible onscreen
    bool hasbeenonscreen;
    int mSpriteIdx;      		// which sprite should this object be drawn with
    int xDirection;					// the direction to where the object is looking/heading to
    int yDirection;					// same for vertical
    
    int scrx, scry;           		// x,y pixel position on screen
    
    virtual void pumpEvent(const CEvent *evPtr);
    
    virtual void processEvents();
    
    // Bounding Boxes
    BoundingBox m_BBox;
    
    // if zero, priority tiles will not be honored and object will always
    // appear in front of the background
    bool honorPriority;
    bool dontdraw;	// tells the object whether to get drawn or not. The object is still existent in this while
    bool solid;
    
    bool canbezapped;         // if 0 ray will not stop on hitdetect
    
    bool inhibitfall;         // if true common_enemy_ai will not do falling
    
    bool cansupportplayer;
    
    unsigned char blockedl, blockedr, blockedu, blockedd;
    bool onslope;
    signed int xinertia, yinertia;
    CSpriteObject *pSupportedbyobject;
    
    bool mIsDead, dying;
    
    
    bool m_jumpdownfromobject;
    
    
    
    /*
     * \brief Calculate Bounding Boxes fro collision
     */
    void calcBoundingBoxes();
    
    /**
     * @brief setupCollisionModel Should be called when an object is created. It will setup the collision model for ingame as required.
     */
    void setupCollisionModel();
    
    void performCollisionsSameBox();
    void performCollisions();
    void alignToTile();
    void setScrPos( int px, int py );
    bool PoleCollision();
    virtual bool calcVisibility();
    
    /**
     * @brief This will verify whether object has to fall or not.
	 * 				This function must be called when it might be moving
	 * 				because it also checks the lower sloped tiles cases
	 *
     * @todo		This function should also be called by foes in some cases
	 */
    bool verifyForFalling();
    
    /**
	 * \description	This function will change the direction of an object when
	 * 				it detects a coming gap while moving
	 * \param		speed Amount of CSFed coordinates to move the object back
	 * 				when it over that gap
	 */
    void performCliffStop(const int &speed);
    
    
    
    // Moving parts
    void moveToForce(const Vector2D<int> &dir);
    void moveToForce(const int new_x, const int new_y);
    void moveDir(const Vector2D<int> &dir);
    void moveToHorizontal(const int& new_x);
    void moveToVertical(const int& new_y);
    void moveTo(const Vector2D<Uint32> &new_loc);
    void moveTo(const int new_x, const int new_y);
    void moveXDir(const int amount, const bool force = false);
    void moveYDir(const int amount);
    virtual void moveLeft(const int amnt, const bool = false);
    virtual void moveRight(const int amnt, const bool = false);
    virtual void moveUp(const int amnt);
    virtual void moveDown(const int amnt);
    
    // new moving parts
    /**
	 * \brief	This function checks if there is any collision and moves the object safely
	 * \param	dir	The direction where the object has to go to...
	 */
    void processMove(const Vector2D<int>& dir);
    void processMoveBitLeft();
    void processMoveBitRight();
    virtual void processMoveBitDown();
    void processMoveBitUp();
    void processMove(const int move_x, const int move_y);
    
    /*
	 * \brief As especially in Galaxy some tiles still can get into blocks where they shouldn't
	 *  	  So this function will pull them out. Same method is used in the original games
	 *  	  and will give a more Commander Keen like feeling
	 */
    void processPushOutCollision();
    
    void decreaseXInertia(const int value);
    void InertiaAndFriction_X(const int friction_rate = 10);
    
    virtual void process() { }
    
    /**
	 * @brief turnAroundOnCliff Turn around when there is a cliff where the foe
	 *                          must not fall.
	 * @param x1 most left point of the Sprite Box
	 * @param x2 most right point of the Sprite Box
	 * @param y2 lowest point of the Sprite Box
	 * @return true if the foe will blocked so he can turn around, otherwise false.
	 * @note slopes do not count as cliffs.
	 */
    bool turnAroundOnCliff( int x1, int x2, int y2 );
    
    /**
	 * @brief turnAroundOnSlope make the foe turn around, whenever a slope is detected
	 * @param x1 most left point of the Sprite Box
	 * @param x2 most right point of the Sprite Box
	 * @param y2 lowest point of the Sprite Box
	 * @return true if the foe will blocked so he can turn around, otherwise false.
	 * @note slopes do not count as cliffs.
	 */
    bool turnAroundOnSlope( int x1, int x2, int y2 );
    
    
    bool hitdetect(CSpriteObject &hitobject);
    bool hitdetectWithTile(const int num, const int lx, const int ly, const int lw, const int lh, const int res);
    bool hitdetectWithTilePropertyRect(const Uint16 Property, int &lx, int &ly, const int lw, const int lh, const int res);
    bool hitdetectWithTilePropertyRectRO(const Uint16 Property, const int lx, const int ly, const int lw, const int lh, const int res);
    bool hitdetectWithTilePropertyHor(const Uint16 Property, const int lxl, const int lxr, const int ly, const int res);
    bool hitdetectWithTilePropertyVert(const Uint16 Property, const int lx, const int lyu, const int lyd, const int res);
    bool hitdetectWithTileProperty(const int Property, const int x, const int y);

    virtual void kill(const bool force = false,
                      const bool noDieProcess = false);

    void blink(Uint16 frametime);
    
    // Collision parts
    /**
	 * \brief 	Those functions check the the collision states and return also a number of what type of block
	 * 			the collision is happening.
	 * \return	returns the number of the block property. 0 means no collision. Any other number depends.
	 * 			1 is blocked, the other depends on the engine. In Keen Galaxy they mostly represent sloped tiles
	 * 			In vorticons the up part have other numbers which represent ice and slippery.
	 */
    int checkSolidR(int x1, int x2, int y1, int y2);
    int checkSolidL(int x1, int x2, int y1, int y2);
    virtual int checkSolidU( int x1, int x2, int y1, const bool push_mode=false );
    virtual int checkSolidD( int x1, int x2, int y2, const bool push_mode=false );
    
    virtual bool checkMapBoundaryL(const int x1);
    virtual bool checkMapBoundaryR(const int x2);
    virtual bool checkMapBoundaryU(const int y1);
    virtual bool checkMapBoundaryD(const int y2);
    
    
    // special functions for sloped tiles
    bool checkslopedU( int c, int y1, Sint8 blocked);
    bool checkslopedD( int c, int y2, Sint8 blocked);
    void adjustSlopedTiles( int x, int y1, int y2, const int xspeed );
    bool moveSlopedTileDown( int x, int y, const int xspeed );
    void moveSlopedTileUp( int x, int y, const int xspeed );
    
    // getters for positions
    Vector2D<int> &getPosition()
    { return m_Pos; }
    
    auto getXPosition() const -> int
    { return int(m_Pos.x); }
    
    auto getYPosition() const -> int
    { return int(m_Pos.y); }
    
    auto getXLeftPos() const -> int
    { return int(m_Pos.x)+m_BBox.x1; }
    
    auto getXRightPos()  const -> int
    { return int(m_Pos.x)+m_BBox.x2; }
    
    auto getXMidPos()  const -> int
    { return int(m_Pos.x)+(m_BBox.x2-m_BBox.x1)/2; }
    
    auto getYUpPos() const -> int
    { return int(m_Pos.y)+m_BBox.y1; }
    
    auto getYDownPos() const -> int
    { return int(m_Pos.y)+m_BBox.y2; }
    
    auto getYMidPos() const -> int
    { return int(m_Pos.y)+(m_BBox.y2-m_BBox.y1)/2; }
    
    auto getMidPos() const -> Vector2D<int>
    { return Vector2D<int>(getXMidPos(), getYMidPos()); }
    
    void processFallPhysics(const int boost);
    void processFallPhysics();
    virtual void processFalling();
    virtual void getTouchedBy(CSpriteObject&) {}
    virtual bool isNearby(CSpriteObject&) { return true; }
    virtual void getShotByRay(object_t &);
    void kill_intersecting_tile(int mpx, int mpy, CSpriteObject &theObject);
    CMap *getMapPtr() { return mpMap; }
    
    /**
	 *  \description plays certain sound of an object. Stereo will automatically applied when used
	 */
    void playSound( const GameSound snd,
		    const SoundPlayMode mode=SoundPlayMode::PLAY_NOW );
    
    virtual void draw();
    
    virtual ~CSpriteObject();
    
    int getSpecialIdx() const
    {   return mPlayerIdx;    }

    int getSpriteVariantIdx() const
    {   return mSprVar;    }

    void setSpecialIdx(const int i)
    {   mPlayerIdx = i;    }

    void setSpriteVariantId(const int i)
    {   mSprVar = i;    }
  protected:
    
    /**
     * @brief cancelAllMoveTasks cancels out all the already created move tasks
     */
    void cancelAllMoveTasks();
    
    // This container will held the triggered events of the object
    // TODO: This create too much fragmentation. Find a way to make this better
    std::vector< ObjMove* > mMoveTasks;
    
    
    
#if USE_PYTHON3
    /**
     * @brief loadAiGetterInteger
     * @param pModule
     * @param pyMethodStr
     * @param value
     * @return
     */
    bool loadAiGetterInteger(PyObject * pModule, const std::string &pyMethodStr, int &value);
    
    
    /**
     * @brief loadAiGetterBool
     * @param pModule
     * @param pyMethodStr
     * @param value
     * @return
     */
    bool loadAiGetterBool(PyObject * pModule, const std::string &pyMethodStr, bool &value);
    
    
    
    /**
     * @brief loadPythonScripts     Load an external script file which might modify the behaviour of the sprite object
     * @param scriptBaseName        Basename is the filename with any extension or path. Recommendation: Use the name of the foe
     * @return if load was successful true, otherwise false.
     */
    virtual bool loadPythonScripts(const std::string &scriptBaseName);
#endif
    
    CMap *mpMap;
    
    Uint16 m_blinktime;
    bool mInvincible = false;   /** Shot might hit the object but it has no effect at all */
    bool mRecoverFromStun = false; /** If foe get shot they might be able to recover at later time */
    bool mNeverStop = false;        /** This will make foe continue walking and never change actions (Keen 9 - Cybloog) */
    bool mPogoStunnable = false;        /** This will make foe continue walking and never change actions (Keen 9 - Cybloog) */
    bool mJumpStunnable = false;
    bool mMayShoot = false;         /** If enemy if allowed to shoot. Not all of them are able to do that.*/
    
    GameSound mWalkSound;
    
    Vector2D<int> m_Pos; 	// x,y location in map coords, CSFed, represent as 2D Vector
    
    static int m_number_of_objects;
    
    // Action Format related stuff
    ActionFormatType m_Action;
    
    Uint8 transluceny;
    
protected:

    int mSprVar = 0; // Sprite variant, which is used by the Spritemap

    int mPlayerIdx = 0; // Special index for different usages. In case of players, that is its index

};

/**
 *	\description This event will spawn a new object
 *
 *	\param		pObject Pointer to the allocated memory of the Object. Caution: This allocation
 */
struct EventSpawnObject : CEvent {
    
    const CSpriteObject *pObject;
    
    EventSpawnObject( const CSpriteObject* pObject ) :
      pObject( pObject ) {}
};

#endif // CSpriteObject_H_
