// This code is copyright © 1993 Richard Black. All rights are reserved. You may use this code only if it includes a statement to that effect.
// http://www.cl.cam.ac.uk/research/srg/bluebook/21/crc/node6.html

#include "crc.h"
#include <stdlib.h>
#include <string.h>

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#define QUOTIENT 0x04C11DB7

static unsigned int crctab[256];

void crc32_init()
{
    unsigned int crc;
	
    for (unsigned int i = 0; i < 256; i++)
    {
        crc = i << 24;
        for (unsigned int j = 0; j < 8; j++)
        {
            if (crc & 0x80000000)
            {
                crc = (crc << 1) ^ QUOTIENT;
            }
            else
            {
                crc = crc << 1;
            }
        }
        crctab[i] = crc; // may need to account for big endian
    }
}

unsigned int getcrc32(unsigned char *data, int len)
{
    unsigned int        result;
    unsigned int        p;
    unsigned char       *e = (data + len);

    memcpy( &p, data, sizeof(p) );
    result = ~p;
    data += sizeof(p);
    while( data<e )
    {
#if defined(LITTLE_ENDIAN)
        result = crctab[result & 0xff] ^ result >> 8;
        result = crctab[result & 0xff] ^ result >> 8;
        result = crctab[result & 0xff] ^ result >> 8;
        result = crctab[result & 0xff] ^ result >> 8;
#else
        result = crctab[result >> 24] ^ result << 8;
        result = crctab[result >> 24] ^ result << 8;
        result = crctab[result >> 24] ^ result << 8;
        result = crctab[result >> 24] ^ result << 8;
#endif
        memcpy( &p, data, sizeof(p) );
        result ^= p;
        data += sizeof(p);
    }

    return ~result;
}
