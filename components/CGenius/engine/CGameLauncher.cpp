/*
 * CGameLauncher.cpp
 *
 *  Created on: 22.09.2009
 *      Author: gerstrong
 */

#include "CGameLauncher.h"
#include <base/GsLogging.h>
#include <base/video/CVideoDriver.h>
#include <base/CInput.h>
#include <base/InputEvents.h>
#include <base/GsApp.h>
#include <widgets/GsTextSelectionList.h>
#include <widgets/GsBanner.h>
#include <widgets/GsButton.h>
#include <widgets/GsText.h>

#include <graphics/GsGraphics.h>
#include <base/utils/FindFile.h>
#include <base/utils/StringUtils.h>
#include <widgets/GsMenuController.h>
#include <base/GsArguments.h>

#include "core/VGamepads/vgamepadsimple.h"
#include "core/mode/CGameMode.h"
#include "core/menu/SettingsMenu.h"
#include "sdl/audio/Audio.h"
#include "fileio/ResourceMgmt.h"
#include "fileio/KeenFiles.h"

#include "../version.h"

#include <iostream>
#include <fstream>
#include <SDL_image.h>

#include "keen/vorticon/VorticonEngine.h"
#include "keen/galaxy/GalaxyEngine.h"

#include "keen/dreams/dreamsengine.h"




/// Main Class implementation
CGameLauncher::CGameLauncher(const bool first_time,
                             const int start_game_no,
                             const int start_level) :
mDonePatchSelection(false),
mDoneExecSelection(false),
mLauncherDialog(CGUIDialog(GsRect<float>(0.1f, 0.1f, 0.8f, 0.85f),
                           CGUIDialog::FXKind::EXPAND)),
mGameScanner(),
m_firsttime(first_time),
m_start_game_no(start_game_no),
m_start_level(start_level)
{	
    gSound.unloadSoundData();
    // The last menu has been removed. Restore back the game status
    gBehaviorEngine.setPause(false);

    gMenuController.clearMenuStack();
    letchooseagain();

#ifdef VIRTUALPAD
    gInput.mpVirtPad.reset(new VirtualKeenControl);

    if( !gInput.mpVirtPad->init() )
    {
        const std::string err = "Error loading the Virtual Gamepad!";

        gLogging.textOut(err);
    }
#endif

}

CGameLauncher::~CGameLauncher()
{}

