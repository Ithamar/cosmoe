#include <View.h>
#include <Invoker.h>
#include <Path.h>

#include <string>
#include <vector>



class IconView;
class BitmapView;

class Icon
{
public:
    Icon( const char* pzTitle, const char* pzPath, const struct stat& sStat );
    ~Icon();
    const std::string& 		GetName() const { return( m_cTitle ); }
    BRect                   GetBounds( BFont* pcFont );
    BRect                   GetFrame( BFont* pcFont );
    void                    Draw( BView* pcView, const BPoint& cOffset, bool bLarge, bool bBlendText );
    void                    Select( BitmapView* pcView, bool bSelected );
    void                    Select( IconView* pcView, bool bSelected );
    bool                    IsSelected() const { return( m_bSelected ); }
    BBitmap*				GetBitmap();
  
    BPoint        			m_cPosition;
    bool                	m_bSelected;
    struct stat        		m_sStat;
private:
    float 					GetStrWidth( BFont* pcFont );
    static BBitmap*			s_pcBitmap[16];
    static int				s_nCurBitmap;
    BRect        			m_cBounds;
    float        			m_vStringWidth;
    int                		m_nMaxStrLen;
    bool        			m_bBoundsValid;
    bool        			m_bStrWidthValid;
    std::string        		m_cTitle;
    uint8        			m_anSmall[16*16*4];
    uint8        			m_anLarge[32*32*4];
};

class IconView : public BView, public BInvoker
{
public:
	IconView( BRect cFrame, const char* pzPath, BBitmap* pcBitmap = NULL );
	~IconView();

	void            	LayoutIcons();
	virtual void		SetDirChangeMsg( BMessage* pcMsg );
	virtual void		Invoked();
	virtual void		DirChanged( const std::string& cNewPath );

	void				SetPath( const std::string& cPath );
	std::string			GetPath();

	Icon*               FindIcon( const BPoint& cPos );
	void                Erase( const BRect& cFrame );

	virtual void        AttachedToWindow();
	virtual void  	    KeyDown( const char *bytes, int32 numBytes );
	virtual void        MouseDown( BPoint cPosition );
	virtual void        MouseUp( BPoint cPosition );
	virtual void        MouseMoved( BPoint cNewPos, uint32 nCode, const BMessage* pcData );
	virtual void        Draw( BRect cUpdateRect );

private:
	static int32		ReadDirectory( void* pData );
	void				ReRead();

	struct ReadDirParam
	{
		ReadDirParam( IconView* pcView ) { m_pcView = pcView; m_bCancel = false; }
		IconView* m_pcView;
		bool      m_bCancel;
	};
	BPath                m_cPath;
	ReadDirParam*        m_pcCurReadDirSession;

	BMessage*            m_pcDirChangeMsg;
	BPoint               m_cLastPos;
	BPoint               m_cDragStartPos;
	BRect                m_cSelRect;
	bigtime_t            m_nHitTime;
	BBitmap*             m_pcBitmap;
	std::vector<Icon*>   m_cIcons;
	bool                 m_bCanDrag;
	bool                 m_bSelRectActive;
};
