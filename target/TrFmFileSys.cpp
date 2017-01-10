#include "StdAfx.h"
#include "TrFmFileSys.h"
#include "../common/MD5.h"
#include "../main/common.h"
#include "../main/ImpersonateWrapper.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif



DWORD TrFmFileSys::ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytes)
{
	DWORD sz;
	BOOL b = ::ReadFile(hFile, lpBuffer, nNumberOfBytes, &sz, 0);

	if (!b) return ::GetLastError();

	return (sz==nNumberOfBytes) ? 0 : ERROR_READ_FAULT;
}

DWORD TrFmFileSys::WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytes)
{
	DWORD sz;
	BOOL b = ::WriteFile(hFile, lpBuffer, nNumberOfBytes, &sz, 0);

	if (!b) return ::GetLastError();

	return (sz==nNumberOfBytes) ? 0 : ERROR_WRITE_FAULT;
}


INT64 TrFmFileSys::GetFileSize(HANDLE hFile)
{
	LARGE_INTEGER size;
	if (!::GetFileSizeEx(hFile, &size)) throw RLException("Error %d GetFileSizeEx()", ::GetLastError());
	return size.QuadPart;
}


INT64 TrFmFileSys::GetFileSizeByName(LPCWSTR fileName)
{
	HANDLE hFile = ::CreateFileW(fileName, FILE_READ_DATA, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile==INVALID_HANDLE_VALUE) {
		DWORD dwError = ::GetLastError();
		if (dwError==ERROR_FILE_NOT_FOUND) return -1;
		throw RLException("Error %d while open file '%s'", ::GetLastError(), (LPCSTR)(CStringA)fileName);
	}

	LARGE_INTEGER size;

	if (!::GetFileSizeEx(hFile, &size))
		throw RLException("Error %d in GetFileSizeEx() for '%s'", ::GetLastError(), (LPCSTR)(CStringA)fileName);

	VERIFY(::CloseHandle(hFile)!=0);		

	return size.QuadPart;
}


//returns: 0 if successful and GetLastError() otherwise. path - failed component
//
DWORD TrFmFileSys::DeleteDir(LPCWSTR dirName, CStringW &path)
{
 	WIN32_FIND_DATAW findData;
	CStringW ptDir = dirName;
	TrFmFileSys::ThisFolder(ptDir);

	HANDLE hFind = ::FindFirstFileW(ptDir + L"*", &findData);

	if (hFind==INVALID_HANDLE_VALUE) {
		path = dirName;
		return ::GetLastError();
	}
	
	do 
	{
		CStringW FilePath = ptDir + findData.cFileName;

		if (IsSpecialName(findData.cFileName)) continue;

		if(findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
			::SetFileAttributesW(FilePath, findData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
		}

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			DWORD dwErr = DeleteDir(FilePath, path);
			if(dwErr) return dwErr;
		}
		else 
		{
			if(!::DeleteFileW(FilePath)) {
				path = dirName;
				return ::GetLastError();
			}
		}
	}	 
	while(::FindNextFileW(hFind, &findData));
	
	::FindClose(hFind);

	if(!::RemoveDirectoryW(dirName)) {
		path = dirName;
		return ::GetLastError();
	}
	
	return 0; //ok
}


/*
DWORD TrFmFileSys::GetMD5ForFile(LPCWSTR fileName, INT64& size, void* bufferOut)
{
	const int HASH_BLOCK_SIZE = 16*1024;

	HANDLE hFile = ::CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY, 0);

	if (hFile == INVALID_HANDLE_VALUE)
		return ::GetLastError();

	if (size == -1)
		size = GetFileSize(hFile);

	DWORD dwError = GetMD5ForFile(hFile, size, bufferOut);

	::CloseHandle(hFile);

	return dwError;
}
*/