////
// Initialization Routine
////
bool CGameLauncher::setupMenu()
{
    m_mustquit      = false;
    mDonePatchSelection = false;
    m_chosenGame    = -1;
    m_ep1slot       = -1;
    mLauncherDialog.initEmptyBackground();
    mSelection      = -1;


    // TODO: Put that scanning into a separate so we can show a loading menu
    // Scan for games...
    m_DirList.clear();
    m_Entries.clear();

    gLogging.ftextOut("Game Autodetection Started<br>" );

    // Process any custom labels
    getLabels();

    bool gamesDetected = false;

    // Scan VFS DIR_ROOT for exe's
    gamesDetected |= scanExecutables(DIR_ROOT);
    mGameScanner.setPermilage(100);

    // Recursivly scan into DIR_GAMES subdir's for exe's
    gamesDetected |= scanSubDirectories(DIR_GAMES,
                                        DEPTH_MAX_GAMES,
                                        200, 900);

    mpGameSelectionList = new CGUITextSelectionList();

    // Save any custom labels
    putLabels();

    // Create an empty Bitmap control
    mLauncherDialog.addControl( new CGUIBitmap(),
                                GsRect<float>(0.51f, 0.07f, 0.48f, 0.48f) );

    mCurrentBmp = std::dynamic_pointer_cast< CGUIBitmap >
                  ( mLauncherDialog.getControlList().back() );

    mPreviewBmpPtrVec.resize(m_Entries.size());

	std::vector<GameEntry>::iterator it = m_Entries.begin();
    unsigned int i=0;
    for( ; it != m_Entries.end() ; it++	)
    {
        mpGameSelectionList->addText(it->name);

        // And try to add a preview bitmap
        std::string fullfilename = "preview.bmp";
        fullfilename = getResourceFilename(fullfilename, it->path, false);
        fullfilename = GetFullFileName(fullfilename);

        if(IsFileAvailable(fullfilename))
        {
            mPreviewBmpPtrVec[i].reset(new GsBitmap);
            mPreviewBmpPtrVec[i]->loadHQBitmap(fullfilename);
        }
        i++;
    }

    mpGameSelectionList->setConfirmButtonEvent(new GMStart());
    mpGameSelectionList->setBackButtonEvent(new GMQuit());

    mLauncherDialog.addControl(new CGUIText("Pick a Game"),
                               GsRect<float>(0.0f, 0.01f, 1.0f, 0.05f));

    mLauncherDialog.addControl(new GsButton( "x", new GMQuit(),
                                             GsControl::Style::NONE,
                                             1.0f,
                                             0.75f,
                                             0.75f ),
                               GsRect<float>(0.0f, 0.0f, 0.069f, 0.069f) );

    /*mLauncherDialog.addControl(new GsButton( "VPad", new OpenVGamePadSettingsEvent(),
                                             GsControl::Style::NONE,
                                             0.75f,
                                             1.0f,
                                             1.0f ),
                               GsRect<float>(0.75f, 0.0f, 0.17f, 0.069f) );
                               */

    mLauncherDialog.addControl(new GsButton( "|", new OpenSettingsMenuEvent(),
                                             GsControl::Style::NONE,
                                             0.75f,
                                             1.0f,
                                             1.0f ),
                               GsRect<float>(0.93f, 0.0f, 0.069f, 0.069f) );



    mLauncherDialog.addControl(mpGameSelectionList,
                               GsRect<float>(0.01f, 0.07f, 0.49f, 0.79f));

    #ifdef DOWNLOADER
    verifyGameStore();
    #endif

    mpStartButton = std::dynamic_pointer_cast<GsButton>
                (
                mLauncherDialog.addControl
                   (
                       new GsButton( "Start >",
                                     new GMStart(),
                                     GsControl::NONE,
                                     0.675f, 1.0f, 0.675f),
                                     GsRect<float>(0.60f, 0.865f, 0.25f, 0.07f)
                   )
                ) ;


    mpEpisodeText = new CGUIText("Game");
    mpVersionText = new CGUIText("Version");
    mpDemoText = new CGUIText("Demo");
    mLauncherDialog.addControl(mpEpisodeText, GsRect<float>(0.5f, 0.70f, 0.5f, 0.05f));
    mLauncherDialog.addControl(mpVersionText, GsRect<float>(0.5f, 0.75f, 0.5f, 0.05f));
    mLauncherDialog.addControl(mpDemoText, GsRect<float>(0.5f, 0.80f, 0.5f, 0.05f));

    // This way it goes right to the selection list.
    mLauncherDialog.setSelection(2);

    mGameScanner.setPermilage(1000);

    gLogging.ftextOut("Game Autodetection Finished<br>" );
    // Banner. TODO: Create a class for that...
    CGUIBanner *banner = new CGUIBanner("Commander Genius " CGVERSION "\n"
                    "By Gerstrong,\n"
                    "Zilem,\n"
                    "Pelya,\n"
					"and the CG Contributors\n");
    mLauncherDialog.addControl( banner, GsRect<float>(0.0f, 0.95f, 1.0f, 0.05f) );

    if(!gamesDetected)
    {
        std::stringstream ss;

        const auto searchPathGames =
                JoinPaths(GetFirstSearchPath(),"games");

        ss << "No games detected!\n";
        ss << "You can copy some into:\n";
        ss << "\"" << searchPathGames << "\",\n";
        ss << "or download using \"+ More\" button.\n";

        std::string msg(ss.str());

        showMessageBox(msg);

        return false;
    }

    // Set the first game selected and highlight the start button
    mpGameSelectionList->setSelection(0);
    mLauncherDialog.setSelection(3);


    const std::string gameDir = gArgs.getValue("dir");
    if(!gameDir.empty())
    {
        int chosenGame = 0;
        bool found=false;

        // Check if the given parameter makes one game start.
        for( GameEntry &entry : m_Entries)
        {
            if(entry.path == gameDir)
            {
                // found!
                m_chosenGame = chosenGame;
                gLogging.textOut("Launching game from directory: \"" + gameDir + "\"\n");
                gArgs.removeTag("dir");

                setupModsDialog();
                // Nothing else to do, break the loop
                found = true;
                break;
            }
            chosenGame++;
        }

        if(!found)
        {
            const std::string err = "The game from directory: \"" + gameDir + "\" cannot the launched." +
                    "Maybe it's missing or not compatible. Please check if you can run that through the game launcher.\n";

            gLogging.textOut(err);

            showMessageBox("Given path :\"" + gameDir + "\" unknown.\nPlease check the CGLog File!");
        }
    }

    return true;
}


