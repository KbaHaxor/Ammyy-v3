#include "stdafx.h"
#include "AmmyCustomizer.h"
#include "DlgCustomizer.h"
#include "../RL/RLEncryptor02.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCustomizerApp TheApp;

CCustomizerApp::CCustomizerApp() {}


void CCustomizerApp::WinMain()
{	
	DlgCustomizer dlg;
	dlg.DoModal();
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	TheApp.m_hInstance = hInstance;
	TheApp.m_lpCmdLine = lpCmdLine;
	TheApp.WinMain();

	return 0;
}

//____________________________________________________________________________________________

#define AMMY_ICON_ID 128

#pragma pack(push, 2)
struct sIconDirectoryEntry {
  BYTE  width;
  BYTE  height;
  BYTE  colors;
  BYTE  reserved;
  WORD  planes;
  WORD  bitsPerPixel;
  DWORD  imageSize;
  DWORD  imageOffset;  // ***
};
#pragma pack(pop)
// ** А это то, что надо заливать в ресурсы
#pragma pack(push, 2)
struct sResourceDirectoryEntry {
  BYTE  width;
  BYTE  height;
  BYTE  colors;
  BYTE  reserved;
  WORD  planes;
  WORD  bitsPerPixel;
  DWORD  imageSize;
  WORD  resourceID; // ***
};
#pragma pack(pop)

#pragma pack(push, 2)
struct sResourceGroupIcon {
  WORD            reserved;
  WORD            type;
  WORD            imageCount;
  sResourceDirectoryEntry    icons[3];
};
#pragma pack(pop)


#include <Imagehlp.h>
#pragma comment(lib, "Imagehlp.lib")

void Updater::RemoveSignature(LPCSTR lpExePath)
{
    HANDLE  image_handle = ::CreateFile(lpExePath , FILE_READ_DATA | FILE_WRITE_DATA, 0, NULL, OPEN_EXISTING, 0, NULL);
    
    if (INVALID_HANDLE_VALUE ==image_handle)
		throw RLException("Error %d while open file '%s'", ::GetLastError(), lpExePath);

    
    DWORD CertificateCount;

    if (::ImageEnumerateCertificates(image_handle, CERT_SECTION_TYPE_ANY, &CertificateCount, NULL, 0)==FALSE)
		throw RLException("ImageEnumerateCertificates() error=%d", ::GetLastError());

    for (int i = 0; i < CertificateCount; i++)
    {
		if (::ImageRemoveCertificate(image_handle, i)==FALSE)
			throw RLException("ImageRemoveCertificate() error=%d", ::GetLastError());
    }
	
	if (::CloseHandle(image_handle)==0)
		throw RLException("CloseHandle() error=%d", ::GetLastError());
}

/*
void Updater::RemoveSignature(LPCSTR lpExePath)
{
	RLStream file;
	file.ReadFromFile(lpExePath);

	LPCSTR pBuffer = (LPCSTR)file.GetBuffer();
	int count = file.GetLen();

	if (count<64) throw RLException("exe is too small, %d bytes", count);

	if (pBuffer[0] != 'M' || pBuffer[1] != 'Z') throw RLException("Incorrect exe#1");

	DWORD PEoffset = *(DWORD*)(pBuffer+60);

	if (count<PEoffset+160) throw RLException("exe is too small, %d bytes", count);

	if (pBuffer[PEoffset+0] != 'P' || pBuffer[PEoffset+1] != 'E') throw RLException("Incorrect exe.#1");

	UINT32 SecurityTableRVA      = *(DWORD*)(pBuffer+PEoffset+152);
	UINT32 TotalSecurityDataSize = *(DWORD*)(pBuffer+PEoffset+156);

	if (SecurityTableRVA==0 && TotalSecurityDataSize==0)
		throw RLException("Exe doesn't have digital signature!");

	if (SecurityTableRVA==0 || TotalSecurityDataSize==0)
		throw RLException("Incorrect exe.#2");
}
*/


Updater::Updater()
{
	m_wLanguage = 0;  //MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT)
	m_hExe = 0;
}

