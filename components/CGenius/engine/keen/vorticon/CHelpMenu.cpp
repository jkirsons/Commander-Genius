/*
 * CHelpMenu.cpp
 *
 *  Created on: 22.05.2010
 *      Author: gerstrong
 */
/*
#include "CHelpMenu.h"

#include "CStory.h"
#include "CCredits.h"
#include "COrderingInfo.h"
#include "CAbout.h"
#include "CHelp.h"
#include "CPreviews.h"
#include "fileio/ResourceMgmt.h"

#include <base/utils/StringUtils.h>
#include <fileio/KeenFiles.h>


CHelpMenu::CHelpMenu(const GsControl::Style style) :
GameMenu( GsRect<float>(0.25f, 0.24f, 0.5f, 0.5f), style )
{

    // Create the Menu Dialog and entries
    
    mpMenuDialog->addControl(new GsButton( "The Game",
                                           new StartInfoSceneEvent( new CHelp("Game") ) ) );
    
    mpMenuDialog->addControl(new GsButton( "The Story",
                                           new StartInfoSceneEvent( new CStory ) ) );
    
    mpMenuDialog->addControl(new GsButton( "Ordering Info",
                                           new StartInfoSceneEvent( new COrderingInfo ) ) );
    
    mpMenuDialog->addControl(new GsButton( "About ID",
                                           new StartInfoSceneEvent( new CAbout("ID") ) ) );
    
    mpMenuDialog->addControl(new GsButton( "About CG",
                                           new StartInfoSceneEvent( new CAbout("CG") ) ) );
    
    mpMenuDialog->addControl(new GsButton( "Credits",
                                           new StartInfoSceneEvent( new CCredits() ) ) );
    
    std::string filename = "previews.ck";
    filename += itoa(gBehaviorEngine.getEpisode());	
    
    filename = getResourceFilename(filename, 
                                   gKeenFiles.gameDir, 
                                   false, false);
    
    if( filename != "" )
    {
        mpMenuDialog->addControl(new GsButton( "Previews!",
                                               new StartInfoSceneEvent( new CPreviews() ) ) );
        
    }
}
*/