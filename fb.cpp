/**
 ** $Id$
 **  
 ** fb.cpp: win32 virtual frame buffer, used by minigui application. 
 **     
 ** Copyright (C) 2004 ~ 2007 Feynman Software.
 ** 
 */


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock.h>
#include "stdafx.h"
#include "Vfw.h"
#include "wvfb.h"
#include "vfb.h"

#pragma comment(lib, "ws2_32.lib")

#define SCANCODE_CURSORBLOCKUP          103    /* Cursor key block */
#define SCANCODE_CURSORBLOCKLEFT        105    /* Cursor key block */
#define SCANCODE_CURSORBLOCKRIGHT       106    /* Cursor key block */
#define SCANCODE_CURSORBLOCKDOWN        108    /* Cursor key block */

#define PATH_MAX 1024 * 4

static WVFbHeader *hdr;

static HANDLE hScreen;
static LPVOID lpScreen;   
static int screen_w, screen_h; 
static LPBITMAPINFO pbi;
static HBITMAP hmgbmp = NULL;
static DWORD dwBmpHeadLen = 0;
static unsigned char *imageData = NULL;

static int sockfd;

static char *_map_file = "WVFBScreenMap-";
static char g_map_file[128];
static HANDLE g_map_filehandle;

static PALETTEENTRY palPalEntry_16 [16] = {
    {0, 0, 0},
    {17, 17, 17},
    {34, 34, 34},
    {51, 51, 51},
    {68, 68, 68},
    {85, 85, 85},
    {102, 102, 102},
    {119, 119, 119},
    {136, 136, 136},
    {152, 152, 152},
    {169, 169, 169},
    {186, 186, 186},
    {213, 213, 213},
    {220, 220, 220},
    {237, 237, 237},
    {255, 255, 255},	
};

inline unsigned int get_pitch (int depth, int w)
{
    int pitch;

    if ( depth == 1 )
        pitch = (w*depth+7)/8;
    else
        pitch = ((w*depth+31)/32)*4;

    return pitch;
}

inline static BYTE get_r (int depth, unsigned char *p)
{
    if (depth == 16)
        return (BYTE) ( (*(unsigned short *)p >> 11) << 3 );
    else if (depth == 8)
        return (*(BYTE*)p >> 5) << 5;
    else
        return 0;
}

inline static BYTE get_g (int depth, unsigned char *p)
{
    if (depth == 16)
        return (BYTE) ( ((*(unsigned short *)p >> 5) & 0x3F) << 2);
    else if (depth == 8)
        return ( (*(BYTE*)p >> 2) & 0x07) << 5;
    else
        return 0;
}

inline static BYTE get_b (int depth, unsigned char *p)
{
    if (depth == 16)
        return (BYTE) ( (*(unsigned short *)p & 0x1F) << 3);
    else if (depth == 8)
        return ( *(BYTE*)p & 0x03) << 6;
    else
        return 0;
}

