#ifdef _INSTALLER_
	#include "../installer/stdafx.h"
#endif

#include "stdafx.h"
#include "Common.h"
#include "../common/unzip/LiteUnzip.h"
#include <AccCtrl.h>
#include <Aclapi.h>
#include <time.h>

#include <sys\stat.h>	//for struct stat

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


int CCommon::GetCountSubString(CString& input, CString delimiter)
{
	int count = 0;
	int len_input = input.GetLength();
	int len_delim = delimiter.GetLength();
	int i = 0;

	while(true) {
		int i_found = input.Find(delimiter, i);

		if (i_found==-1) {
			if (len_input > i) count++;
			return count;
		}
		else {
			count++;
			i = i_found+len_delim;
		}
	}
}

CString CCommon::GetSubString(CString& input, CString delimiter, int& i)
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

// return file name without path
CString CCommon::GetShortFileName(CString& fullPath)
{
	int i1 = fullPath.ReverseFind('\\');
	int i2 = fullPath.ReverseFind('/');
	return fullPath.Mid(max(i1,i2)+1);
}


CString CCommon::GetPathA(const CStringA& fileName)
{
	int i1 = fileName.ReverseFind('\\');
	int i2 = fileName.ReverseFind('/');
	int i  = max(i1,i2);
	return (i==-1) ?  "" : fileName.Mid(0, i+1);
}

#ifdef __CSTRING_W_H__INCLUDED__
CStringW CCommon::GetPathW(const CStringW& fileName)
{
	int i1 = fileName.ReverseFind('\\');
	int i2 = fileName.ReverseFind('/');
	int i  = max(i1,i2);
	return (i==-1) ? L"" : fileName.Mid(0, i+1);
}
#endif

/*
bool CCommon::FileIsExistA(CStringA path)
{
	int len = path.GetLength();
	if (len>0 && path[len-1] == '\\') path = path.Mid(0, len-1);
	
	WIN32_FIND_DATAA fd;
	HANDLE hFind = ::FindFirstFileA( path, &fd);
	if (hFind != INVALID_HANDLE_VALUE) ::FindClose(hFind);
	return hFind != INVALID_HANDLE_VALUE;	
}
*/

bool CCommon::FileIsExistW(CStringW path)
{
	int len = path.GetLength();
	if (len>0 && path[len-1] == '\\') path = path.Mid(0, len-1);

	WIN32_FIND_DATAW fd;
	HANDLE hFind = ::FindFirstFileW( path, &fd);
	if (hFind != INVALID_HANDLE_VALUE) ::FindClose(hFind);
	return hFind != INVALID_HANDLE_VALUE;
}


void CCommon::CheckOrCreateDirectoryW(CStringW& path)
{
	if (CCommon::FileIsExistW(path) == FALSE) {
		if (FALSE == ::CreateDirectoryW(path, NULL))
			throw RLException("can't create folder '%s' error %d", (LPCSTR)(CStringA)path, ::GetLastError());
	}
}



//DWORD CCommon::GetFileSize(CString& fileName)
//{
//	HANDLE hFile = ::CreateFile(fileName, FILE_READ_DATA, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//	if (hFile==INVALID_HANDLE_VALUE) {
//		throw  RLException("Error in CCommon::GetFileSize()#1 Can't open file '%s' %d", fileName, ::GetLastError());
//	}
//
//	DWORD dwFileSize = ::GetFileSize(hFile, NULL);
//	ASSERT(dwFileSize!=INVALID_FILE_SIZE);
//
//	VERIFY(::CloseHandle(hFile)!=0);
//
//	return dwFileSize;
//}


CString CCommon::GetFileNameExtension(CString& fileName)
{
	int i = fileName.ReverseFind('.');
	return (i<0) ? "" : fileName.Mid(i+1);
}


CString CCommon::GetFileNameWithoutExtension(CString& fileName)
{
	int i = fileName.ReverseFind('.');
	return (i>=0) ? fileName.Mid(0, i) : fileName;	
}


