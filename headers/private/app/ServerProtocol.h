#ifndef _APPSERVER_PROTOCOL_
#define _APPSERVER_PROTOCOL_

#include <string.h>
#include <SupportDefs.h>

#include <GraphicsDefs.h>
#include <Font.h>
#include <Rect.h>
#include <Region.h>
#include <InterfaceDefs.h>
#include <IPoint.h>

class ServerWindow;

// Server port names. The input port is the port which is used to receive
// input messages from the Input Server. The other is the "main" port for
// the server and is utilized mostly by BApplication objects.
#define SERVER_PORT_NAME "OBappserver"
#define SERVER_INPUT_PORT "OBinputport"


enum
{
// Used for quick replies from the app_server
SERVER_TRUE='_srt',
SERVER_FALSE,
AS_SERVER_BMESSAGE,
AS_SERVER_AREALINK,
AS_SERVER_SESSION,
AS_SERVER_PORTLINK,
AS_CLIENT_DEAD,

// Application definitions
AS_CREATE_APP,
AS_DELETE_APP,
AS_QUIT_APP,

AS_SET_SERVER_PORT,

AS_CREATE_WINDOW,
AR_OPEN_BITMAP_WINDOW,
AS_DELETE_WINDOW,
AS_CREATE_BITMAP,
AS_DELETE_BITMAP,

// Cursor definitions
AS_SET_CURSOR_DATA,	
AS_SET_CURSOR_BCURSOR,
AS_SET_CURSOR_BBITMAP,
AS_SET_CURSOR_SYSTEM,

AS_SET_SYSCURSOR_DATA,
AS_SET_SYSCURSOR_BCURSOR,
AS_SET_SYSCURSOR_BBITMAP,
AS_SET_SYSCURSOR_DEFAULTS,
AS_GET_SYSCURSOR,

AS_SHOW_CURSOR,
AS_HIDE_CURSOR,
AS_OBSCURE_CURSOR,
AS_QUERY_CURSOR_HIDDEN,

AS_CREATE_BCURSOR,
AS_DELETE_BCURSOR,

AS_BEGIN_RECT_TRACKING,
AS_END_RECT_TRACKING,

// Window definitions
AS_SHOW_WINDOW,
AS_HIDE_WINDOW,
AS_QUIT_WINDOW,
AS_SEND_BEHIND,
AS_SET_LOOK,
AS_SET_FEEL, 
AS_SET_FLAGS,
AS_DISABLE_UPDATES,
AS_ENABLE_UPDATES,
AS_BEGIN_UPDATE,
AS_END_UPDATE,
AS_NEEDS_UPDATE,
AS_WINDOW_TITLE,
AS_ADD_TO_SUBSET,
AS_REM_FROM_SUBSET,
AS_SET_ALIGNMENT,
AS_GET_ALIGNMENT,
AS_GET_WORKSPACES,
AS_SET_WORKSPACES,
AS_WINDOW_RESIZE,
AS_WINDOW_MOVE,
AS_SET_SIZE_LIMITS,
AS_ACTIVATE_WINDOW,
AS_WINDOW_MINIMIZE,
AS_UPDATE_IF_NEEDED,
_ALL_UPDATED_,	// this should be moved in place of _UPDATE_IF_NEEDED_ in AppDefs.h


// BPicture definitions
AS_CREATE_PICTURE,
AS_DELETE_PICTURE,
AS_CLONE_PICTURE,
AS_DOWNLOAD_PICTURE,

// Font-related server communications
AS_QUERY_FONTS_CHANGED,
AS_UPDATED_CLIENT_FONTLIST,
AS_GET_FAMILY_ID,
AS_GET_STYLE_ID,
AS_GET_STYLE_FOR_FACE,

// This will be modified. Currently a kludge for the input server until
// BScreens are implemented by the IK Taeam
AS_GET_SCREEN_MODE,

// Global function call defs
AS_SET_UI_COLORS,
AS_GET_UI_COLORS,
AS_GET_UI_COLOR,
AS_SET_DECORATOR,
AS_GET_DECORATOR,
AS_R5_SET_DECORATOR,

AS_COUNT_WORKSPACES,
AS_SET_WORKSPACE_COUNT,
AS_CURRENT_WORKSPACE,
AS_ACTIVATE_WORKSPACE,
AS_SET_SCREEN_MODE,
AS_GET_SCROLLBAR_INFO,
AS_SET_SCROLLBAR_INFO,
AS_IDLE_TIME,
AS_SELECT_PRINTER_PANEL,
AS_ADD_PRINTER_PANEL,
AS_RUN_BE_ABOUT,
AS_SET_FOCUS_FOLLOWS_MOUSE,
AS_FOCUS_FOLLOWS_MOUSE,
AS_SET_MOUSE_MODE,
AS_GET_MOUSE_MODE,

// Hook function messages
AS_WORKSPACE_ACTIVATED,
AS_WORKSPACES_CHANGED,
AS_WINDOW_ACTIVATED,
AS_SCREENMODE_CHANGED,

// Graphics calls
// Are these TRANSACTION codes needed ?
AS_BEGIN_TRANSACTION,
AS_END_TRANSACTION,
AS_SET_HIGH_COLOR,
AS_SET_LOW_COLOR,
AS_SET_VIEW_COLOR,

AS_STROKE_ARC,
AS_STROKE_BEZIER,
AS_STROKE_ELLIPSE,
AS_STROKE_LINE,
AS_STROKE_LINEARRAY,
AS_STROKE_POLYGON,
AS_STROKE_RECT,
AS_STROKE_ROUNDRECT,
AS_STROKE_SHAPE,
AS_STROKE_TRIANGLE,

AS_FILL_ARC,
AS_FILL_BEZIER,
AS_FILL_ELLIPSE,
AS_FILL_POLYGON,
AS_FILL_RECT,
AS_FILL_REGION,
AS_FILL_ROUNDRECT,
AS_FILL_SHAPE,
AS_FILL_TRIANGLE,

AS_MOVEPENBY,
AS_MOVEPENTO,
AS_SETPENSIZE,

AS_DRAW_STRING,
AS_SET_FONT,
AS_SET_FONT_SIZE,

AS_FLUSH,
AS_SYNC,

AS_LAYER_CREATE,
AS_LAYER_DELETE,
AS_LAYER_CREATE_ROOT,
AS_LAYER_DELETE_ROOT,
AS_LAYER_ADD_CHILD, 
AS_LAYER_REMOVE_CHILD,
AS_LAYER_REMOVE_SELF,
AS_LAYER_SHOW,
AS_LAYER_HIDE,
AS_LAYER_MOVE,
AS_LAYER_RESIZE,
AS_LAYER_INVALIDATE,
AS_LAYER_DRAW,

AS_LAYER_GET_TOKEN,
AS_LAYER_ADD,
AS_LAYER_REMOVE,

// View/Layer definitions
AS_LAYER_GET_COORD,
AS_LAYER_SET_FLAGS,
AS_LAYER_SET_ORIGIN,
AS_LAYER_GET_ORIGIN,
AS_LAYER_RESIZE_MODE,
AS_LAYER_CURSOR,
AS_LAYER_BEGIN_RECT_TRACK,
AS_LAYER_END_RECT_TRACK,
AS_LAYER_DRAG_RECT,
AS_LAYER_DRAG_IMAGE,
AS_LAYER_GET_MOUSE_COORDS,
AS_LAYER_SCROLL,
AS_LAYER_SET_LINE_MODE,
AS_LAYER_GET_LINE_MODE,
AS_LAYER_PUSH_STATE,
AS_LAYER_POP_STATE,
AS_LAYER_SET_SCALE,
AS_LAYER_GET_SCALE,
AS_LAYER_SET_DRAW_MODE,
AS_LAYER_GET_DRAW_MODE,
AS_LAYER_SET_BLEND_MODE,
AS_LAYER_GET_BLEND_MODE,
AS_LAYER_SET_PEN_LOC,
AS_LAYER_GET_PEN_LOC,
AS_LAYER_SET_PEN_SIZE,
AS_LAYER_GET_PEN_SIZE,
AS_LAYER_SET_HIGH_COLOR,
AS_LAYER_SET_LOW_COLOR,
AS_LAYER_SET_VIEW_COLOR,
AS_LAYER_GET_COLORS,
AS_LAYER_PRINT_ALIASING,
AS_LAYER_CLIP_TO_PICTURE,
AS_LAYER_CLIP_TO_INVERSE_PICTURE,
AS_LAYER_GET_CLIP_REGION,
AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT,
AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT,
AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT,
AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT,

AS_LAYER_DRAW_STRING,
AS_LAYER_SET_CLIP_REGION,
AS_LAYER_LINE_ARRAY,
AS_LAYER_BEGIN_PICTURE,
AS_LAYER_APPEND_TO_PICTURE,
AS_LAYER_END_PICTURE,
AS_LAYER_COPY_BITS,
AS_LAYER_DRAW_PICTURE,
AS_LAYER_INVAL_RECT,
AS_LAYER_INVAL_REGION,
AS_LAYER_INVERT_RECT,
AS_LAYER_MOVETO,
AS_LAYER_RESIZETO,
AS_LAYER_SET_STATE,
AS_LAYER_SET_FONT_STATE,
AS_LAYER_GET_STATE,
AS_LAYER_SET_VIEW_IMAGE,
AS_LAYER_SET_PATTERN,
AS_SET_CURRENT_LAYER,

AS_ACQUIRE_SERVERMEM,
AS_RELEASE_SERVERMEM,
AS_AREA_MESSAGE,