bool CGameLauncher::scanSubDirectories(const std::string& path,
                                       const size_t maxdepth,
                                       const size_t startPermil,
                                       const size_t endPermil)
{
    bool gamesDetected = false;

	std::set<std::string> dirs;
	FileListAdder fileListAdder;
    GetFileList(dirs, fileListAdder,
                path, false, FM_DIR);

    size_t interval = dirs.size();

    if(interval <= 0)
        interval = 1;

    size_t deltaPerMil = (endPermil-startPermil)/interval;
    size_t permil = startPermil;

    if(deltaPerMil < 1)
        deltaPerMil = 1;

    mGameScanner.setPermilage(int(startPermil));

    for( const auto &subdir : dirs )
	{
        std::string newpath = JoinPaths(path , subdir);

        gamesDetected |= scanExecutables(newpath);

        size_t lastPermil = permil + deltaPerMil;
        if(lastPermil>endPermil)
            lastPermil = endPermil;

        if(maxdepth > 1 && scanSubDirectories(newpath, maxdepth - 1, permil, lastPermil))
        {
            gamesDetected = true;
        }

        permil = lastPermil;
        mGameScanner.setPermilage(int(permil));
	}

    mGameScanner.setPermilage(int(endPermil));

    return gamesDetected;
}

std::string CGameLauncher::filterGameName(const std::string &path)
{
    size_t pos = 0;

    std::string text = path;

    while(1)
    {
        if( (pos = text.find("\\")) == text.npos)
        {
            if( (pos = text.find("/")) == text.npos)
            {
                break;
            }
        }

        text = text.substr(pos+1);
    }

    return text;
}

bool CGameLauncher::scanExecutables(const std::string& path)
{
    bool result = false;

    gLogging.ftextOut("Search: %s<br>", path.c_str() );
printf("Lock Attempt\n");
SDL_LockDisplay();   
    // Episode 1-6 and 7 stands for Keen Dreams
    for(int i = 1; i <= 7; ++i)
    {
		CExeFile executable;
        // Load the exe into memory or a python script
     
		if(!executable.readData(i, path))
        {
            if(!executable.readMainPythonScript(i, path))
            {
                continue;
            }
        }

		// Process the exe for type
		GameEntry newentry;
		newentry.crcpass = executable.getEXECrc();
		newentry.version = executable.getEXEVersion();
		newentry.supported = executable.Supported();
		newentry.episode = i;
		newentry.demo = executable.isDemo();
		newentry.path    = path;
		newentry.exefilename = executable.getFileName();
		// Check for an existing custom label for the menu
		newentry.name    = scanLabels(executable.getFileName());

		std::string verstr;
		std::string gamespecstring = "Detected game Name: " + executable.getFileName();
		if( newentry.version<=0 ) // Version couldn't be read!
		{
			verstr = "unknown";
			gamespecstring += " (Unknown Version)<br>";
		}
		else
		{
			verstr = "v" + itoa(newentry.version/100) + "." + itoa(newentry.version%100);
			gamespecstring += " Version: ";
			gamespecstring += verstr;
			gamespecstring += "<br>";
		}



		if( newentry.name.length() <= 0 )
		{
            newentry.name = filterGameName(newentry.path);
		}

		// Save the type information about the exe
		m_Entries.push_back(newentry);

        gLogging.textOut(gamespecstring);

		// The original episode 1 exe is needed to load gfx's for game launcher menu
		if ( m_ep1slot <= -1 && newentry.crcpass == true )
		{
			m_ep1slot = m_Entries.size()-1;
            gLogging.ftextOut("   Using for in-game menu resources<br>" );
		}
		result = true;
	}
SDL_UnlockDisplay();
    return result;
}


