#if !defined(AFX_DEBUG_H__69FED33F_EACB_4DB5_B794_FA5C478EFADD__INCLUDED_)
#define AFX_DEBUG_H__69FED33F_EACB_4DB5_B794_FA5C478EFADD__INCLUDED_

class CDebug  
{
public:
	CDebug();
	virtual ~CDebug();

	static DWORD GetProcessIdToSessionId(DWORD processId);

	static void EnumWindowStations();

	static CStringA GetInputDesktopName();
};

#endif // !defined(AFX_DEBUG_H__69FED33F_EACB_4DB5_B794_FA5C478EFADD__INCLUDED_)
