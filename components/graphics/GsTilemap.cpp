/*
 * GsTilemap.cpp
 *
 *  Created on: 29.08.2009
 *      Author: gerstrong
 */

#include <fileio/ResourceMgmt.h>
#include <base/video/CVideoDriver.h>
#include <SDL_image.h>
#include <base/utils/FindFile.h>
#include "GsTilemap.h"
#include "GsPalette.h"
#include <base/GsLogging.h>
#include <stdlib.h>
#include <SDL_image.h>


bool GsTilemap::CreateSurface(SDL_Color *sdlPalette, Uint32 Flags,
				Uint16 numtiles, Uint16 pbasesize, Uint16 column)
{
	m_numtiles = numtiles;
	m_pbasesize = pbasesize;
	m_column = column;
    mTileSurface.create(Flags, m_column<<m_pbasesize,
                        (m_numtiles/m_column)<<m_pbasesize, 8, 0, 0, 0, 0);

    mTileSurface.setPaletteColors(sdlPalette);
    mTileSurface.setColorKey(COLORKEY);

    return !mTileSurface.empty();
}

std::string exts[] = { "png", "bmp", "tif", "jpg" };

bool GsTilemap::loadHiresTile( const std::string& filename, const std::string& path )
{  
	std::string fullfilename;  
	// Cycle through possible filename extensions, when more formats are supported
	for( auto &ext : exts )
	{
	    fullfilename = filename + "." + ext;
	    fullfilename = getResourceFilename(fullfilename, path, false);  
	    if(!fullfilename.empty())
	      break;
	}	
	  
	if(!IsFileAvailable(fullfilename))
		return false;

    if(!mTileSurface.empty())
	{	  	  
        SDL_Surface *temp_surface = IMG_Load(GetFullFileName(fullfilename).c_str());
		if(temp_surface)
		{
            GsWeakSurface tempWeak(temp_surface);

            mTileSurface.createCopy(tempWeak);
            SDL_FreeSurface(temp_surface);
			return true;
		}
		else
		{
		  gLogging.textOut(FONTCOLORS::RED, "IMG_Load: %s\n", IMG_GetError());
		  gLogging.textOut(FONTCOLORS::RED, "IMG_Load: CG will ignore those images\n");
		}
	}

	return false;
}

bool GsTilemap::optimizeSurface()
{
    if(!mTileSurface.empty())
	{
        SDL_Surface *temp_surface = gVideoDriver.convertThroughBlitSfc(mTileSurface.getSDLSurface());
        mTileSurface.createFromSDLSfc(temp_surface);

		return true;
	}
	else
    {
		return false;
    }
}

///////////////////////////////////
///// Initialization Routines /////
///////////////////////////////////
SDL_Surface *GsTilemap::getSDLSurface()
{
    return mTileSurface.getSDLSurface();
}



////////////////////////////
/////    Getters       /////
////////////////////////////


int GsTilemap::getDimension()
{
	return (1<<m_pbasesize);
}

////////////////////////////
///// Drawing Routines /////
////////////////////////////

#ifdef DEBUG_COLLISION

#define CSF    9
#define TILE_S			4
#define STC (CSF-TILE_S)

void FillSlopeRect(SDL_Surface *dst, const SDL_Rect dst_rect, Uint32 color, Sint8 blocked)
{
	/**
	 * So far only used in Galaxy. This is the code for sloped tiles downside
	 * This is performed when Keen walks into a sloped tile
	 *
	 * 0	Fall through		1	Flat
	 * 2	Top -> Middle		3	Middle -> bottom
	 * 4	Top -> bottom		5	Middle -> top
	 * 6	Bottom -> middle	7	Bottom -> top
	 * 8	Unused			9	Deadly, can't land on in God mode
	 */

	int yb1, yb2;

	if(!blocked)
		return;

	if( blocked == 2 )
		yb1 = 0,	yb2 = 256;
	else if( blocked == 3 )
		yb1 = 256,	yb2 = 512;
	else if( blocked == 4 )
		yb1 = 0,	yb2 = 512;
	else if( blocked == 5 )
		yb1 = 256,	yb2 = 0;
	else if( blocked == 6 )
		yb1 = 512,	yb2 = 256;
	else if( blocked == 7 )
		yb1 = 512,	yb2 = 0;
	else
		yb1 = 0, yb2 = 0;

	SDL_Rect sloperect = dst_rect;
	sloperect.w = 1;

	if( blocked > 1 )
	{
		for( int c = 0 ; c<512 ; c++ )
		{
			sloperect.x = dst_rect.x + (c>>STC);
			sloperect.y = dst_rect.y + ((yb1+c*(yb2-yb1)/512)>>STC);
			sloperect.h = dst_rect.h - sloperect.y;
			SDL_FillRect( dst, &sloperect, 0xFFFFFFFF);
		}
	}

	SDL_Rect line1_rect = dst_rect;
	line1_rect.h = 1;
	SDL_FillRect( dst, &line1_rect, 0xFF545454);
	SDL_Rect line2_rect = dst_rect;
	line2_rect.w = 1;
	SDL_FillRect( dst, &line2_rect, 0xFF545454);
}
#endif


