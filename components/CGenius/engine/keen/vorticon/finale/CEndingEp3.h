/*
 * CEndingEp3.h
 *
 *  Created on: 04.11.2009
 *      Author: gerstrong
 */

#ifndef CENDINGEP3_H_
#define CENDINGEP3_H_

#include "CFinale.h"
#include "engine/core/CMap.h"
#include "../ai/CPlayer.h"
#include "../dialog/CMessageBoxVort.h"
#include <base/GsTimer.h>
#include <vector>
#include <list>
#include <SDL.h>
#include <memory>

class CEndingEp3 : public CFinale
{
public:
	CEndingEp3(std::list< std::shared_ptr<CMessageBoxVort> > &messageBoxes,
			const std::shared_ptr<CMap> &pMap, 
			std::vector<CPlayer> &Player, 
			std::vector< std::unique_ptr<CVorticonSpriteObject> > &Object);

    void ponder();
    void render();

    // Ponder part
	void HonorScene();
	void PaparazziScene();
	void AwardScene();

    // Render part
    void AwardSceneRender();

private:
	bool m_mustsetup;
	Uint32 m_starttime;
	Uint32 m_timepassed;
	CTimer mTimer;
	int m_counter;

	std::vector<CPlayer> &m_Player;

	std::unique_ptr<CFinaleStaticScene> mpFinaleStaticScene;
};

#endif /* CENDINGEP3_H_ */
