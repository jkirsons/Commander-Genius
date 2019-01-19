/*
 * CGUITextSelectionList.h
 *
 *  Created on: 01.11.2011
 *      Author: gerstrong
 *
 *  A Graphical List Control which has some item which are shown as Text and can be selected
 */

#ifndef CGUITEXTSELECTIONLIST_H_
#define CGUITEXTSELECTIONLIST_H_

#include "GsControl.h"
#include "GsScrollbar.h"
#include <base/utils/Color.h>

#include <list>
#include <string>


class CGUITextSelectionList : public GsControl
{
public:

    CGUITextSelectionList();

	void setConfirmButtonEvent(CEvent *ev);
	void setBackButtonEvent(CEvent *ev);

	bool sendEvent(const InputCommand command);
	void addText(const std::string &text);    
	void processLogic();
    void processRender(const GsRect<float> &RectDispCoordFloat);

    /**
     * @brief getSelection  return position of currently selected item, or -1 if nothing is selected at al
     * @return position or -1
     */
    int getSelection() const
    {
        return mReleasedSelection;
    }

    /**
     * @brief setSelection  selects item given by index
     * @param sel   index
     */
    void setSelection(const int sel)
    {
        const int last = static_cast<int>(mItemList.size());

        if(sel < 0)
        {
            mReleasedSelection = 0;
            mSelected = false;
            return;
        }
        if(sel >= last)
        {
            mReleasedSelection = last-1;
            mSelected = false;
            return;
        }

        mReleasedSelection = sel;
    }

    const std::string &getItemString(const int sel) const
    {
        return mItemList[sel].mText;
    }

private:

    struct item
    {
        item(const std::string &text) : mText(text) {}

        std::string mText; // Text of the current item
        GsColor mBgColor = GsColor(0xFF, 0xFF, 0xFF, 0xFF);  // Current background color
    };

    std::vector<item> mItemList;

    int mHoverSelection = 0;
    int mPressedSelection = -1;
    int mReleasedSelection = -1;
    
    GsScrollbar mScrollbar;
	
	std::shared_ptr<CEvent> mConfirmEvent;
	std::shared_ptr<CEvent> mBackEvent;

    const int mBorderHeight = 16;

};

#endif /* CGUITEXTSELECTIONLIST_H_ */
