/*
 * CMessageBoxBitmapGalaxy.h
 *
 *  Created on: 04.05.2011
 *      Author: gerstrong
 */

#ifndef CMESSAGEBOXBITMAPGALAXY_H_
#define CMESSAGEBOXBITMAPGALAXY_H_

#include "CMessageBoxGalaxy.h"
#include "graphics/CGfxEngine.h"

class CMessageBoxBitmapGalaxy : public CMessageBoxGalaxy {
public:

	/**
	 * \brief This constructor creates a typical Keen 4 Message Box
	 * \param Text 				Message to be shown
	 * \param BitmapId 			Bitmap that will be shown to the left side in the box
	 */
	CMessageBoxBitmapGalaxy(const std::string& Text, const Uint16 BitmapId );

	/**
	 * \brief the cycle to be processed for rendering the Message-Box
	 */
	virtual void process();

private:
	CBitmap &m_Bitmap;
};

#endif /* CMESSAGEBOXBITMAPGALAXY_H_ */