bool CGameLauncher::start()
{
    // CRC init when Launcher starts.
    crc32_init();

    // Here it always makes sense to have the mouse cursor active
    SDL_ShowCursor(SDL_ENABLE);
    Check("CGameLauncher::start()");
    // Set the native resolution
    //gVideoDriver.setNativeResolution(gVideoDriver.getVidConfig().mDisplayRect);

    // In some cases especially when another game was running, the scene wasn't cleaned up.
    // We do this here
    GsWeakSurface blit(gVideoDriver.getBlitSurface());
    blit.fillRGB(0, 0, 0);
Check("CGameLauncher 2");
    // Load the graphics for menu and background.
    // Resources for the main menu
    // This is only for the menu. We only need one fontmap for the list of games and some buttons
    gGraphics.createEmptyFontmaps(1);
    GsFont &Font = gGraphics.getFont(0);

    const auto height = gVideoDriver.getHeight();
Check("CGameLauncher 3");
    // Depending on the height the font is loaded as scaled
    // so readability is better on higher resolutions.
    if(height >= 1920)
    {
        Font.loadinternalFont(8);
    }
    else if(height >= 960)
    {
        Font.loadinternalFont(4);
    }
    else if(height >= 480)
    {
        Font.loadinternalFont(2);
    }
    else
    {
        Font.loadinternalFont(1);
    }
Check("CGameLauncher 4");
    struct GamesScan : public Action
    {
        CGameLauncher &mGameLauncher;

        GamesScan(CGameLauncher &launcher) :
            mGameLauncher(launcher) {}

        int handle()
        {
            if(!mGameLauncher.setupMenu())
            {
                gLogging.textOut(FONTCOLORS::RED,"No game can be launched, because game data files are missing.<br>");
                return 0;
            }

            return 1;
        }
    };        

    mGameScanner.setStyle(PROGRESS_STYLE_TEXT);
    mGameScanner.RunLoadActionBackground(new GamesScan(*this));
    mGameScanner.start();
Check("CGameLauncher 5");
    return true;
}


struct PatchListFiller
{
    std::set<std::string> list;

    bool operator() (const std::string& filename) {
        std::string ext = GetFileExtension(filename);
        if (stringcaseequal(ext, "pat"))
        {
            list.insert(filename);
        }

        return true;
    }
};





void CGameLauncher::showMessageBox(const std::string &text)
{
    std::vector<std::string> strVec = explode(text, "\n");

    mpMsgDialog.reset(new CGUIDialog(GsRect<float>(0.1f, 0.1f, 0.8f, 0.85f),
                                     CGUIDialog::FXKind::EXPAND));

    mpMsgDialog->initEmptyBackground();

    float yStart = 0.1f;
    for( auto &txtItem : strVec)
    {
        mpMsgDialog->addControl(new CGUIText(txtItem), GsRect<float>(0.1f, yStart, 0.8f, 0.05f));
        yStart += 0.05f;
    }


    auto *pRetryButton = new GsButton("Retry", new GMSwitchToGameLauncher());
    mpMsgDialog->addControl(pRetryButton, GsRect<float>(0.2f, 0.85f, 0.2f, 0.05f));

    auto *pOkButton = new GsButton("Ok", new CloseBoxEvent());
    mpMsgDialog->addControl(pOkButton, GsRect<float>(0.6f, 0.85f, 0.2f, 0.05f));
    pOkButton->select(true);
    mpMsgDialog->setCurrentControl(pOkButton);
}


void CGameLauncher::setupModsDialog()
{
    const std::string dataDir = getDirectory( m_chosenGame );

    // TODO: fetch the List of available patch files
    // Get the list of ".pat" files
    PatchListFiller patchlist;
    FindFiles(patchlist, dataDir, false, FM_REG);

    if( patchlist.list.empty() )
    {
        mPatchFilename = "";
        mDonePatchSelection=true;
        return;
    }

    // If the there are not at least 2 mods to select, do not create the patch selection dialog
    if( patchlist.list.size() == 1 )
    {
        mPatchFilename = *(patchlist.list.begin());
        mDonePatchSelection=true;
        return;
    }

    mpPatchDialog.reset(new CGUIDialog(GsRect<float>(0.1f, 0.1f, 0.8f, 0.85f),
                                       CGUIDialog::FXKind::EXPAND));
    mpPatchDialog->initEmptyBackground();

    if(!mPatchStrVec.empty())
        mPatchStrVec.clear();

    mpPatchSelList = new CGUITextSelectionList();


    for( auto &elem : patchlist.list )
    {
        const std::string dirname = GetDirName(elem);
        std::string name = elem.substr(dirname.size()+1);
        name = name.substr(0, name.size()-4);
        mPatchStrVec.push_back(elem);
        mpPatchSelList->addText(name);
    }

    mpPatchSelList->setConfirmButtonEvent(new GMPatchSelected());
    mpPatchSelList->setBackButtonEvent(new GMQuit());

    mpPatchDialog->addControl(new CGUIText("Choose your patch:"), GsRect<float>(0.0f, 0.0f, 1.0f, 0.05f));
    mpPatchDialog->addControl(mpPatchSelList, GsRect<float>(0.01f, 0.07f, 0.49f, 0.87f));


    mpPatchDialog->addControl(new GsButton( "Start >", new GMPatchSelected() ), GsRect<float>(0.65f, 0.865f, 0.3f, 0.07f) );
}