int VFBInit(HWND hwnd, int p_pid, int w, int h, int depth)
{
    HDC hdc;
    UINT uPicMode;
    DWORD *mask;
    int color_num = 0;
    int pitch, datalen;
	struct sockaddr_in serv_ad;
	WORD wVersionRequested;
    WSADATA wsaData;

	wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);

    if(hwnd == 0)
        return -1;

    /***************  create shared memory  *****************/
    if(depth <= 8)
        color_num = 1 << depth;

    pitch   = get_pitch (depth, w);
    datalen = pitch * h + sizeof(WVFbHeader) + color_num * sizeof(WVFbPalEntry);

    {
        char tmp[PATH_MAX];
        GetTempPath(sizeof(tmp)/sizeof(tmp[0]), tmp);
        sprintf (g_map_file, "%s/%s-%d", tmp, _map_file, p_pid);
    }

	g_map_filehandle = CreateFile(g_map_file,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if(g_map_filehandle == INVALID_HANDLE_VALUE){
	   printf("cannot create file %s (%d).\n", g_map_file, GetLastError());
	   return -1;
	}

	hScreen = CreateFileMapping(g_map_filehandle, NULL,  PAGE_READWRITE, 0, datalen, NULL);
    if (hScreen == NULL)
        return -1;

    lpScreen = MapViewOfFile (hScreen, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (lpScreen == NULL)
	{
		CloseHandle (hScreen);
        return -1;
	}

    hdr                     = (WVFbHeader *)lpScreen;
    hdr->width              = w;
    hdr->height             = h;
    hdr->depth              = depth;
    hdr->pitch              = pitch;
    hdr->dirty              = 0;
    hdr->dirty_rc_l         = 0;
    hdr->dirty_rc_t         = 0;
    hdr->dirty_rc_r         = 0;
    hdr->dirty_rc_b         = 0;
    hdr->palette_changed    = 0;
    hdr->palette_offset     = sizeof(WVFbHeader);
    hdr->fb_offset          = sizeof(WVFbHeader) + color_num * sizeof(WVFbPalEntry);

    screen_w = w;
    screen_h = h;

    uPicMode = DIB_PAL_COLORS;
    switch (hdr->depth)
    {		
        case 1:
            hdr->MSBLeft = 1;
            dwBmpHeadLen = sizeof(BITMAPINFO) + sizeof(RGBQUAD);
            pbi = (LPBITMAPINFO)calloc(1, dwBmpHeadLen);
            pbi->bmiHeader.biClrUsed  =  2;
            memset(pbi->bmiColors, 0, 2 * sizeof(RGBQUAD));
            break;
        case 2:
            hdr->MSBLeft = 1;
            dwBmpHeadLen = sizeof(BITMAPINFO) + 15 * sizeof(RGBQUAD);
            pbi = (LPBITMAPINFO)calloc(1, dwBmpHeadLen);
            pbi->bmiHeader.biClrUsed  = 16;
            memset(pbi->bmiColors, 0, 16 * sizeof(RGBQUAD));
            break;
        case 4:
            hdr->MSBLeft = 1;
            dwBmpHeadLen = sizeof(BITMAPINFO) + 15 * sizeof(RGBQUAD);
            pbi = (LPBITMAPINFO)calloc(1, dwBmpHeadLen);
            pbi->bmiHeader.biClrUsed  = 16;
            memset(pbi->bmiColors, 0, 16 * sizeof(RGBQUAD));
            break;
        case 8:
            dwBmpHeadLen = sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD);
            pbi = (LPBITMAPINFO)calloc(1, dwBmpHeadLen);
            pbi->bmiHeader.biClrUsed  = 256;
            memset(pbi->bmiColors, 0, 256 * sizeof(RGBQUAD));
            break;
        case 16:
            dwBmpHeadLen = sizeof(BITMAPINFO) + 3 * sizeof(DWORD);
            pbi = (LPBITMAPINFO)calloc(1, dwBmpHeadLen);
            pbi->bmiHeader.biClrUsed  = 0;
            mask = (DWORD *)&pbi->bmiColors;
            pbi->bmiHeader.biCompression = BI_BITFIELDS;
            hdr->Rmask = 0xF800;
            hdr->Gmask = 0x07E0;
            hdr->Bmask = 0x001F;
            hdr->Amask = 0x0000;
            *mask++ = 0xF800;
            *mask++ = 0x07E0;
            *mask = 0x001F;
            break;
        case 32:
            hdr->Rmask = 0x00FF0000;
            hdr->Gmask = 0x0000FF00;
            hdr->Bmask = 0x000000FF;
            hdr->Amask = 0xFF000000;
        default:
            dwBmpHeadLen = sizeof(BITMAPINFO);
            pbi = (LPBITMAPINFO)calloc(1, dwBmpHeadLen);
            pbi->bmiHeader.biClrUsed  = 0;
            uPicMode = DIB_RGB_COLORS;
    }
    if(hdr->depth != 2)
    {
        imageData = (unsigned char*)hdr + hdr->fb_offset;

        pbi->bmiHeader.biBitCount		 = hdr->depth;
        pbi->bmiHeader.biSizeImage     = hdr->pitch * screen_h;
    }
    else
    {
        imageData = (unsigned char *)malloc(get_pitch(4, w) * screen_h);

        pbi->bmiHeader.biBitCount		 = 4;
        pbi->bmiHeader.biSizeImage     = get_pitch(4, w) * screen_h;
    }
    pbi->bmiHeader.biWidth         = screen_w;
    pbi->bmiHeader.biHeight        = -screen_h;
    pbi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbi->bmiHeader.biPlanes        = 1;
    pbi->bmiHeader.biXPelsPerMeter = 0;
    pbi->bmiHeader.biYPelsPerMeter = 0;    
    pbi->bmiHeader.biClrImportant  = 0;

    hdc = GetDC(hwnd);
    hmgbmp = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER)pbi, CBM_INIT, imageData, pbi, uPicMode); 
    if( !imageData || !hmgbmp)
    {
        UnmapViewOfFile (lpScreen);
        CloseHandle (hScreen);
        DeleteObject (hmgbmp);	
        free(pbi);
        pbi = NULL;
        ReleaseDC(hwnd, hdc);
        return -1;
    }
    ReleaseDC(hwnd, hdc);

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	serv_ad.sin_family = AF_INET;
    serv_ad.sin_port = htons(p_pid);
    serv_ad.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//serv_ad.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (0 != connect(sockfd, (struct sockaddr *)&serv_ad, sizeof(serv_ad))) {
		exit(1);
	}

    return 0;
}