bool CCommon::IsFolderWrittable(CStringW& path)
{
	LPCSTR files[] = {"settings.bin", "ammyy.log", "settings.rdp", "_tmp\\AMMYY_Admin.exe"};

	_log.WriteInfo("IsFolderWrittable()#0 %s", (LPCSTR)(CStringA)path);

	//try to open file
	for (int i=0; i<sizeof(files)/sizeof(files[0]); i++) {
		CStringW fileName = path + files[i];

		if (!CCommon::FileIsExistW(fileName)) continue;

		HANDLE hFile = ::CreateFileW(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile==INVALID_HANDLE_VALUE) {
			_log.WriteInfo("IsFolderWrittable()#1 %u %s", ::GetLastError(), (LPCSTR)files[i]);
			return false;
		}
		else {
			::CloseHandle(hFile);
		}
	}

	_log.WriteInfo("IsFolderWrittable()#end OK");

	return true;
}


/*
void CCommon::RecursiveDelete(CString szPath)
{
	CheckFolderTailA(szPath);

	WIN32_FIND_DATA fd;

	HANDLE hFind = ::FindFirstFile(szPath+"*", &fd);

	if (hFind==INVALID_HANDLE_VALUE) return;

	while(true)
	{
		bool directory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		if (!directory || fd.cFileName[0] != '.')  // skip '.' and '..' folders
		{
			CString path = szPath + fd.cFileName;

			if (!directory) {
				if (::DeleteFile(path) == 0) {
					::FindClose(hFind);
					throw RLException("Error %d while deleting file '%s'", ::GetLastError(), path);
				}
			}
			else {	// folder here
				RecursiveDelete(path);
				if (::RemoveDirectory(path) == 0) {
					::FindClose(hFind);
					throw RLException("Error %d while deleting folder '%s'", ::GetLastError(), path);
				}
			}
		}

		if (::FindNextFile(hFind, &fd)==0) {
			::FindClose(hFind);
			return;
		}
	}
}
*/


//void CCommon::CheckFolderTailA(CStringA &folder)
//{
//	int len = folder.GetLength();
//	if (len>0) {
//		char c = folder[len-1]; // last char
//		if (c!='\\' && c!='/') folder += '\\';
//	}
//}

void CCommon::CheckFolderTailW(CStringW &folder)
{
	int len = folder.GetLength();
	if (len>0) {
		WCHAR c = folder[len-1]; // last char
		if (c!='\\' && c!='/') folder += '\\';
	}
}

CStringW CCommon::GetTempPathW()
{
	CStringW str;
	::GetTempPathW(MAX_PATH, str.GetBuffer(MAX_PATH));
	str.ReleaseBuffer();
	CCommon::CheckFolderTailW(str);
	return str;
}


void CCommon::DeleteFileW(LPCWSTR fileName)
{
	if (::DeleteFileW(fileName) == 0)
		throw RLException("Can't delete file '%s' error=%d", (LPCSTR)(CStringA)fileName, ::GetLastError());
}


void CCommon::DeleteFileIfExistW(LPCWSTR fileName)
{
	if (CCommon::FileIsExistW(fileName)) {
		CCommon::DeleteFileW(fileName);
	}
}

//void CCommon::MoveFileA(LPCSTR extFileName, LPCSTR newFileName)
//{
//	if (::MoveFileA(extFileName, newFileName)==0)
//		throw RLException("error=%d while renaming '%s' to '%s'", ::GetLastError(), (LPCSTR)extFileName, (LPCSTR)newFileName);
//}

void CCommon::MoveFileW(LPCWSTR extFileName, LPCWSTR newFileName)
{
	if (::MoveFileW(extFileName, newFileName)==0)
		throw RLException("error=%d while renaming '%s' to '%s'", ::GetLastError(), (LPCSTR)(CStringA)extFileName, (LPCSTR)(CStringA)newFileName);
}



LPCSTR CCommon::GetBuildDateTime()
{
	return __DATE__ " at " __TIME__;
}