DWORD TrFmFileSys::GetMD5ForFile(HANDLE hFile, INT64 size, void* bufferOut)
{
	const int HASH_BLOCK_SIZE = 16*1024;

	ASSERT(size>=0);

	MD5_CTX m_md5;
	MD5Init(&m_md5, 0);
	
	DWORD dwError = 0; // ok

	INT64 remain = size;

	while (remain>0) 
	{
		BYTE buffer[HASH_BLOCK_SIZE];
		DWORD count = (remain < HASH_BLOCK_SIZE) ? remain : HASH_BLOCK_SIZE;

		dwError = TrFmFileSys::ReadFile(hFile, buffer, count);

		if (dwError) break;

		MD5Update(&m_md5, buffer, count);
				
		remain -= count;
	}

	MD5Final(&m_md5);
	memcpy(bufferOut, m_md5.digest, 16);
	return dwError;
}


UINT32 TrFmFileSys::SearchRecursively(LPCWSTR pathName, FileList& fList, bool relative, const CStringW& relPath, DWORD& dwError, CStringW& errorPath)
{
	WIN32_FIND_DATAW fd;
	CStringW fullPath = pathName;  fullPath += '*';
	
	HANDLE handle = ::FindFirstFileW(fullPath, &fd);

	if (handle == INVALID_HANDLE_VALUE) {
		dwError = ::GetLastError();
		errorPath = pathName;
		return 0;
	}

	UINT32 fCount = 0;
	TrFmFileSys::FileSize file_item;

	do {
		if (IsSpecialName(fd.cFileName)) continue;
		if (IsDirAndLink(fd.dwFileAttributes)) continue;

		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) 
		{
			CStringW fullPath = pathName;
			fullPath += fd.cFileName;
			fullPath += '\\';
			CStringW relativePath;
			if (relative) {
				relativePath = relPath + fd.cFileName + '\\';
			}
			fCount += SearchRecursively(fullPath, fList, relative, relativePath, dwError, errorPath);
			if (dwError) break;
		}
		else {
			file_item.name = (relative) ? relPath : pathName;
			file_item.name += fd.cFileName;
			file_item.size = (fd.nFileSizeHigh << 32) + fd.nFileSizeLow;
			fList.push_back(file_item);
			fCount++;
		}
	}while (::FindNextFileW(handle, &fd));

	::FindClose(handle);

	// add empty folder if it has no files
	if (fCount==0 && dwError==0) {
		file_item.name = ((relative) ? relPath : pathName) +'\\';
		file_item.size = 0;
		fList.push_back(file_item);
		fCount = 1;
	}

	return fCount;
}

/* 4+4
int TrFmFileSys::AddFileSize(RLStream& buffer, UINT64& size)
{
	if (size >= 0x80000000) {
		buffer.AddUINT32((size >> 32) | 0x80000000);
		buffer.AddUINT32(size & 0xffffffff);
		return 8;
	}
	else {
		buffer.AddUINT32(size & 0xffffffff);
		return 4;
	}
}

void TrFmFileSys::GetFileSize(RLStream& buffer, UINT64& size)
{
	size = buffer.GetUINT32();
	if ((size & 0x80000000) != 0) {
		size = (size & 0x000000007fffffff) << 32;
		size |= buffer.GetUINT32();
	}
}
*/

DWORD TrFmFileSys::CreateRecurseDir(LPCWSTR dirName)
{
	CStringW path = dirName;
	WCHAR *lpPtr0 = path.GetBuffer(0);

	lpPtr0 = wcschr(lpPtr0, '\\');

	if (!lpPtr0) return 0;

	while(true)
	{
		lpPtr0 = wcschr(lpPtr0+1, '\\');

		if(!lpPtr0) return 0; // it's file

		*lpPtr0 = 0;

		if (!::CreateDirectoryW(path, NULL))
		{
			DWORD dwError = ::GetLastError();
			if(dwError != ERROR_ALREADY_EXISTS)
			{
				return dwError;
			}
		}
		*lpPtr0 = '\\';		

	}
}

int TrFmFileSys::AddFileSize(RLStream& buffer, UINT64 size)
{
	size <<= 1;
	if (size >= 0x1000000) {
		size |= 1;
		buffer.AddRaw(&size, 6);
		return 6;
	}
	else {
		buffer.AddRaw(&size, 3);
		return 3;
	}
}

