/*
 * CTextViewer.h
 *
 *  Created on: 12.08.2009
 *      Author: gerstrong
 */

#ifndef CTEXTVIEWER_H_
#define CTEXTVIEWER_H_

#include <graphics/GsSurface.h>

#include <vector>
#include <string>
#include <memory>

class CTextViewer
{
public:
	CTextViewer(int x, int y, int w, int h);

	void initialize();
	void renderBox();
    void ponder(const float deltaT);
    void render();

	void formatText(const std::string &text);


    /**
     * \brief This will load for you a text from a file into memory and of course automatically format it for you :-)
     * \parm	filename	filename to open
     * \return true if everything went fine, otherwise false
     */
	bool loadTextfromFile(const std::string &filename);

	bool hasClosed() { return m_mustclose; }

	void setNextPos();
	void setPrevPos();
	void setPosition(int pos);

	void scrollDown();
	void scrollUp();

private:

	unsigned char getnextwordlength(const std::string nextword);
	void drawTextlines();

	int m_x;
	int m_y;
	int m_w;
	int m_h;

	int m_8x8tileheight;
	int m_8x8tilewidth;

	std::string mp_text;
	std::vector<std::string> m_textline;

	int m_linepos;
	char m_scrollpos; // Goes from 0 to textheight and is only used for a smooth scroll effect
	bool m_mustclose;
	int m_timer;

    GsSurface mTextVSfc;
};

#endif /* CTEXTVIEWER_H_ */