void CCommon::GetBuildDateTime(FILETIME& time) 
{
	CString str = GetBuildDateTime();

	CString month_str = str.Mid(0, 3);
	month_str.MakeLower();

	int month;

	if      (month_str=="jan") month=1;
	else if (month_str=="feb") month=2;
	else if (month_str=="mar") month=3;
	else if (month_str=="apr") month=4;
	else if (month_str=="may") month=5;
	else if (month_str=="jun") month=6;
	else if (month_str=="jul") month=7;
	else if (month_str=="aug") month=8;
	else if (month_str=="sep") month=9;
	else if (month_str=="oct") month=10;
	else if (month_str=="nov") month=11;
	else if (month_str=="dec") month=12;
	else 
		throw RLException("Invalid month %s", (LPCSTR)month_str);


	SYSTEMTIME st;
	memset(&st, 0, sizeof(st));
	
	st.wSecond = atoi(str.Mid(21, 2));
	st.wMinute = atoi(str.Mid(18, 2));
	st.wHour   = atoi(str.Mid(15, 2));
	st.wDay    = atoi(str.Mid(4, 2));
	st.wMonth  = month;
	st.wYear   = atoi(str.Mid(7, 4));

	VERIFY(::SystemTimeToFileTime(&st, &time)!=0);
}


CStringA CCommon::ConvertIDToString(UINT32 val, LPCSTR null)
{
	if (val==0) return null;

	CStringA str;
	str.Format("%u", val);
	int len = str.GetLength();
	while(true) {
		len -= 3;

		if (len>0)
			str.Insert(len, " ");
		else
			break;
	}	
	return str;
}


bool CCommon::IsThreadWorking(HANDLE hThread)
{
	if (hThread) {
		DWORD exitCode;
		VERIFY(::GetExitCodeThread(hThread, &exitCode)!=0);
		if (exitCode == STILL_ACTIVE)
			return true;
	}

	return false;
}


void CCommon::SetDirectoryForEveryone(LPCWSTR folder)
{
    PSID pSID1 = NULL;
    PACL pACL = NULL;
    EXPLICIT_ACCESS ea[1];    

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    ZeroMemory(&ea, 1 * sizeof(EXPLICIT_ACCESS));

    // Create a SID for the BUILTIN\USERS group.
	//SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    //if(!::AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS, 0,0,0,0,0,0, &pSID1))    
	//	throw RLException("AllocateAndInitializeSid() error=%u", ::GetLastError());

    // Create a SID for the Everyone group.
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    if (!::AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSID1))
		throw RLException("AllocateAndInitializeSid() error=%u", ::GetLastError());

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the Users group full access
    ea[0].grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR)pSID1;

    // Create a new ACL that contains the new ACEs.
    if (::SetEntriesInAcl(1, &ea[0], NULL, &pACL) != ERROR_SUCCESS)
        throw RLException("SetEntriesInAcl() error=%u", ::GetLastError());

    // Initialize a security descriptor.  
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (NULL == pSD) 
		throw RLException("LocalAlloc() error=%u", ::GetLastError());    
 
    if (!::InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
		throw RLException("InitializeSecurityDescriptor() error=%u", ::GetLastError());
 
    // Add the ACL to the security descriptor. 
    if (!::SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE))
		throw RLException("SetSecurityDescriptorDacl() error=%u", ::GetLastError());

    if(!::SetFileSecurityW(folder, DACL_SECURITY_INFORMATION,  (SECURITY_DESCRIPTOR*)pSD))
		throw RLException("SetFileSecurityA() error=%u %s", ::GetLastError(), (LPCSTR)(CStringA)folder);

    // Initialize a security attributes structure.
	//SECURITY_ATTRIBUTES sa;
    //sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    //sa.lpSecurityDescriptor = pSD;
    //sa.bInheritHandle = FALSE;
	
    //if (::CreateDirectoryA(folder, &sa)==FALSE)
	//	throw RLException("CreateDirectory() error=%u", ::GetLastError());

    if (pSID1)	::FreeSid(pSID1);
    if (pACL)	::LocalFree(pACL);
    if (pSD)	::LocalFree(pSD);
}


