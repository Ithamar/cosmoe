#ifndef __F_SCREENPANEL_H_
#define __F_SCREENPANEL_H_

#include <LayoutView.h>

#include <vector>
#include <map>



class BButton;
class BListView;
class BCheckBox;
class CDropDownList;
class Spinner;
class BStringView;

class ScreenPanel : public LayoutView
{
public:
  
    ScreenPanel( const BRect& cFrame );
    virtual void Draw( BRect cUpdateRect );
    virtual void KeyDown( const char *bytes, int32 numBytes );
    virtual void FrameResized( float inWidth, float inHeight );
    virtual void AllAttached();
    virtual void MessageReceived( BMessage* pcMessage );
  
private:
    void UpdateResList();

    struct ColorSpace
    {
        ColorSpace( color_space eMode ) { m_eMode = eMode; }
        bool operator < ( const ColorSpace& cInst ) const { return( m_eMode < cInst.m_eMode ); }
        color_space                   m_eMode;
        std::vector<screen_mode> m_cResolutions;
    };

    screen_mode m_sOriginalMode;
    screen_mode m_sCurrentMode;
    
    std::map<color_space,ColorSpace> m_cColorSpaces;
  
    BButton*         m_pcOkBut;
    BButton*         m_pcCancelBut;
    BCheckBox*       m_pcAllCheckBox;
    CDropDownList*   m_pcColorSpaceList;
    Spinner*         m_pcRefreshRate;

    BStringView*     m_pcColorSpaceStr;
    BStringView*     m_pcRefreshRateStr;
  
    BListView*        m_pcModeList;
};

#endif // __F_SCREENPANEL_H_