void VFBClose (void)
{   
    UnmapViewOfFile (lpScreen);
    CloseHandle (hScreen);
    CloseHandle(g_map_filehandle);
	DeleteFile(g_map_file);
    DeleteObject (hmgbmp);
    free(pbi);
    pbi = NULL;
	closesocket((SOCKET)sockfd);
	WSACleanup();
}

void VFBSendMouseData(const POINT* pt, int btns)
{
    DWORD w_len;
    WVFbEventData event;

    event.event_type = 0;
    event.mousedata.x = (unsigned short)pt->x;
    event.mousedata.y = (unsigned short)pt->y;
    event.mousedata.button = btns;

	send (sockfd, (const char *)&event, sizeof(event), 0);
}

void VFBSendKbData (int keycode, bool press, int repeat)
{
    WVFbEventData event;
    DWORD w_len;

    event.event_type = 1;
    event.kbdata.keycode = keycode; 
	event.kbdata.keystate = press ? 1 : 0;

	send (sockfd, (const char *)&event, sizeof(event), 0);
}

void VFBSendIMData (char *str)
{
	int len;
	char szbuff[1024];
	WVFbIMEventData *event = (WVFbIMEventData*)szbuff;
	if (str == NULL || (len = strlen (str)) <= 0)
		return;
	event->event_type = IME_MESSAGE_TYPE;
	event->size = len;
	strcpy (event->buff, str);

	send (sockfd, (const char *)event, 8 + len, 0);
}

void VFBRecvEvent (HWND hWnd)
{
    char buf[1024];
    int type;
    fd_set readfds;
    struct timeval tv;

    while (1) {
        memset (&tv, 0, sizeof(tv));
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        if (select (sockfd + 1, &readfds, NULL, NULL, &tv) <= 0)
            return;

        if (recv(sockfd, (char *)&type, sizeof(type), 0) <= 0)
            return;

        switch (type) {
            case CAPTION_TYPE:
                {
                    int size;
                    if (recv (sockfd, (char *)&size, sizeof(size), 0) != sizeof(size))
                        return;
                    else if (size <= 0)
                        return;
                    while (size > 0) {
                        int nread;
                        nread = recv (sockfd, buf, size > (sizeof(buf) - 1) ? (sizeof(buf) - 1) : size, 0);
                        buf[nread] = '\0';
                        size -= nread;
                    }
                    SetWindowText (hWnd, buf);
                }
                break;
            case SHOW_HIDE_TYPE:
                {
                    int show;
                    if (recv (sockfd, (char *)&show, sizeof(show), 0) != sizeof(show))
                        return;
                    if (show)
                    {
                        ShowWindow (hWnd, SW_MAXIMIZE);
                        ShowWindow (hWnd, SW_SHOWNORMAL);
                    } else {
                        ShowWindow (hWnd, SW_MINIMIZE);
                    }
                }
                break;
            default:
                break;
        }
    }
}