void CGameLauncher::pumpEvent(const CEvent *evPtr)
{
    #ifdef DOWNLOADER
    if( dynamic_cast<const GMDownloadDlgOpen*>(evPtr) )
    {
        setupDownloadDialog();
    }
    #endif


    if( dynamic_cast<const GMStart*>(evPtr) )
    {
        setChosenGame(mpGameSelectionList->getSelection());

        if(m_chosenGame >= 0)
        {
            setupModsDialog();
        }
    }
    else if( dynamic_cast<const GMPatchSelected*>(evPtr) )
    {
        const auto sel = mpPatchSelList->getSelection();
        mPatchFilename = mPatchStrVec[sel];
        mDonePatchSelection = true;
    }
    #ifdef DOWNLOADER
    else if( dynamic_cast<const GameStorePullGame*>(evPtr) )
    {
        pullGame(mpGSSelList->getSelection());
    }
    #endif
    else if( dynamic_cast<const CloseBoxEvent*>(evPtr) )
    {
        if(mpMsgDialog)
        {
            mpMsgDialog = nullptr;
        }
        if(mpGameStoreDialog)
        {
            mpGameStoreDialog = nullptr;
        }
    }
    else if( dynamic_cast<const CancelDownloadEvent*>(evPtr))
    {
        mCancelDownload = true;
        mpDloadCancel->enable(false);
        mpDloadProgressCtrl->setUserAbort(true);
    }
    else if( dynamic_cast<const OpenSettingsMenuEvent*>(evPtr) )
    {
        gEventManager.add( new OpenMenuEvent(
                               new SettingsMenu(GsControl::Style::NONE) ) );
    }
#ifdef VIRTUALPAD
    else if( dynamic_cast<const OpenVGamePadSettingsEvent*>(evPtr) )
    {
        gInput.setInVPadConfigState();
    }
#endif




    // Check Scroll events happening on this Launcher
    if( const MouseWheelEvent *mwe = dynamic_cast<const MouseWheelEvent*>(evPtr) )
    {
        // Wrapper for the simple mouse scroll event
        if(mwe->amount.y > 0.0f)
        {
            mLauncherDialog.sendEvent(new CommandEvent( IC_UP ));
        }
        else if(mwe->amount.y < 0.0f)
        {
            mLauncherDialog.sendEvent(new CommandEvent( IC_DOWN ));
        }
        if(mwe->amount.x < 0.0f)
        {
            mLauncherDialog.sendEvent(new CommandEvent( IC_RIGHT ));
        }
        else if(mwe->amount.x > 0.0f)
        {
            mLauncherDialog.sendEvent(new CommandEvent( IC_LEFT ));
        }
    }
}



void CGameLauncher::ponderGameSelDialog(const float deltaT)
{
    // If GameScanner is running, don't do anything else
    if(mGameScanner.isRunning())
    {
        mGameScanner.run(deltaT);
        return;
    }

    // Did the user press (X)?
    if( gInput.getExitEvent() )
    {
        m_mustquit = true;
        return;
    }

    // Command (Keyboard/Joystick) are handled here
    for( int cmd = IC_LEFT ; cmd < MAX_COMMANDS ; cmd++ )
    {
        if( gInput.getPressedCommand(cmd) )
        {
            mLauncherDialog.sendEvent(new CommandEvent( static_cast<InputCommand>(cmd) ));
            break;
        }
    }

    // Check if the selection changed. Update the right data panel
    if(mSelection != mpGameSelectionList->getSelection())
    {                                
        if(!m_Entries.empty())
        {
            mSelection = mpGameSelectionList->getSelection();
            auto &entry = m_Entries[mSelection];
            const std::string nameText = "Episode " + itoa(entry.episode);
            mpEpisodeText->setText(nameText);
            float fVer = entry.version;
            fVer /= 100.0f;
            mpVersionText->setText("Version: " + ftoa(fVer));

            mpDemoText->setText(entry.demo ? "Demo" : "");

            // Now update the bitmap
            mCurrentBmp->setBitmapPtr(mPreviewBmpPtrVec[mSelection]);
        }
    }

    mLauncherDialog.processLogic();
}



