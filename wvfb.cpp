/**
 ** $Id$
 **  
 ** wvfb.cpp: win32 virtual frame buffer, used by minigui application. 
 **     
 ** Copyright (C) 2004 ~ 2007 Feynman Software.
 ** 
 */

#include <stdio.h>
#include <shlobj.h>
#include <commdlg.h>
#include <commctrl.h>
#include <cderr.h>
#include <imm.h>
#pragma comment(lib,"IMM32.lib")
#include "stdafx.h"
#include "resource.h"
#include "getconfiginfo.h" 
#include "Vfw.h"
#include "vfb.h" 
#include "wvfb.h"

static WVFBCONFIGINFO config; 
static BITMAP bmBackGround;
static HBITMAP hbmBackGround = 0;
static int  nCurConfigIndex = -1;
static DWORD dwFreshRate = 30;
static int  yCurrentScroll;
static int  IsKeyboardStatus = 0;
static DWORD dwZoom = 1;
static int width = 320, height = 240, depth = 16; 
static int p_pid = 0;

static void LoadConfigInfo (int typeindex);
static void ChangeConfigUpdate(WVFBCONFIGINFO *pcfg, HWND hWnd, int *yMaxScroll);


// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING] = _T("wvfb");	// The title bar text
TCHAR szSaveFilePath[MAX_PATH] = _T("");

static unsigned char deptharray[6] = {1, 4, 8, 16, 24, 32};

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int, int, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    selectConfigProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK	RefreshProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void GB2312ToUTF_8 (const unsigned char * pIn, unsigned char *pOut, int *pOutLen)
{
	int wLen = MultiByteToWideChar (CP_ACP, 0, (LPCSTR)pIn, -1, NULL, 0);
	LPWSTR wStr = new WCHAR[wLen];
	MultiByteToWideChar (CP_ACP, 0, (LPCSTR)pIn, -1, wStr, wLen);
	int aLen = WideCharToMultiByte (CP_UTF8, 0, wStr, -1, NULL, 0, NULL, NULL);
	*pOutLen = WideCharToMultiByte (CP_UTF8, 0, wStr, -1, (LPSTR)pOut, aLen, NULL, NULL);
	pOut[*pOutLen]=0;
	delete []wStr;
}

inline void ResetScroll(HWND hWnd, int *yMaxScroll)
{    
    SetScrollRange (hWnd, SB_VERT, 0, *yMaxScroll, FALSE);
    yCurrentScroll = min (yCurrentScroll, *yMaxScroll);
    SetScrollPos (hWnd, SB_VERT, yCurrentScroll, TRUE);
}

