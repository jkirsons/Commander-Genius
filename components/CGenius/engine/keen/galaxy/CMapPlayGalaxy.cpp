/*
 * CMapPlayGalaxy.cpp
 *
 *  Created on: 25.03.2011
 *      Author: gerstrong
 */

#include "CMapPlayGalaxy.h"
#include "common/ai/platform/CPlatform.h"
#include "common/ai/CPlayerBase.h"
#include "engine/core/CBehaviorEngine.h"
#include "ep4/CMapLoaderGalaxyEp4.h"
#include "ep5/CMapLoaderGalaxyEp5.h"
#include "ep6/CMapLoaderGalaxyEp6.h"
#include "ep4/ai/CInchWorm.h"
#include "ep4/ai/CFoot.h"
#include "ep4/ai/CSmokePuff.h"
#include "engine/core/VGamepads/vgamepadsimple.h"
#include <base/GsLogging.h>
#include <base/video/CVideoDriver.h>

#include <boost/property_tree/ptree.hpp>
#include <base/utils/Base64.h>

#include "GalaxyEngine.h"

CMapPlayGalaxy::CMapPlayGalaxy(std::vector<CInventory> &inventoryVec) :
mActive(false),
mInventoryVec(inventoryVec),
mMsgBoxOpen(false)
{}


void CMapPlayGalaxy::setActive(const bool value)
{
	mActive = value;

	if(mActive)
	{
        mMap.drawAll();
        gVideoDriver.updateScrollBuffer(mMap.m_scrollx, mMap.m_scrolly);
        gVideoDriver.setRefreshSignal(true);
	}
}




void CMapPlayGalaxy::pumpEvent(const CEvent *evPtr)
{
    if( const EventSpawnObject *ev = dynamic_cast<const EventSpawnObject*>(evPtr) )
    {
        std::shared_ptr<CGalaxySpriteObject> obj( static_cast<CGalaxySpriteObject*>(
                            const_cast<CSpriteObject*>(ev->pObject) ) );
        mObjectPtr.push_back( move(obj) );
    }
    else if( const EventSpawnFoot *ev = dynamic_cast<const EventSpawnFoot*>(evPtr) ) // Special Case where the Foot is created
    {                                                                                // Episode 4 Secret level
        // kill all the InchWorms in that case, so they can't do any spawning
        for( auto obj=mObjectPtr.rbegin() ; obj!=mObjectPtr.rend() ; obj++ )
        {
            galaxy::CInchWorm *inchworm = dynamic_cast<galaxy::CInchWorm*>(obj->get());
            if( inchworm != nullptr )
            {
                inchworm->exists = false;
            }
        }


        // Create the foot with Smoke Puff
        int posX = ev->x;
        int posY = ev->y-(6<<CSF);
        for( int x=-1 ; x<2 ; x++ )
        {
            for( int y=-1 ; y<2 ; y++ )
            {
                std::shared_ptr<CGalaxySpriteObject> smoke(new galaxy::CSmokePuff( &mMap, posX+(x<<CSF), posY+(y<<CSF), 0 ));
                mObjectPtr.push_back( smoke );
            }
        }

        bool alreadySpawned = false;

        // Check if there is already is a spawned foot
        for ( std::shared_ptr<CGalaxySpriteObject> &ptr : mObjectPtr )
        {
            if( std::dynamic_pointer_cast<galaxy::CFoot>(ptr) )
            {
                alreadySpawned = true;
                break;
            }
        }

        if(!alreadySpawned)
        {
            std::shared_ptr<CGalaxySpriteObject> foot(new galaxy::CFoot( &mMap, ev->foeID, 0x2EF4, posX, posY));
            mObjectPtr.push_back( foot );
        }
    }


    for( auto obj = mObjectPtr.begin(); obj != mObjectPtr.end() ; obj++)
    {
        auto &objRef = *(obj->get());
        objRef.pumpEvent(evPtr);
    }
}



