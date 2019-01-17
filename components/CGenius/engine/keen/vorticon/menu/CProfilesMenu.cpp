/*
 * CProfilesMenu.cpp
 *
 *  Created on: 14.08.2010
 *      Author: gerstrong
 */

#include "CProfilesMenu.h"
#include "CSelectionMenu.h"
#include "engine/core/CBehaviorEngine.h"
#include <base/video/CVideoDriver.h>
#include <base/GsTimer.h>
#include "engine/core/CBehaviorEngine.h"
#include "engine/core/CSettings.h"
#include <base/GsEvent.h>
#include "VorticonMenu.h"
#include "widgets/Button.h"
#include <list>
#include <string>


/**
 * \brief This sets the default settings for a classic gameplay
 */
/*
class SetDefaultClassic : public InvokeFunctorEvent
{
public:
    void operator()() const
	{
        auto &option = gBehaviorEngine.mOptions;
		gSettings.loadDrvCfg();
		st_camera_bounds &CameraBounds = gVideoDriver.getCameraBounds();
		CameraBounds.left = 140;
		CameraBounds.up = 50;
		CameraBounds.right = 180;
		CameraBounds.down = 130;
		CameraBounds.speed = 5;
		gTimer.setFPS(30);
        option[GameOption::ALLOWPKING].value = 0;
        option[GameOption::KEYSTACK].value = 0;
        option[GameOption::LVLREPLAYABILITY].value = 0;
        option[GameOption::RISEBONUS].value = 0;
        option[GameOption::MODERN].value = 1;
        option[GameOption::HUD].value = 0;
        option[GameOption::SPECIALFX].value = 0;
		gSettings.saveDrvCfg();
	}
};
*/

/**
 * \brief This sets the default settings for an enhanced gameplay
 */
/*
class SetDefaultEnhanced : public InvokeFunctorEvent
{
public:
    void operator()() const
	{
        auto &option = gBehaviorEngine.mOptions;
		gSettings.loadDrvCfg();
		st_camera_bounds &CameraBounds = gVideoDriver.getCameraBounds();
		CameraBounds.left = 152;
		CameraBounds.up = 92;
		CameraBounds.right = 168;
		CameraBounds.down = 108;
		CameraBounds.speed = 20;
		gTimer.setFPS(60);
        option[GameOption::KEYSTACK].value = 1;
        option[GameOption::RISEBONUS].value = 1;
        option[GameOption::MODERN].value = 1;
        option[GameOption::HUD].value = 1;
        option[GameOption::SPECIALFX].value = 1;
		gSettings.saveDrvCfg();
	}
};

namespace vorticon
{

CProfilesMenu::CProfilesMenu() :
VorticonMenu( GsRect<float>(0.25f, 0.4f, 0.5f, 0.2f) )
{
    mpMenuDialog->addControl(new Button( "Classic mode",
										     new SetDefaultClassic() ) );

    mpMenuDialog->addControl(new Button( "Enhanced mode",
										     new SetDefaultEnhanced() ) );
}

};
*/
