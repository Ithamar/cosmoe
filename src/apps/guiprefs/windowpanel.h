#ifndef __F_WINDOWPANEL_H_
#define __F_WINDOWPANEL_H_

#include <LayoutView.h>

#include <vector>
#include <map>



class BButton;
class BListView;
class BCheckBox;
class CDropDownList;
class Spinner;
class BStringView;

class ColorEdit;

class WindowPanel : public LayoutView
{
public:
  
    WindowPanel( const BRect& cFrame );
    
    virtual void  Draw( BRect cUpdateRect );
    virtual void  KeyDown( const char *bytes, int32 numBytes );
    virtual void  FrameResized( float inWidth, float inHeight );
    virtual void  AllAttached();
    virtual void  MessageReceived( BMessage* pcMessage );
  
private:
//    void Layout();
    void UpdateDecoratorList();

    struct ColorSpace
    {
        ColorSpace( color_space eMode ) { m_eMode = eMode; }
        bool operator < ( const ColorSpace& cInst ) const { return( m_eMode < cInst.m_eMode ); }
        color_space                   m_eMode;
        std::vector<screen_mode> m_cResolutions;
    };

    std::map<color_space,ColorSpace> m_cColorSpaces;
  
    BButton*            m_pcRefreshBut;
    BButton*            m_pcOkBut;
    BButton*            m_pcCancelBut;
    BListView*           m_pcDecoratorList;
    CDropDownList*		m_pcPenMenu;
    ColorEdit*          m_pcColorEdit;
    BCheckBox*          m_pcPopupWndCheckBox;
};

#endif // __F_WINDOWPANEL_H_
