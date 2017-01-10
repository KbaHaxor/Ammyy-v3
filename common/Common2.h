#if !defined(AFX_COMMON2_H__7402734A__INCLUDED_)
#define AFX_COMMON2_H__7402734A__INCLUDED_

class Common2
{
public:
	static CString GetModuleFileNameA(HMODULE hModule);
	//static CString GetFileNameWithoutExtension(CString& fileName);
	static CString GetPath(const CString& fileName);
	static CString GetFileName(const CString& fileName);
	static CString GetSubString(CString& input, CString delimiter, int& i);
	static bool FileIsExistA(CStringA strPath);
	static void CreateDirectoryA(LPCSTR path);
	//static CStringA GetShortFileName(CStringA& fullPath);
	static void DeleteFileA(LPCSTR fileName);
	static void MoveFile(LPCSTR extName, LPCSTR newName);

	static UINT64 GetSystemTime();
	static UINT32 UINT64ToUnixTime(UINT64 timeIn);
	static UINT64 UnixTimeToUINT64(UINT32 timeSec, UINT16 timeMs);
	static UINT64 SystemTimeToUINT64(const SYSTEMTIME& timeIn);
	static void   UINT64ToSystemTime(UINT64 timeIn, SYSTEMTIME& timeOut);

	static CStringA IPv4toString(UINT32 ip);
};

class RLMD5
{
public:
	void SetEmpty();
	bool IsEmpty();
	void Calculate(LPCSTR str);

	BYTE hash[16];
private:
	static const BYTE Empty[16];
};

bool operator==(const RLMD5& s1, const RLMD5& s2);


#ifdef _WIN32
	#define PATH_DELIMETER "\\"
#else
	#define PATH_DELIMETER "/"
#endif

#endif // !defined(AFX_COMMON2_H__7402734A__INCLUDED_)
