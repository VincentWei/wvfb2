#include <windows.h>

static HBITMAP hZap;
static BYTE *pBits;
static int bmpSize;
static BITMAPINFO     * pbmi ;
static int bmpheight;

void* CreateDibSectionFromDibFile (PTSTR szFileName)
{
     BITMAPFILEHEADER bmfh ;
     BOOL             bSuccess ;
     DWORD            dwInfoSize, dwBytesRead ;
     HANDLE           hFile ;
     HBITMAP          hBitmap ;

     // Open the file: read access, prohibit write access

     hFile = CreateFile (szFileName, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, 0, NULL) ;

     if (hFile == INVALID_HANDLE_VALUE)
          return NULL ;

     // Read in the BITMAPFILEHEADER

     bSuccess = ReadFile (hFile, &bmfh, sizeof (BITMAPFILEHEADER), 
                          &dwBytesRead, NULL) ;

     if (!bSuccess || (dwBytesRead != sizeof (BITMAPFILEHEADER))         
                   || (bmfh.bfType != * (WORD *) "BM"))
     {
          CloseHandle (hFile) ;
          return NULL ;
     }

     // Allocate memory for the BITMAPINFO structure & read it in

     dwInfoSize = bmfh.bfOffBits - sizeof (BITMAPFILEHEADER) ;

     pbmi = (BITMAPINFO *)malloc (dwInfoSize) ;

     bSuccess = ReadFile (hFile, pbmi, dwInfoSize, &dwBytesRead, NULL) ;

     if (!bSuccess || (dwBytesRead != dwInfoSize))
         goto failure;

     // Create the DIB Section

     hBitmap = CreateDIBSection (NULL, pbmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0) ;

     if (hBitmap == NULL)
         goto failure;

     // Read in the bitmap bits

     ReadFile (hFile, pBits, bmfh.bfSize - bmfh.bfOffBits, &dwBytesRead, NULL) ;
     bmpheight = pbmi->bmiHeader.biHeight;
     bmpSize = dwBytesRead;

     free (pbmi) ;
     CloseHandle (hFile) ;
     hZap = hBitmap;
     return pBits;
     //return hBitmap ;

failure:
     free (pbmi) ;
     CloseHandle (hFile) ;
     return NULL;
}

