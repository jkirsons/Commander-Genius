/*
 * CSoundChannel.h
 *
 *  Created on: 23.05.2009
 *      Author: gerstrong
 */

#ifndef CSOUNDCHANNEL_H_
#define CSOUNDCHANNEL_H_

#include <SDL.h>
#include <map>
#include "CSoundSlot.h"
//#include "common/CAudioResources.h"

// sound play modes
// start playing sound now, unless a higher priority sound is playing
enum class SoundPlayMode
{
    PLAY_NOW,// Normal play now
    PLAY_NORESTART, // will not restart the sound if it is already playing.
    PLAY_FORCE, // plays the sound regardless of priority, and does not allow any other
    PLAY_PAUSEALL   // This will play the sound and pause the gameplay while it does
};

// 8 bit sound
#define WAVEFORM_VOLUME_8	5
#define WAVE_SILENCE_U8         128
#define WAVE_SILENCE_S8         0
#define WAVE_IN_U8		(WAVE_SILENCE_U8-WAVEFORM_VOLUME_8)
#define WAVE_OUT_U8		(WAVE_SILENCE_U8+WAVEFORM_VOLUME_8)
#define WAVE_IN_S8		(WAVE_SILENCE_S8-WAVEFORM_VOLUME_8)
#define WAVE_OUT_S8		(WAVE_SILENCE_S8+WAVEFORM_VOLUME_8)

// 16 bit sound
#define WAVEFORM_VOLUME_16	1280
#define WAVE_SILENCE_U16        32768
#define WAVE_SILENCE_S16        0
#define WAVE_IN_U16		(WAVE_SILENCE_U16-WAVEFORM_VOLUME_16)
#define WAVE_OUT_U16		(WAVE_SILENCE_U16+WAVEFORM_VOLUME_16)
#define WAVE_IN_S16		(WAVE_SILENCE_S16-WAVEFORM_VOLUME_16)
#define WAVE_OUT_S16		(WAVE_SILENCE_S16+WAVEFORM_VOLUME_16)

#define SLOW_RATE      90		// Wait time for resampling PC Speaker Sound.


class CSoundChannel
{
public:
    CSoundChannel(const SDL_AudioSpec &AudioSpec);

    CSoundChannel(const CSoundChannel &chnl);


    virtual ~CSoundChannel();

	void stopSound();
    bool isPlaying();
    bool isForcedPlaying() { return (isPlaying() && mSoundForced); }
    CSoundSlot *getCurrentSoundPtr() { return mpCurrentSndSlot; }

	/**
	 * \brief	Reads the sound of a specified slot into the waveform which normally is mixed
	 * \param	waveform 8-bit data array where the mixform will be written to
	 * \param	length in bytes that have to be read
	 * \warning	If there is no sound curently assigned to be played, please don't call that function.
	 * 			It might crash. Call setupSound first before you call this one!
	 */
    //void readWaveform( Uint8 * const waveform, const Uint32 len );
	template <typename T>
	void transintoStereoChannels(T* waveform, const Uint32 len);

    short getBalance() { return mBalance; }
    void setBalance(short value) { mBalance = value; }

	/**
	 * \brief	Sets up the slot to play a sound
	 * \param	SndSlottoPlay	Reference to the slot that has to be played
	 * \param	sound_forced	This will play a sound again even if it's already playing. Use this one wise
	 */
	void setupSound(CSoundSlot &SndSlottoPlay,
					const bool sound_forced );

private:

    CSoundSlot *mpCurrentSndSlot = nullptr;		// Pointer to the slot of the currently playing sound
    Uint32 mSoundPtr = 0;               	// position within sound that we're at
    bool mSoundPaused = true;             	// true = pause playback
    bool mSoundForced = false;

    short mBalance = 0;					// This variable is used for stereo sound, and to calculate where the sound must be played!

    SDL_AudioSpec m_AudioSpec;

    static int mTotNumChannels;
    int mId = 0;
};

#endif /* CSOUNDCHANNEL_H_ */