void CMapPlayGalaxy::ponderBase(const float deltaT)
{
    bool oneInvOpen = false;

    // Check if the engine need to be paused
    for( auto &inv : mInventoryVec)
    {
        oneInvOpen |= inv.showStatus();
    }

    // Animate the tiles of the map
    mMap.m_animation_enabled = !oneInvOpen;

    if(mMsgBoxOpen)
    {
        mMap.m_animation_enabled = true;
    }

    mMap.animateAllTiles();

    if(!oneInvOpen && !mMsgBoxOpen)
    {
        for( auto obj = mObjectPtr.begin(); obj != mObjectPtr.end() ; obj++)
        {
            auto &objRef = *(obj->get());
            bool visibility = false;

            if( objRef.exists )
            {
                visibility = objRef.calcVisibility();

                if( visibility )
                {
                    // Process the AI of the object as it's given
                    objRef.process();

                    // Check collision between objects using NlogN order
                    auto theOther = obj; theOther++;
                    for( ; theOther != mObjectPtr.end() ; theOther++)
                    {
                        auto &theOtherRef = *(theOther->get());
                        if( !theOtherRef.exists )
                            continue;

                        objRef.isNearby(theOtherRef);
                        theOtherRef.isNearby(objRef);

                        if( objRef.hitdetect(theOtherRef) )
                        {
                            objRef.getTouchedBy(theOtherRef);
                            theOtherRef.getTouchedBy(objRef);
                        }
                    }
                }
            }

            // If the Player is not only dying, but also lost it's existence, meaning he got out of the screen
            // show the death-message or go gameover.
            if( galaxy::CPlayerBase *player = dynamic_cast<galaxy::CPlayerBase*>(obj->get()) )
            {
                if(player->exists)
                {
                    // Special cases, when Keen is god, but still has to die,
                    // kill him anyways.
                    if( player->touchedBottomOfMap() )
                    {
                        player->kill(true);
                    }

                    // Is he really dead?
                    if( player->mIsDead || (!visibility && player->mDying) )
                    {
                        player->processDead();
                    }
                }
            }

            objRef.processEvents();
        }
	}

}

void CMapPlayGalaxy::render()
{
    gVideoDriver.blitScrollSurface();

    // Draw all the sprites without player
    // The player sprites are drawn at the end. Reverse order of drawing
    galaxy::CPlayerBase* player[] = {nullptr, nullptr, nullptr, nullptr};

    int pIt = 0;
    auto objVecSize = mObjectPtr.size();
    for( int ctr = 0 ; ctr < int(objVecSize)  ; ctr++)
    {
        auto &obj = mObjectPtr[objVecSize-ctr-1];

        if( galaxy::CPlayerBase* curplayer = dynamic_cast<galaxy::CPlayerBase*>(obj.get()) )
        {
            player[pIt] = curplayer;
            pIt++;
            continue;
        }

        if(obj->honorPriority )
            obj->draw();
    }

    for(int i=0 ; i<pIt ; i++)
    {
        player[i]->draw();
    }

    // RenderShake Effect
    mMap.renderShaking();

    // Draw foregroundtiles here!
    mMap._drawForegroundTiles();

    for( auto obj=mObjectPtr.rbegin() ;
         obj!=mObjectPtr.rend() ; obj++ )
    {
        if(!(*obj)->honorPriority)
            (*obj)->draw();
    }

    const auto &optHUD = gBehaviorEngine.mOptions[GameOption::HUD];

    if( optHUD.value )
    {
        for( int pId = mInventoryVec.size()-1 ; pId>=0 ; pId-- )
        {
            mInventoryVec[pId].drawHUD();
        }
    }

}


// Saved GameState stuff for that individual map

void CMapPlayGalaxy::operator>>(CSaveGameController &savedGame)
{
	const Uint16 level = mMap.getLevel();
	savedGame.encodeData( level );

	std::vector< std::shared_ptr<CGalaxySpriteObject> > filteredObjects;

	// let's filter the Foe out that won't do any good!
	for( auto &it : mObjectPtr )
	{
	    if( it->mFoeID != 0 )
	    {
            filteredObjects.push_back( it );
	    }
	}

	const size_t size = filteredObjects.size();

	// save the number of objects on screen
	savedGame.encodeData(size);

	for( auto &it : filteredObjects )
	{
		// save all the objects states
		unsigned int newYpos = it->getYPosition();
		unsigned int newXpos = it->getXPosition();

		savedGame.encodeData( it->mFoeID );
		savedGame.encodeData( newXpos );
		savedGame.encodeData( newYpos );
		savedGame.encodeData( it->mIsDead );
		savedGame.encodeData( it->onscreen );
		savedGame.encodeData( it->hasbeenonscreen );
		savedGame.encodeData( it->exists );
		savedGame.encodeData( it->blockedd );
		savedGame.encodeData( it->blockedu );
		savedGame.encodeData( it->blockedl );
		savedGame.encodeData( it->blockedr );
		savedGame.encodeData( it->xDirection );
		savedGame.encodeData( it->yDirection );
		savedGame.encodeData( it->mHealthPoints );
		savedGame.encodeData( it->canbezapped );
		savedGame.encodeData( it->cansupportplayer );
		savedGame.encodeData( it->inhibitfall );
		savedGame.encodeData( it->honorPriority );
		savedGame.encodeData( it->mSpriteIdx );
		savedGame.encodeData( it->m_ActionNumber );
        it->serialize(savedGame);
	}

	// Save the map_data as it is left
	savedGame.encodeData(mMap.m_width);
	savedGame.encodeData(mMap.m_height);

	const Uint32 mapSize = mMap.m_width*mMap.m_height*sizeof(word);

	savedGame.addData( reinterpret_cast<byte*>(mMap.getBackgroundData()), mapSize );
	savedGame.addData( reinterpret_cast<byte*>(mMap.getForegroundData()), mapSize );
	savedGame.addData( reinterpret_cast<byte*>(mMap.getInfoData()), mapSize );
}