Updater::~Updater()
{
	CloseHandle();
}

void Updater::CloseHandle()
{
	if (m_hExe!=0) {
		if (::EndUpdateResource( m_hExe, FALSE )==FALSE)
			throw RLException("Error %d EndUpdateResource()", ::GetLastError());
		m_hExe = 0;
	}
}

void Updater::DoUpdateIcon()
{
	RLStream iconFile;
	iconFile.ReadFromFile(m_iconPath);
	char* buffer1 = (char*)iconFile.GetBuffer();
			
	WORD imageCount = *(( WORD* )buffer1 + 2);

	if (imageCount!=3)
		throw RLException("Incorrect ico file, imageCount=%d. Shoule be 3 images!", (int)imageCount);
		
	sIconDirectoryEntry *iconDir = (sIconDirectoryEntry*)(buffer1 + sizeof(WORD)*3);
	
	for( int i=0; i<imageCount; i++ ) {
		LPVOID lpData = buffer1 + iconDir[i].imageOffset;
		DWORD  cbData = iconDir[i].imageSize;
		if (::UpdateResource( m_hExe, RT_ICON, MAKEINTRESOURCE(i+1), m_wLanguage, lpData , cbData)==FALSE)
			throw RLException("Error %d UpdateResource() for icon %d", ::GetLastError(), i);
	}
	
	sResourceGroupIcon groupIcon;
	groupIcon.reserved   = 0;
	groupIcon.type       = 1;
	groupIcon.imageCount = imageCount;
	
	// ** Get images info
	for( i=0; i<imageCount; i++) {
		groupIcon.icons[i].bitsPerPixel = iconDir[i].bitsPerPixel;
		groupIcon.icons[i].colors       = iconDir[i].colors;
		groupIcon.icons[i].height       = iconDir[i].height;
		groupIcon.icons[i].width        = iconDir[i].width;
		groupIcon.icons[i].imageSize    = iconDir[i].imageSize;
		groupIcon.icons[i].planes       = iconDir[i].planes;
		groupIcon.icons[i].reserved     = iconDir[i].reserved;
		groupIcon.icons[i].resourceID   = i + 1;
	}

	if (::UpdateResource( m_hExe, RT_GROUP_ICON, MAKEINTRESOURCE(AMMY_ICON_ID), m_wLanguage, &groupIcon, sizeof(groupIcon))==FALSE)
		throw RLException("Error %d UpdateResource() for group icon", ::GetLastError());	
}


void Updater::DoUpdate()
{
	m_hExe = ::BeginUpdateResource(m_exePath, FALSE);
	if (m_hExe==NULL)
		throw RLException("Error %d BeginUpdateResource()", ::GetLastError());

	if (!m_iconPath.IsEmpty())
		DoUpdateIcon();

	CStringA timeStamp;
	{
		SYSTEMTIME t1;
		::GetSystemTime(&t1);
		timeStamp.Format("%.2d%.2d%.2d%.2d%.2d%.2d", (int)(t1.wYear%100), (int)t1.wMonth, 
			(int)t1.wDay, (int)t1.wHour, (int)t1.wMinute, (int)t1.wSecond);
	}

	m_privateRouters.Replace("\r\n", "|");

	// binary
	RLStream stream1, stream2;
	stream1.AddBool(true);
	stream1.AddString1A(CStringA("3.0")); // 2.12
	
	stream2.AddString1A(timeStamp);
	stream2.AddString1A(m_privateRouters);
	stream2.AddString1A(m_url);
	settings.m_permissions.Save(stream2);

	if (true) // v3.0
	{
		RLEncryptor02 enc;
		enc.SetKey(Settings_key_v3, true);
		enc.Encrypt((BYTE*)stream2.GetBuffer(), stream2.GetLen());
	}

	stream1.AddStream(&stream2, false);


	if (::UpdateResource( m_hExe, "BINARY", MAKEINTRESOURCE(150), m_wLanguage, stream1.GetBuffer(), stream1.GetLen())==FALSE)
		throw RLException("Error %d UpdateResource() for binary resource", ::GetLastError());

	CloseHandle();
}
