#if !defined(AFX_COMMON_H__7451734A_1AD2_403F_B05F_5DF3EB94C439__INCLUDED_)
#define AFX_COMMON_H__7451734A_1AD2_403F_B05F_5DF3EB94C439__INCLUDED_

class CCommon  
{
public:
	static int GetCountSubString(CString& input, CString delimiter);
	static CString GetSubString(CString& input, CString delimiter, int& i);
	static CString GetShortFileName(CString& fileName);
	static DWORD GetFileSize(CString& fileName);
	static CString GetFileNameExtension(CString& fileName);
	static CString GetFileNameWithoutExtension(CString& fileName);
	static void CheckOrCreateDirectoryW(CStringW& path);
	static bool IsFolderWrittable(CStringW& path);
	static void RecursiveDelete(CString szPath);
	static CStringW GetTempPathW();
	static void CheckFolderTailA(CStringA &folder);
	static void CheckFolderTailW(CStringW &folder);
	
	//static bool FileIsExistA(CStringA strPath);
	static bool FileIsExistW(CStringW strPath);
	static void DeleteFileW(LPCWSTR fileName);
	static void DeleteFileIfExistW(LPCWSTR fileName);
	static void MoveFileA(LPCSTR  extFileName, LPCSTR  newFileName);
	static void MoveFileW(LPCWSTR extFileName, LPCWSTR newFileName);

	static LPCSTR GetBuildDateTime();
	static void GetBuildDateTime(FILETIME& time);
	static CStringA ConvertIDToString(UINT32 val, LPCSTR null);	
	static bool IsThreadWorking(HANDLE hThread);

	static  void SetDirectoryForEveryone(LPCWSTR folder);
	static  CStringW GetFolderCandidate(int index);
	
	static LPCGUID  GetLPGUID(LPCGUID lpGuid);
	static CStringW WrapMarks(const CStringW& str);
	static void     FillRandom(void* buffer, int size);

	static CStringA GetPathA(const CString& fileName);

	static bool  TimeIsReady(DWORD dwTicks);
	static DWORD TimeGetSpan(DWORD dwTicks);

	static CStringA GetComputerName();
	static void     AddOSTypeAndVersion(RLStream& stream);

#ifdef __CSTRING_W_H__INCLUDED__
	static CStringW GetPathW(const CStringW& fileName);
	static CStringW GetModuleFileNameW(HMODULE hModule);
	static void     ConvToUTF8 (LPCWSTR src, RLStream& dst, bool withNull=true);
	static CStringW ConvToUTF16(LPCSTR  srcUTF8, int size=-1);
#endif
};


class BitArray
{
public:
	BitArray(void* ptr);

	bool Read();
	void Write(bool b);
	int GetSize() { return m_ptr - m_ptr_begin; }

	static int GetCountBits(BYTE* ptr, int bytes);

private:
	BYTE* m_ptr;
	BYTE* m_ptr_begin;
	int   m_n;
};

#endif // !defined(AFX_COMMON_H__7451734A_1AD2_403F_B05F_5DF3EB94C439__INCLUDED_)
