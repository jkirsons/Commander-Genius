/*
 * CEGASprit.h
 *
 *  Created on: 11.07.2009
 *      Author: gerstrong
 */

#ifndef CEGASPRIT_H_
#define CEGASPRIT_H_

#include <string>
#include <SDL.h>
#include <vector>
#include "graphics/GsGraphics.h"
#include "engine/core/CResourceLoader.h"


class CEGASprit {
public:
	CEGASprit(int planesize,
			  long spritestartloc,
			  int numsprites,
			  long spriteloc,
			  const std::string &gamepath,
              size_t episode,
              CResourceLoaderBackground &loader);
	virtual ~CEGASprit();

	bool loadHead(char *data);
	bool loadData(const std::string& filename, bool compresseddata);

private:
	int mNumsprites;
	int m_planesize;
	long m_spritestartloc;
	long m_spriteloc;
	const std::string &m_gamepath;
	size_t m_Episode;

	struct st_sprite{
		short width;
		short height;
		short location_offset; // usually. See shikadi.net for more info
		short location;
		short hitbox_u;
		short hitbox_l;
		short hitbox_b;
		short hitbox_r;
		char name[12];
		long hv_offset; // Unused in Keen games. Used in later games such as Shadow Knights
		// There are 3 copies of the same Elements in the file. There were used for performance
		// in DOS but are ignored here.
    } *EGASpriteModell = nullptr;

    CResourceLoaderBackground &mLoader;

	void generateSprite( const int points, GsSprite &sprite );
	void LoadSpecialSprites( std::vector<GsSprite> &sprite );
    void DerivePlayerSprites( const int id, std::vector<GsSprite> &sprites );
	void DeriveSpecialSprites( GsTilemap &tilemap, std::vector<GsSprite> &sprites );
	void CreateYellowSpriteofTile( GsTilemap &tilemap, Uint16 tile, GsSprite& sprite );
	void ApplySpecialFX();
};

#endif /* CEGASPRIT_H_ */
