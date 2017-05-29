/**
 ** $Id$
 **  
 ** wvfb.h: win32 virtual frame buffer, used by minigui application. 
 **     
 ** Copyright (C) 2004 ~ 2007 Feynman Software.
 ** 
 */

#if !defined(AFX_WVFB_H__DEB9A38B_4D28_430E_8BD3_E97D1C3248BC__INCLUDED_)
#define AFX_WVFB_H__DEB9A38B_4D28_430E_8BD3_E97D1C3248BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __cplusplus
extern "C" {
#endif

#define _USE_CONFIG 1
#define WVFB_IS_PROCESS

#ifdef WVFB_IS_DLL
#include "resource.h"
#define WVFB_EXPORT __declspec (dllexport)
#else
#define WVFB_EXPORT __declspec (dllimport)
#endif

typedef struct _lcd_info_ {
	short height;			/*the height of the lcd*/
	short width;			/*the width of the lcd*/
	short bpp;				/*the bpp of the lcd*/
	short type;				/*pixel type*/
	short rlen;				/*length of one raser line in bytes*/
	void *fb;				/*Frame buffer*/
}lcd_info;

WVFB_EXPORT int wvfb_init (int x, int y, int w, int h, int depth, LRESULT *ownwndproc);
WVFB_EXPORT int wvfb_drv_lcd_init ();
WVFB_EXPORT int wvfb_drv_lcd_getinfo (lcd_info *li);
WVFB_EXPORT int wvfb_comm_kb_getdata (short *key, short *status);
WVFB_EXPORT int wvfb_comm_wait_for_input (void);
WVFB_EXPORT int wvfb_comm_ts_getdata (short *x, short *y, short *button);

WVFB_EXPORT int drv_lcd_init ();
WVFB_EXPORT void drv_lcd_getinfo (lcd_info *li);
WVFB_EXPORT int comm_kb_getdata (short *key, short *status);
WVFB_EXPORT int comm_wait_for_input (void);
WVFB_EXPORT int comm_ts_getdata (short *x, short *y, short *button);
WVFB_EXPORT LRESULT CALLBACK wvfb_dll_default_winproc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#ifdef __cplusplus
}
#endif

#define MAX_LOADSTRING 100
#define ID_TIMER	100  //¶¨Ê±Æ÷ID

#define SHIFT_KEY	0x01
#define CTRL_KEY	0x02
#define ALT_KEY		0x04
#define CONFIGFILE  "config\\configure.ini"

#define DEFAULTWIDTH 640
#define DEFAULTHEIGHT 480
#define MINIWIDTH 150
#define  MINIHEIGHT 220

#define SCANCODE_HOME                   102
#define SCANCODE_CURSORBLOCKUP          103    /* Cursor key block */
#define SCANCODE_PAGEUP                 104
#define SCANCODE_CURSORBLOCKLEFT        105    /* Cursor key block */
#define SCANCODE_CURSORBLOCKRIGHT       106    /* Cursor key block */
#define SCANCODE_END                    107
#define SCANCODE_CURSORBLOCKDOWN        108    /* Cursor key block */
#define SCANCODE_PAGEDOWN               109
#define SCANCODE_INSERT                 110
#define SCANCODE_REMOVE                 111


#endif // !defined(AFX_WVFB_H__DEB9A38B_4D28_430E_8BD3_E97D1C3248BC__INCLUDED_)
