#include "stdafx.h"
#include "trService.h"
#include "../common/omnithread/omnithread.h"
#include "../main/common.h"
#include "../main/AmmyyApp.h"
#include "../common/unzip/Unzip.h"
#include "../main/resource.h"
#include "../main/Service.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


// Static routines only used on Windows NT to ensure we're in the right desktop
// These routines are generally available to any thread at any time.

// - SelectHDESK(HDESK)
// Switches the current thread into a different desktop by desktop handle
// This call takes care of all the evil memory management involved

BOOL TrService::SelectHDESK(HDESK new_desktop)
{
	/*
	char new_name[256];
	if (!TrService::GetDesktopName(new_desktop, new_name, 256)) {
		_log2.Print(LL_ERR, VTCLOG("GetDesktopName() failed"));
		return FALSE;
	}
	_log2.Print(LL_INF, VTCLOG("SelectHDESK() to '%s' (%x)"), new_name, new_desktop);
	*/	

	// Switch the desktop
	if(!::SetThreadDesktop(new_desktop)) {
		_log2.Print(LL_ERR, VTCLOG("unable to SetThreadDesktop(), error=%d"), GetLastError());
		return FALSE;
	}

	return TRUE;
}

// - SelectDesktop(char *)
// Switches the current thread into a different desktop, by name
// Calling with a valid desktop name will place the thread in that desktop.

