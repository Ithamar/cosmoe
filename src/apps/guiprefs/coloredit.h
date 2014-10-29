#include <View.h>

class Spinner;
class BStringView;


class ColorEdit : public BView
{
public:
    ColorEdit( BRect cFrame, const char* pzName, const rgb_color& sColor );
    void             SetValue( const rgb_color& sColor );
    const rgb_color& GetValue() const;

    virtual void  AllAttached();
    virtual void  FrameResized( float inWidth, float inHeight );
    virtual void  GetPreferredSize( float* outWidth, float* outHeight );
    virtual void  MessageReceived( BMessage* pcMessage );
    virtual void  Draw( BRect cUpdateRect );
  
private:
    Spinner*        m_pcRedSpinner;
    BStringView*    m_pcRedStr;
    Spinner*        m_pcGreenSpinner;
    BStringView*    m_pcGreenStr;
    Spinner*        m_pcBlueSpinner;
    BStringView*    m_pcBlueStr;
    BRect           m_cTestRect;
  
    rgb_color       m_sColor;
};