// This is for loading the game
bool CMapPlayGalaxy::operator<<(CSaveGameController &savedGame)
{
	Uint16 level;
	savedGame.decodeData( level );

	std::unique_ptr<galaxy::CMapLoaderGalaxy> mapLoader;
    const auto episode = gBehaviorEngine.getEpisode();

	if(episode == 4)
	{
        mapLoader.reset( new galaxy::CMapLoaderGalaxyEp4( mObjectPtr, mInventoryVec) );
	}
	else if(episode == 5)
	{
        mapLoader.reset( new galaxy::CMapLoaderGalaxyEp5( mObjectPtr, mInventoryVec) );
	}
	else if(episode == 6)
	{
        mapLoader.reset( new galaxy::CMapLoaderGalaxyEp6( mObjectPtr, mInventoryVec, gBehaviorEngine.isDemo()) );
	}
	else
	{
		gLogging.textOut("Error loading the file. This game is not supported!");
		return false;
	}

	// Load the World map level.
	mapLoader->loadMap( mMap, level );

    // Load the Background Music
	gMusicPlayer.stop();

    if( !galaxy::loadLevelMusic(level) )
    {
    	gLogging.textOut("Warning: The music cannot be played. Check that all the files have been correctly copied!");
    }
    else
    {
    	gMusicPlayer.play();
    }


	// load the number of objects on screen
	Uint32 size;
	Uint32 x, y;
	Uint16 foeID;
	uint16_t actionNumber;
	savedGame.decodeData(size);

	// Now load the previously created objects

	if(!mObjectPtr.empty())
		mObjectPtr.clear();

	mMap.mNumFuses = 0;
	mMap.mFuseInLevel = false;

	for( Uint32 i=0 ; i<size ; i++ )
	{
		savedGame.decodeData(foeID);
		savedGame.decodeData(x);
		savedGame.decodeData(y);

		CGalaxySpriteObject *pNewfoe = mapLoader->addFoe(mMap, foeID, x, y);

		// TODO: Be careful here is a bad Null Pointer inside that structure
        if(pNewfoe == nullptr)
		{
            pNewfoe = new CGalaxySpriteObject(&mMap, foeID, x, y, 0);
		}

		savedGame.decodeData( pNewfoe->mIsDead );
		savedGame.decodeData( pNewfoe->onscreen );
		savedGame.decodeData( pNewfoe->hasbeenonscreen );
		savedGame.decodeData( pNewfoe->exists );
		savedGame.decodeData( pNewfoe->blockedd );
		savedGame.decodeData( pNewfoe->blockedu );
		savedGame.decodeData( pNewfoe->blockedl );
		savedGame.decodeData( pNewfoe->blockedr );
		savedGame.decodeData( pNewfoe->xDirection );
		savedGame.decodeData( pNewfoe->yDirection );
		savedGame.decodeData( pNewfoe->mHealthPoints );
		savedGame.decodeData( pNewfoe->canbezapped );
		savedGame.decodeData( pNewfoe->cansupportplayer );
		savedGame.decodeData( pNewfoe->inhibitfall );
		savedGame.decodeData( pNewfoe->honorPriority );
		savedGame.decodeData( pNewfoe->mSpriteIdx );
		savedGame.decodeData( actionNumber );
		pNewfoe->deserialize(savedGame);

		if(pNewfoe->exists)
		{
		    pNewfoe->setActionForce(actionNumber);
		    std::shared_ptr<CGalaxySpriteObject> newFoe(pNewfoe);
		    mObjectPtr.push_back(newFoe);
		}
	}

	// Save the map_data as it is left
	savedGame.decodeData(mMap.m_width);
	savedGame.decodeData(mMap.m_height);

	savedGame.readDataBlock( reinterpret_cast<byte*>(mMap.getBackgroundData()) );
	savedGame.readDataBlock( reinterpret_cast<byte*>(mMap.getForegroundData()) );
	savedGame.readDataBlock( reinterpret_cast<byte*>(mMap.getInfoData()) );

	if( mMap.m_width * mMap.m_height > 0 )
	{
		mMap.drawAll();
	}

	return true;
}


