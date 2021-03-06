/*
 * CVideoDriver.cpp
 *
 *  Created on: 17.03.2009
 *      Author: gerstrong
 *
 *  Driver which will handle the Video Functions. This Driver handle the window mode and resolution mgmt.
 *  The Rendering itself is performed under COpenGL or CSDLVideo Class depending on what is enabled.
 */
#include "CVideoDriver.h"

#include <base/video/CSDLVideo.h>
#include <base/video/COpenGL.h>
#include "resolutionlist.h"

#include "graphics/GsGraphics.h"
#include <base/GsLogging.h>
#include <base/utils/FindFile.h>
#include <iostream>
#include <fstream>
#include <SDL_syswm.h>
#include <SDL_image.h>

CVideoDriver::CVideoDriver() :
m_mustrefresh(false),
mSDLImageInUse(false)
{}

CVideoDriver::~CVideoDriver()
{
  if(mSDLImageInUse)
  {
    // unload the dynamically loaded image libraries
    IMG_Quit();
  }
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
void CVideoDriver::addTextureRefToRender(GsTexture& textureRef, const GsRect<float> &dstRect)
{
    const GsRect<Uint16> clickGameArea = mpVideoEngine->getActiveAreaRect();

    const auto dpadX  = Uint16(float(clickGameArea.w) * dstRect.x);
    const auto dpadY  = Uint16(float(clickGameArea.h) * dstRect.y);
    const auto dpadWidth  = Uint16(float(clickGameArea.w) * dstRect.w);
    const auto dpadHeight = Uint16(float(clickGameArea.h) * dstRect.h);

    const GsRect<Uint16> dpadRect(dpadX, dpadY,
                                  dpadWidth, dpadHeight);


    std::tuple< GsTexture&, const GsRect<Uint16>, const GsRect<Uint16> >
            triple( textureRef, {0, 0, 0, 0}, dpadRect );

    mpVideoEngine->mRenderTexturePtrs.push(triple);
}
#endif


bool CVideoDriver::init()
{
    mVidConfig.reset();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
    {
        gLogging.textOut(FONTCOLORS::RED, "Could not initialize SDL: %s<br>", SDL_GetError());
        return false;
    }

    gLogging.textOut(FONTCOLORS::GREEN, "SDL was successfully initialized!<br>");

	initResolutionList();


	if(!mSDLImageInUse)
	{
	  // load support for the JPG and PNG image formats
      const int flags = IMG_INIT_JPG|IMG_INIT_PNG;
      const int inittedFlags = IMG_Init(flags);
      if( (inittedFlags & flags) != flags)
	  {
	      gLogging.textOut(FONTCOLORS::RED, "IMG_Init: Failed to init required jpg and png support!\n");
	      gLogging.textOut(FONTCOLORS::RED, "IMG_Init: %s\n", IMG_GetError());
          gLogging.textOut(FONTCOLORS::RED, "IMG_Init: CG will continue without it.\n");
	  }
	  else
	  {
	    mSDLImageInUse = true;
	  }
	}

	// take the first default resolution. It might be changed if there is a config file already created
	// If there are at least two possible resolutions choose the second one, as this is normally one basic small resolution
	setMode(m_Resolutionlist.front());

    return true;
}

// initResolutionList() reads the local list of available resolution.
// This function can only be called internally
// TODO: This should return something!
void CVideoDriver::initResolutionList()
{
	// This call will get the resolution we have right now and set it up for the system
	// On Handheld devices this means, they will only take that resolution and that would it be.
	// On the PC, this is the current resolution but will add others.


#if SDL_VERSION_ATLEAST(2, 0, 0)
    GsRect<Uint16> resolution(GsRect<Uint16>(800, 600));
#else
    //GsRect<Uint16> resolution(SDL_GetVideoInfo());
	GsRect<Uint16> resolution(GsRect<Uint16>(320, 200));
#endif

#if defined(ANDROID)
	resolution.w = 320;
	resolution.h = 200;
#endif

	GsRect<Uint16> desktopResolution(resolution);

	// We have a resolution list, clear it and create a new one.
    if(!m_Resolutionlist.empty())
    {
        m_Resolutionlist.clear();
    }

#if SDL_VERSION_ATLEAST(2, 0, 0)

#else

// TODO: Not sure if those defines are really needed anymore.
#if (TARGET_OS_IPHONE) || (TARGET_IPHONE_SIMULATOR)
	resolution.w = 320; //  320;
	resolution.h = 200;//  480;
#elif defined(ANDROID)
	resolution.w = 320;
	resolution.h = 200;
#endif

	// Now on non-handheld devices let's check for more resolutions.
#if 0 //!defined(EMBEDDED)
	for (unsigned int c = 0; c < NUM_MAIN_RESOLUTIONS; c++) {
		// Depth won't be read anymore! Take the one the system is using actually
		if (sscanf(ResolutionsList[c], "%hux%hu", &resolution.w, &resolution.h)
				>= 2) {
			// Now check if it's possible to use this resolution
			verifyResolution(resolution, SDL_FULLSCREEN);
		}
	}

	// In case there is no fullscreen, we will adapt the resolution it fits best to the window
    if (!mVidConfig.mFullscreen) {
		int e = 1;
		resolution.w = 320;
		resolution.h = 200;

		int maxwidth = SDL_GetVideoInfo()->current_w;

		while (resolution.w < maxwidth) {
			resolution.w = 320 * e;
			resolution.h = 200 * e;

			// Now check if it's possible to use this resolution
			verifyResolution(resolution, 0);
			e++;
		}
	}
#endif


#endif

	// The last resolution in the list is the desktop normally, therefore the highest
	m_Resolutionlist.push_back(desktopResolution);

	m_Resolution_pos = m_Resolutionlist.begin();

}

void CVideoDriver::verifyResolution(GsRect<Uint16>& resolution,
                                    const int flags)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)