inline VOID CALLBACK timer_proc (HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{    
    RECT ZoomRect;
    ZoomRect = config.screenrect;
    InflateRect(&ZoomRect, (ZoomRect.right - ZoomRect.left) * dwZoom, 
                    (ZoomRect.bottom - ZoomRect.top) * dwZoom);
    InvalidateRect (hwnd, &ZoomRect, FALSE);
    //InvalidateRect (hwnd, &ZoomRect, TRUE);
	VFBRecvEvent (hwnd);

}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;
    TCHAR mode[16];				

    if(strlen(lpCmdLine) != 0) 
    {
        p_pid = atoi(lpCmdLine);
        strncpy(szTitle, strchr(lpCmdLine,' ')+1, 
                strrchr(lpCmdLine,' ')-strchr(lpCmdLine,' '));
        strcpy(mode, strrchr(lpCmdLine, ' ') + 1);
        width = atoi(mode);
        height = atoi(strchr(mode, 'x')+1);
        depth = atoi (strrchr (mode, '-') + 1);
    }

    if (!MyRegisterClass(hInstance))
    {
        return FALSE;
    }

    // Perform application initialization:
    if (!InitInstance (hInstance, width, height, nCmdShow)) 
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_WVFB);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX); 

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= (WNDPROC)WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_WVFB);
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName	= 0;//(LPCSTR)IDC_WVFB;
    wcex.lpszClassName	= szWindowClass;
    wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int width, int height, int nCmdShow)
{
    HWND hWnd;

    memset(&config, 0, sizeof(WVFBCONFIGINFO));

	width += GetSystemMetrics(SM_CXFRAME)<<1;
	height += GetSystemMetrics(SM_CYCAPTION) + (GetSystemMetrics(SM_CYFRAME)<<1);

    hInst = hInstance; // Store instance handle in our global variable
    hWnd = CreateWindow(szWindowClass, szTitle, 
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX 
            | WS_VSCROLL | WS_CAPTION,
            CW_USEDEFAULT, CW_USEDEFAULT, 
            width, height, NULL, NULL, hInstance, NULL);

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC   hdc;
    POINT pt;
    DWORD buttons;
    int   nVirKey;
    int   keystate = 0;
	unsigned int   keycode;
    long  nClickValue;
    RECT  rcTemp;
    static int yMinScroll;
    static int yMaxScroll;
    RECT scrc = config.screenrect;
    HMENU hmenu;
	//handling input method context
	static bool in_ime_composition = false;
    static bool in_ime_endcomposition = false;

    switch (message) 
    {
        case WM_CREATE:
            if (VFBInit (hWnd, p_pid, width, height, depth) < 0)
                exit (-1);
            SetRect(&config.screenrect, 0, 0, width, height);
            config.depth = depth;
            InitConfigfile(&config, CONFIGFILE);   
            
            yMaxScroll = max (bmBackGround.bmHeight, 0);
            ResetScroll(hWnd, &yMaxScroll);
            hmenu = GetMenu(hWnd);
            ModifyMenu(hmenu, IDM_ZOOM1, MF_BYCOMMAND | MF_CHECKED, 
                    IDM_ZOOM1, "Zoom scale &1\tAlt+1");
            ModifyMenu(hmenu, IDM_ZOOM2, MF_BYCOMMAND | MF_UNCHECKED, 
                    IDM_ZOOM2, "Zoom scale &2\tAlt+2");			
            SetTimer (hWnd, ID_TIMER, 1000 / dwFreshRate, timer_proc);
            return TRUE;
        case WM_MOUSEMOVE:
            buttons = wParam;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam) + yCurrentScroll;
            pt.x /= dwZoom;
            pt.y /= dwZoom;
            if (nCurConfigIndex == -1) 
            {
                VFBSendMouseData (&pt, buttons);
				return TRUE;
			}

            if (PtInRect (&scrc, pt))
            {
                pt.x = pt.x - scrc.left;
                pt.y = pt.y - scrc.top;
                VFBSendMouseData(&pt, buttons);
            }
            //InvalidateRect (hWnd, &scrc, FALSE);
            return TRUE;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            buttons = wParam;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam) + yCurrentScroll;
            pt.x /= dwZoom;
            pt.y /= dwZoom;

            if (nCurConfigIndex == -1) 
            {
                VFBSendMouseData(&pt, buttons);
                //InvalidateRect (hWnd, &scrc, FALSE);
				return TRUE;
            }

            if (PtInRect (&scrc, pt))
            {
                pt.x = pt.x - scrc.left;
                pt.y = pt.y - scrc.top;
                VFBSendMouseData (&pt, buttons);
            }
            else 
            {
                nClickValue = GetClickKeyValue(&config, pt);
                if (nClickValue == 0)
					return TRUE;
			    if ((message ==WM_LBUTTONDOWN) || (message == WM_RBUTTONDOWN)) 
                {
                    VFBSendKbData (nClickValue, true, 0);
                    VFBSendKbData (nClickValue, false, 0);
                }			
            }
            //InvalidateRect (hWnd, &scrc, FALSE);
            return TRUE;
		case WM_IME_STARTCOMPOSITION:
			in_ime_composition = true;
            break;
		case WM_IME_ENDCOMPOSITION:
			in_ime_composition = false;
            in_ime_endcomposition = true;
            break;
		case WM_IME_COMPOSITION:
			if (!(lParam & GCS_RESULTSTR))
				break;
			{
				int IMStrLen;
				unsigned char IMStr[1024];
				unsigned char IMStr_utf8[1024];
				HIMC hIMC = ImmGetContext (hWnd);
				IMStrLen = ImmGetCompositionString (hIMC, GCS_RESULTSTR, IMStr, 1024);
				ImmReleaseContext (hWnd, hIMC);
				IMStr[IMStrLen] = 0;
				GB2312ToUTF_8 (IMStr, IMStr_utf8, &IMStrLen);
				VFBSendIMData ((char *)IMStr_utf8);
			}
            break;
		case WM_IME_CHAR:
			return TRUE;
		case WM_CHAR:
			return TRUE;
        case WM_KEYDOWN:
        case WM_KEYUP:
			/*break if IME is in use*/
			if (in_ime_composition || wParam == VK_PROCESSKEY)
				return TRUE;
            if (in_ime_endcomposition) {
                in_ime_endcomposition = false;
                return TRUE;
            }

            nVirKey = (int)wParam;
            //FIXME
			if (GetKeyState(VK_NUMLOCK) & 0x01){
                if (nVirKey == VK_DELETE)
                    keycode = SCANCODE_REMOVE;
                else
                    keycode = (lParam >> 16) & 0x00FF;
			} else {
				if (nVirKey == VK_DOWN)
					keycode = SCANCODE_CURSORBLOCKDOWN;
				else if (nVirKey == VK_UP)
					keycode = SCANCODE_CURSORBLOCKUP;
				else if (nVirKey == VK_LEFT)
					keycode = SCANCODE_CURSORBLOCKLEFT;
				else if (nVirKey == VK_RIGHT)
					keycode = SCANCODE_CURSORBLOCKRIGHT;
				else if(nVirKey == VK_PRIOR)
					keycode = SCANCODE_PAGEUP;
				else if(nVirKey == VK_NEXT)
					keycode = SCANCODE_PAGEDOWN;
                else if (nVirKey == VK_DELETE)
                    keycode = SCANCODE_REMOVE;
				else
					keycode = (lParam >> 16) & 0x00FF;
			}

            VFBSendKbData(keycode, message == WM_KEYDOWN ? true : false, 0);//lParam & 0xFFFF);
            //InvalidateRect (hWnd, &scrc, FALSE);
            return TRUE;
        case WM_VSCROLL:
            {
                int yDelta;
                int yNewPos;
                switch (LOWORD(wParam)) 
                {
                    case SB_PAGEUP:
                        yNewPos = yCurrentScroll - 50;
                        break;
                    case SB_PAGEDOWN:
                        yNewPos = yCurrentScroll + 50;
                        break;
                    case SB_LINEUP:
                        yNewPos = yCurrentScroll - 5;
                        break;
                    case SB_LINEDOWN:
                        yNewPos = yCurrentScroll + 5;
                        break;
                    case SB_THUMBPOSITION:
                        yNewPos = HIWORD(wParam);
                        break;
                    default:
                        yNewPos = yCurrentScroll;
                }

                yNewPos = max (0, yNewPos);
                yNewPos = min (yMaxScroll, yNewPos);

                if (yNewPos == yCurrentScroll)
                    break;

                yDelta = yNewPos - yCurrentScroll;
                yCurrentScroll = yNewPos;
                ScrollWindow (hWnd, 0, -yDelta, NULL, NULL);
                UpdateWindow (hWnd);
                SetScrollPos (hWnd, SB_VERT, yCurrentScroll, TRUE);
            }
            return TRUE;		
        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
            switch (wmId)
            {
                case IDM_ABOUT:
                    DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                case IDC_RESETWVFB:	                    
                    KillTimer (hWnd, ID_TIMER);
                    VFBClose();
                    if (VFBInit(hWnd, p_pid, scrc.right - scrc.left, scrc.bottom - scrc.top, config.depth) < 0)
                    {
                        MessageBox (hWnd, "wvfb init error", "information", MB_OK);
                        exit(1);
                    }						
                    SetTimer(hWnd, ID_TIMER, 1000 / dwFreshRate, timer_proc);
                    InvalidateRect (hWnd, NULL, TRUE);
                    break;
                case IDM_CONFIGURE:
                    if (DialogBox (hInst, (LPCTSTR)IDD_CONFIG, hWnd, (DLGPROC)selectConfigProc) == IDOK) 
                    {
                        ChangeConfigUpdate(&config, hWnd, &yMaxScroll);
                    }
                    break;
                case IDM_ZOOM1:
                    hmenu = GetMenu(hWnd);
                    dwZoom = 1;
                    ModifyMenu(hmenu, IDM_ZOOM1, MF_BYCOMMAND | MF_CHECKED, 
                            IDM_ZOOM1, "Zoom scale &1\tAlt+1");
                    ModifyMenu(hmenu, IDM_ZOOM2, MF_BYCOMMAND | MF_UNCHECKED, 
                            IDM_ZOOM2, "Zoom scale &2\tAlt+2");
                    InvalidateRect (hWnd, NULL, TRUE);
                    break;
                case IDM_ZOOM2:
                    if(!hbmBackGround)
                    {
                        hmenu = GetMenu(hWnd);
                        dwZoom = 2;
                        ModifyMenu(hmenu, IDM_ZOOM2, MF_BYCOMMAND | MF_CHECKED, 
                                IDM_ZOOM2, "Zoom &scale &2\tAlt+2");
                        ModifyMenu(hmenu, IDM_ZOOM1, MF_BYCOMMAND | MF_UNCHECKED, 
                                IDM_ZOOM1, "&Zoom scale &1\tAlt+1");					
                        InvalidateRect (hWnd, NULL, TRUE);
                    }
                    break;
                case IDM_LOADCONFIG:
                    {
                        OPENFILENAME ofn; 
                        char szFile[MAX_PATH] = "\0";
                        static CHAR szFilter[] = "Configure(*.ini)\0*.ini\0" ;

                        ZeroMemory(&ofn, sizeof(OPENFILENAME));                       
                        ofn.lStructSize = sizeof(OPENFILENAME); 
                        ofn.hwndOwner = hWnd;                        
                        ofn.lpstrFile = szFile; 
                        ofn.nMaxFile = MAX_PATH;
                        ofn.lpstrFilter = szFilter;                     
                        ofn.nFilterIndex = sizeof(szFilter); 
                        ofn.lpstrFileTitle =NULL;
                        ofn.nMaxFileTitle = 0;
                        ofn.lpstrInitialDir = NULL;                      
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;                      

                        if(GetOpenFileName(&ofn) ==  TRUE)
                        {   
                            int ret; 
                            DeleteObject (hbmBackGround);
                            hbmBackGround = NULL;
                            memset(&bmBackGround, 0, sizeof(BITMAP));
                            ret = LoadIniConfigInfo(&config, ofn.lpstrFile);
                            nCurConfigIndex = 0;
                            if ( ret < 0) 
                            {
                                MessageBox (NULL, "Read Config information error!", "information", MB_OK);
                                if(ret <= -1 && ret >= -3) 
                                {
                                    FreeConfigInfo(&config);
                                    SetRect(&config.screenrect, 0, 0, DEFAULTWIDTH, DEFAULTHEIGHT);
                                    config.configfile[0] = 0;
                                    config.configname[0] = 0;
                                    nCurConfigIndex = -1;
                                    config.depth = 16;
                                    DeleteObject(hbmBackGround);
                                    hbmBackGround = NULL;
                                    memset(&bmBackGround, 0, sizeof(BITMAP));   
                                }
                            }                            

                            if(config.bmpname)
                            {
                                TCHAR *p;
                                TCHAR bmppath[MAX_PATH];
                                strncpy(bmppath, ofn.lpstrFile, MAX_PATH);                                

                                p = _tcsrchr(bmppath,_T('\\')) ;
                                if(p) lstrcpy(p + 1, _T(config.bmpname));
                                hbmBackGround = (HBITMAP)LoadImage(NULL, bmppath, IMAGE_BITMAP ,0,0,LR_LOADFROMFILE);
                                if (hbmBackGround)
                                {
                                    GetObject (hbmBackGround, sizeof(BITMAP), (void *)&bmBackGround);            
                                }
                                else
                                {
                                    MessageBox (NULL, "Load bitmap error",  "information", MB_OK);
                                    memset(&bmBackGround, 0, sizeof(BITMAP));
                                }
                            }
                            ChangeConfigUpdate(&config, hWnd, &yMaxScroll);                    
                        }                    
                    }
                    break;
                case IDM_SAVEAS:
                    if(szSaveFilePath[0] == 0)
                    {
                        BROWSEINFO bi;
                        ITEMIDLIST *pidl;
                        char Dir[256];
                        bi.hwndOwner = hWnd;
                        bi.pidlRoot = NULL;
                        bi.pszDisplayName = (LPSTR)Dir;
                        bi.lpszTitle = "Please select a directory for save picture";
                        bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_DONTGOBELOWDOMAIN|BIF_EDITBOX;
                        bi.lpfn = NULL;
                        bi.lParam = 0;
                        bi.iImage = 0;                        
                        pidl = SHBrowseForFolder(&bi);                
                        if ( pidl == NULL )
                            Dir[0] = 0;
                        if (!SHGetPathFromIDList( pidl, Dir ))
                            Dir[0] = 0; 
                        if(Dir[0] != 0)
                        {
                            strncpy(szSaveFilePath, Dir, 256);
                        }
                    }
                    VFBSaveAsPicture(szSaveFilePath);
                    break;
                case IDM_REFREASHRATE:
                    if (DialogBox (hInst, (LPCTSTR)IDD_REFRATE, hWnd, (DLGPROC)RefreshProc) == IDOK) 
                    {	
                        KillTimer(hWnd, ID_TIMER);                        
                        SetTimer(hWnd, ID_TIMER, 1000 / dwFreshRate, timer_proc);
                    }					
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            return TRUE;
        case WM_ERASEBKGND:	
            {           
                hdc = (HDC)wParam;
                GetClientRect (hWnd, &rcTemp);
                if(bmBackGround.bmWidth != 0)
                {
                    HDC hdcMem;
                    if(rcTemp.right > bmBackGround.bmWidth)
                    {
                        rcTemp.left = bmBackGround.bmWidth;
                        rcTemp.left = bmBackGround.bmWidth;
                        FillRect (hdc, &rcTemp, (HBRUSH)GetStockObject(WHITE_BRUSH));                  
                    }

                    if(rcTemp.bottom > bmBackGround.bmHeight)
                    {
                        rcTemp.left = 0;
                        rcTemp.right = bmBackGround.bmWidth;
                        rcTemp.top = bmBackGround.bmHeight - yCurrentScroll;
                        FillRect (hdc, &rcTemp, (HBRUSH)GetStockObject(WHITE_BRUSH));
                    }
                    hdcMem = CreateCompatibleDC(hdc);
                    SelectObject(hdcMem, hbmBackGround);
                    BitBlt(hdc, 0,  -yCurrentScroll, 
						bmBackGround.bmWidth, bmBackGround.bmHeight, hdcMem, 0, 0, SRCCOPY);
                    DeleteDC (hdcMem); 
                    return 1;
                }				

                if(rcTemp.right > config.screenrect.right)
                {
                    rcTemp.left = config.screenrect.right * dwZoom;
                    FillRect (hdc, &rcTemp, (HBRUSH)GetStockObject(WHITE_BRUSH));
                }

                if(rcTemp.bottom >  config.screenrect.bottom)
                {
                    rcTemp.left = 0;
                    rcTemp.right = config.screenrect.right* dwZoom;
                    rcTemp.top = config.screenrect.bottom * dwZoom;
                    FillRect (hdc, &rcTemp, (HBRUSH)GetStockObject(WHITE_BRUSH));
                }
				return TRUE;
            }	
        case WM_PAINT:
            {
                hdc = BeginPaint(hWnd, &ps); 
                GetClientRect(hWnd, &rcTemp);
                VFBDrawScreen(hdc, config.screenrect, rcTemp, yCurrentScroll, dwZoom);     
                EndPaint(hWnd, &ps);
            }
            return TRUE;
        case WM_SIZE:
            {   
                int yNewSize = HIWORD(lParam); 
                yMaxScroll = max (bmBackGround.bmHeight - yNewSize, 0);
                ResetScroll(hWnd, &yMaxScroll);		
            }
            return TRUE;
        case WM_DESTROY:
            KillTimer (hWnd, ID_TIMER);
            VFBClose();
            FreeConfigInfo (&config);
            DeleteObject (hbmBackGround);
            hbmBackGround = NULL;
            memset(&bmBackGround, 0, sizeof(BITMAP));
            PostQuitMessage(0);
            return TRUE;
        default:
			break;
    }

    //return 0;
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}