	DR_GET_DEFAULT_FONT,

	AR_CREATE_FONT,
	AR_DELETE_FONT,
	AR_SET_FONT_FAMILY_AND_STYLE,
	AR_GET_FONT_FAMILY_AND_STYLE,
	AR_SET_FONT_PROPERTIES,
	AR_GET_STRING_WIDTHS,
	AR_GET_STRING_LENGTHS,

	AR_CREATE_SPRITE,
	AR_DELETE_SPRITE,
	AR_MOVE_SPRITE,

	AR_RESCAN_FONTS,
	AR_GET_FONT_FAMILY_COUNT,
	AR_GET_FONT_FAMILY,
	AR_GET_FONT_STYLE_COUNT,
	AR_GET_FONT_STYLE,
	AR_GET_SCREENMODE_COUNT,
	AR_GET_SCREENMODE_INFO,
	AR_LOCK_DESKTOP,
	AR_UNLOCK_DESKTOP,
	AR_SET_SCREEN_MODE,
	AR_GET_FONT_SIZES,

	WR_GET_VIEW_FRAME = 20000,
	WR_TOGGLE_VIEW_DEPTH,
	WR_RENDER,
	WR_BEGIN_DRAG,
	WR_WND_MOVE_REPLY,
	WR_UPDATE_REGIONS,
	WR_GET_PEN_POSITION,
	WR_GET_MOUSE,
	WR_SET_MOUSE_POS,
};