// Saves the level using the Savegamecontroller in XML.
void CMapPlayGalaxy::operator>>(boost::property_tree::ptree &levelNode)
{
    // Coding here
    const Uint16 level = mMap.getLevel();
    levelNode.put("level", level);

    std::vector< std::shared_ptr<CGalaxySpriteObject> > filteredObjects;

    // let's filter the Foe out that won't do any good, by having an invalid foe id
    for( auto &it : mObjectPtr )
    {
        if( it->mFoeID != 0 )
        {
            filteredObjects.push_back( it );
        }
    }

    const size_t size = filteredObjects.size();

    // save the number of objects on screen
    levelNode.put("NumSprites", size);

    for( auto &it : filteredObjects )
    {
        auto &spriteNode = levelNode.add("Sprite", "");

        // save all the objects states
        unsigned int newYpos = it->getYPosition();
        unsigned int newXpos = it->getXPosition();

        spriteNode.put("<xmlattr>.id", it->mFoeID);
        spriteNode.put("<xmlattr>.variant", it->getSpriteVariantIdx());
        spriteNode.put("<xmlattr>.x", newXpos);
        spriteNode.put("<xmlattr>.y", newYpos);

        spriteNode.put("dead", it->mIsDead);
        spriteNode.put("onscreen", it->onscreen);
        spriteNode.put("hasbeenonscreen", it->hasbeenonscreen);
        spriteNode.put("exists", it->exists);
        spriteNode.put("blockedd", it->blockedd);
        spriteNode.put("blockedu", it->blockedu);
        spriteNode.put("blockedl", it->blockedl);
        spriteNode.put("blockedr", it->blockedr);
        spriteNode.put("xDirection", it->xDirection);
        spriteNode.put("yDirection", it->yDirection);
        spriteNode.put("health", (int)it->mHealthPoints);
        spriteNode.put("canbezapped", it->canbezapped);
        spriteNode.put("cansupportplayer", it->cansupportplayer);
        spriteNode.put("inhibitfall", it->inhibitfall);
        spriteNode.put("honorPriority", it->honorPriority);
        spriteNode.put("spritePic", it->mSpriteIdx);
        spriteNode.put("Actionumber", it->m_ActionNumber);
        it->serialize(spriteNode);
    }

    // Save the map_data as it is left
    {
        auto &mapNode = levelNode.put("Map", "");
        mapNode.put("width", mMap.m_width);
        mapNode.put("height", mMap.m_height);

        const Uint32 mapSize = mMap.m_width*mMap.m_height*sizeof(word);

        const std::string b64textBG   = base64Encode( reinterpret_cast<byte*>(mMap.getBackgroundData()), mapSize);
        const std::string b64textFG   = base64Encode( reinterpret_cast<byte*>(mMap.getForegroundData()), mapSize);
        const std::string b64textInfo = base64Encode( reinterpret_cast<byte*>(mMap.getInfoData()), mapSize);

        mapNode.put("bgdata", b64textBG);
        mapNode.put("fgdata", b64textFG);
        mapNode.put("infodata", b64textInfo);
    }
}