void EnableGroupControl(HWND hDlg, BOOL interselected)
{
    if(interselected)
    {
        EnableWindow(GetDlgItem (hDlg, IDC_CM_CONFIG), TRUE);        
        EnableWindow(GetDlgItem (hDlg, IDC_CM_DEPTH), FALSE);	
        EnableWindow(GetDlgItem (hDlg, IDC_ED_MGRIGHT), FALSE);
        EnableWindow(GetDlgItem (hDlg, IDC_ED_MGBOTTOM), FALSE);                
    }
    else
    {	
        EnableWindow(GetDlgItem (hDlg, IDC_CM_CONFIG), FALSE);
        EnableWindow(GetDlgItem (hDlg, IDC_CM_DEPTH), TRUE);	
        EnableWindow(GetDlgItem (hDlg, IDC_ED_MGRIGHT), TRUE);	
        EnableWindow(GetDlgItem (hDlg, IDC_ED_MGBOTTOM), TRUE); 
    }
}

LRESULT CALLBACK selectConfigProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int nCount;
    int i;
    char chConfigName[256];
    char strTmp[255];
    char chTemp[20];
    int nTemp;
    int depth;
    char strRect[2][5];
    RECT rc, rcDlg, rcOwner;
    HWND hwndOwner;
    TCHAR path[MAX_PATH];
    TCHAR * p;
    HDC hdc;
    PAINTSTRUCT ps;

    switch (message)
    {
        case WM_INITDIALOG:             
            hwndOwner = GetParent(hDlg);
            GetWindowRect(hwndOwner, &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            CopyRect(&rc, &rcOwner);
            OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
            OffsetRect(&rc, -rc.left, -rc.top);
            OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
            if(rc.right < 0)
                rc.right = 0;
            if(rc.bottom < 0)
                rc.bottom = 0;
            SetWindowPos(hDlg, HWND_TOP, rcOwner.left + rc.right / 2,
                    rcOwner.top + rc.bottom / 2, 0, 0, SWP_NOSIZE);
            sprintf(chTemp, "%s", "1bit color");
            SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_ADDSTRING, 0, (LPARAM)chTemp);
            sprintf(chTemp, "%s", "4bit color");
            SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_ADDSTRING, 0, (LPARAM)chTemp);	
            sprintf(chTemp, "%s", "8bit color");
            SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_ADDSTRING, 0, (LPARAM)chTemp);			
            sprintf(chTemp, "%s", "16bit color");
            SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_ADDSTRING, 0, (LPARAM)chTemp);			
            sprintf(chTemp, "%s", "24bit color");
            SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_ADDSTRING, 0, (LPARAM)chTemp);
            sprintf(chTemp, "%s", "32bit color");
            SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_ADDSTRING, 0, (LPARAM)chTemp);
            SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_SETCURSEL, 3, 0);	

            SendDlgItemMessage(hDlg, IDC_ED_MGRIGHT, EM_SETLIMITTEXT, 4, 0);
            SendDlgItemMessage(hDlg, IDC_ED_MGBOTTOM, EM_SETLIMITTEXT, 3, 0);			
            GetModuleFileName(NULL, path, sizeof(path)) ;
            p = _tcsrchr(path,_T('\\')) ;
            if(p) lstrcpy(p + 1, _T(CONFIGFILE));
            InitConfigfile(&config, path);
            nCount = GetConfigCountFromFile (&config);
            if(nCount > 0)
            {
                for (i = 0; i < nCount; i++) 
                {
                    GetConfigName (&config, i + 1, chConfigName);
                    SendMessage (GetDlgItem (hDlg, IDC_INTERNAL_CONFIG), CB_ADDSTRING, 0, (LPARAM)chConfigName);				
                }
            }

            if(nCount == 0 || nCurConfigIndex == -1)
            {
                char tmpstr[5];		
                switch(config.depth) 
                {
                    case 1:
                        depth = 0;
                        break;
                    case 4:
                        depth = 1;
                        break;
                    case 8:
                        depth = 2;
                        break;
                    case 16:
                        depth = 3;
                        break;
                    case 24:
                        depth = 4;
                    case 32:
                        depth = 5;
                        break;
                }
                SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_SETCURSEL, depth, 0);	
                SetWindowText(GetDlgItem(hDlg, IDC_ED_MGRIGHT), itoa(config.screenrect.right, tmpstr, 10));
                SetWindowText(GetDlgItem(hDlg, IDC_ED_MGBOTTOM), itoa(config.screenrect.bottom, tmpstr, 10));				
                EnableGroupControl(hDlg, FALSE);
                SendDlgItemMessage(hDlg, IDC_RD_CUSTCOFIG, BM_SETCHECK, 1, 0);
                SendDlgItemMessage(hDlg, IDC_RD_INTERCONFIG, BM_SETCHECK, 0, 0);                
                return TRUE;
            }

            SendDlgItemMessage(hDlg, IDC_RD_INTERCONFIG, BM_SETCHECK, 1, 0);
            SendDlgItemMessage(hDlg, IDC_INTERNAL_CONFIG, CB_SETCURSEL, nCurConfigIndex - 1, 0);			
            EnableGroupControl(hDlg, TRUE);					
            return TRUE;				
        case WM_PAINT:
            hdc = BeginPaint(hDlg, &ps);
            sprintf(strTmp, "Current default:  W %dpixel  H %dpixel   Color depth  %dbit", 
                    config.screenrect.right - config.screenrect.left, 
                    config.screenrect.bottom - config.screenrect.top,
                    config.depth);
            SetTextColor(hdc, RGB(0, 0, 255));
            SetBkMode(hdc, TRANSPARENT);	//*look on the bottom for more info
            TextOut(hdc, 25, 218, strTmp, strlen(strTmp));			
            EndPaint(hDlg, &ps);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_RD_INTERCONFIG:
                    SendDlgItemMessage(hDlg, IDC_RD_INTERCONFIG, BM_SETCHECK, nCurConfigIndex, 0);					
                    EnableGroupControl(hDlg, TRUE);	
                    return TRUE;
                case IDC_RD_CUSTCOFIG:
                    EnableGroupControl(hDlg, FALSE);	
                    return TRUE;

                case IDC_DEFAULT:
                    {						
                        WVFBCONFIGINFO tempconfig;
                        memcpy(&tempconfig, &config, sizeof(WVFBCONFIGINFO));
                        if(BST_CHECKED == SendDlgItemMessage(hDlg, IDC_RD_INTERCONFIG, BM_GETCHECK, 0, 0))
                        {
                            nTemp = SendMessage (GetDlgItem (hDlg, IDC_INTERNAL_CONFIG), CB_GETCURSEL, 0, 0);
                            if(nTemp == -1)
                            {
                                MessageBox(hDlg, "No selected save!", "error", MB_OK | MB_ICONERROR);
                                return TRUE;
                            }
                            SaveToDefault(&tempconfig, nTemp + 1);
                            return TRUE;
                        }

                        depth = SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_GETCURSEL, 0, 0);
                        tempconfig.depth = deptharray[depth];					
                        GetWindowText(GetDlgItem(hDlg, IDC_ED_MGRIGHT), strRect[0], 5);
                        GetWindowText(GetDlgItem(hDlg, IDC_ED_MGBOTTOM), strRect[1], 5);
                        if(!strRect[0] || !strRect[1] || atoi(strRect[0]) == 0 || 
                                atoi(strRect[1]) == 0  || atoi(strRect[0]) > 1024 || 
                                atoi(strRect[1]) > 768)
                        {
                            MessageBox(hDlg,"MiniGUI Resolution error", "Error", MB_OK | MB_ICONERROR);
                            return TRUE;							
                        }
                        SetRect(&tempconfig.screenrect, 0, 0, atoi(strRect[0]), atoi(strRect[1]));
                        SaveToDefault(&tempconfig, 0);
                    }
                    return TRUE;
                case IDOK:

                    FreeConfigInfo (&config);
                    DeleteObject (hbmBackGround);
                    hbmBackGround = NULL;
                    memset(&bmBackGround, 0, sizeof(BITMAP));                     
                    if(BST_CHECKED == SendDlgItemMessage(hDlg, IDC_RD_INTERCONFIG, BM_GETCHECK, 0, 0))                       
                    {
                        nTemp = SendMessage (GetDlgItem (hDlg, IDC_INTERNAL_CONFIG), CB_GETCURSEL, 0, 0);						              
                        LoadConfigInfo(nTemp + 1);	
                        EndDialog(hDlg, LOWORD(wParam));
                        return TRUE;
                    }

                    depth = SendDlgItemMessage(hDlg, IDC_CM_DEPTH, CB_GETCURSEL, 0, 0);
                    config.depth = deptharray[depth];
                    GetWindowText(GetDlgItem(hDlg, IDC_ED_MGRIGHT), strRect[0], 5);
                    GetWindowText(GetDlgItem(hDlg, IDC_ED_MGBOTTOM), strRect[1], 5);
                    if(!strRect[0] || !strRect[1] || atoi(strRect[0]) == 0 || 
                            atoi(strRect[1]) == 0  
                            || atoi(strRect[0]) > 1024 || atoi(strRect[1]) > 768)
                    {
                        MessageBox(hDlg,"MiniGUI Resolution error", "Error", MB_OK | MB_ICONERROR);
                        return TRUE;							
                    }
                    SetRect(&config.screenrect, 0, 0, atoi(strRect[0]), atoi(strRect[1]));
                    nCurConfigIndex = -1;
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

