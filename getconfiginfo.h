#if !defined(_GETCONFIGINFO_H)
#define _GETCONFIGINFO_H
typedef struct _wvfbinfo_t 
{	
    char configfile[256];
    char configname[20];
	RECT screenrect;
	unsigned char depth;
	int keycount;
	RECT *pkeyrect;
	int *keyvalue;
	char *bmpname;
}WVFBCONFIGINFO;

typedef struct _keymap_t
{
	const char *keyname;
	int   keyvalue;
}KEYMAP;

extern RECT *rcKey;
extern RECT rcMobileDisp;
extern int *keyvalue;

int InitConfigfile(WVFBCONFIGINFO *pinfo, const char *cfgpathname);

BOOL LoadIniConfigInfo(WVFBCONFIGINFO *pinfo, const char *cfgpathname);

int GetClickKeyValue (WVFBCONFIGINFO *pinfo, POINT pt);

int GetKeyInfoFromFile (WVFBCONFIGINFO *pinfo, int typeindex);

void FreeConfigInfo (WVFBCONFIGINFO *pinfo);

int  GetConfigCountFromFile (WVFBCONFIGINFO *pinfo);

void  GetConfigName (WVFBCONFIGINFO *pinfo, int index, char *configname);

BOOL SaveToDefault(WVFBCONFIGINFO *pinfo, int index);

#endif _GETCONFIGINFO_H