enum
{
	DRC_PING,
	DRC_WRITE_PIXEL32,
	_DRC_unused3,
	DRC_LINE32,
	DRC_LINE_ARRAY32,
	DRC_FILL_RECT32,
	DRC_COPY_RECT,
	DRC_STROKE_RECT32,
	DRC_SET_COLOR32,
	DRC_SET_PEN_POS,
	DRC_SET_FONT,
	DRC_DRAW_STRING,
	DRC_DRAW_BITMAP,
	DRC_SET_FRAME,
	DRC_SCROLL_VIEW,
	DRC_BEGIN_UPDATE,
	DRC_END_UPDATE,
	DRC_SET_DRAWING_MODE,
	DRC_SHOW_VIEW,
	DRC_INVALIDATE_VIEW,
	DRC_INVALIDATE_RECT,
	DRC_STROKE_ELLIPSE,
	DRC_FILL_ELLIPSE,
	DRC_STROKE_ARC,
	DRC_FILL_ARC
};

#define AS_PATTERN_SIZE 8
#define AS_SET_COLOR_MSG_SIZE 8+4
#define AS_STROKE_ARC_MSG_SIZE 8+6*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_BEZIER_MSG_SIZE 8+8*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_ELLIPSE_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_LINE_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_RECT_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_ROUNDRECT_MSG_SIZE 8+6*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_TRIANGLE_MSG_SIZE 8+10*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_ARC_MSG_SIZE 8+6*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_BEZIER_MSG_SIZE 8+8*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_ELLIPSE_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_RECT_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_ROUNDRECT_MSG_SIZE 8+6*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_TRIANGLE_MSG_SIZE 8+10*sizeof(float)+AS_PATTERN_SIZE