// This is for loading the game
void CMapPlayGalaxy::operator<<(boost::property_tree::ptree &levelNode)
{
    int level = levelNode.get<int>("level", 0);

    std::unique_ptr<galaxy::CMapLoaderGalaxy> mapLoader;
    const unsigned int episode = gBehaviorEngine.getEpisode();

    if(episode == 4)
    {
        mapLoader.reset( new galaxy::CMapLoaderGalaxyEp4( mObjectPtr, mInventoryVec) );
    }
    else if(episode == 5)
    {
        mapLoader.reset( new galaxy::CMapLoaderGalaxyEp5( mObjectPtr, mInventoryVec) );
    }
    else if(episode == 6)
    {
        mapLoader.reset( new galaxy::CMapLoaderGalaxyEp6( mObjectPtr, mInventoryVec, gBehaviorEngine.isDemo() ) );
    }
    else
    {
        gLogging.textOut("Error loading the file. This game is not supported!");
        return;
    }

    // Load the World map level.
    mapLoader->loadMap( mMap, level );

    gMusicPlayer.stop();

    // Prepare to load the Background Music
    if( !galaxy::loadLevelMusic(level) )
    {
        gLogging.textOut("Warning: The music cannot be played. Check that all the files have been correctly copied!");
    }
    else
    {
        gMusicPlayer.play();
    }


    // load the number of objects on screen
    Uint32 x, y;
    Uint16 foeID;

    // Now load the previously created objects

    //const size_t size = levelNode.get<int>("NumSprites", 0);

    // Now load the previously created objects
    if(!mObjectPtr.empty())
        mObjectPtr.clear();

    mapLoader->resetNumLoadedPlayers();

    mMap.mNumFuses = 0;
    mMap.mFuseInLevel = false;

    gLogging.textOut("Restoring enemies status");

    for( auto &levelItem : levelNode )
    {
        if(levelItem.first == "Sprite")
        {
            auto &spriteNode = levelItem.second;

            foeID = spriteNode.get<int>("<xmlattr>.id");
            int sprVarID = spriteNode.get<int>("<xmlattr>.variant", 0);
            x = spriteNode.get<int>("<xmlattr>.x");;
            y = spriteNode.get<int>("<xmlattr>.y");;

            CGalaxySpriteObject *pNewfoe = mapLoader->addFoe(mMap, foeID, x, y);

            // TODO: Be careful here is a bad Null Pointer inside that structure
            if(pNewfoe == nullptr)
            {
                pNewfoe = new CGalaxySpriteObject(&mMap, foeID, x, y, sprVarID);
            }

            pNewfoe->mIsDead = spriteNode.get<bool>("dead", false);
            pNewfoe->onscreen = spriteNode.get<bool>("onscreen", false);
            pNewfoe->hasbeenonscreen = spriteNode.get<bool>("hasbeenonscreen", false);
            pNewfoe->exists = spriteNode.get<bool>("exists", false);
            pNewfoe->blockedd = spriteNode.get<bool>("blockedd", false);
            pNewfoe->blockedu = spriteNode.get<bool>("blockedu", false);
            pNewfoe->blockedl = spriteNode.get<bool>("blockedl", false);
            pNewfoe->blockedr = spriteNode.get<bool>("blockedr", false);
            pNewfoe->xDirection = spriteNode.get<int>("xDirection", 0);
            pNewfoe->yDirection = spriteNode.get<int>("yDirection", 0);
            pNewfoe->mHealthPoints = spriteNode.get<int>("health", 1);
            pNewfoe->canbezapped = spriteNode.get<bool>("canbezapped", false);
            pNewfoe->cansupportplayer = spriteNode.get<bool>("cansupportplayer", false);
            pNewfoe->inhibitfall = spriteNode.get<bool>("inhibitfall", false);
            pNewfoe->honorPriority = spriteNode.get<bool>("honorPriority", false);
            pNewfoe->mSpriteIdx = spriteNode.get<int>("spritePic", 1);
            const Uint16 actionNumber = spriteNode.get<int>("Actionumber", 1);
            pNewfoe->deserialize(spriteNode);

            if(pNewfoe->exists)
            {
                pNewfoe->setActionForce(actionNumber);
                std::shared_ptr<CGalaxySpriteObject> newFoe(pNewfoe);
                mObjectPtr.push_back(newFoe);
            }
        }
    }

    gLogging.textOut("Restoring map status");

    // Save the map_data as it is left
    {
        auto &mapNode = levelNode.get_child("Map");
        mMap.m_width = mapNode.get<int>("width");
        mMap.m_height = mapNode.get<int>("height");

        const std::string b64textBG   = mapNode.get<std::string>("bgdata");
        const std::string b64textFG   = mapNode.get<std::string>("fgdata");
        const std::string b64textInfo = mapNode.get<std::string>("infodata");

        base64Decode(reinterpret_cast<byte*>(mMap.getBackgroundData()), b64textBG);
        base64Decode(reinterpret_cast<byte*>(mMap.getForegroundData()), b64textFG);
        base64Decode(reinterpret_cast<byte*>(mMap.getInfoData()), b64textInfo);
    }

    if( mMap.m_width * mMap.m_height > 0 )
    {
        mMap.drawAll();
    }
}


