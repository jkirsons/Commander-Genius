/*
 * CEffects.h
 *
 *  Created on: 14.12.2009
 *      Author: gerstrong
 *
 *  This special class is used for the graphical effects that are seen in the game.
 *  Some are normal effects seen in the DOS-Version of Commander Keen, other are
 *  new and others were partly included somewhere else in the past versions of CG.
 *  There were workarounds and now are well integrated.
 */

#ifndef CEFFECTS_H_
#define CEFFECTS_H_

class CEffects
{
public:
	CEffects() :
	mFinished(false) {}

    virtual void ponder(const float deltaT) = 0;

    virtual void render() = 0;

	bool finished() { return mFinished; }
    void setFinished(const bool value) { mFinished = value; }

	virtual ~CEffects() {}

protected:
	bool mFinished;
};

#endif /* CEFFECTS_H_ */
