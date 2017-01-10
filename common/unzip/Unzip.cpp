#include "stdafx.h"
#include "Unzip.h"
#include "LiteUnzip.h"
#include "../main/common.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


/*
bool CUnzip::UnzipFromFile(CString fileName, CString pathTo, CStringArray& unzippedFiles)
{
	unzippedFiles.RemoveAll();

	HUNZIP	huz;
	DWORD	result;

	result = UnzipOpenFileA(&huz, fileName, "c:\\");

	if (result) {
		//char msg[256];
		//UnzipFormatMessageA(result, msg, sizeof(msg));
		return false;
	}

	ZIPENTRY	ze;
	DWORD		numitems;

	// Find out how many items are in the archive.
	ze.Index = (DWORD)-1;
	result = UnzipGetItem(huz, &ze);

	if (result != FALSE) {		
		UnzipClose(huz);
		return false;
	}

	numitems = ze.Index;

	// Unzip each item, using the name stored (in the zip) for that item.
	for (ze.Index = 0; ze.Index < numitems; ze.Index++)
	{
		result = UnzipGetItem(huz, &ze);
		
		CString fileName = ze.Name;
		CString fullFileName = pathTo + CCommon::GetShortFileName((CString)ze.Name);
		
		if (_FileIsExist(fullFileName) != FALSE) {
			UnzipClose(huz);
			return false;
		}


		unzippedFiles.Add(fullFileName);
		
		result = UnzipItemToFile(huz, fullFileName, &ze);

		if (result)	{
			UnzipClose(huz);
			return false;
		}
	}
		
	// Done unzipping files, so close the ZIP archive.
	UnzipClose(huz);
	return true;
}
*/


void CUnzip::UnzipFirstFile(LPCWSTR fileNameZip, const CStringW& fileNameUnZip)
{
	HUNZIP huz;
	DWORD result = UnzipOpenFileW(&huz, fileNameZip, "");

	if (result) {
		throw RLException("UnzipFirstFile()#1");
		//char msg[256];
		//UnzipFormatMessageA(result, msg, sizeof(msg));
		//return false;
	}

	ZIPENTRY	ze;

	// Find out how many items are in the archive.
	ze.Index = (DWORD)-1;
	result = UnzipGetItem(huz, &ze);

	if (result != FALSE) {
		UnzipClose(huz);
		throw RLException("UnzipFirstFile()#2");
		//return false;
	}

	DWORD numitems = ze.Index;

	if (numitems!=1)
		throw RLException("UnzipFirstFile()#3");

	// Unzip each item, using the name stored (in the zip) for that item.
	for (ze.Index = 0; ze.Index < numitems; ze.Index++)
	{
		result = UnzipGetItemA(huz, &ze);
		
		//CString fileName = CCommon::GetShortFileName(CString(ze.Name));
		//CString fileNameUnZip = unzipPath + fileName;
		
		if (CCommon::FileIsExistW(fileNameUnZip)) {
			UnzipClose(huz);
			throw RLException("UnzipFirstFile()#4");
		}
		
		result = UnzipItemToFileW(huz, fileNameUnZip, &ze);

		if (result)	{
			UnzipClose(huz);
			throw RLException("UnzipFirstFile()#5");
		}
	}
		
	UnzipClose(huz); // Done unzipping files, so close the ZIP archive.
}


void CUnzip::UnzipFromBuffer(LPCVOID pBuffer, DWORD bufferLen, LPCWSTR fileNameOut)
{
	HUNZIP	huz;

	if (UnzipOpenBuffer(&huz, (LPVOID)pBuffer, bufferLen, NULL)) {
		throw RLException("UnzipFromBuffer()#1 %s", (LPCSTR)(CStringA)fileNameOut);
	}

	ZIPENTRY	ze;
	ze.Index = 0; // unzip only first file
	if (UnzipGetItemA(huz, &ze)) {
		UnzipClose(huz);
		throw RLException("UnzipFromBuffer()#2 %s", (LPCSTR)(CStringA)fileNameOut);
	}	
		
	if (UnzipItemToFileW(huz, fileNameOut, &ze)) {
		UnzipClose(huz);
		throw RLException("UnzipFromBuffer()#3 %s", (LPCSTR)(CStringA)fileNameOut);
	}	
		
	UnzipClose(huz); // Done unzipping files, so close the ZIP archive.
}