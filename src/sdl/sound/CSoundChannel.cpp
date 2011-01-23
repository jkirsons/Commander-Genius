/*
 * CSoundChannel.cpp
 *
 *  Created on: 23.05.2009
 *      Author: gerstrong
 */

#include "CSoundChannel.h"
#include <vector>

CSoundChannel::CSoundChannel(SDL_AudioSpec AudioSpec) :
m_AudioSpec(AudioSpec)
{
	m_freq_corr = 0;	// used for correcting PC-Speaker sampling for different frequencies
	m_sound_ptr = 0;
	m_sound_timer = 0;
	m_sound_playing = false;
	m_waveState = 0;
	m_sound_paused = true;
	m_sound_forced = false;
	m_desiredfreq = 0;     	// current desired frequency in hz
	m_changerate = 0;      	// frequency in samples (calculated)
	m_freqtimer = 0;       	// time when to change waveform state
	m_waveState = 0;       	// current position of the output waveform
	m_balance = 0;
	
	setFormat(AudioSpec.format);
	setFrequencyCorrection(AudioSpec.freq);
}

CSoundChannel::~CSoundChannel() {
}

void CSoundChannel::setFrequencyCorrection(int freq)
{
	switch (freq)
	{
		case 48000: m_freq_corr = 16; break;
		case 44100: m_freq_corr = 15; break;
		case 22050: m_freq_corr = 14; break;
		case 11025: m_freq_corr = 12; break;
		default: m_freq_corr = 0; break;
	}
}

void CSoundChannel::stopSound(void)
{
    m_sound_ptr = 0;
    m_sound_timer = 0;
    m_sound_playing = false;
    m_waveState = m_wavein;
    m_freqtimer = 0;
}

void CSoundChannel::setFormat( Uint16 format )
{
	m_AudioSpec.format = format;
	switch( m_AudioSpec.format )
	{
		case AUDIO_U8:
			m_volume	= WAVEFORM_VOLUME_8;
			m_waveout	= WAVE_OUT_U8;
			m_wavein	= WAVE_IN_U8;
			break;
		case AUDIO_S8:
			m_volume	= WAVEFORM_VOLUME_8;
			m_waveout	= WAVE_OUT_S8;
			m_wavein	= WAVE_IN_S8;
			break;
		case AUDIO_U16:
			m_volume	= WAVEFORM_VOLUME_16;
			m_waveout	= WAVE_OUT_U16;
			m_wavein	= WAVE_IN_U16;
			break;
		case AUDIO_S16:
			m_volume	= WAVEFORM_VOLUME_16;
			m_waveout	= WAVE_OUT_S16;
			m_wavein	= WAVE_IN_S16;
			break;
	}

	m_waveState = m_wavein;
}


void CSoundChannel::setupSound(GameSound current_sound,
							   unsigned int sound_timer,
							   bool playing,
							   unsigned int freqtimer,
							   bool sound_forced,
							   Uint16 format)
{
	setFormat( format );
	m_current_sound = current_sound;
	m_sound_timer = sound_timer;
	m_sound_playing = playing;
	m_sound_ptr = 0;
	m_waveState = m_wavein;
	m_freqtimer = freqtimer;
	m_sound_forced = sound_forced;
}