void CGameLauncher::ponderPatchDialog()
{
    if(mpPatchDialog)
        mpPatchDialog->processLogic();

    // Launch the code of the Startmenu here in case a game has been chosen
    if( mDonePatchSelection ) // Means a game has been selected
    {
        mpPatchDialog = nullptr;

        //// Game has been chosen. Launch it!
        // Get the path were to Launch the game
        const std::string DataDirectory = getDirectory( m_chosenGame );

        // We have to check which Episode will be used
        const int episode = getEpisode( m_chosenGame );

        gBehaviorEngine.mPatchFname = mPatchFilename;

        if( episode > 0 ) // The game has to have a valid episode!
        {
            // Get the EXE-Data of the game and load it into the memory.
            bool ok = false;

            if(gKeenFiles.exeFile.readData(episode, DataDirectory))
            {
               ok = true;
            }
            else if(gKeenFiles.exeFile.readMainPythonScript(episode, DataDirectory))
            {
               ok = true;
            }

            if(!ok)
            {
                letchooseagain();
            }
            else
            {
                gKeenFiles.gameDir = DataDirectory;

                if(episode >= 1 && episode <= 7)
                {
                    // Now let's decide which engine we have to start.
                    if(episode >= 1 && episode <= 3)
                    {
                        gEventManager.add( new StartVorticonEngine(false, episode, DataDirectory) );
                    }
                    else if(episode >= 4 && episode <= 6)
                    {
                        gEventManager.add( new StartGalaxyEngine(false, episode, DataDirectory) );
                    }
                    else if(episode == 7)
                    {
                        gEventManager.add( new StartDreamsEngine(false, DataDirectory) );
                    }

                }

                else // Everything else cannot be
                {
                    gLogging.textOut("Something is wrong with your Episode configuration! Please check the game your chose");
                }
            }
        }
        else
        {
            letchooseagain();
            gLogging.textOut(FONTCOLORS::RED,"No Suitable game was detected in this path! Please check its contents!\n");
        }
    }
}


////
// Process Routine
////
void CGameLauncher::ponder(const float deltaT)
{
    // In case after display/video setting changes, we need to reset the native resolution
    if(gVideoDriver.getRefreshSignal())
    {
        // Set the native resolution
        gVideoDriver.setNativeResolution(gVideoDriver.getVidConfig().mDisplayRect);

        gVideoDriver.setRefreshSignal(false);
    }

    if(mpMsgDialog)
    {
        // Command (Keyboard/Joystick) events for the game center dialog
        for( int cmd = IC_JUMP ; cmd < MAX_COMMANDS ; cmd++ )
        {
            if( gInput.getPressedCommand(cmd) )
            {
                mpMsgDialog->sendEvent(new CommandEvent( static_cast<InputCommand>(cmd) ));
                break;
            }
        }

        mpMsgDialog->processLogic();
        return;
    }

    if(gMenuController.active())
    {
        return;
    }

    #ifdef DOWNLOADER
    if(mpGameStoreDialog)
    {
        // Command (Keyboard/Joystick) events for the game center dialog
        for( int cmd = IC_LEFT ; cmd < MAX_COMMANDS ; cmd++ )
        {
            if( gInput.getPressedCommand(cmd) )
            {
                mpGameStoreDialog->sendEvent(new CommandEvent( static_cast<InputCommand>(cmd) ));
                break;
            }
        }

        mpGameStoreDialog->processLogic();
        ponderDownloadDialog();
        return;
    }
    #endif

    // Button should disabled unless a game was selected
    if(mpStartButton)
    {
        if(mpGameSelectionList->getSelection() >= 0)
        {
            mpStartButton->enable(true);
        }
        else
        {
            mpStartButton->enable(false);
        }
    }


    if(!mDonePatchSelection && m_chosenGame < 0)
    {
        ponderGameSelDialog(deltaT);
    }
    else if(m_chosenGame >= 0)
    {
        ponderPatchDialog();
    }
    else if(getQuit())
    {
        // User chose "exit". So make CG quit...
        gEventManager.add( new GMQuit() );
    }
}


