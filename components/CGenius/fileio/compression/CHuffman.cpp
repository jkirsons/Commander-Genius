/*
 * CHuffman.cpp
 *
 *  Created on: 27.12.2009
 *      Author: gerstrong
 */

#include "CHuffman.h"
#include <base/utils/FindFile.h>
#include <fstream>

const unsigned int DICT_SIG_BYTES = 6;
const uint8_t DICTSIG[DICT_SIG_BYTES] = { 0xFD, 0x01, 0x00, 0x00, 0x00, 0x00 };

bool CHuffman::readDictionaryNumber( const CExeFile& ExeFile,
                                     const int dictnum,
                                     const unsigned int dictOffset )
{
    uint8_t dictnumleft = dictnum;

    if( dictOffset == 0) // don't seek to offset
    {
        uint8_t *data_ptr = ExeFile.getRawData();

        for( Uint32 i=0; i<ExeFile.getExeDataSize() ; i++ )
        {
            if( memcmp(data_ptr, DICTSIG, DICT_SIG_BYTES) == 0 )
            {
                if(dictnumleft == 0)
                {
                    uint8_t *dictdata = data_ptr-(DICT_SIZE*sizeof(nodestruct))+DICT_SIG_BYTES;
                    const Uint32 size = DICT_SIZE*sizeof(nodestruct);
                    memcpy(m_nodes, dictdata, size);
                    return true;
                }
                dictnumleft--;
            }
            data_ptr++;
        }
        return false;
    }
    else // Otherwise copy the dictionary normally
    {
        uint8_t *dictdata = (byte*)(ExeFile.getHeaderData())+dictOffset;
        memcpy(reinterpret_cast<char*>(m_nodes), dictdata, DICT_SIZE*sizeof(nodestruct));
        return true;
    }
}




bool CHuffman::readDictionaryNumberfromEnd(const CExeFile& ExeFile)
{        
    const uint32_t bytesToCheck = ExeFile.getExeDataSize()-DICT_SIG_BYTES;

    uint8_t *data_ptr = static_cast<uint8_t*>(ExeFile.getHeaderData())+bytesToCheck;

    for( Uint32 i=0; i<bytesToCheck ; i++ )
    {
        data_ptr--;
        if( memcmp(data_ptr, DICTSIG, DICT_SIG_BYTES) == 0 )
        {
       		uint8_t *dictdata = data_ptr-(DICT_SIZE*sizeof(nodestruct))+DICT_SIG_BYTES;
       		const Uint32 size = DICT_SIZE*sizeof(nodestruct);
       		memcpy(m_nodes, dictdata, size);

            //dumpToExternalFile("dump.huffmann");

       		return true;
        }
    }



    return false;
}

bool CHuffman::readDictionaryFromFile( const std::string &filename )
{
	std::ifstream file;

	if(!OpenGameFileR(file, filename, std::ios::binary))
	{
		return false;
	}

	file.read(reinterpret_cast<char*>(m_nodes), DICT_SIZE*sizeof(nodestruct));

	return true;
}



void CHuffman::readDictionaryAt( byte *p_exedata, unsigned long offset)
{
	p_exedata += offset;
	const Uint32 size = DICT_SIZE*sizeof(nodestruct);
	memcpy(m_nodes, p_exedata, size);
}


/* Expand huffman-compressed input file into output buffer */
void CHuffman::expand(byte *pin, byte *pout,
                      const unsigned long inlen,
                      const unsigned long outlen)
{
	unsigned short curnode;
	unsigned long incnt = 0, outcnt = 0;
	unsigned char c, mask;
	unsigned short nextnode;

	curnode = 254; /* Head node */

	do
	{
		mask = 1;
		c = *(pin++);
		incnt++;

		do
		{
			if(c & mask)
				nextnode = m_nodes[curnode].bit1;
			else
				nextnode = m_nodes[curnode].bit0;


			if(nextnode < 256)
			{
				/* output a char and move back to the head node */
				*(pout++) = nextnode;
				outcnt++;
				curnode = 254;
			}
			else
			{
				/* move to the next node */
				curnode = nextnode & 0xFF;
			}
			/* Move to consider the next bit */
			mask <<= 1;
		}
		while(outcnt < outlen && mask != 0);

	}
	while(incnt < inlen && outcnt < outlen);
}

void CHuffman::dumpToExternalFile(const std::string &fname)
{
    std::ofstream file(fname.c_str());
    file.write(reinterpret_cast<const char*>(m_nodes), sizeof(m_nodes));
}