#else
    if (SDL_VideoModeOK(resolution.w, resolution.h, 32, flags)) {
		std::list< GsRect<Uint16> >::iterator i;
		for (i = m_Resolutionlist.begin(); i != m_Resolutionlist.end(); i++) {
			if (*i == resolution)
				break;
		}

		if (i == m_Resolutionlist.end()) {
#ifdef DEBUG
			gLogging.ftextOut(FONTCOLORS::BLUE, "Resolution %ix%ix%i %X added\n",
					resolution.w, resolution.h, 32);
#endif
			m_Resolutionlist.push_back(resolution);
		}
	}
#endif
}

void CVideoDriver::setVidConfig(const CVidConfig& VidConf)
{
    mVidConfig = VidConf;
    setMode(mVidConfig.mDisplayRect);
}

void CVideoDriver::setMode(int width, int height, int depth) {
	GsRect<Uint16> res(width, height);
	setMode(res);
}

void CVideoDriver::setMode(const GsRect<Uint16>& res)
{
    mVidConfig.setResolution(res);

    // Cycle through the list until the matching resolution is matched. If it doesn't exist
	// add it;
    for (m_Resolution_pos = m_Resolutionlist.begin();
         m_Resolution_pos != m_Resolutionlist.end();
         m_Resolution_pos++)
    {
		if (*m_Resolution_pos == res)
			break;
    }

    if (m_Resolution_pos == m_Resolutionlist.end())
    {
		m_Resolutionlist.push_back(res);
		m_Resolution_pos--;
	}
}

bool CVideoDriver::applyMode()
{
    const GsRect<Uint16> &Res = mVidConfig.mDisplayRect;
    const GsRect<Uint16> &GameRect = mVidConfig.mGameRect;

	// Before the resolution is set, check, if the zoom factor is too high!
    while (((Res.w / GameRect.w) < mVidConfig.Zoom
            || (Res.h / GameRect.h) < mVidConfig.Zoom)
            && (mVidConfig.Zoom > 1))
        mVidConfig.Zoom--;

	// Check if some zoom/filter modes are illogical and roll them back accordingly
    if ((mVidConfig.Zoom == 3 && mVidConfig.m_ScaleXFilter == 1)
            && !mVidConfig.mOpengl)
        mVidConfig.Zoom = 2;

    if (mVidConfig.Zoom == 0)
        mVidConfig.Zoom = 1;

    mVidConfig.mDisplayRect = *m_Resolution_pos;

	return true;
}

bool CVideoDriver::setNativeResolution(const GsRect<Uint16> &dispRect)
{
    return mpVideoEngine->createSurfaces(dispRect);
}

bool CVideoDriver::start()
{
	bool retval;
    const std::string caption = "Commander Genius";
#if !SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_WM_SetCaption(caption.c_str(), caption.c_str());
#endif

#ifndef WIN32
	atexit(SDL_Quit);
#endif

	gLogging.textOut("Starting graphics driver...<br>");

#ifdef USE_OPENGL
    if (mVidConfig.mOpengl) // Try to use OpenGL if enabled by the user
	{
        mpVideoEngine.reset(new COpenGL(mVidConfig));
		retval = mpVideoEngine->init();

		if (!retval)
		{
            mVidConfig.mOpengl = false;
			applyMode();
            mpVideoEngine.reset(new CSDLVideo(mVidConfig));
			retval = mpVideoEngine->init();
            gLogging.textOut("will be using SDL Video<br>");
        }
        else
        {
            gLogging.textOut("will be using OpenGL<br>");
        }
    }
    else
#endif
    {
        CSDLVideo *sdlVideoPtr = new CSDLVideo(mVidConfig);
        mpVideoEngine.reset(sdlVideoPtr);
		retval = mpVideoEngine->init();
        gLogging.textOut("will be using SDL Video<br>");
	}
Check("createSurfaces");
	// Now SDL will tell if the bpp works or changes it, if not supported.
	// this value is updated here!
	retval &= mpVideoEngine->createSurfaces();
	m_mustrefresh = true;
Check("createSurfaces - after");
	return retval;
}

void CVideoDriver::setFilter(const filterOptionType value)
{
    mVidConfig.m_ScaleXFilter = value;
} // 1 means no filter

void CVideoDriver::setZoom(short value) {
    mVidConfig.Zoom = value;
}