void CGameLauncher::render()
{
    if(mpMsgDialog)
    {
        mpMsgDialog->processRendering();
        return;
    }

    if(!mDonePatchSelection && m_chosenGame < 0)
    {
        // If GameScanner is running, don't do anything else
        if(mGameScanner.isRunning())
        {
            mGameScanner.render();
            return;
        }

        // Do the rendering of the dialog
        mLauncherDialog.processRendering();
    }


    /*
    // Test picture code for Screen tilting
    auto sfc = gVideoDriver.getBlitSurface();

    SDL_Rect borderRect;

    SDL_FillRect(sfc, nullptr, SDL_MapRGB(sfc->format,255,255,0));

    borderRect.x = 0; borderRect.y = 0;
    borderRect.h = 2; borderRect.w = sfc->w;
    SDL_FillRect(sfc, &borderRect, SDL_MapRGB(sfc->format,255,0,255));

    borderRect.x = 0; borderRect.y = sfc->h-2;
    borderRect.h = 2; borderRect.w = sfc->w;
    SDL_FillRect(sfc, &borderRect, SDL_MapRGB(sfc->format,255,0,255));

    borderRect.x = 0; borderRect.y = 0;
    borderRect.h = sfc->h; borderRect.w = 2;
    SDL_FillRect(sfc, &borderRect, SDL_MapRGB(sfc->format,0,255,0));

    borderRect.x = sfc->w-2; borderRect.y = 0;
    borderRect.h = sfc->h; borderRect.w = 2;
    SDL_FillRect(sfc, &borderRect, SDL_MapRGB(sfc->format,0,255,0));
*/
    // Do the rendering of the dialog
    if(mpPatchDialog)
        mpPatchDialog->processRendering();

    if(mpGameStoreDialog)
        mpGameStoreDialog->processRendering();

}


void CGameLauncher::getLabels()
{
    bool found;
    Uint16 i;
    std::string line, dir;
    std::ifstream gamescfg;

    m_Names.clear();
    m_Paths.clear();

    OpenGameFileR(gamescfg, GAMESCFG);
    if (gamescfg.is_open())
    {
        while ( !gamescfg.eof() )
        {
            getline(gamescfg,line);

            if (strncmp(line.c_str(),GAMESCFG_DIR,strlen(GAMESCFG_DIR)) == 0)
            {
                dir = line.substr(strlen(GAMESCFG_DIR));

                // Check for duplicates
                found = false;
                for ( i=0; i<m_Paths.size(); i++ )
                {
                    if (strncmp(m_Paths.at(i).c_str(),dir.c_str(),dir.length()) == 0)
                    {
                        found = true;
                        break;
                    }
                }

                // If not a duplicate get the custom name
                if (!found)
                {
                    getline(gamescfg,line);
                    if (strncmp(line.c_str(),GAMESCFG_NAME,strlen(GAMESCFG_NAME)) == 0)
                    {
                        m_Paths.push_back( dir );

                        std::string gamePath = line.substr(strlen(GAMESCFG_NAME));

                        m_Names.push_back( filterGameName(gamePath) );
                    }
                }
            }
        }
        gamescfg.close();
    }
}

std::string CGameLauncher::scanLabels(const std::string& path)
{
    Uint16 i;

    for ( i=0; i<m_Paths.size(); i++ )
    {
        if (strncmp(m_Paths.at(i).c_str(),path.c_str(),path.length()) == 0)
        {
            return m_Names.at(i);
        }
    }
    return "";
}

void CGameLauncher::putLabels()
{
    Uint16 i;
    std::string line;
    std::ofstream gamescfg;

    OpenGameFileW(gamescfg, GAMESCFG);
    if (gamescfg.is_open())
    {
        for ( i=0; i<m_Entries.size(); i++ )
        {
            line = GAMESCFG_DIR + m_Entries.at(i).exefilename;
            gamescfg << line << std::endl;
            line = GAMESCFG_NAME + m_Entries.at(i).name;
            gamescfg << line << std::endl << std::endl;
        }
        gamescfg.close();
    }
}
