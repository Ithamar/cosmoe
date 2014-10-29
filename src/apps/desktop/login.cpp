#include <stdio.h>

#include <Window.h>
#include <View.h>
//#include <TableView.h>
#include <TextView.h>
#include <StringView.h>
#include <Button.h>
#include <Screen.h>
#include <Message.h>



#define ID_OK 1
#define ID_CANCEL 2

static bool g_bRun      = true;
static bool g_bSelected = false;

static BString g_cName;
static BString g_cPassword;

class LoginView : public BView
{
public:
  LoginView( BRect cFrame );
  ~LoginView();
  
  virtual void AllAttached();
  virtual void FrameResized( float inWidth, float inHeight );
  
private:
  void Layout();

  BTextView*      m_pcNameView;
  BStringView*    m_pcNameLabel;
  BTextView*      m_pcPasswordView;
  BStringView*    m_pcPasswordLabel;
  BButton*        m_pcOkBut;
  BButton*        m_pcCancelBut;
};

class LoginWindow : public BWindow
{
public:
  LoginWindow( const BRect& cFrame );

  
  virtual bool        OkToQuit() { g_bRun = false; return( true ); }
  virtual void        MessageReceived( BMessage* pcMessage );
private:
  LoginView* m_pcView;
};

LoginWindow::LoginWindow( const BRect& cFrame ) : BWindow( cFrame, "Login:", B_UNTYPED_WINDOW, 0 )
{
  m_pcView = new LoginView( Bounds() );
  AddChild( m_pcView );

}

void LoginWindow::MessageReceived( BMessage* pcMsg )
{
dbprintf( "LoginWindow::MessageReceived\n" );
	switch( pcMsg->what )
	{
		case ID_OK:
dbprintf( "LoginWindow: OK selected\n" );
		g_bSelected = true;
		case ID_CANCEL:
		PostMessage( B_QUIT_REQUESTED );
		break;
		default:
		BWindow::MessageReceived( pcMsg );
		break;
	}
}

LoginView::LoginView( BRect cFrame ) : BView( cFrame, "password_view", B_FOLLOW_ALL, B_WILL_DRAW )
{
	m_pcOkBut        = new BButton( BRect( 0, 0, 0, 0 ), "ok_but", "Ok", new BMessage( ID_OK ), B_FOLLOW_NONE );
	m_pcCancelBut    = new BButton( BRect( 0, 0, 0, 0 ), "cancel_but", "Cancel", new BMessage( ID_CANCEL ), B_FOLLOW_NONE );
	m_pcNameView     = new BTextView( BRect( 0, 0, 0, 0 ), "name_view", "", B_FOLLOW_NONE );
	m_pcPasswordView = new BTextView( BRect( 0, 0, 0, 0 ), "pasw_view", "", B_FOLLOW_NONE );

	m_pcNameLabel    = new BStringView( BRect(0,0,1,1), "string", "Login name:", B_ALIGN_RIGHT );
	m_pcPasswordLabel= new BStringView( BRect(0,0,1,1), "string", "Password:", B_ALIGN_RIGHT );

	m_pcPasswordView->SetPasswordMode( true );

	AddChild( m_pcNameView );
	AddChild( m_pcNameLabel );
	AddChild( m_pcPasswordView );
	AddChild( m_pcPasswordLabel );
	AddChild( m_pcOkBut );
	AddChild( m_pcCancelBut );

	Layout();
}

LoginView::~LoginView()
{
	g_cName     = m_pcNameView->GetBuffer()[0];
	g_cPassword = m_pcPasswordView->GetBuffer()[0];
}

void LoginView::AllAttached()
{
	m_pcNameView->MakeFocus();
	Window()->SetDefaultButton( m_pcOkBut );
}


void LoginView::FrameResized( float inWidth, float inHeight )
{
	Layout();
}


void LoginView::Layout()
{
	BRect cBounds = Bounds();

	BRect cButFrame(0,0,0,0);
	BRect cLabelFrame(0,0,0,0);

	float buttonH, buttonV;
	float labelH, labelV;
	float nameViewH, nameViewV;

	m_pcCancelBut->GetPreferredSize( &buttonH, &buttonV );
	m_pcNameLabel->GetPreferredSize( &labelH, &labelV );
	m_pcNameView->GetPreferredSize( &nameViewH, &nameViewV );

	if ( nameViewV > labelV )
	{
		labelV = nameViewV;
	}

	cButFrame.right = buttonH - 1;
	cButFrame.bottom = buttonV - 1;

	cLabelFrame.right = labelH - 1;
	cLabelFrame.bottom = labelV - 1;

	BRect cEditFrame( cLabelFrame.right + 10, cLabelFrame.top, cBounds.right - 20, cLabelFrame.bottom );

	m_pcNameLabel->SetFrame( cLabelFrame.OffsetByCopy( 10, 20 ) );
	m_pcNameView->SetFrame( cEditFrame.OffsetByCopy( 10, 20 ) );

	m_pcPasswordLabel->SetFrame( cLabelFrame.OffsetByCopy( 10, 50 + labelV ) );
	m_pcPasswordView->SetFrame( cEditFrame.OffsetByCopy( 10, 50 + labelV ) );

	m_pcOkBut->SetFrame( cButFrame.OffsetByCopy( cBounds.right - buttonH - 15, cBounds.bottom - buttonV - 10 ) );
	m_pcCancelBut->SetFrame( cButFrame.OffsetByCopy( cBounds.right - buttonH*2 - 30, cBounds.bottom - buttonV - 10 ) );
}


bool get_login( std::string* pcName, std::string* pcPassword )
{
	g_bRun = true;
	g_bSelected = false;

	BRect cFrame( 0, 0, 249, 129 );

	IPoint cScreenRes;

		// Need a new scope to reduce the time the desktop is locked.
	{ cScreenRes = BScreen().GetResolution();  }

	cFrame.OffsetBy( cScreenRes.x / 2 - (cFrame.Width()+1.0f) / 2, cScreenRes.y / 2 - (cFrame.Height()+1.0f) / 2 );

	BWindow* pcWnd = new LoginWindow( cFrame );

	pcWnd->Show();
	pcWnd->Activate();

	while( g_bRun )
	{
		snooze( 20000 );
	}

	*pcName     = g_cName.String();
	*pcPassword = g_cPassword.String();
	return( g_bSelected );
}




