#include "dreamsengine.h"

#include "engine/core/CBehaviorEngine.h"
#include "engine/keen/KeenEngine.h"
#include <base/GsLogging.h>
#include <base/GsTimer.h>
#include <base/GsApp.h>
#include <fileio/CExeFile.h>
#include <fileio/KeenFiles.h>
#include <fileio/CPatcher.h>
#include <base/video/CVideoDriver.h>
#include <base/CInput.h>
#include <SDL.h>


#include "dreamscontrolpanel.h"
#include "dreamsintro.h"
#include "dreamsgameplay.h"

#ifndef REFKEEN_VER_KDREAMS_ANYEGA_ALL
#define REFKEEN_VER_KDREAMS_ANYEGA_ALL
#endif


dreams::DreamsEngine *gDreamsEngine;


enum GameStateSwitch
{
    GSS_INTRO_TEXT,     // The famous screen where hardware is detected and some notes about the versions are told
    GSS_NONE
} gGameStateChange = GSS_INTRO_TEXT;


// TODO: Ugly wrapper for the refkeen variables used. It serves as interface to C. Might be improved in future.
extern "C"
{


#include "../../refkeen/kdreams/kd_def.h"

 //Lock SDL_SemWait( gDataLock );
char *dreamsengine_datapath = nullptr;

//extern void RefKeen_Patch_id_ca(void);
extern void RefKeen_Patch_id_us(void);
extern void RefKeen_Patch_id_rf(void);
extern void RefKeen_Patch_kd_play(void);

typedef struct {
    const char *filename;
    int filesize;
    uint32_t crc32;
} BE_GameFileDetails_T;

// Describes a file originally embedded somewhere (in an EXE file)
typedef struct {
    BE_GameFileDetails_T fileDetails;
    int offset; // Location of file/chunk in the EXE
} BE_EmbeddedGameFileDetails_T;


typedef enum {
    BE_EXECOMPRESSION_NONE, BE_EXECOMPRESSION_LZEXE9X
} BE_ExeCompression_T;


/* This is separate from be_cross.h since the compiled code
 * depends on version-related macros
 */

/*typedef enum {
    BE_GAMEVER_KDREAMSE113,
    BE_GAMEVER_KDREAMSE193,
    BE_GAMEVER_KDREAMSE120,
    BE_GAMEVER_LAST
} BE_GameVer_T;*/


typedef struct {
    const BE_GameFileDetails_T *reqFiles;
    const BE_EmbeddedGameFileDetails_T *embeddedFiles;
    const char *writableFilesDir;
    const char *exeName;
    int decompExeSize;
    BE_ExeCompression_T compressionType;
    BE_GameVer_T verId;
} BE_GameVerDetails_T;



static const BE_GameFileDetails_T g_be_reqgameverfiles_kdreamse113[] = {
    {"KDREAMS.AUD", 3498, 0x80ac85e5},
    {"KDREAMS.CMP", 14189, 0x97628ca0},
    {"KDREAMS.EGA", 213045, 0x2dc94687},
    {"KDREAMS.EXE", 81619, 0x9dce0a39},
    {"KDREAMS.MAP", 65673, 0x8dce09af},
    {"LAST.SHL", 1634, 0xc0a3560f},
    {0}
};

static const BE_EmbeddedGameFileDetails_T g_be_embeddedgameverfiles_kdreamse113[] = {
    {"AUDIODCT.KDR", 1024, 0x8b6116d7, 0x2bc42},
    {"AUDIOHHD.KDR", 340, 0x499e0cbf, 0x24480},
    {"CONTEXT.KDR", 1283, 0x5a33439d, 0x245e0},
    {"EGADICT.KDR", 1024, 0xa69af202, 0x2b446},
    {"EGAHEAD.KDR", 12068, 0xb9d789ee, 0x1e720},
    {"GAMETEXT.KDR", 413, 0xb0df2792, 0x24af0},
    {"MAPDICT.KDR", 1020, 0x9faa7213, 0x2b846},
    {"MAPHEAD.KDR", 11824, 0xb2f36c60, 0x21650},
    {"STORY.KDR", 2526, 0xcafc1d15, 0x24c90},
    {0}
};

static const BE_GameVerDetails_T g_be_gamever_kdreamse113 = {
    g_be_reqgameverfiles_kdreamse113,
    g_be_embeddedgameverfiles_kdreamse113,
    "kdreamse_113",
    "KDREAMS.EXE",
    213536,
    BE_EXECOMPRESSION_LZEXE9X,
    BE_GAMEVER_KDREAMSE113
};



// (REFKEEN) Used for patching version-specific stuff
uint16_t refkeen_compat_kd_play_objoffset;

//extern	uint8_t	*EGAhead;
extern	uint8_t	*EGAdict;
extern	uint8_t	*maphead;
extern	uint8_t	*mapdict;
extern	uint8_t	*audiohead;
extern	uint8_t	*audiodict;

// (REFKEEN) Used for patching version-specific stuff
extern char *gametext, *context, *story;

mapfiletype_modern  mapFile;

extern SDL_Surface *gpBlitSfc;

// if an external event like closing the window is send, this variable
// will force refkeen to close everything.
int gDreamsForceClose;


void BEL_ST_UpdateHostDisplay(SDL_Surface *sfc);

void BE_ST_PollEvents(SDL_Event event);

void BE_ST_ApplyScreenMode(int mode);

id0_char_t *USL_GiveSaveName(id0_word_t game);

extern id0_boolean_t (*USL_SaveGame)(BE_FILE_T), (*USL_LoadGame)(BE_FILE_T);

void USL_HandleError(id0_int_t num);

}