void CVideoDriver::setScaleType(bool IsNormal)
{ mVidConfig.m_normal_scale = IsNormal; }

// defines the scroll-buffer that is used for blitScrollSurface(). It's normally passed by a CMap Object
// it might have when a level-map is loaded.
void CVideoDriver::updateScrollBuffer(const Sint16 SBufferX, const Sint16 SBufferY)
{	
    const int drawMask = getScrollSurface()->w-1;

    mpVideoEngine->UpdateScrollBufX(SBufferX, drawMask);
    mpVideoEngine->UpdateScrollBufY(SBufferY, drawMask);
}

void CVideoDriver::blitScrollSurface() // This is only for tiles
                                       // Therefore the name should be changed
{
	mpVideoEngine->blitScrollSurface();
}

void CVideoDriver::collectSurfaces()
{
	mpVideoEngine->collectSurfaces();
}

void CVideoDriver::clearSurfaces()
{
	mpVideoEngine->clearSurfaces();
}

void CVideoDriver::updateDisplay()
{
    if(mVidConfig.mHorizBorders > 0)
    {
        mpVideoEngine->drawHorizBorders();
    }

    mpVideoEngine->scaleAndFilter();
    mpVideoEngine->transformScreenToDisplay();
}

void CVideoDriver::saveCameraBounds(st_camera_bounds &CameraBounds)
{
	int &left = CameraBounds.left;
	int &up = CameraBounds.up;
	int &right = CameraBounds.right;
	int &down = CameraBounds.down;
	int &speed = CameraBounds.speed;

	if (left > right) {
		const int halfWidth = (left - right) / 2;
		left -= halfWidth;
		right += halfWidth;
		if (left > right)
			left--;
	}

	if (up > down) {
		const int halfHeight = (up - down) / 2;
		up -= halfHeight;
		down += halfHeight;
		if (up > down)
			up--;
	}

	bool invalid_value = (left < 50) || (up < 50) || (right < 50) || (down < 50)
			|| (speed < 1) || (left > 270) || (up > 150) || (right > 270)
			|| (down > 150) || (speed > 50);

    st_camera_bounds &cam = mVidConfig.m_CameraBounds;

	if (invalid_value)
		cam.reset();
	else
		cam = CameraBounds;
}

CVidConfig &CVideoDriver::getVidConfig()
{
    return mVidConfig;
}


void CVideoDriver::isFullscreen(const bool value) {
    mVidConfig.mFullscreen = value;
}

bool CVideoDriver::getFullscreen() {
    return mVidConfig.mFullscreen;
}

unsigned int CVideoDriver::getWidth() const {
    return mVidConfig.mDisplayRect.w;
}

unsigned int CVideoDriver::getHeight() const {
    return mVidConfig.mDisplayRect.h;
}

unsigned short CVideoDriver::getDepth() const
{
	return 32;
}

SDL_Surface *CVideoDriver::getScrollSurface()
{
	return mpVideoEngine->getScrollSurface();
}

st_camera_bounds &CVideoDriver::getCameraBounds()
{
    return mVidConfig.m_CameraBounds;
}

////
//// Drawing stuff related Stuff
////
SDL_Rect CVideoDriver::toBlitRect(const GsRect<float> &rect)
{
	GsRect<Uint16> GameRes = getGameResolution();
	GsRect<float> screenRect(0, 0, GameRes.w, GameRes.h);
	GsRect<float> RectDispCoordFloat = rect;

    // Limit the boundaries
    RectDispCoordFloat.x = std::max(RectDispCoordFloat.x, 0.0f);
    RectDispCoordFloat.y = std::max(RectDispCoordFloat.y, 0.0f);
    RectDispCoordFloat.h = std::min(RectDispCoordFloat.h, 1.0f);
    RectDispCoordFloat.w = std::min(RectDispCoordFloat.w, 1.0f);

	// Transform to the blit coordinates
	RectDispCoordFloat.transform(screenRect);

	GsRect<Uint16> RectDispCoord;
	RectDispCoord = RectDispCoordFloat;
	return RectDispCoord.SDLRect();
}


int CVideoDriver::getOptimalScaling()
{
    // Calculate a proper font size for the resolution
    const int desiredScaleW = getBlitSurface()->h/200;
    const int desiredScaleH = getBlitSurface()->w/320;

    int desiredScale = std::min(desiredScaleW, desiredScaleH);
    desiredScale = std::max(desiredScale, 1);
    desiredScale = std::min(desiredScale, 4);

    return desiredScale;
}



SDL_Surface *CVideoDriver::convertThroughBlitSfc( SDL_Surface *sfc )
{
    SDL_Surface *blit = mpVideoEngine->getBlitSurface();

    // Not sure why yet,
    // but we need to call this twice in order to have correct masks for blits

    SDL_Surface *newSfc  = SDL_ConvertSurface(sfc, blit->format, 0 );
    SDL_Surface *newSfc2 = SDL_ConvertSurface(newSfc, blit->format, 0 );

    SDL_FreeSurface(newSfc);

    return newSfc2;
}


