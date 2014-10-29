#include "coloredit.h"

#include <Spinner.h>
#include <StringView.h>
#include <Message.h>



enum { COLOR_CHANGED };

static BPoint get_max_width( BPoint* pcFirst, BPoint* pcSecond, BPoint* pcThird )
{
	BPoint cSize = *pcFirst;

	if ( pcSecond->x > cSize.x )
	{
		cSize = *pcSecond;
	}

	if ( pcThird->x > cSize.x )
	{
		cSize = *pcThird;
	}

	return( cSize );
}

ColorEdit::ColorEdit( BRect cFrame, const char* pzName, const rgb_color& sColor ) : BView( cFrame, pzName, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW )
{
	m_sColor = sColor;

	m_pcRedSpinner   = new Spinner( BRect(0,0,0,0), "red_spinner", sColor.red, new BMessage( COLOR_CHANGED ) );
	m_pcGreenSpinner = new Spinner( BRect(0,0,0,0), "green_spinner", sColor.green, new BMessage( COLOR_CHANGED ) );
	m_pcBlueSpinner  = new Spinner( BRect(0,0,0,0), "blue_spinner", sColor.blue, new BMessage( COLOR_CHANGED ) );

	m_pcRedStr   = new BStringView( BRect(0,0,0,0), "red_txt", "Red", B_ALIGN_CENTER );
	m_pcGreenStr = new BStringView( BRect(0,0,0,0), "green_txt", "Green", B_ALIGN_CENTER );
	m_pcBlueStr  = new BStringView( BRect(0,0,0,0), "blue_txt", "Blue", B_ALIGN_CENTER );

	m_pcRedSpinner->SetMinPreferredSize( 3 );

	AddChild( m_pcRedSpinner );
	AddChild( m_pcGreenSpinner );
	AddChild( m_pcBlueSpinner );

	m_pcRedSpinner->SetFormat( "%.0f" );
	m_pcRedSpinner->SetMinMax( 0.0f, 255.0f );
	m_pcRedSpinner->SetStep( 1.0f );
	m_pcRedSpinner->SetScale( 1.0f );

	m_pcGreenSpinner->SetFormat( "%.0f" );
	m_pcGreenSpinner->SetMinMax( 0.0f, 255.0f );
	m_pcGreenSpinner->SetStep( 1.0f );
	m_pcGreenSpinner->SetScale( 1.0f );

	m_pcBlueSpinner->SetFormat( "%.0f" );
	m_pcBlueSpinner->SetMinMax( 0.0f, 255.0f );
	m_pcBlueSpinner->SetStep( 1.0f );
	m_pcBlueSpinner->SetScale( 1.0f );

	AddChild( m_pcRedStr );
	AddChild( m_pcGreenStr );
	AddChild( m_pcBlueStr );

	FrameResized( 0.0f, 0.0f );
}

void ColorEdit::MessageReceived( BMessage* pcMessage )
{
	switch( pcMessage->what )
	{
		case COLOR_CHANGED:
			SetValue( rgb_color( m_pcRedSpinner->Value(), m_pcGreenSpinner->Value(), m_pcBlueSpinner->Value(), 255 ) );
			break;
		default:
			BView::MessageReceived( pcMessage );
			break;
	}
}

void ColorEdit::SetValue( const rgb_color& sColor )
{
	m_sColor = sColor;

	m_pcRedSpinner->SetValue( float( sColor.red ) );
	m_pcGreenSpinner->SetValue( float( sColor.green ) );
	m_pcBlueSpinner->SetValue( float( sColor.blue ) );

	rgb_color sOldCol = ViewColor();
	SetViewColor( m_sColor );
	DrawFrame( m_cTestRect, FRAME_RECESSED );
	SetViewColor( sOldCol );
}


const rgb_color& ColorEdit::GetValue() const
{
	return( m_sColor );
}


void ColorEdit::AllAttached()
{
	m_pcRedSpinner->SetTarget( this );
	m_pcGreenSpinner->SetTarget( this );
	m_pcBlueSpinner->SetTarget( this );
}


void ColorEdit::GetPreferredSize( float* outWidth, float* outHeight )
{
	m_pcRedSpinner->GetPreferredSize( outWidth, outHeight );

	if (outWidth)
	{
		*outWidth *= 3.0f;
		*outWidth += 8.0f;
	}

	if (outHeight)
	{
		*outHeight *= 5.0f;
	}
}


void ColorEdit::FrameResized( float inWidth, float inHeight )
{
	BRect cBounds = Bounds();
	float width, height;

	m_pcRedSpinner->GetPreferredSize(&width, &height);
	width = 16 + m_pcRedSpinner->StringWidth( "8888" );

	BPoint cRStrSize, cGStrSize, cBStrSize;
	m_pcRedStr->GetPreferredSize(&cRStrSize.x, &cRStrSize.y);
	m_pcGreenStr->GetPreferredSize(&cGStrSize.x, &cGStrSize.y);
	m_pcBlueStr->GetPreferredSize(&cBStrSize.x, &cBStrSize.y);
	BPoint cStrSize = get_max_width( &cRStrSize, &cGStrSize, &cBStrSize );

	BRect cStrRect( 0, 0, cStrSize.x - 1, cStrSize.y - 1 );
	BRect cSpinnerRect( 0, 0, width - 1.0f, height - 1.0f );

	if ( cStrSize.y < height )
	{
		cStrSize += BPoint( 0, (height - cStrSize.y) / 2  );
	}
	else
	{
		height += ((cStrSize.y - height) / 2 );
	}

	m_pcRedSpinner->SetFrame( cSpinnerRect.OffsetByCopy( 4, cStrSize.y + 8 ) );
	m_pcGreenSpinner->SetFrame( cSpinnerRect.OffsetByCopy( (cBounds.Width()+1.0f) / 2 - width / 2, cStrSize.y + 8 ) );
	m_pcBlueSpinner->SetFrame( cSpinnerRect.OffsetByCopy( (cBounds.Width()+1.0f) - width - 4, cStrSize.y + 8 ) );

	m_pcRedStr->SetFrame( cStrRect.OffsetByCopy( 4, 4 ) );
	m_pcGreenStr->SetFrame( cStrRect.OffsetByCopy( (cBounds.Width()+1.0f) / 2 - cStrSize.x / 2, 4 ));
	m_pcBlueStr->SetFrame( cStrRect.OffsetByCopy( cBounds.Width() + 1.0f - cStrSize.x - 4, 4 ) );

	m_cTestRect = BRect( 10, cSpinnerRect.right + 14, cBounds.right - 10, cBounds.bottom - 10 );
}


void ColorEdit::Draw( BRect cUpdateRect )
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	DrawFrame( Bounds(), FRAME_RECESSED );

	SetViewColor( m_sColor );
	DrawFrame( m_cTestRect, FRAME_RECESSED );
}