#ifdef CSIDL_COMMON_APPDATA

// CSIDL_COMMON_APPDATA		C:\ProgramData					C:\Documents and Settings\All Users\Application Data
// CSIDL_APPDATA			C:\Users\{USER}\AppData\Roaming	C:\Documents and Settings\{USER}\Application Data
// CSIDL_LOCAL_APPDATA		C:\Users\{USER}\AppData\Local	C:\Documents and Settings\{USER}\Local Settings\Application Data

CStringW CCommon::GetFolderCandidate(int index)
{
	CStringW path;
	int nFolder = (index==1) ? CSIDL_COMMON_APPDATA : CSIDL_LOCAL_APPDATA;
	HRESULT  hr = ::SHGetFolderPathW( NULL, nFolder, NULL, 0, path.GetBuffer(MAX_PATH));
	if (FAILED(hr))
		throw RLException("SHGetFolderPathW() Error=%X", hr);
	path.ReleaseBuffer();
	
	CCommon::CheckFolderTailW(path);

	path += L"AMMYY\\";
	return path;
}

#endif


LPCGUID CCommon::GetLPGUID(LPCGUID lpGuid)
{
	const char* p = (const char*)lpGuid;

	for (int i=0; i<sizeof(GUID); i++) {
		if (p[i]!=0) return lpGuid;
	}

	return NULL;
}


bool CCommon::TimeIsReady(DWORD dwTicks)
{
	DWORD dwNow = ::GetTickCount();

	if (dwTicks<=dwNow) {
		return ((dwNow-dwTicks) < (1<<31)); // true - normal, fasle - dwTicks was over
	}
	else {
		return ((dwTicks-dwNow) > (1<<31)); // true - dwNow was over, false - normal
	}
}

DWORD CCommon::TimeGetSpan(DWORD dwTicks)
{
	DWORD dwNow = ::GetTickCount();

	if (dwNow>=dwTicks) 
		return (dwNow-dwTicks);
	else
		return (0xFFFFFFFF-dwTicks)+1+dwNow; // dwNow was over 2^32
}

CStringA CCommon::GetComputerName()
{
	CStringA str;

	DWORD len = 1024;
	if (!::GetComputerName(str.GetBuffer(len), &len)) {
		throw RLException("GetComputerName() error=%d", ::GetLastError());
	}
	//else if (len>1024) throw RLException("GetComputerName() error");

	str.ReleaseBuffer(len);

	return str;
}

void CCommon::AddOSTypeAndVersion(RLStream& stream)
{
	char buffer[64];
	CStringA str;

	OSVERSIONINFOEX info;
	info.dwOSVersionInfoSize = sizeof(info);
	VERIFY(FALSE != ::GetVersionEx((OSVERSIONINFO*)&info));
	
	sprintf(buffer, "%u.%u.%u", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);
	str += buffer;

	if (info.wServicePackMajor!=0 || info.wServicePackMinor!=0)
	{
		sprintf(buffer, " SP%u.%u", info.wServicePackMajor, info.wServicePackMinor);
		str += buffer;
	}


	stream.AddString1A(CStringA("Windows"));
	stream.AddString1A(str);
}

#ifdef __CSTRING_W_H__INCLUDED__

CStringW CCommon::WrapMarks(const CStringW& str)
{
	CStringW strOut;
	strOut.GetBuffer(str.GetLength()+2);
	strOut += '"';
	strOut += str;
	strOut += '"';
	return strOut;
}

void CCommon::FillRandom(void* buffer, int size)
{
	srand(time(NULL));

	char* buffer1 = (char*)buffer;
	char* buffer1_end = buffer1+size;

	while (buffer1<buffer1_end) {
		*buffer1++ = rand() % 256;
	}
}