BOOL TrService::SelectDesktop(LPCSTR name)
{
	// Attempt to open the named desktop
	HDESK desktop = ::OpenDesktop(name, 0, FALSE,
						DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
						DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
	
	// Did we succeed?
	if (desktop == NULL) {		
		_log2.Print(LL_INF, VTCLOG("unable to open desktop name %X, '%s', error=%d"), name, (name) ? name : "", ::GetLastError());
		return FALSE;
	}

	// Switch to the new desktop
	if (!SelectHDESK(desktop)) {
		// Failed to enter the new desktop, so free it!
		_log2.Print(LL_ERR, VTCLOG("SelectDesktop() failed to select desktop"));
		if (!::CloseDesktop(desktop))
			_log2.Print(LL_ERR, VTCLOG("SelectDesktop failed to close desktop, error=%d"), GetLastError());
		return FALSE;
	}

	// We successfully switched desktops!
	return TRUE;
}


void TrService::RunTaskManager()
{
	char path[MAX_PATH];
	::GetSystemDirectory(path, MAX_PATH);
	strcat(path, "\\taskmgr.exe");
	::ShellExecute(NULL, "open", path, "", "", SW_SHOW);
}


// Static routine used to fool Winlogon into thinking CtrlAltDel was pressed

void* TrService::SimulateCtrlAltDelThreadFn(void *)
{
	_log2.Print(LL_INF, VTCLOG("generating ctrl-alt-del"));

	HDESK old_desktop = ::GetThreadDesktop(::GetCurrentThreadId());

	// Switch into the Winlogon desktop	
	if (!TrService::SelectDesktop("Winlogon"))
	{
		_log2.Print(LL_INF, VTCLOG("failed to select logon desktop"));
		RunTaskManager();
		return NULL;
	}

	HWND hwndCtrlAltDel = ::FindWindow("SAS window class", "SAS window");
	if (hwndCtrlAltDel == NULL) {
		_log2.Print(LL_ERR, VTCLOG("\"SAS window\" not found"));
		hwndCtrlAltDel = HWND_BROADCAST;
	}

	::PostMessage(hwndCtrlAltDel, WM_HOTKEY, 0, MAKELONG(MOD_ALT | MOD_CONTROL, VK_DELETE));

	// Switch back to our original desktop
	if (old_desktop) TrService::SelectHDESK(old_desktop);

	return NULL;
}


bool TrService::IsActualFile(LPCWSTR fullFileName, FILETIME& time, bool del)
{
	if (!CCommon::FileIsExistW(fullFileName)) return false;

	DWORD dwFileSize = 0;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;

	HANDLE hFile = ::CreateFileW(fullFileName, FILE_READ_DATA, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		throw RLException("IsActualFile()#1 CreateFile(%s), error=%d", (LPCSTR)(CStringA)fullFileName, ::GetLastError());

	VERIFY(::GetFileTime(hFile, &ftCreationTime, &ftLastAccessTime, &ftLastWriteTime)!=0);

	VERIFY(::CloseHandle(hFile)!=0);

	if (time.dwLowDateTime  != ftCreationTime.dwLowDateTime   || 
		time.dwHighDateTime != ftCreationTime.dwHighDateTime  ||
		time.dwLowDateTime  != ftLastWriteTime.dwLowDateTime  || 
		time.dwHighDateTime != ftLastWriteTime.dwHighDateTime)   
	{
		if (del) {
			if (!::DeleteFileW(fullFileName))
				throw RLException("DeleteFileW(%s) error=%d", (LPCSTR)(CStringA)fullFileName, ::GetLastError());
		}

		return false;
	}

	return true;
}



void TrService::SetFileTime(LPCWSTR fullFileName, FILETIME& time)
{
	HANDLE hFile = ::CreateFileW(fullFileName, FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		throw RLException("Error in SetFileTime()#1 can't open file '%s' %d", (LPCSTR)(CStringA)fullFileName, ::GetLastError());

	VERIFY(::SetFileTime(hFile, &time, NULL, &time)!=0);
	VERIFY(::CloseHandle(hFile)!=0);
}


void TrService::CopyFile(LPCWSTR fileNameOut, int resourceId)
{
	FILETIME build_time;

	CCommon::GetBuildDateTime(build_time);

	if (IsActualFile(fileNameOut, build_time, true)) return; // we already have correct file

	LPCVOID pData;
	DWORD dwsize = RLResource::Load(NULL, "BINARY", MAKEINTRESOURCE(resourceId), 0, &pData);

	CUnzip::UnzipFromBuffer(pData, dwsize, fileNameOut);

	// set time of file to mark it as actual
	SetFileTime(fileNameOut, build_time);
}



//  simulate Ctrl-Alt-Del locally
//

void TrService::SimulateCtrlAltDel_v6()
{
    DWORD dwCurrentSessionId = 0;

    if(!::ProcessIdToSessionId(GetCurrentProcessId(), &dwCurrentSessionId))
    {
		_log.WriteError("ProcessIdToSessionId() error=%d", ::GetLastError());
        return;
    }

    if(dwCurrentSessionId!=0)
    {
		CStringA str;
		str.Format(" -dosas_%u", dwCurrentSessionId);

		CStringW path = CCommon::WrapMarks(CCommon::GetModuleFileNameW(0)) + CStringW(str);
		
		CService::CreateProcess1(0, NULL, path); // create process in Zero session		
    }
    else
    {
        CallSAS(0);
    }
}

// for Vista or Win7
void TrService::CallSAS(DWORD session)
{
	typedef DWORD( __stdcall *_WmsgSendMessage )(DWORD, DWORD, DWORD, DWORD);

    CONST WCHAR* wszGPOKey    = L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy Objects";
    CONST WCHAR* wszPolicyKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
    CONST WCHAR* wszMachineSuffix = L"Machine";
    CONST WCHAR* wszSASGeneration = L"SoftwareSASGeneration";
    CONST WCHAR* wszDelSASGeneration = L"**del.SoftwareSASGeneration";

    DWORD dwDisposition;
    DWORD dwKeyType;
    DWORD dwLength;

    HKEY hGPO = NULL;
    BOOL bFound = FALSE;

    HKEY hKeyUserPol = NULL;
    WCHAR wszKeyUserPol[512];

    DWORD dwUserValue;
    BOOL bUserValueValid   = FALSE;
    BOOL bUserRemovePolicy = FALSE;

    HKEY hPolicy = NULL;
    DWORD dwMachineValue;
    BOOL bMachineValueValid = FALSE;

    if (::RegOpenKeyExW(HKEY_CURRENT_USER, wszGPOKey, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_CREATE_SUB_KEY, &hGPO) == ERROR_SUCCESS)
    {
        DWORD dwIndex = 0;
        FILETIME ft;
        WCHAR wszKeyName[256];
        DWORD dwKeyNameLength = 256;
        while (::RegEnumKeyExW(hGPO, dwIndex, wszKeyName, &dwKeyNameLength, NULL, NULL, NULL, &ft) != ERROR_NO_MORE_ITEMS)
        {
            WCHAR* wszGuidEnd = NULL;
            if (wszGuidEnd = wcsstr(wszKeyName, wszMachineSuffix))
            {
                bFound = TRUE;
                break;
            }

            dwKeyNameLength = 256;
            dwIndex++;
        }

        if (bFound)
        {
            wsprintfW(wszKeyUserPol, L"%s\\%s", wszKeyName, wszPolicyKey);

            if (::RegCreateKeyExW(hGPO, wszKeyUserPol, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKeyUserPol, &dwDisposition) == ERROR_SUCCESS)
            {
                DWORD dwNewValue = 3;

                dwLength = sizeof(DWORD);
                bUserValueValid = (::RegQueryValueExW(hKeyUserPol, wszSASGeneration, NULL, &dwKeyType, (LPBYTE)&dwUserValue, &dwLength) == ERROR_SUCCESS);

                ::RegSetValueExW(hKeyUserPol, wszSASGeneration, 0, REG_DWORD, (LPBYTE)&dwNewValue, sizeof(DWORD));

                bUserRemovePolicy = (::RegDeleteValueW(hKeyUserPol, wszDelSASGeneration) == ERROR_SUCCESS);
            }
        }
    }

    // Update policy
    if (::RegCreateKeyExW(HKEY_LOCAL_MACHINE, wszPolicyKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hPolicy, &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD dwNewValue = 3;

        dwLength = sizeof(DWORD);
        bMachineValueValid = (RegQueryValueExW(hPolicy, wszSASGeneration, NULL, &dwKeyType, (LPBYTE)&dwMachineValue, &dwLength) == ERROR_SUCCESS);

        ::RegSetValueExW(hPolicy, wszSASGeneration, 0, REG_DWORD, (LPBYTE)&dwNewValue, sizeof(DWORD));
    }


    {
        HINSTANCE hDll = ::LoadLibraryW(L"wmsgapi.dll");
        if (hDll)
        {
            _WmsgSendMessage WmsgSendMessage = (_WmsgSendMessage)::GetProcAddress(hDll, "WmsgSendMessage");
            if (WmsgSendMessage)
            {
                DWORD bV = 0;
                DWORD dwRet = WmsgSendMessage(session, 0x208, 0, (DWORD)&bV);
            }
            ::FreeLibrary(hDll);
        }
    }

    ::SleepEx(100, FALSE);

    // Clean-up
    if (hPolicy)
    {
        if (bMachineValueValid)        
            ::RegSetValueExW(hPolicy, wszSASGeneration, 0, REG_DWORD, (LPBYTE)&dwMachineValue, sizeof(DWORD));        
        else        
            ::RegDeleteValueW(hPolicy, wszSASGeneration);
        
        ::RegCloseKey(hPolicy);
    }

    if (bFound)
    {
        if (bUserValueValid)
            ::RegSetValueExW(hKeyUserPol, wszSASGeneration, 0, REG_DWORD, (LPBYTE)&dwUserValue, sizeof(DWORD));        
        else
            ::RegDeleteValueW(hKeyUserPol, wszSASGeneration);
        
        if (bUserRemovePolicy)
        {
            WCHAR* wszSimpleData = L" ";
            ::RegSetValueExW(hKeyUserPol, wszDelSASGeneration, 0, REG_SZ, (LPBYTE)wszSimpleData, 2 * sizeof(WCHAR));
        }
    }

    if (hKeyUserPol) ::RegCloseKey(hKeyUserPol);
    if (hGPO)        ::RegCloseKey(hGPO);
}

void TrService::SimulateCtrlAltDel()
{
	try {
		_log2.Print(LL_INF, VTCLOG("preparing to generate ctrl-alt-del %X %d"), TheApp.m_dwOSVersion, (int)TheApp.m_systemUser);

		if (TheApp.m_dwOSVersion>=0x60000) 
		{
			if (TheApp.m_systemUser)
				SimulateCtrlAltDel_v6();
			else
				RunTaskManager();
		}
		else {
			// We simulate Ctrl+Alt+Del by posting a WM_HOTKEY message to the
			// "SAS window" on the Winlogon desktop.
			// This requires that the current thread is part of the Winlogon desktop.
			// But the current thread has hooks set & a window open, so it can't
			// switch desktops, so I instead spawn a new thread & let that do the work...

			omni_thread *thread = omni_thread::create(SimulateCtrlAltDelThreadFn);
			if (thread == NULL) return;
			thread->join(NULL);
		}
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}
}
