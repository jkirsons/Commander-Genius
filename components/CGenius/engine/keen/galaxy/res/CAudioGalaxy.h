/*
 * CAudioGalaxy.h
 *
 *  Created on: 23.01.2011
 *      Author: gerstrong
 */

#ifndef CAUDIOGALAXY_H_
#define CAUDIOGALAXY_H_

#include <sdl/audio/CAudioResources.h>
#include "fileio/CExeFile.h"

class CAudioGalaxy : public CAudioResources
{
public:


    bool readPCSpeakerSoundintoWaveForm(CSoundSlot &soundslot, const byte *pcsdata, const Uint8 formatsize );

    /**
     * @brief LoadFromAudioCK   Load audio from the CK files
     * @return true if everything went finde otherwise false
     */
    bool LoadFromAudioCK(const unsigned int dictOffset);

    bool loadSoundData(const unsigned int dictOffset);


    void unloadSound() {}
	
	std::map< unsigned int, std::map<GameSound, int> > sndSlotMapGalaxy;
	
private:
	void setupAudioMap();

};

#endif /* CAUDIOGALAXY_H_ */
