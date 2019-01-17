/*
 * CAudioSettings.cpp
 *
 *  Created on: 28.11.2009
 *      Author: gerstrong
 */

#include "AudioSettings.h"
#include <base/utils/StringUtils.h>

#include "engine/core/CBehaviorEngine.h"
#include "engine/core/CSettings.h"
#include "sdl/audio/music/CMusic.h"
#include "sdl/audio/Audio.h"


CAudioSettings::CAudioSettings(const GsControl::Style &style) :
GameMenu(GsRect<float>(0.075f, 0.24f, 0.85f, 0.4f), style )
{

#if !defined(EMBEDDED)

    mpRate = new ComboSelection( "Rate", gSound.getAvailableRateList(), style);
	mpMenuDialog->addControl( mpRate );

#endif

    mpStereo = new Switch( "Stereo", style );
	mpMenuDialog->addControl( mpStereo );


#if !defined(EMBEDDED)

    mpDepth = new ComboSelection( "Depth", filledStrList( 2, "8-bit", "16-bit" ), style );
	mpMenuDialog->addControl( mpDepth );

#endif

    mpSBToggle = new ComboSelection( "Card", filledStrList( 2, "PC Speaker", "Soundblaster" ), style );
	mpMenuDialog->addControl( mpSBToggle );

    mpSoundVolume = new NumberControl( "Sound Vol", 0, 100, 4, false,
                                       gSound.getSoundVolume(), style );
	mpMenuDialog->addControl( mpSoundVolume );


    mpMusicVolume = new NumberControl( "Music Vol", 0, 100, 4, false,
                                       gSound.getMusicVolume(), style );
	mpMenuDialog->addControl( mpMusicVolume );

	setMenuLabel("SNDEFFMENULABEL");

}


void CAudioSettings::refresh()
{
	mAudioSpec = gSound.getAudioSpec();
	mSoundblaster = gSound.getSoundBlasterMode();
	mSoundVolume = mpSoundVolume->getSelection();
    mMusicVolume = mpMusicVolume->getSelection();

    mpSoundVolume->setSelection(gSound.getSoundVolume());
    mpMusicVolume->setSelection(gSound.getMusicVolume());

#if !defined(EMBEDDED)
	mpRate->setSelection( itoa(mAudioSpec.freq) );
#endif
	mpStereo->enable( mAudioSpec.channels == 2 );

#if !defined(EMBEDDED)
	mpDepth->setSelection( mAudioSpec.format == AUDIO_U8 ? "8-bit" : "16-bit" );
#endif
	mpSBToggle->setSelection( mSoundblaster ? "Soundblaster" : "PC Speaker" );
    gMusicPlayer.play();
}


void CAudioSettings::ponder(const float deltaT)
{
    GameMenu::ponder(0);

	if( mSoundVolume != mpSoundVolume->getSelection() )
    {
		gSound.playSound(SOUND_GET_ITEM);
    }

	mSoundVolume = mpSoundVolume->getSelection();
    mMusicVolume = mpMusicVolume->getSelection();

    gSound.setSoundVolume( mSoundVolume );
    gSound.setMusicVolume( mMusicVolume );
}


void CAudioSettings::release()
{
#if !defined(EMBEDDED)
	mAudioSpec.freq = atoi( mpRate->getSelection().c_str() );
#endif

	mAudioSpec.channels = mpStereo->isEnabled() ? 2 : 1;

#if !defined(EMBEDDED)
	mAudioSpec.format = mpDepth->getSelection() == "8-bit" ? AUDIO_U8 : AUDIO_S16;
#endif

	mSoundblaster = ( mpSBToggle->getSelection() == "Soundblaster" ? true : false );

//	gSound.unloadSoundData();
//	gSound.destroy();
	gSound.setSettings(mAudioSpec, mSoundblaster);
	gSound.init();

//	gMusicPlayer.reload();

	gSettings.saveDrvCfg();
}

