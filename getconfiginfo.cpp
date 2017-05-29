#include "stdafx.h"
#include "resource.h"
#include "stdio.h"

#include "getconfiginfo.h"


static const KEYMAP keytable[]={
	"SCANCODE_ESCAPE",                 1,
	"SCANCODE_1",                      2,
	"SCANCODE_2",                      3,
	"SCANCODE_3",                      4,
	"SCANCODE_4",                      5,
	"SCANCODE_5",                      6,
	"SCANCODE_6",                      7,
	"SCANCODE_7",                      8,
	"SCANCODE_8",                      9,
	"SCANCODE_9",                      10,
	"SCANCODE_0",                      11,
	"SCANCODE_MINUS",                  12,
	"SCANCODE_EQUAL",                  13,
	"SCANCODE_BACKSPACE",              14,
	"SCANCODE_TAB",                    15,
	"SCANCODE_Q",                      16,
	"SCANCODE_W",                      17,
	"SCANCODE_E",                      18,
	"SCANCODE_R",                      19,
	"SCANCODE_T",                      20,
	"SCANCODE_Y",                      21,
	"SCANCODE_U",                      22,
	"SCANCODE_I",                      23,
	"SCANCODE_O",                      24,
	"SCANCODE_P",                      25,
	"SCANCODE_BRACKET_LEFT",           26,
	"SCANCODE_BRACKET_RIGHT",          27,
	"SCANCODE_ENTER",                  28,
	"SCANCODE_LEFTCONTROL",            29,
	"SCANCODE_A",                      30,
	"SCANCODE_S",                      31,
	"SCANCODE_D",                      32,
	"SCANCODE_F",                      33,
	"SCANCODE_G",                      34,
	"SCANCODE_H",                      35,
	"SCANCODE_J",                      36,
	"SCANCODE_K",                      37,
	"SCANCODE_L",                      38,
	"SCANCODE_SEMICOLON",              39,
	"SCANCODE_APOSTROPHE",             40,
	"SCANCODE_GRAVE",                  41,
	"SCANCODE_LEFTSHIFT",              42,
	"SCANCODE_BACKSLASH",              43,

	"SCANCODE_Z",                      44,
	"SCANCODE_X",                      45,
	"SCANCODE_C",                      46,
	"SCANCODE_V",                      47,
	"SCANCODE_B",                      48,
	"SCANCODE_N",                      49,
	"SCANCODE_M",                      50,
	"SCANCODE_COMMA",                  51,
	"SCANCODE_PERIOD",                 52,
	"SCANCODE_SLASH",                  53,
	"SCANCODE_RIGHTSHIFT",             54,
	"SCANCODE_KEYPADMULTIPLY",         55,
	"SCANCODE_LEFTALT",                56,
	"SCANCODE_SPACE",                  57,
	"SCANCODE_CAPSLOCK",               58,
	"SCANCODE_F1",                     59,
	"SCANCODE_F2",                     60,
	"SCANCODE_F3",                     61,
	"SCANCODE_F4",                     62,
	"SCANCODE_F5",                     63,
	"SCANCODE_F6",                     64,
	"SCANCODE_F7",                     65,
	"SCANCODE_F8",                     66,
	"SCANCODE_F9",                     67,
	"SCANCODE_F10",                    68,
	"SCANCODE_NUMLOCK",                69,
	"SCANCODE_SCROLLLOCK",             70,
	"SCANCODE_KEYPAD7",                71,
	"SCANCODE_CURSORUPLEFT",           71,
	"SCANCODE_KEYPAD8",                72,
	"SCANCODE_CURSORUP",               72,
	"SCANCODE_KEYPAD9",                73,
	"SCANCODE_CURSORUPRIGHT",          73,
	"SCANCODE_KEYPADMINUS",            74,
	"SCANCODE_KEYPAD4",                75,
	"SCANCODE_CURSORLEFT",             75,
	"SCANCODE_KEYPAD5",                76,
	"SCANCODE_KEYPAD6",                77,
	"SCANCODE_CURSORRIGHT",            77,
	"SCANCODE_KEYPADPLUS",             78,
	"SCANCODE_KEYPAD1",                79,
	"SCANCODE_CURSORDOWNLEFT",         79,
	"SCANCODE_KEYPAD2",                80,
	"SCANCODE_CURSORDOWN",             80,
	"SCANCODE_KEYPAD3",                81,
	"SCANCODE_CURSORDOWNRIGHT",        81,
	"SCANCODE_KEYPAD0",                82,
	"SCANCODE_KEYPADPERIOD",           83,

	"SCANCODE_LESS",                   86,

	"SCANCODE_F11",                    87,
	"SCANCODE_F12",                    88,

	"SCANCODE_KEYPADENTER",            96,
	"SCANCODE_RIGHTCONTROL",           97,
	"SCANCODE_CONTROL",                97,
	"SCANCODE_KEYPADDIVIDE",           98,
	"SCANCODE_PRINTSCREEN",            99,
	"SCANCODE_RIGHTALT",               100,
	"SCANCODE_BREAK",                  101,    /* Beware: is 119     */
	"SCANCODE_BREAK_ALTERNATIVE",      119,    /* on some keyboards! */

	"SCANCODE_HOME",                   102,
	"SCANCODE_CURSORBLOCKUP",          103,    /* Cursor key block */
	"SCANCODE_PAGEUP",                 104,
	"SCANCODE_CURSORBLOCKLEFT",        105,    /* Cursor key block */
	"SCANCODE_CURSORBLOCKRIGHT",       106,    /* Cursor key block */
	"SCANCODE_END",                    107,
	"SCANCODE_CURSORBLOCKDOWN",        108,    /* Cursor key block */
	"SCANCODE_PAGEDOWN",               109,
	"SCANCODE_INSERT",                 110,
	"SCANCODE_REMOVE",                 111,

	"SCANCODE_PAUSE",                  119,

	"SCANCODE_POWER",                  120,
	"SCANCODE_SLEEP",                  121,
	"SCANCODE_WAKEUP",                 122,

	"SCANCODE_LEFTWIN",                125,
	"SCANCODE_RIGHTWIN",               126,
	"SCANCODE_MENU",                   127
};


