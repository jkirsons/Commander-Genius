/*
 * ActionFormat.h
 *
 *  Created on: 12.08.2010
 *      Author: gerstrong
 *
 *  This special struct is used for now in Galaxy and will hold the actions to perform
 *  when certain (sprites) does an action. As there is a location to that action we read it in
 *  and extend with member function will get the proper action.
 *
 *  Special thanks to lemm for his hints!
 *
 */

#ifndef ACTIONFORMAT_H_
#define ACTIONFORMAT_H_

#include <stdint.h>
#include <stdio.h>
//#include <string.h>
#include <string>

enum GalaxyActionType
{
	AT_UnscaledOnce = 0,			// Unscaled Motion, Thinks once.
	AT_ScaledOnce = 1,				// Scaled Motion, Thinks once.
	AT_Frame = 2,					// No Motion, Thinks each frame (doesn't advance action)
	AT_UnscaledFrame = 3,			// Unscaled Motion, Thinks each frame
	AT_ScaledFrame = 4				// Scaled Motion, Thinks each frame
};

struct ActionFormatType
{
	int16_t spriteLeft;          // 124-400
	int16_t spriteRight;         // 124-400
    int16_t movement_param;		 // 0-4
    int16_t change_h;            // 0/1
    int16_t change_v;            // 0/1
    int16_t delay;				 // (Anim speed)  0-8000
    int16_t h_anim_move;				 // +- any value
    int16_t v_anim_move;  				 // +- any value
	int16_t Behavior;            // [Start of behavior codes only]
	int16_t Behavior_segment;    // [Segment values only]
	int16_t Check_sprites;       // [Start of check sprite codes only]
	int16_t Check_segment;       // [Segment values only]
	int16_t Check_tiles;         // [Start of check tile code only]
	int16_t Check_segment2;      // [Segment values only]
	int16_t Next_action;         // [Valid actions only]

	/*void (*think)(struct CK_object *obj);
	void (*collide)(struct CK_object *obj, struct CK_object *other);
	void (*draw)(struct CK_object *obj);*/

	/**
	 * \brief	set Action Format of the sprite
	 * \param	sprite_offset	Offset of the sprite. This is per sprite(object) just one and the same
	 * 							direction
	 */
	void setActionFormat( const size_t sprite_offset );

	/**
	 * \brief just sets the next action format struct
	 */
	void setNextActionFormat();

	/**
	 * \brief	get Action Format of the sprite
	 * \param	sprite_offset	Offset of the sprite. This is per sprite(object) just one and the same
	 * 							direction
	 */
	bool getActionFormat( const size_t sprite_offset );
};

/**
 * @brief dumpActionFormatToFile   Dumps the action format
 * @param fileName  name of the file
 * @param numChunks  Number of action chunks to dump (one change is 30 bytes long)
 * @return true if everything went fine, otherwise false.
 */
bool dumpActionFormatToFile(const std::string &fileName,
                            const size_t numChunks);

#endif /* ACTIONFORMAT_H_ */
