/*
 * GsApp.cpp
 *
 *  Created on: 01.05.2009
 *      Author: gerstrong
 *  This Game-engine stripes down the main function
 *  and provides more dynamic control over the game engine itself
 * 
 *  It also manages the load of drivers and main game cycle
 */

#include "GsApp.h"

#include <base/GsTimer.h>
#include <base/GsLogging.h>
#include <base/video/CVideoDriver.h>
#include <base/video/GsEffectController.h>
#include <base/utils/StringUtils.h>
#include <graphics/GsGraphics.h>
#include <widgets/GsMenuController.h>


#include <base/CInput.h>
#include <base/GsArguments.h>


std::string getArgument( int argc, char *argv[], const std::string& text )
{
    std::string argument;
    for( int i=1 ; i<argc ; i++ )
    {
        argument = argv[i];
        if( argument.find(text) == 0 ) // argument was found!
            return argument;
    }
    return "";
}

bool getBooleanArgument( int argc, char *argv[], const std::string& text )
{
    std::string argument;
    for( int i=1 ; i<argc ; i++ )
    {
        argument = argv[i];
        if( argument.find(text) == 0 ) // argument was found!
            return true;
    }
    return false;
}




GsApp::GsApp() :
mSink(this)
{
    gEventManager.regSink(&mSink);
}


///////////////////////////////
// Cleanup Game Engine here! //
///////////////////////////////
/**
 * \brief  			This function cleans up all the used engines and the singleton
 * 					classes that were used during the program.
 * 					This can happen at the end of the program
 * 					or when an engine may be changed.
 */

void GsApp::cleanup()
{
    gInput.shutdown();
}

GsApp::~GsApp()
{
    gEventManager.unregSink(&mSink);
}


//////////////////////////////////
// Initialize Game Engine here! //
//////////////////////////////////

/**
 * \brief  This is the function where CG beings
 *
 * \param	argc	number of arguments
 * \param	argv	pointer to  char arrays where
 * 					where the passed arguments are stored
 * 					in the process
 * \return			If the function could setup
 * 					the game, it will return true, else it
 * 					will be false.
 */
bool GsApp::init(int argc, char *argv[])
{
    // Pass all the arguments
    gArgs.passArgs(argc, argv);

	// Setup the Hardware using the settings we have loaded
	gLogging.textOut(FONTCOLORS::GREEN,"Loading hardware settings...<br>");
    if(!loadDrivers())
	{
        gLogging.textOut(FONTCOLORS::RED,"The program cannot start, because you do not meet the hardware requirements.<br>");
		return false;
	}

    return true;
}


void GsApp::pumpEvent(const CEvent *evPtr)
{
    if( const SwitchEngineEvent *swEng = dynamic_cast<const SwitchEngineEvent*>(evPtr) )
    {
        SwitchEngineEvent *swEngVar = const_cast<SwitchEngineEvent*>(swEng);
        mpCurEngine.swap( swEngVar->mpEnginePtr );

        if(!mpCurEngine->start())
        {
            gEventManager.add( new GMQuit() );
        }
    }
    else if( dynamic_cast<const GMQuit*>(evPtr) )
    {
        mpCurEngine.release();
    }
    else if( const InvokeFunctorEvent *iEv = dynamic_cast<const InvokeFunctorEvent*>(evPtr) )
    {
        (*iEv)();
    }
    else // none of the above, let's see if the children have events to be processed
    {
        mpCurEngine->pumpEvent(evPtr);
        gMenuController.pumpEvent(evPtr);
    }
}


void GsAppEventSink::pumpEvent(const CEvent *evPtr)
{
    mpApp->pumpEvent(evPtr);
}


void GsApp::pollEvents()
{        
    if( gInput.getExitEvent() )
    {        
      mpCurEngine.release();
      return;
    }
}




/**
 * \brief  			This function will try to load the hardware
 * 					resources, which are the video driver and the sound.
 * 					Input stuff is only loaded, when CG needs it, what mostly
 * 					happens, when it has started.
 *
 * \return			If the function could load
 * 					the drivers, it will return true, else it
 * 					will be false.
 */
