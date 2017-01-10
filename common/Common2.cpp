// it has common functions, but multi Platform (Windows, Unix)

#include "stdafx.h"
#include "Common2.h"

#include <sys/stat.h>	//for struct stat

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#ifdef _LINUX
#include <unistd.h>
#include <sys/timeb.h>
#endif
#ifdef _FreeBSD
#include <sys/sysctl.h>
#include <sys/timeb.h>
#endif

CStringA Common2::GetModuleFileNameA(HMODULE hModule)
{
	CStringA str;

#ifdef _WIN32
	VERIFY(::GetModuleFileNameA(hModule, str.GetBuffer(MAX_PATH), MAX_PATH) != 0);
	str.ReleaseBuffer();
#endif
#ifdef _FreeBSD
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	char buf[1024];
	size_t cb = 512;
	sysctl(mib, 4, str.GetBuffer(cb), &cb, NULL, 0);
	str.ReleaseBuffer();
	ASSERT(str.GetLength()>0);
#endif
#ifdef _LINUX
	ASSERT(hModule==0);
	int h = readlink("/proc/self/exe", str.GetBuffer(512), 512);
	ASSERT(h>0);
	str.ReleaseBuffer(h);
#endif

	return str;
}


//CString Common2::GetFileNameWithoutExtension(CString& fileName)
//{
//	int i = fileName.ReverseFind('.'); // TODO: need to find '\\' or '/' before '.'
//	return (i>=0) ? fileName.Mid(0, i) : fileName;	
//}


CString Common2::GetPath(const CString& fileName)
{
	int i1 = fileName.ReverseFind('\\');
	int i2 = fileName.ReverseFind('/');
	if (i1<i2) i1=i2; // i1 = max(i1,i2);
	return (i1==-1) ? "" : fileName.Mid(0, i1+1);
}

CString Common2::GetFileName(const CString& fileName)
{
	int i1 = fileName.ReverseFind('\\');
	int i2 = fileName.ReverseFind('/');
	if (i1<i2) i1=i2; // i1 = max(i1,i2);
	return (i1==-1) ? "" : fileName.Mid(i1+1);
}

CString Common2::GetSubString(CString& input, CString delimiter, int& i)
{
	CString subString;
	int i_len	= input.GetLength();
	if (i>=i_len) return "";
	int i_found = input.Find(delimiter, i);

	if (i_found==-1) {
		i_found = i_len;
	}

	subString = input.Mid(i, i_found-i);

	i = i_found + delimiter.GetLength();

	return subString;
}


// for derictories should also work (for Windows tested)

bool Common2::FileIsExistA(CStringA strPath)
{
	int nSlash = strPath.ReverseFind('\\');
	if( nSlash == strPath.GetLength()-1)
		strPath = strPath.Left(nSlash);
	/*
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile( strPath, &fd );
	if ( hFind != INVALID_HANDLE_VALUE )
		FindClose( hFind );
	return hFind != INVALID_HANDLE_VALUE;
	*/
	struct stat stat_buf;
	if (stat(strPath, &stat_buf) >= 0)
		return true;
	else
		return false;
}

void Common2::CreateDirectoryA(LPCSTR path)
{
#ifdef _WIN32
	if (FALSE == ::CreateDirectoryA(path, NULL))
		throw RLException("can't create folder '%s' error %d", (LPCSTR)path, ::GetLastError());
#else
	if (::mkdir((LPCSTR)path, S_IRWXO)!=0)
		throw RLException("can't create folder '%s' error %d", (LPCSTR)path, errno);
#endif
}


// move file or directory
void Common2::MoveFile(LPCSTR extName, LPCSTR newName)
{
#ifdef _WIN32
	if (FALSE == ::MoveFile(extName, newName))
		throw RLException("MoveFile('%s','%s') error=%d", extName, newName, ::GetLastError());
#else
	if (::rename(extName, newName)!=0)
		throw RLException("MoveFile('%s','%s') error=%d", extName, newName, errno);
#endif
}



/*
// return file name without path
CStringA Common2::GetShortFileName(CStringA& fullPath)
{
	int i1 = fullPath.ReverseFind('\\');
	int i2 = fullPath.ReverseFind('/');
	return fullPath.Mid(max(i1,i2)+1);
}
*/


void Common2::DeleteFileA(LPCSTR fileName)
{
#ifdef _WIN32
	if (::DeleteFile(fileName)==0)
		throw RLException("Error %d while deleting file '%s'", ::GetLastError(), fileName);
#else
	if (::unlink(fileName)<0) 
		throw RLException("Error %d while deleting file '%s'", errno, fileName);
#endif
}