LRESULT CALLBACK RefreshProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD position;
    char strTmp[15];
    HWND hwndOwner;
    RECT rcOwner, rcDlg, rc;

    switch (message)
    {
        case WM_INITDIALOG: 
            hwndOwner = GetParent(hDlg);
            GetWindowRect(hwndOwner, &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            CopyRect(&rc, &rcOwner);
            OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
            OffsetRect(&rc, -rc.left, -rc.top);
            OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
            if(rc.right < 0)
                rc.right = 0;
            if(rc.bottom < 0)
                rc.bottom = 0;
            SetWindowPos(hDlg, HWND_TOP, rcOwner.left + rc.right / 2,
                    rcOwner.top + rc.bottom / 2, 0, 0, SWP_NOSIZE);
            SendDlgItemMessage(hDlg, ID_TRACKBAR, TBM_SETRANGE, 
                    (WPARAM) TRUE,                   // redraw flag 
                    (LPARAM) MAKELONG(1, 10));  // min. & max. positions 
            SendDlgItemMessage(hDlg, ID_TRACKBAR, TBM_SETPAGESIZE, 
                    0, (LPARAM) 1);                  // new page size 

            SendDlgItemMessage(hDlg, ID_TRACKBAR, TBM_SETSEL, 
                    (WPARAM) FALSE,                  // redraw flag 
                    (LPARAM) MAKELONG(0, dwFreshRate / 10)); 
            SendDlgItemMessage(hDlg, ID_TRACKBAR, TBM_SETPOS, 
                    (WPARAM) TRUE,                   // redraw flag 
                    (LPARAM) dwFreshRate / 10 + 1); 
            sprintf(strTmp, "%d fps", dwFreshRate);
            SetWindowText(GetDlgItem(hDlg, IDC_ST_FPS), strTmp);
            return TRUE;
        case WM_HSCROLL:				
            position = SendDlgItemMessage(hDlg, ID_TRACKBAR, TBM_GETPOS, 0, 0); 
            sprintf(strTmp, "%d fps", position * 10);
            SetWindowText(GetDlgItem(hDlg, IDC_ST_FPS), strTmp);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDOK:
                    dwFreshRate = SendDlgItemMessage(hDlg, ID_TRACKBAR, TBM_GETPOS, 
                            0, 0); 
                    dwFreshRate ++;
                    dwFreshRate *= 10;
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
            }
        default:
            break;
    }
    return FALSE;		
}



