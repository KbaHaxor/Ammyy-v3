#if !defined(AFX_SERVICEMANAGER_H__3F4DA014_E5F9_4262_9498_8EF3384E3E0D__INCLUDED_)
#define AFX_SERVICEMANAGER_H__3F4DA014_E5F9_4262_9498_8EF3384E3E0D__INCLUDED_

class CServiceManager
{
public:
	CServiceManager(LPCSTR lpServiceName, LPCSTR lpDisplayName);
	void GetStatus(bool& bInstall, bool& bStart, bool& bStop, bool& bRemove);
	void RunCmd(HWND hWnd, WORD cmd);
	void JustLunch(int sessionId);

private:
	void Install(SC_HANDLE hsrvmanager);
	void StopAndRemove(SC_HANDLE hsrvmanager, BOOL bRemove);
	void Start(SC_HANDLE hsrvmanager);
	void TurnSafeMode(bool on);

public:
	CStringA m_addArguments;

private:
	CStringA m_serviceName;
	CStringA m_displayName;	
};

#endif // !defined(AFX_SERVICEMANAGER_H__3F4DA014_E5F9_4262_9498_8EF3384E3E0D__INCLUDED_)
