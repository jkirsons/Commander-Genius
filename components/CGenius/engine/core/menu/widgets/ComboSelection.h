#ifndef __COMBOSELECTION_H__
#define __COMBOSELECTION_H__

#include <widgets/GsComboSelection.h>

class ComboSelection : public CGUIComboSelection
{
public:
    ComboSelection( const std::string& text,
                    const std::list<std::string>& optionsList,
                    const Style &style);

    void cycleOption();

    void setupButtonSurface(const std::string &optionText);

    void setSelection( const std::string& selectionText );

    void processRender(const GsRect<float> &RectDispCoordFloat);
};

#endif // __COMBOSELECTION_H__
