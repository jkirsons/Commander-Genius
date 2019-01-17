/*
 * CRLE.h
 *
 *  Created on: 31.05.2010
 *      Author: gerstrong
 */

#ifndef CRLE_H_
#define CRLE_H_

#include <base/TypeDefinitions.h>
#include <vector>

class CRLE
{
public:
	CRLE();
	void expand( std::vector<word>& dst, std::vector<byte>& src, word key );
	void expandSwapped( std::vector<word>& dst, std::vector<byte>& src, word key );
};

#endif /* CRLE_H_ */