// Load the driver needed to start the game
bool GsApp::loadDrivers()
{
	// Init graphics
    if (!gVideoDriver.start()) return false;
		
	return true;
}



////
// Process Routine
////
// This function is run every time, the Timer says so, through.
void GsApp::ponder(const float deltaT)
{
    gInput.ponder();
    pollEvents();

    // Process the game control object if no effects are being processed
    if(mpCurEngine)
    {
        mpCurEngine->ponder(deltaT);
    }

    // Apply graphical effects if any. It does not render, it only prepares for the rendering task.
    gEffectController.run(deltaT);

    gMenuController.ponder(deltaT);
}

void GsApp::render()
{
    if(mpCurEngine)
    {
        mpCurEngine->render();
    }

    gMenuController.render();

    gInput.render();    
}



void GsApp::setEngine(GsEngine *engPtr)
{
    mpCurEngine.reset(engPtr);
}


///////////////////////
// This is the main cycle //
///////////////////////
/**
 * \brief  	This is the main run cycle of the game,
 * 		no matter what happens in the game logic or
 * 		which engine is chosen, it always get to this point
 * 		Mainly timer and logic processes are performed here.
 */
void GsApp::runMainCycle()
{
    // I hope the engine has been set. Otherwise quit the app
    assert(mpCurEngine);

    mpCurEngine->start();

    float acc = 0.0f;
    float start = 0.0f;
    float elapsed = 0.0f;
    float total_elapsed = 0.0f;
    float curr = 0.0f;
    int counter = 0;

    while(1)
    {
        const float logicLatency = gTimer.LogicLatency();

        const  bool vsyncEnabled = gVideoDriver.isVsync();

        curr = timerTicks();

        if(gTimer.resetLogicSignal())
            start = curr;        

        if(vsyncEnabled)
        {
            start = timerTicks();

            // Game cycle
            {                            
                // Poll Inputs
                gInput.pollEvents();

                // Process App Events
                gEventManager.processSinks();

                // Ponder Game Control
                ponder(logicLatency);
            }

            // Now we render the whole GameControl Object to the blit surface
            render();

            // Apply graphical effects if any.
            gEffectController.render();

            // Pass all the surfaces to one. Some special surfaces are used and are collected here
            gVideoDriver.collectSurfaces();

            // Now you really render the screen
            // When enabled, it also applies Filters
            gVideoDriver.updateDisplay();

            elapsed = timerTicks() - start;
            total_elapsed += elapsed;

            if( mustShutdown() )
                break;
        }
        else
        {

            const float renderLatency = gTimer.RenderLatency();

            elapsed = curr - start;

            start = timerTicks();

            acc += elapsed;

            // Perform the game cycle
            while( acc > logicLatency )
            {
                // Poll Inputs
                gInput.pollEvents();

                // Process App Events
                gEventManager.processSinks();

                // Ponder Game Control
                ponder(logicLatency);

                acc -= logicLatency;
            }

            // Now we render the whole GameControl Object to the blit surface
            render();

            // Apply graphical effects if any.
            gEffectController.render();

            // Pass all the surfaces to one. Some special surfaces are used and are collected here
            gVideoDriver.collectSurfaces();

            // Now you really render the screen
            // When enabled, it also applies Filters
            gVideoDriver.updateDisplay();


            elapsed = timerTicks() - start;
            total_elapsed += elapsed;

            if( mustShutdown() )
                break;

            // If renderLatency is zero or less, delays won't happens.
            // This means the system renders as much as possible
            // with violating the the LPS
            auto fWaitTime = -elapsed;

            if(renderLatency > 0.0)
            {
                fWaitTime = renderLatency+fWaitTime;
            }

            // wait time remaining in current loop
            if( fWaitTime > 0.0 )
            {
                const auto waitTime = static_cast<Uint32>(fWaitTime);
                timerDelay( waitTime );
                total_elapsed += static_cast<float>(waitTime);
            }

        }

        // This will refresh the fps display, so it stays readable and calculates an average value.
        counter++;
        if(counter >= 100)
        {
            counter = 0;
            gTimer.setTimeforLastLoop(total_elapsed/100.0f);
            total_elapsed = 0.0f;
        }
    }

    cleanup();
}