static void WvfbSetPalette(void)
{
    int i, color_num;
    WVFbPalEntry* palette;

    color_num = 1 << hdr->depth;

    palette = (WVFbPalEntry *)((char *)hdr + hdr->palette_offset);

    pbi->bmiHeader.biClrUsed  =  color_num;

    for(i=0 ;i < color_num; i++) 
    {
        pbi->bmiColors[i].rgbRed      = palette->r;
        pbi->bmiColors[i].rgbGreen    = palette->g;
        pbi->bmiColors[i].rgbBlue     = palette->b;
        pbi->bmiColors[i].rgbReserved = palette->a;
        palette++;
    }
    return;
}
/* draw screen, rcUpdate is the region to update */
void VFBDrawScreen (HDC hdc, RECT rcSreen, RECT rcClient, int scrollpos, DWORD zoom)
{
    int width, height;
    unsigned int i;
    unsigned char *bits, *src, *dst;
    UINT uPicMode = DIB_PAL_COLORS;	

    if(!imageData || !hmgbmp) 
        return;    

    if(screen_w == 0 || screen_h == 0)
        return;

    if(hdr->depth > 8)
        uPicMode = DIB_RGB_COLORS;

    width = screen_w;
    height = screen_h;

    if(rcClient.right <= rcSreen.right - rcSreen.right)
        width = rcClient.right - rcSreen.left;

    if((rcClient.bottom <= rcSreen.bottom - rcSreen.top) && !scrollpos)
        height = rcClient.bottom - rcSreen.top;

    /* 2 bits color , used 4 bits bitmap.*/ 
    if(hdr->depth == 2)
    {
        dst = imageData;
        bits = (unsigned char*)hdr + hdr->fb_offset;
        for(i = 0; i < get_pitch(4, width) * height ; i += 2 )
        {
            src = bits++;
            *dst++ = ((*src >> 6) & 0x03) << 4 | ((*src >> 4) & 0x03);
            *dst++ = ((*src >> 2) & 0x03) << 4 | ((*src >> 0) & 0x03);
        }
    }
    /* palette changed */
    if(hdr->depth <= 8 && hdr->palette_changed)
    {
        WvfbSetPalette();
        hdr->palette_changed = 0;
    }

	if (zoom == 1)
	{
		SetDIBitsToDevice(hdc, rcSreen.left, rcSreen.top - scrollpos, 
			width, height, 0, 0, 0 , height, imageData, pbi,DIB_RGB_COLORS);
	} else {
		SetStretchBltMode(hdc, COLORONCOLOR);
		StretchDIBits(hdc, rcSreen.left, rcSreen.top - scrollpos, width * zoom, height * zoom, 0, 0,
            width, height, imageData, pbi, DIB_RGB_COLORS, MERGECOPY);
	}

    hdr->dirty = 0;

    return;
}

BOOL VFBSaveAsPicture(const char * path)
{
    BITMAPFILEHEADER bmfHdr; 
    DWORD dwDIBSize;
    HANDLE hfile;
    DWORD count, bitsize;
    time_t now;
    char chTmp[100];
    char filename[256];

    if(!imageData || !hmgbmp) 
        return FALSE; 

    time(&now);
    strftime(chTmp, 256, "%Y%m%d%H%M_%S.bmp", localtime(&now));

    sprintf(filename, "%s\\%s", path, chTmp);
    hfile = CreateFile(filename, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
    if(hfile == 0)
    {
        MessageBox(NULL, "Create file failure!", "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    bmfHdr.bfType = 0x4d42;  // "BM"  
    bitsize = hdr->pitch * hdr->height;	
    dwDIBSize = dwBmpHeadLen + bitsize; 
    bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER);
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + dwBmpHeadLen;
    WriteFile(hfile, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &count, NULL);
    if(count != sizeof(BITMAPFILEHEADER))
    {
        MessageBox(NULL, "Write file failure!", "Error", MB_OK | MB_ICONERROR);
        CloseHandle(hfile);
        return FALSE;
    }

    WriteFile(hfile, (void *)pbi, dwBmpHeadLen, &count, NULL);    
    if(count != dwBmpHeadLen)
    {
        MessageBox(NULL, "Write file failure!", "Error", MB_OK | MB_ICONERROR);
        CloseHandle(hfile);
        return FALSE;
    }
    WriteFile(hfile, imageData, bitsize, &count, NULL);
    if(count != bitsize)
    {
        MessageBox(NULL, "Write file failure!", "Error", MB_OK | MB_ICONERROR);
        CloseHandle(hfile);
        return FALSE;
    }

    FlushFileBuffers(hfile);
    CloseHandle(hfile);
    return TRUE;
}