// defference beetwen 1970.01.01 and 1601.01.01 in milliseconds 134774*24*3600*1000
#ifdef _WIN32
#define LINUX_FILETIME_EPOCH 11644473600000
#else
#define LINUX_FILETIME_EPOCH 11644473600000LL
#endif


// return milliseconds since January 1, 1601 (UTC).
UINT64 Common2::GetSystemTime()
{
	UINT64 time;
#ifdef _WIN32
	FILETIME ft;
	::GetSystemTimeAsFileTime(&ft); // return 100-nanoseconds since January 1, 1601 (UTC).
	time = (((UINT64)ft.dwHighDateTime << 32) + ft.dwLowDateTime) / 10000;
#else
	struct timeb t1;
	ftime(&t1);	// return since January 1, 1970 (UTC).
	time = (UINT64)t1.time * 1000 + t1.millitm + LINUX_FILETIME_EPOCH;
#endif
	return time;
}

// return count seconds from 1970.01.01
//
UINT32 Common2::UINT64ToUnixTime(UINT64 timeIn)
{
	timeIn-=LINUX_FILETIME_EPOCH;
	return (UINT32)(timeIn/1000);
}

UINT64 Common2::UnixTimeToUINT64(UINT32 timeSec, UINT16 timeMs)
{
	UINT64 t = ((UINT64)timeSec)*1000 + LINUX_FILETIME_EPOCH + timeMs;
	return t;
}


#ifdef _WIN32
UINT64 Common2::SystemTimeToUINT64(const SYSTEMTIME& timeIn)
{
	FILETIME t;

	if (::SystemTimeToFileTime(&timeIn, &t)==0)
		throw RLException("SystemTimeToUnixTime");

	UINT64 v = *((UINT64*)&t);
	return v/10000; // convert from 100-nanoseconds
}
#endif


void Common2::UINT64ToSystemTime(UINT64 timeIn, SYSTEMTIME& timeOut)
{
#ifdef _WIN32
	timeIn *= 10000; // convert to 100-nanoseconds
	FILETIME t;
	t.dwHighDateTime = ((DWORD*)&timeIn)[1];
	t.dwLowDateTime  = ((DWORD*)&timeIn)[0];

	if (::FileTimeToSystemTime(&t, &timeOut)==0)
		throw RLException("UINT64ToSystemTime");
#else
	timeIn -= LINUX_FILETIME_EPOCH;
	time_t time           = timeIn / 1000;
	timeOut.wMilliseconds = timeIn % 1000;

	struct tm* t2 = gmtime(&time);

	timeOut.wYear   = t2->tm_year + 1900;
	timeOut.wMonth  = t2->tm_mon + 1;
	timeOut.wDay    = t2->tm_mday;
	timeOut.wHour   = t2->tm_hour;
	timeOut.wMinute = t2->tm_min;
	timeOut.wSecond = t2->tm_sec;

#endif
}

CStringA Common2::IPv4toString(UINT32 ip)
{
	CStringA str;

	int len = sprintf(str.GetBuffer(32), "%u.%u.%u.%u",
		(int)(((BYTE*)&ip)[0]),
		(int)(((BYTE*)&ip)[1]),
		(int)(((BYTE*)&ip)[2]),
		(int)(((BYTE*)&ip)[3]));

	str.ReleaseBuffer(len);	
	return str;
}

//_____________________________________________________________________________________________________

#include "../common/MD5.h"

const BYTE RLMD5::Empty[16] = { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xE9, 0x80, 0x09, 0x98, 0xEC, 0xF8, 0x42, 0x7E };

void RLMD5::SetEmpty()
{
	memcpy(hash, Empty, 16);
}

bool RLMD5::IsEmpty()
{
	return (memcmp(hash, RLMD5::Empty, 16)==0);
}

void RLMD5::Calculate(LPCSTR str)
{
	int len = strlen(str);
	MD5_CTX m_md5;
	MD5Init(&m_md5, 0);
	MD5Update(&m_md5, (BYTE*)(LPCSTR)str, len);
	MD5Final(&m_md5);
	memcpy(hash, m_md5.digest, 16);
}

bool operator==(const RLMD5& s1, const RLMD5& s2)
{
	return (memcmp(s1.hash, s2.hash, 16)==0);
}