static void GetRectFromStr (RECT *rcKey, const char *keystr)
{
    sscanf (keystr, "%d %d %d %d", &rcKey->left, &rcKey->top, &rcKey->right, &rcKey->bottom);
    return;
}


static void GetKeyScanValue (int *keyvalue, const char *keystr)
{
    char chTemp[256];
    int  nTemp;
    int  tablecount = sizeof(keytable) /sizeof(KEYMAP);
    int  i;
    
    sscanf (keystr, "%d %d %d %d %s", &nTemp, &nTemp, &nTemp, &nTemp, chTemp);
    
    for (i = 0; i < tablecount; i++) 
    {
        if (strcmp (chTemp, keytable [i].keyname) == 0) 
        {
            *keyvalue = keytable [i].keyvalue;
            return;
        }
    }
    
    *keyvalue = 0;
    return;
}

static int GetConfigInfo(WVFBCONFIGINFO *pinfo)
{
    int i;
    char chTemp [256];
    char chKeyStr [256];
    if(pinfo->bmpname == NULL)
    {    	
        pinfo->bmpname = (char *)calloc(1, 256);    	    
    }    
    GetPrivateProfileString (pinfo->configname, "bitmap", "", pinfo->bmpname, 255, pinfo->configfile);    
    if(strcmp(pinfo->bmpname, "") == 0)
    {
        free(pinfo->bmpname);
        pinfo->bmpname = NULL;
    }   

    GetPrivateProfileString (pinfo->configname, "screen", "", chTemp, 255, pinfo->configfile);
    if (strlen (chTemp) == 0)
    {
        free(pinfo->bmpname);
        pinfo->bmpname = NULL;
        return -3;
    }
    else 
    {
        GetRectFromStr (&pinfo->screenrect, chTemp);
    }

    pinfo->depth = GetPrivateProfileInt (pinfo->configname, "depth", 16, pinfo->configfile);
    

    pinfo->keycount = GetPrivateProfileInt (pinfo->configname, "keycount", 0, pinfo->configfile);
    
    if (pinfo->keycount <= 0)
        return 0;
    
    /*alloc memory*/
    pinfo->keyvalue = (int *)calloc (pinfo->keycount, sizeof (int));
    if (pinfo->keyvalue == NULL)
        return -4;
    
    pinfo->pkeyrect = (RECT *)calloc (pinfo->keycount, sizeof (RECT));
    if (pinfo->pkeyrect == NULL) 
    {
        free (pinfo->keyvalue);
        pinfo->keyvalue = NULL;
        pinfo->keycount = 0;
        return -5;
    } 

    for (i = 0; i < pinfo->keycount; i++) 
    {
        wsprintf (chTemp, "KEY%d", i + 1);
        GetPrivateProfileString (pinfo->configname, chTemp, "", chKeyStr, 255, pinfo->configfile);
        GetKeyScanValue (&pinfo->keyvalue [i], chKeyStr);
        GetRectFromStr (&pinfo->pkeyrect [i], chKeyStr);
    }	
    return 0; 
}