void setupObjOffset()
{
    switch (refkeen_current_gamever)
    {
    /*case BE_GAMEVER_KDREAMSC105:
        refkeen_compat_kd_play_objoffset = 0x7470;
        break;*/
    case BE_GAMEVER_KDREAMSE113:
        refkeen_compat_kd_play_objoffset = 0x712A;
        break;
    case BE_GAMEVER_KDREAMSE193:
        refkeen_compat_kd_play_objoffset = 0x707A;
        break;
    case BE_GAMEVER_KDREAMSE120:
        refkeen_compat_kd_play_objoffset = 0x734C;
        break;
    case BE_GAMEVER_LAST:
        // ** This case must never happen. We might want to catch that exception?
        break;
    }
}

std::map< std::string, std::vector<uint8_t> > gDataMapVector;

extern "C"
{
bool USL_saveTheGame(int i, int n);
}

namespace dreams
{






bool SaveGameEvent::save() const
{
    auto ok = USL_saveTheGame(1, mN);

    return ok;
}




struct SwitchSceneEvent : CEvent
{
    SwitchSceneEvent(GsEngine *ptr) :
        mpEnginePtr( std::unique_ptr<GsEngine>(ptr) ) {}

    std::unique_ptr<GsEngine> mpEnginePtr;
};


// Mapping the strings of the filenames to the pointers where we store the embedded data
std::map< std::string, unsigned int > gOffsetMap;



bool extractEmbeddedFilesIntoMemory(const BE_GameVerDetails_T &gameVerDetails)
{

    std::map< std::string, uint8_t **> dataMap;    

    std::map< std::string, uint32_t> dataSizes;

    // CA
    //dataMap.insert ( std::pair<std::string, uint8_t **>("EGAHEAD.KDR", &EGAhead) );
    dataMap.insert ( std::pair<std::string, uint8_t **>("EGADICT.KDR", &EGAdict) );
    dataMap.insert ( std::pair<std::string, uint8_t **>("MAPHEAD.KDR", &maphead) );
    dataMap.insert ( std::pair<std::string, uint8_t **>("MAPDICT.KDR", &mapdict) );
    dataMap.insert ( std::pair<std::string, uint8_t **>("AUDIOHHD.KDR", &audiohead) );
    dataMap.insert ( std::pair<std::string, uint8_t **>("AUDIODCT.KDR", &audiodict) );

    // US
    dataMap.insert ( std::pair<std::string, uint8_t **>("GAMETEXT.KDR", (uint8_t **) &gametext) );
    dataMap.insert ( std::pair<std::string, uint8_t **>("CONTEXT.KDR", (uint8_t **) &context) );
    dataMap.insert ( std::pair<std::string, uint8_t **>("STORY.KDR", (uint8_t **) &story) );


    CExeFile &ExeFile = gKeenFiles.exeFile;

    uint8_t* headerData = static_cast<uint8_t*>(ExeFile.getHeaderData());

    for (const BE_EmbeddedGameFileDetails_T *embeddedfileDetailsBuffer = gameVerDetails.embeddedFiles; embeddedfileDetailsBuffer->fileDetails.filename; ++embeddedfileDetailsBuffer)
    {
        const std::string fName = embeddedfileDetailsBuffer->fileDetails.filename;
        auto it = dataMap.find(fName);

        const unsigned int dataSize = embeddedfileDetailsBuffer->fileDetails.filesize;

        unsigned int offset = embeddedfileDetailsBuffer->offset;
        gOffsetMap[fName] = offset;

        auto offsetPtr = headerData+offset;

        auto &localVector = gDataMapVector[fName];

        localVector.resize(dataSize);

        memcpy(localVector.data(), offsetPtr, dataSize);


        if(it == dataMap.end())
            continue;

        // Legacy C implementation
        {
            uint8_t **data = it->second;            

            *data = (uint8_t*) malloc(dataSize);

            memcpy(*data, offsetPtr, dataSize);

            dataSizes[it->first] = dataSize;
        }
    }

    mapFile.RLEWtag = 0;
    unsigned char	*mapheadPtr = maphead;
    memcpy(&mapFile.RLEWtag, mapheadPtr, 2 );

    mapheadPtr += 2;
    memcpy(&mapFile.headeroffsets, mapheadPtr, 400 );

    mapheadPtr += 400;
    memcpy(&mapFile.headersize, mapheadPtr, 100 );

    mapheadPtr += 100;

    // The first entry seems to describe "SPEED"
    const unsigned int tileinfoStart = SPEEDOFFSET;
    mapFile.tileinfo.resize(dataSizes["MAPHEAD.KDR"]-tileinfoStart);

    memcpy(mapFile.tileinfo.data(), mapheadPtr, mapFile.tileinfo.size() );


    // If we have a defined EGAHEAD, get it.

    return true;
}






DreamsEngine::~DreamsEngine()
{}

///
// This is used for loading all the resources of the game the use has chosen.
// It loads graphics, sound and text into the memory
///
bool DreamsEngine::loadResources()
{
    gLogging.ftextOut("Loading Dreams Engine...<br>");

    gTimer.setLPS(60.0f);

    mEngineLoader.setStyle(PROGRESS_STYLE_BAR);
    const std::string threadname = "Loading Keen Dreams";



    struct DreamsDataLoad : public Action
    {
        CResourceLoaderBackground &mLoader;

        DreamsDataLoad(CResourceLoaderBackground &loader) :
            mLoader(loader) {}

        int handle()
        {
            gEventManager.add(new FinishedLoadingResources());

            return 1;
        }
    };

    mEngineLoader.RunLoadActionBackground(new DreamsDataLoad(mEngineLoader));
    mEngineLoader.start();    

    return true;
}





void DreamsEngine::GameLoop()
{
}



#define GFX_TEX_WIDTH 320
#define GFX_TEX_HEIGHT 200
#define VGA_TXT_TEX_WIDTH 720
#define VGA_TXT_TEX_HEIGHT 400
//#define EGACGA_TXT_TEX_WIDTH 640
//#define EGACGA_TXT_TEX_HEIGHT 200

void DreamsEngine::applyScreenMode()
{
    unsigned int sdlTexWidth, sdlTexHeight;

    // Chaning the resolution still breaks the system so we leave at GFX_TEX_WIDTHxGFX_TEX_HEIGHT for now...

    switch (mChangeMode)
    {
    case 3:
        sdlTexWidth = VGA_TXT_TEX_WIDTH;
        sdlTexHeight = VGA_TXT_TEX_HEIGHT;
        break;
    case 4:
        sdlTexWidth = GFX_TEX_WIDTH;
        sdlTexHeight = GFX_TEX_HEIGHT;
        break;
    case 0xD:
        sdlTexWidth = GFX_TEX_WIDTH;
        sdlTexHeight = GFX_TEX_HEIGHT;
        break;
    case 0xE:
        sdlTexWidth = 2*GFX_TEX_WIDTH;
        sdlTexHeight = GFX_TEX_HEIGHT;
        break;
    }

    const GsRect<Uint16> gameRect(sdlTexWidth, sdlTexHeight);
    gVideoDriver.setNativeResolution(gameRect);

    // Mode changed, set it to zero
    mChangeMode = 0;
}


bool DreamsEngine::start()
{
    CExeFile &ExeFile = gKeenFiles.exeFile;

    //mLoader.setPermilage(10);

    // Patch the EXE-File-Data directly in the memory.
    CPatcher Patcher(ExeFile, gBehaviorEngine.mPatchFname);
    Patcher.process();

    //mLoader.setPermilage(50);

    extractEmbeddedFilesIntoMemory(g_be_gamever_kdreamse113);

    // Global for the legacy refkeen code.
    gDreamsEngine = this;    
    //gpRenderLock = SDL_CreateSemaphore(1);

    gKeenFiles.setupFilenames(7);

    setScreenMode(3);

    dreamsengine_datapath = const_cast<char*>(mDataPath.c_str());

    // This function extracts the embedded files. TODO: We should integrate that to our existing system
    // Load the Resources
    loadResources();

    //RefKeen_Patch_id_ca();
    //RefKeen_Patch_id_us();
    RefKeen_Patch_id_rf();
    setupObjOffset();

    mpScene.reset( new DreamsDosIntro );

    gGameStateChange = GSS_INTRO_TEXT;

    mpScene->start();

    return true;
}

bool mResourcesLoaded = false;

void DreamsEngine::pumpEvent(const CEvent *evPtr)
{
    GameEngine::pumpEvent(evPtr);

    if( dynamic_cast<const FinishedLoadingResources*>(evPtr) )
    {
        mResourcesLoaded = true;
    }

    if( dynamic_cast<const SwitchToIntro*>(evPtr) )
    {
        mpScene.reset( new DreamsIntro );
        gGameStateChange = GSS_NONE;
    }


    if( dynamic_cast<const LaunchControlPanel*>(evPtr) )
    {
        mpScene.reset( new DreamsControlPanel );
        mpScene->start();
        gGameStateChange = GSS_NONE;
        gInput.flushAll();
        IN_ClearKeysDown();
    }   
    if( dynamic_cast<const SwitchToGamePlay*>(evPtr) )
    {
        mpScene.reset( new DreamsGamePlay );
        mpScene->start();
        gGameStateChange = GSS_NONE;
        gInput.flushAll();
        IN_ClearKeysDown();
    }
    /*if( dynamic_cast<const NullifyScene*>(evPtr) )
    {
        mpScene = nullptr;
        gGameStateChange = GSS_NONE;
        gInput.flushAll();
        IN_ClearKeysDown();
    }*/

    if(mpScene)
    {
        mpScene->pumpEvent(evPtr);
    }
}

void DreamsEngine::ponder(const float deltaT)
{

    if(!mResourcesLoaded)
        return;

    std::vector<SDL_Event> evVec;
    gInput.readSDLEventVec(evVec);

    for(SDL_Event event : evVec)
    {
        BE_ST_PollEvents(event);
    }        


    if(mpScene)
    {
        mpScene->ponder(deltaT);
    }
}


void DreamsEngine::render()
{
    BE_ST_EGASetPelPanning(panx & 6);

    if(mpScene)
    {
        mpScene->render();
    }

    if(mChangeMode)
    {
        applyScreenMode();
    }

    SDL_Surface *blitSfc = gVideoDriver.getBlitSurface();
    BEL_ST_UpdateHostDisplay(blitSfc);
}

}