void GsTilemap::drawTile(SDL_Surface *dst,
                         const int x,
                         const int y,
                         const Uint16 t)
{
    SDL_Rect src_rect, dst_rect;
	src_rect.x = (t%m_column)<<m_pbasesize;
	src_rect.y = (t/m_column)<<m_pbasesize;
	const int size = 1<<m_pbasesize;
	src_rect.w = src_rect.h = dst_rect.w = dst_rect.h = size;

	dst_rect.x = x;		dst_rect.y = y;

    if( dst_rect.y + src_rect.h > dst->h )
    {
        src_rect.h = dst->h - dst_rect.y;
    }

    if( dst_rect.x + src_rect.w > dst->w )
    {
        src_rect.w = dst->w - dst_rect.x;
    }

    auto rawSDLSfc = mTileSurface.getSDLSurface();
    BlitSurface(rawSDLSfc, &src_rect, dst, &dst_rect);

#ifdef DEBUG_COLLISION
	//std::vector<CTileProperties> &TileProp = gBehaviorEngine.getTileProperties(1);
	//FillSlopeRect(dst, dst_rect, 0xFFFFFFFF, TileProp[t].bup);
#endif

}

void GsTilemap::drawTile(GsWeakSurface &dst,
                         const int x,
                         const int y,
                         const Uint16 t)
{
    drawTile(dst.getSDLSurface(), x, y, t);
}


void GsTilemap::drawTileBlended(SDL_Surface *dst,
                                const int x, const int y,
                                const Uint16 t, const Uint8 alpha)
{
    if(mTileSurfaceAlpha.empty())
    {
        mTileSurfaceAlpha.createCopy(mTileSurface);
    }

    SDL_Rect src_rect, dst_rect;
    src_rect.x = (t%m_column)<<m_pbasesize;
    src_rect.y = (t/m_column)<<m_pbasesize;
    const int size = 1<<m_pbasesize;
    src_rect.w = src_rect.h = dst_rect.w = dst_rect.h = size;

    dst_rect.x = x;		dst_rect.y = y;

    if( dst_rect.y + src_rect.h > dst->h )
    {
        src_rect.h = dst->h - dst_rect.y;
    }

    if( dst_rect.x + src_rect.w > dst->w )
    {
        src_rect.w = dst->w - dst_rect.x;
    }

    const auto oldAlpha = mTileSurfaceAlpha.getAlpha();

    mTileSurfaceAlpha.setAlpha(alpha);

    auto rawSDLSfc = mTileSurfaceAlpha.getSDLSurface();
    BlitSurface(rawSDLSfc, &src_rect, dst, &dst_rect);

    mTileSurfaceAlpha.setAlpha(oldAlpha);
}


void GsTilemap::applyGalaxyHiColourMask()
{

    auto rawSDLSfc = mTileSurface.getSDLSurface();

#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_Surface *newSfc =
            SDL_ConvertSurfaceFormat(rawSDLSfc, SDL_PIXELFORMAT_RGBA8888, 0);
#else
    SDL_Surface *blit = gVideoDriver.getBlitSurface();
    SDL_Surface *newSfc = SDL_ConvertSurface(rawSDLSfc, blit->format, 0 );
#endif


    mTileSurface.createFromSDLSfc(newSfc);

    const auto format = mTileSurface.getSDLSurface()->format;

#if SDL_VERSION_ATLEAST(2, 0, 0)
    mTileSurface.setBlendMode(SDL_BLENDMODE_BLEND);
#endif

    mTileSurface.lock();

    // Pointer of start pixel
    Uint8 *pxStart = static_cast<Uint8*>(mTileSurface.getSDLSurface()->pixels);

    // From 0 to half width for every row ...
    int midRow = mTileSurface.width()/2;

    const auto pitch = mTileSurface.getSDLSurface()->pitch;

    for( int y=0 ; y<mTileSurface.height() ; y++ )
    {
        Uint8 *pxRow = pxStart + y*pitch;
        for( int x=0 ; x<midRow ; x++ )
        {
            Uint8 *px = pxRow + x*format->BytesPerPixel;
            Uint8 *pxMask = px + midRow*format->BytesPerPixel;

            Uint32 pix = 0;
            Uint32 mask = 0;
            memcpy(&pix, px, format->BytesPerPixel);
            memcpy(&mask, pxMask, format->BytesPerPixel);

            // Get the mask part
            Uint8 mask_r, mask_g, mask_b;
            SDL_GetRGB(mask, format, &mask_r, &mask_g, &mask_b);

            // Get the color
            Uint8 r, g, b;
            SDL_GetRGB(pix, format, &r, &g, &b);

            // Calculate the new alpha, which will do the transparency and even allow translucency
            const Uint8 alpha = 255-(( (mask_r<<16) + (mask_g<<8) + mask_b ) >> 16);

            pix = SDL_MapRGBA( format, r, g, b, alpha );

            memcpy(px, &pix, format->BytesPerPixel);
        }
    }

    mTileSurface.unlock();
}

