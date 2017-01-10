#if (!defined(_SERVICE_H__INCLUDED_676897634))
#define _SERVICE_H__INCLUDED_676897634

class TrService
{
public:
	// Routine to set the current thread into the given desktop
	static BOOL SelectHDESK(HDESK newdesktop);

	// Routine to set the current thread into the named desktop
	static BOOL SelectDesktop(LPCSTR name);	

	// Routine to fake a CtrlAltDel to winlogon when required, *** This is a nasty little hack...
	static void CallSAS(DWORD session);
	static void SimulateCtrlAltDel();	

private:
	static void SimulateCtrlAltDel_v6();
	static void* SimulateCtrlAltDelThreadFn(void *);
	static void RunTaskManager();	
	static void SetFileTime (LPCWSTR fullFileName, FILETIME& time);
	static bool IsActualFile(LPCWSTR fullFileName, FILETIME& time, bool del);
	static void CopyFile(LPCWSTR fileNameOut, int    resourceId);
};

#endif // _SERVICE_H__INCLUDED_676897634