// generates len bytes of waveform for the channel.
template <typename T>
void CSoundChannel::generateWaveform(T *waveform, CSoundSlot &SndSlot, unsigned int len, int frequency, bool stereo )
{ 
	len /= sizeof(T);	
	std::vector<T> WaveBuffer(len);
	
	unsigned int halffreq = (frequency>>1);
	
	int waittime = // How much do we divide wait time for change?
	(m_freq_corr == 16) ?
		(frequency / SLOW_RATE)
	:
		(((m_freq_corr*frequency)>>4) / SLOW_RATE);
	
	// setup so we process a new byte of the sound first time through
	bool firsttime = true;
	
	for(unsigned int index=0 ; index<len ; index++)
	{
		if (!m_sound_timer || firsttime)
		{
			// get new frequency and compute how fast we have to
			// change the wave data
			if(m_sound_ptr < SndSlot.getSoundlength())
				m_desiredfreq = SndSlot.getSoundData()[m_sound_ptr];
			else
				m_desiredfreq = 0xffff;
			
			if (m_desiredfreq==0xffff)
			{  // end of sound...fill rest of buffer with silence
				for(;index<len;index++)
				{
					WaveBuffer[index] = m_AudioSpec.silence;
				}
				m_sound_playing = false;
				
				memcpy(waveform, &WaveBuffer[0],len*sizeof(T));
				
				m_sound_ptr = 0;
				
				return;
			}
			else if (m_desiredfreq == 0x0000)
			{
				m_waveState = m_waveout;
			}
			else
			{  // compute change rate
				m_changerate = (halffreq / m_desiredfreq);
			}
			
			m_sound_ptr++;
			if (!firsttime) m_sound_timer = waittime;
			firsttime = 0;
		}
		
		if (m_sound_timer) m_sound_timer--;
		
		if (m_desiredfreq==0x0000)
		{   // silence
			WaveBuffer[index] = m_AudioSpec.silence;
		}
		else
		{
			// time to change waveform state?
			if (m_freqtimer > m_changerate)
			{  // toggle waveform, generating a square wave
				int i=0;
				
				if(m_sound_ptr > 0)
					i=m_volume;
				
				if (m_waveState == m_AudioSpec.silence-i)
					m_waveState = m_AudioSpec.silence+i;
				else
					m_waveState = m_AudioSpec.silence-i;
				
				m_freqtimer = 0;
			}
			else m_freqtimer++;
			
			// put wave data into buffer
			WaveBuffer[index] = m_waveState;
		}
		
		if(stereo) // Primitive stereo transformation
		{
			if(index < len)
			{
				index++;
				WaveBuffer[index] = m_waveState;
			}
		}
	}
	memcpy(waveform, &WaveBuffer[0],len*sizeof(T));
}

/** \brief This program reads the balance information and balances the stereo sound
 * 	\param waveform	pass it as 8-bit or 16-bit Waveform pointer depeding on what depth you have
 *  \param len 		length in bytes of the waveform
 */
template <typename T>
void CSoundChannel::transintoStereoChannels(T* waveform, const Uint32 len)
{
	if(m_balance != 0) // Because, if it is not zero, there is no balance, and waves must be adapted
	{
		// -127 is only for the left speaker, while 127 is for the right speaker. 0 Is center
		Sint32 Pulse32;
		const Sint32 Silence = m_AudioSpec.silence;
		const Sint32 balance = m_balance;
		const Uint32 length = len/sizeof(T);
		
		// balance the left channel.
		for( Uint32 index = 0 ; index < length ; )
		{
			// balance here!
			// first the left channel
			Pulse32 = waveform[index] - Silence;
			Pulse32 *= (129 - balance);
			Pulse32 >>= 8;
			waveform[index++] = Pulse32 + Silence;

			// then the right channel
			Pulse32 = waveform[index] - Silence;
			Pulse32 *= (129 + balance);
			Pulse32 >>= 8;
			waveform[index++] = Pulse32 + Silence;
		}
	}
}

void CSoundChannel::readWaveform(CSoundSlot *pSndSlot, Uint8* waveform, int len, Uint8 channels, int frequency)
{
    if (m_sound_playing)
    {
    	CSoundSlot &SndSlot = pSndSlot[m_current_sound];

    	if(SndSlot.isHighQuality()) // There is no HQ sound in the buffer
     	{
     		stHQSound& hqsound = *(SndSlot.getHQSoundPtr());

         	if ((m_sound_ptr + (Uint32)len) >= hqsound.sound_len)
         	{
         		// Fill the rest with silence
         		memset(waveform, m_AudioSpec.silence, len );
         		m_sound_ptr = 0;
         		m_sound_playing = false;
         	}
         	else
         	{
         		memcpy(waveform, hqsound.sound_buffer + m_sound_ptr, len);
             	m_sound_ptr += len;
         	}
       	}
     	else
     	{
			if ( m_AudioSpec.format == AUDIO_U16 || m_AudioSpec.format == AUDIO_S16 )
				generateWaveform( (Sint16*) (void *) waveform, SndSlot,  len, frequency, (channels==2) ? true : false );
			else
				generateWaveform( (Uint8*) waveform, SndSlot, len, frequency, (channels==2) ? true : false );
     	}
		
    	if(channels == 2)
    	{
    		if(m_AudioSpec.format == AUDIO_U16 || m_AudioSpec.format == AUDIO_S16)
    			transintoStereoChannels((Sint16*) (void *) waveform, len);
    		else
    			transintoStereoChannels(waveform, len);
    	}

    }
	else
		memset(waveform, m_AudioSpec.silence, len);
}