UINT64 TrFmFileSys::GetFileSize(RLStream& buffer)
{
	UINT64 size = 0;
	buffer.GetRaw(&size, 3);
	if (size & 1) {
		buffer.GetRaw(((char*)&size+3), 3);
	}
	size >>= 1;
	return size;
}

UINT64 TrFmFileSys::GetFileSize(Transport* pTranport)
{
	UINT64 size = 0;
	pTranport->ReadExact(&size, 3);
	if (size & 1) {
		pTranport->ReadExact((char*)&size+3, 3);
	}
	size >>= 1;
	return size;
}

void TrFmFileSys::FillLinks(CStringW* links)
{
	ImpersonateWrapper impersonate; //without condition "settings.m_impersonateFS"

	::SHGetSpecialFolderPathW(0, links[0].GetBuffer(MAX_PATH), CSIDL_DESKTOPDIRECTORY, FALSE);
	::SHGetSpecialFolderPathW(0, links[1].GetBuffer(MAX_PATH), CSIDL_PERSONAL, FALSE);
	links[0].ReleaseBuffer();
	links[1].ReleaseBuffer();
	if (links[0].GetLength()>0) TrFmFileSys::ThisFolder(links[0]);
	if (links[1].GetLength()>0) TrFmFileSys::ThisFolder(links[1]);
}

CStringW TrFmFileSys::GetFileExt(LPCWSTR fileName)
{
	int i;
	for (i = wcslen(fileName); --i >= 0; ) {
		if (fileName[i] == '\\') return L""; // no extension
		if (fileName[i] == '.') break;
	}

	if (i == -1) i=0;
	return &fileName[i];
}


bool TrFmFileSys::IsFolder(const CStringW& path)
{ 
	int len = path.GetLength();
	if (len==0) return false;
	return (path[len-1] == '\\');
}

void TrFmLookUpBase::Do()
{
	m_dwError = 0;

	if (m_path[0]==0)
	{		
		//buffer[0] = 0;
		//buffer[1] = 0;
		//::GetLogicalDriveStringsW(sizeof(buffer)/sizeof(WCHAR), buffer);
		//WCHAR* n = buffer;
		//while (*n != 0) {
		//	m_items.push_back(VrFmItem(n, TrFmFileSys::TYPE_DISK, 0, fTime, 0));
		//	while (*n++ != 0) {}
		//}

		DWORD drives = ::GetLogicalDrives();
		if (drives==0) {
			m_dwError = ::GetLastError();
			ASSERT(m_dwError>0);
			OnBegin();
			return;
		}

		OnBegin();
		OnBeginDrives(drives);

		WCHAR path[] = L"*:\\";

		for (int i=0; i<26; i++) {
			if (((drives >> i) & 1) == 0) continue;
			path[0] = 'A' + i;
			OnAddDrive(path);
		}
		OnEndDrives();
	}
	else {
		WCHAR buffer[1024];
		wcscpy(buffer, m_path);
		wcscat(buffer, L"*");

		WIN32_FIND_DATAW dat;
		HANDLE handle = ::FindFirstFileW(buffer, &dat);
				
		if (handle == INVALID_HANDLE_VALUE) {
			m_dwError = ::GetLastError();
			
			// if path is disk and no files on disk it's NOT error
			if (m_dwError==ERROR_FILE_NOT_FOUND && (m_path[0]>0) && (m_path[1]==':') && (m_path[2]=='\\') && (m_path[3]==0)) 
				m_dwError = 0;

			OnBegin();
			return;
		}

		OnBegin();

		do {
			if (TrFmFileSys::IsSpecialName(dat.cFileName)) continue;
			if (TrFmFileSys::IsDirAndLink(dat.dwFileAttributes)) continue;
			this->OnAdd(dat);
		} while (::FindNextFileW(handle, &dat) != 0);
		::FindClose(handle);
	}
}

