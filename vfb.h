/**
 ** $Id$
 **  
 ** vfb.h: win32 virtual frame buffer, used by minigui application. 
 **     
 ** Copyright (C) 2004 ~ 2007 Feynman Software.
 ** 
 */

#define	CAPTION_TYPE		2
#define	IME_MESSAGE_TYPE	4
#define	SHOW_HIDE_TYPE		5

typedef struct _WVFbHeader
{
	unsigned int info_size;

    int width;
    int height;
    int depth;
    int pitch;

    int dirty;
	int dirty_rc_l;
	int dirty_rc_t;
	int dirty_rc_r;
	int dirty_rc_b;

	int palette_changed;
	int palette_offset;

	int fb_offset;

    int MSBLeft;
    
    int Rmask;
    int Gmask;
    int Bmask;
    int Amask;
} WVFbHeader;

typedef struct _WVFbPalEntry {
    unsigned char r, g, b, a;
} WVFbPalEntry;

typedef struct _WVFbKeyData
{
    unsigned short keycode;
    unsigned short keystate;
} WVFbKeyData;

typedef struct _WVFbMouseData
{
    unsigned short x;
    unsigned short y;
    unsigned int button;
} WVFbMouseData;

typedef struct _WVFbEventData
{
    int event_type;
    union 
    {
        WVFbKeyData kbdata;
	    WVFbMouseData mousedata;
    };
} WVFbEventData;

typedef struct _WVFbIMEventData
{
	int event_type;
	int size;
	char buff[1];
} WVFbIMEventData;

/* ----------- function declarations --------------- */
int  VFBInit(HWND hwnd, int pid, int w, int h, int depth);
void VFBClose();
void VFBSendMouseData (const POINT* pt, int btns);
void VFBSendKbData (int keycode, bool press, int repeat);
void VFBSendIMData (char *str);
void VFBRecvEvent (HWND hWnd);
void VFBDrawScreen (HDC hdc, RECT rcSreen, RECT rcClient, int scrollpos, DWORD Zoom);
void VFBResetEvent(void);
BOOL VFBSaveAsPicture(const char * path);