CStringW CCommon::GetModuleFileNameW(HMODULE hModule)
{
	CStringW str;

	if (::GetModuleFileNameW(hModule, str.GetBuffer(MAX_PATH), MAX_PATH) == 0)
		throw RLException("ERROR %d in GetModuleFileNameW()", ::GetLastError());

	str.ReleaseBuffer();
	return str;
}

void CCommon::ConvToUTF8(LPCWSTR src, RLStream& dst, bool withNull)
{
	int len1 = ::WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
	if (len1<=0)
		throw RLException("ConvToUTF8()#1 %d", ::GetLastError());

	dst.SetMinExtraCapasity(len1);
	
	int len2 = ::WideCharToMultiByte(CP_UTF8, 0, src, -1, (LPSTR)dst.GetBufferWr(), len1, NULL, NULL);
	if (len2<=0)
		throw RLException("ConvToUTF8()#2 %d", ::GetLastError());

	if (len2!=len1)
		throw RLException("ConvToUTF8()#3 %d %d", len1, len2);

	if (!withNull) --len2;
	dst.SetLen(dst.GetLen() + len2);
	//return len2;
}


CStringW CCommon::ConvToUTF16(LPCSTR srcUTF8, int size)
{
	CStringW str;

	if (size==0) return str;
	
	int len1 = ::MultiByteToWideChar(CP_UTF8, 0, srcUTF8, size, NULL, 0);
	if (len1 <= 0)
		throw RLException("ConvToUTF16()#1 %d", ::GetLastError());
	
	int len2 = ::MultiByteToWideChar(CP_UTF8, 0, srcUTF8, size, str.GetBuffer(len1), len1);
	if (len2 <= 0)
		throw RLException("ConvToUTF16()#2 %d", ::GetLastError());

	if (len2 != len1) 
		throw RLException("ConvToUTF16()#3 %d %d", len1, len2);

	if (size>=0) {
		str.GetBuffer(0)[len1] = 0;
		str.ReleaseBuffer(); // we don't know if we had zero-terminated source or not
	}
	else {
		str.ReleaseBuffer(len1-1);
	}
	
	return str;
}




/*
void CCommon::ConvertWideCharToAnsi(WCHAR* strSrc, CString& strDst)
{
	int len1 = ::WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	if (len1<=0)
		throw RLException("ConvertWideCharToUTF8()#1 %d", ::GetLastError());

	int len2 = ::WideCharToMultiByte(CP_ACP, 0, strSrc, -1, strDst.GetBuffer(len1), len1, NULL, NULL);
	if (len2<=0)
		throw RLException("ConvertWideCharToUTF8()#2 %d", ::GetLastError());

	if (len2!=len1) 
		throw RLException("ConvertWideCharToUTF8()#3");

	strDst.ReleaseBuffer(len2-1);
}
 
*/

#endif // __CSTRING_W_H__INCLUDED__


//____________________________________________________________________________________________________

int BitArray::GetCountBits(BYTE* ptr, int bytes)
{
	BYTE* ptr_end = ptr + bytes;

	int count = 0;

	while (ptr<ptr_end)
	{
		BYTE b = *ptr++;
		if ((b & (1<<7))!=0) count++;
		if ((b & (1<<6))!=0) count++;
		if ((b & (1<<5))!=0) count++;
		if ((b & (1<<4))!=0) count++;
		if ((b & (1<<3))!=0) count++;
		if ((b & (1<<2))!=0) count++;
		if ((b & (1<<1))!=0) count++;
		if ((b & (1<<0))!=0) count++;
	}
	return count;
}

BitArray::BitArray(void* ptr)
{
	m_ptr_begin = m_ptr = (BYTE*)ptr - 1;
	m_n = -1;
}

bool BitArray::Read()
{
	if (m_n<0) {
		m_ptr++;
		m_n = 7;
	}
	return ((*m_ptr >> m_n--) & 1) != 0;
}

void BitArray::Write(bool b)
{
	if (m_n<0) {
		m_ptr++;			
		m_n = 7;
		*m_ptr = 0;
	}

	if (b) *m_ptr |= (1 << m_n);
	m_n--;
}