typedef	struct
{
	uint32 nCmd;
	uint32 nSize;
	uint32 hViewToken;
} GRndHeader_s;

struct GRndSetFrame_s : GRndHeader_s
{
	BRect cFrame;
};

struct GRndShowView_s : GRndHeader_s
{
	bool bVisible;
};

typedef	struct
{
	GRndHeader_s	sHdr;
	BPoint			sToPos;
} GRndLine32_s;

typedef	struct
{
	GRndHeader_s	sHdr;
	int				sCount;
	BPoint*			sFromPos;
	BPoint*			sToPos;
	rgb_color*		sColor;
} GRndLineArray32_s;

struct GRndRect32_s
{
	GRndHeader_s	sHdr;
	BRect			sRect;
	rgb_color		sColor;
};

struct GRndRoundRect_s
{
	GRndHeader_s	sHdr;
	BRect			sRect;
	float			sXRadius;
	float			sYRadius;
	rgb_color		sColor;
};

struct GRndArc_s
{
	GRndHeader_s	sHdr;
	BRect			sRect;
	float			sAngle;
	float			sSpan;
	rgb_color		sColor;
};

struct GRndCopyRect_s
{
	GRndHeader_s	sHdr;
	BRect			cSrcRect;
	BRect			cDstRect;
};

typedef	struct
{
	GRndHeader_s	sHdr;
	int				hBitmapToken;
	BRect			cSrcRect;
	BRect			cDstRect;
} GRndDrawBitmap_s;

enum
{
	PEN_HIGH,
	PEN_LOW,
	PEN_ERASE
};

struct GRndSetDrawingMode_s : GRndHeader_s
{
	int nDrawingMode;
};

typedef	struct
{
	GRndHeader_s	sHdr;
	int				nWhichPen;
	rgb_color		sColor;
} GRndSetrgb_color;

typedef	struct
{
	GRndHeader_s	sHdr;
	bool			bRelative;
	BPoint			sPosition;
} GRndSetPenPos_s;

typedef	struct
{
	GRndHeader_s	sHdr;
	int				hFontID;
} GRndSetFont_s;

typedef	struct
{
	GRndHeader_s	sHdr;
	int				nLength;
	char			zString[1];	/* String of nLength characters, or null terminated if nLength == -1	*/
} GRndDrawString_s;

struct GRndScrollView_s : GRndHeader_s
{
    BPoint			cDelta;
};

struct GRndInvalidateRect_s : GRndHeader_s
{
    bool			m_bRecurse;
    BRect			m_cRect;
};

struct GRndInvalidateView_s : GRndHeader_s
{
    bool			m_bRecurse;
};

/***	Messages sent to main thread of the display server	***/


struct AR_DeleteWindow_s
{
	ServerWindow* pcWindow;
};

struct AR_LockDesktop_s
{
	AR_LockDesktop_s( port_id hReply, int nDesktop ) { m_hReply = hReply; m_nDesktop = nDesktop; }

	port_id m_hReply;
	int	  m_nDesktop;
};

struct AR_LockDesktopReply_s
{
	int	        m_nCookie;
	int	        m_nDesktop;
	IPoint      m_cResolution;
	color_space m_eColorSpace;
	area_id     m_hFrameBufferArea;
};

struct AR_UnlockDesktop_s
{
	AR_UnlockDesktop_s( int nCookie ) { m_nCookie = nCookie; }
	int m_nCookie;
};

typedef	struct
{
	int		nLength;
	char	zString[1];
} StringHeader_s;

typedef struct
{
	port_id	 hReply;
	int		 hFontToken;
	int		 nStringCount;
	StringHeader_s sFirstHeader;
	/*** nStringCount - 1, string headers follows	***/
} AR_GetStringWidths_s;


typedef struct
{
	int	nError;
	int	anLengths[1];
	/*** nStringCount - 1, lengths follow	***/
} AR_GetStringWidthsReply_s;

typedef struct
{
	port_id	 hReply;
	int		 hFontToken;
	int		 nStringCount;
	int		 nWidth;	/* Pixel width	*/
	int		 bIncludeLast;	// Should be a bool
	StringHeader_s sFirstHeader;
	/*** nStringCount - 1, string headers follows	***/
} AR_GetStringLengths_s;