static void LoadConfigInfo (int typeindex)
{
    int ret;
    nCurConfigIndex = typeindex;     

    ret = GetKeyInfoFromFile (&config, typeindex);
    if ( ret < 0) 
    {
        MessageBox (NULL, "Read Config information error!", "information", MB_OK);
        if(ret <= -1 && ret >= -3) 
        {
            FreeConfigInfo(&config);
            SetRect(&config.screenrect, 0, 0, width, height);
            config.configfile[0] = 0;
            config.configname[0] = 0;
            nCurConfigIndex = -1;
            config.depth = depth;
            DeleteObject(hbmBackGround);
            hbmBackGround = NULL;
            memset(&bmBackGround, 0, sizeof(BITMAP));   
        }
    }


    if(config.bmpname)
    {
        TCHAR *p;
        TCHAR bmppath[MAX_PATH];
        GetModuleFileName(NULL, bmppath, sizeof(bmppath)) ;
        p = _tcsrchr(bmppath,_T('\\')) ;
        if(p) lstrcpy(p + 1, _T(config.bmpname));
        hbmBackGround = (HBITMAP)LoadImage(NULL, bmppath, IMAGE_BITMAP ,0,0,LR_LOADFROMFILE);
        if (hbmBackGround)
        {
            GetObject (hbmBackGround, sizeof(BITMAP), (void *)&bmBackGround);            
        }
        else
        {
            MessageBox (NULL, "Load bitmap error",  "information", MB_OK);
            memset(&bmBackGround, 0, sizeof(BITMAP));
        }
    }
    return;    
}

