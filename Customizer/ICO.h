#if !defined(ICO_H__B2B583EC_7CEC_483C_A4F9_2513E6C8E3D9__INCLUDED_)
#define ICO_H__B2B583EC_7CEC_483C_A4F9_2513E6C8E3D9__INCLUDED_

#pragma once

#include "resource.h"
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <iostream>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <strsafe.h>
#include <string>

#define MAX_NR_BUNDLES 1000

char *types[25]=
{
	"NULL","RT_CURSOR","RT_BITMAP","RT_ICON","RT_MENU","RT_DIALOG","RT_STRING","RT_FONTDIR","RT_FONT",
	"RT_ACCELERATORS","RT_RCDATA","RT_MESSAGETABLE","RT_GROUP_CURSOR","NULL",
	"RT_GROUP_ICON","NULL","RT_VERSION","RT_DLGINCLUDE","NULL","RT_PLUGPLAY","RT_VXD","RT_ANICURSOR", //21 de la 0 
	"RT_ANIICON","RT_HTML","RT_MANIFEST"
};

WORD nMaxID=0;
HANDLE hFile, hFileEx;
HINSTANCE currentUI;
UINT currentLangId;

CHAR *szReplaceFile={"Replace.txt"};

//id-urile bundleurilor GROUP_ICON
typedef struct
{
	UINT nBundles;
	UINT nLangId;	
} BUNDLES, *PBUNDLES;
int cBundles=0;

BUNDLES pBundles[MAX_NR_BUNDLES];


HWND hWndMain = 0;

// These next two structs represent how the icon information is stored
// in an ICO file.
typedef struct
{
	BYTE	bWidth;               // Width of the image
	BYTE	bHeight;              // Height of the image (times 2)
	BYTE	bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE	bReserved;            // Reserved
	WORD	wPlanes;              // Color Planes
	WORD	wBitCount;            // Bits per pixel
	DWORD	dwBytesInRes;         // how many bytes in this resource?
	DWORD	dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;
typedef struct 
{
	WORD			idReserved;   // Reserved
	WORD			idType;       // resource type (1 for icons)
	WORD			idCount;      // how many images?
	LPICONDIRENTRY	idEntries; // the entries for each image
} ICONDIR, *LPICONDIR;

// The following two structs are for the use of this program in
// manipulating icons. They are more closely tied to the operation
// of this program than the structures listed above. One of the
// main differences is that they provide a pointer to the DIB
// information of the masks.
typedef struct
{
	UINT			Width, Height, Colors; // Width, Height and bpp
	LPBYTE			lpBits;                // ptr to DIB bits
	DWORD			dwNumBytes;            // how many bytes?
	LPBITMAPINFO	lpbi;                  // ptr to header
	LPBYTE			lpXOR;                 // ptr to XOR image bits
	LPBYTE			lpAND;                 // ptr to AND image bits
} ICONIMAGE, *LPICONIMAGE;



typedef struct
{
	BYTE	bWidth;               // Width of the image
	BYTE	bHeight;              // Height of the image (times 2)
	BYTE	bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE	bReserved;            // Reserved
	WORD	wPlanes;              // Color Planes
	WORD	wBitCount;            // Bits per pixel
	DWORD	dwBytesInRes;         // how many bytes in this resource?
	WORD	nID;                  // the ID
} MEMICONDIRENTRY, *LPMEMICONDIRENTRY;
typedef struct 
{
	WORD			idReserved;   // Reserved
	WORD			idType;       // resource type (1 for icons)
	WORD			idCount;      // how many images?
	LPMEMICONDIRENTRY	idEntries; // the entries for each image
} MEMICONDIR, *LPMEMICONDIR;

#endif // !defined(ICO_H__B2B583EC_7CEC_483C_A4F9_2513E6C8E3D9__INCLUDED_)