typedef struct
{
	int	nError;
	int	anLengths[1];
	/*** nStringCount - 1, lengths follow	***/
} AR_GetStringLengthsReply_s;

/***	Bitmap messages	***/

struct AR_CreateBitmap_s
{
	port_id	hReply;
	BRect	rect;
	color_space	eColorSpc;
	int32	flags;
	int32	bytesperline;
};

struct AR_CreateBitmapReply_s
{
	AR_CreateBitmapReply_s() {}
	AR_CreateBitmapReply_s( int hHandle, area_id hArea, int32 offset ) { m_hHandle = hHandle; m_hArea = hArea; areaOffset = offset;}
	int	  m_hHandle;
	area_id m_hArea;
	int32 areaOffset;
};

struct AR_DeleteBitmap_s
{
	AR_DeleteBitmap_s( int nHandle ) { m_nHandle = nHandle; }
	int m_nHandle;
};

struct AR_CreateSprite_s
{
	AR_CreateSprite_s( port_id hReply, const BRect& cFrame, int nBitmap ) {
	m_hReply = hReply; m_cFrame = cFrame; m_nBitmap = nBitmap;
	}
	port_id m_hReply;
	int	    m_nBitmap;
	BRect   m_cFrame;
};

struct AR_CreateSpriteReply_s
{
	AR_CreateSpriteReply_s() {}
	AR_CreateSpriteReply_s( uint32 nHandle, int nError ) { m_nHandle = nHandle; m_nError = nError; }

	uint32 m_nHandle;
	int    m_nError;
};

struct AR_DeleteSprite_s
{
	AR_DeleteSprite_s( uint32 nHandle ) { m_nSprite = nHandle; }
	uint32 m_nSprite;
};

struct AR_MoveSprite_s
{
	AR_MoveSprite_s( uint32 nHandle, const BPoint& cNewPos ) {
	m_nSprite = nHandle; m_cNewPos = cNewPos;
	}
	uint32 m_nSprite;
	BPoint m_cNewPos;
};

struct AR_GetQualifiers_s
{
	AR_GetQualifiers_s( port_id hReply ) { m_hReply = hReply; }
	port_id m_hReply;
};

struct AR_GetQualifiersReply_s
{
	AR_GetQualifiersReply_s() {}
	AR_GetQualifiersReply_s( uint32 nQualifiers ) { m_nQualifiers = nQualifiers; }
	uint32	m_nQualifiers;
};

struct AR_GetScreenModeCount_s
{
	port_id	m_hReply;
};

struct AR_GetScreenModeCountReply_s
{
	int	m_nCount;
};

struct AR_GetScreenModeInfo_s
{
	port_id	m_hReply;
	int		m_nIndex;
};

struct AR_GetScreenModeInfoReply_s
{
	int	      m_nError;
	int	      m_nWidth;
	int 	  m_nHeight;
	int	      m_nBytesPerLine;
	color_space m_eColorSpace;
};

enum { SCRMF_RES = 0x0001, SCRMF_COLORSPACE = 0x0002, SCRMF_REFRESH = 0x0004, SCRMF_POS = 0x0008, SCRMF_SIZE = 0x0010 };

struct AR_SetScreenMode_s
{
	int		m_nVersion;
	int		m_nDesktop;
	uint32	m_nValidityMask;
	int		m_nWidth;
	int		m_nHeight;
	color_space	m_eColorSpace;
	float	m_vRefreshRate;
	float	m_vHPos;
	float	m_vVPos;
	float	m_vHSize;
	float	m_vVSize;
};

typedef struct
{
	int	nKeyCode;
	int	nQualifiers;
	char	zString[64];
} SR_KbdEvent_s;

/*** Messages sent to window threads in the display server ***/

struct WR_Request_s
{
	int m_hTopView;
};

enum { RENDER_BUFFER_SIZE = 500 };

struct WR_Render_s : WR_Request_s
{
	port_id	hReply;
	int		nCount;	/* Number of render instructions	*/
	uint8	aBuffer[RENDER_BUFFER_SIZE];
};

struct WR_GetPenPosition_s : WR_Request_s
{
	port_id m_hReply;
	int	    m_hViewHandle;
};

struct WR_GetPenPositionReply_s
{
	BPoint m_cPos;
};

#endif
