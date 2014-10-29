#ifndef __F_FONTPANEL_H__
#define __F_FONTPANEL_H__

#include <LayoutView.h>
#include <Font.h>

#include <map>



class BButton;
class BListView;
class Spinner;
class BTextView;
class CDropDownList;

enum
{
  MID_ABOUT,
  MID_HELP,

  MID_LOAD,
  MID_INSERT,
  MID_SAVE,
  MID_SAVE_AS,
  MID_DUP_SEL,
  MID_DELETE_SEL,
  MID_RESTORE,

  MID_SNAPSHOT,
  MID_RESET,
  MID_QUIT,

  GID_COUNT
};

class FontPanel : public LayoutView
{
public:
  
    FontPanel( const BRect& cFrame );
    virtual void Draw( BRect cUpdateRect );
    virtual void KeyDown( const char *bytes, int32 numBytes );
    virtual void FrameResized( float inWidth, float inHeight );
    virtual void AllAttached();
    virtual void MessageReceived( BMessage* pcMessage );
  
private:
    void UpdateSelectedConfig();

    BRect          m_cTestArea;

    struct ConfigNode {
        std::string         m_cName;
        font_properties m_sConfig;
        font_properties m_sOrigConfig;
        bool                m_bModified;
    };
    BFont::size_list_t       m_cBitmapSizes;
    std::vector<ConfigNode> m_cConfigList;
    int                     m_nCurSelConfig;
    
    BListView*        m_pcFontList;
    CDropDownList*    m_pcFontConfigList;
    CDropDownList*    m_pcBitmapSizeList;
    Spinner*          m_pcSizeSpinner;

    BTextView*        m_pcTestView;
    BButton*          m_pcRescanBut;
    BButton*          m_pcOkBut;
    BButton*          m_pcCancelBut;
};

#endif // __F_FONTPANEL_H__