static void ChangeConfigUpdate(WVFBCONFIGINFO *pcfg, HWND hWnd, int *yMaxScroll)
{
    int yNewSize;						
    int newwidth,newheight;
    POINT point;
    RECT rcTemp, scrc;

    KillTimer (hWnd, ID_TIMER);
    VFBClose();
    GetClientRect(hWnd, &rcTemp);

    point.x = rcTemp.left;
    point.y = rcTemp.top;
    ClientToScreen(hWnd, &point);
    rcTemp.left = point.x;
    rcTemp.top = point.y;
    scrc = config.screenrect;
    if (VFBInit(hWnd, p_pid, scrc.right - scrc.left, scrc.bottom - scrc.top, config.depth) < 0)
    {
        MessageBox (hWnd, "Wvfb init error", "information", MB_OK);
        exit(1);
    }

    InvalidateRect (hWnd, NULL, TRUE);
    if(nCurConfigIndex < 0 || !config.bmpname || (config.bmpname && config.bmpname[0] == 0))
    {
        newwidth = scrc.right + 10;
        newheight = scrc.bottom + GetSystemMetrics(SM_CXSIZE) + GetSystemMetrics(SM_CYMENU) + 10;
        if (newwidth < MINIWIDTH) 
        {
            newwidth = MINIWIDTH;					
        }
        if(newheight < MINIHEIGHT)
        {
            newheight = MINIHEIGHT;
        }		
        MoveWindow(hWnd, rcTemp.left, rcTemp.top, newwidth, newheight, TRUE);	
    }
    else
    {
        MoveWindow(hWnd, rcTemp.left, rcTemp.top, bmBackGround.bmWidth, bmBackGround.bmHeight, TRUE);

    }					
    GetClientRect (hWnd, &rcTemp);
    yNewSize = rcTemp.bottom; 
    *yMaxScroll = max (bmBackGround.bmHeight - yNewSize, 0);
    ResetScroll(hWnd, yMaxScroll);
    SetTimer(hWnd, ID_TIMER, 1000 / dwFreshRate, timer_proc);
}