int InitConfigfile(WVFBCONFIGINFO *pinfo, const char *cfgpathname)
{
	int nSelect;
    if(pinfo == NULL)
        return -1;
        
    if(cfgpathname)
    {
        strncpy(pinfo->configfile, cfgpathname, 255);
		nSelect = GetPrivateProfileInt("profiles", "selected", 0, pinfo->configfile); 
		return nSelect;
    }
	memset(pinfo->configfile, 0, 256);   
	return -1;
}



int LoadIniConfigInfo(WVFBCONFIGINFO *pinfo, const char *cfgpathname)
{    
    if(!pinfo)
        return FALSE;    
    FreeConfigInfo (pinfo);
    InitConfigfile(pinfo, cfgpathname);
    strncpy(pinfo->configname, "configure", 30);
    return GetConfigInfo(pinfo);
}


int GetClickKeyValue (WVFBCONFIGINFO *pinfo, POINT pt)
{
	int i;
	if(pinfo == NULL || pinfo->keycount == 0)
	    return 0;
	    
	for (i = 0; i< pinfo->keycount; i++) 
	{
		if (PtInRect (&pinfo->pkeyrect[i], pt))
			return pinfo->keyvalue [i];
	}
	return 0;
}


int  GetConfigCountFromFile (WVFBCONFIGINFO *pinfo)
{
    int nCount;
    if(pinfo == NULL || strcmp(pinfo->configfile,"") == 0)
	    return 0;
	
    nCount = GetPrivateProfileInt("profiles", "count", 0, pinfo->configfile); 
	return nCount; 
}


void  GetConfigName (WVFBCONFIGINFO *pinfo, int index, char *configname)
{
	char chTemp[256];
	if(pinfo == NULL || strcmp(pinfo->configfile,"") == 0)
	    return;
	/*get mobile name*/
	wsprintf (chTemp, "type%d", index);
	GetPrivateProfileString ("profiles", chTemp, "", configname, 255, pinfo->configfile);
}


int GetKeyInfoFromFile (WVFBCONFIGINFO *pinfo, int typeindex)
{
	int nCount;
	char chTemp [256];

	nCount = GetConfigCountFromFile (pinfo);

	if (nCount <= 0 || typeindex > nCount || typeindex <= 0)
		return -1;


	wsprintf (chTemp, "type%d", typeindex);
	GetPrivateProfileString ("profiles", chTemp, "", pinfo->configname, 20, pinfo->configfile);
	if (strlen (pinfo->configname) == 0)
		return -2;
	return GetConfigInfo(pinfo);
	
}


BOOL SaveToDefault(WVFBCONFIGINFO *pinfo, int index)
{
	int ret;
	int nCount;
	char chTemp [256];

    if(pinfo == NULL || strcmp(pinfo->configfile,"") == 0)
	    return 0;

	nCount = GetConfigCountFromFile (pinfo);
	if(index > 0 && index <= nCount)
	{
		sprintf(chTemp, "%d", index);
		ret = WritePrivateProfileString("profiles", "selected", chTemp, pinfo->configfile);
		if(ret > 0)
			return TRUE;		
		return FALSE;
	}
	ret = WritePrivateProfileString("profiles", "selected", "1", pinfo->configfile);
	ret &= WritePrivateProfileString("default", "bitmap", "", pinfo->configfile);	
	ret &= WritePrivateProfileString("default", "keycount", "", pinfo->configfile);
	
	sprintf(chTemp, "0 0 %d %d", pinfo->screenrect.right, pinfo->screenrect.bottom);
	ret &= WritePrivateProfileString("default", "screen", chTemp, pinfo->configfile);
	
	sprintf(chTemp, "%d", pinfo->depth);
	ret &= WritePrivateProfileString("default", "depth", chTemp, pinfo->configfile);
	if(ret > 0)
		return TRUE;
		
	return FALSE;
}

void FreeConfigInfo (WVFBCONFIGINFO *pinfo)
{

    if (pinfo->pkeyrect != NULL) 
	{
       free (pinfo->pkeyrect);
	   pinfo->pkeyrect = NULL;	     
	}

	if (pinfo->keyvalue != NULL) 
	{
		free (pinfo->keyvalue);
		pinfo->keyvalue = NULL;
	}

	if(pinfo->bmpname)
	{
	    free (pinfo->bmpname);
	    pinfo->bmpname = NULL;
	}	
    pinfo->configname[0] = 0;
    pinfo->depth = 16;
    SetRect(&pinfo->screenrect, 0, 0, 0, 0);
    pinfo->keycount = 0;
}

