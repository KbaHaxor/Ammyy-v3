#if !defined(AFX_TSSESSIONS_H__0FE83246_9AD4_4767_82E3_3D9A8A1E0F1B__INCLUDED_)
#define AFX_TSSESSIONS_H__0FE83246_9AD4_4767_82E3_3D9A8A1E0F1B__INCLUDED_

class CTSSessions  
{
public:
	CTSSessions();

	DWORD WTSGetActiveConsoleSessionId();
	DWORD GetSessionId();

	//BOOL  SetConsoleSession(DWORD sessionId);

	bool IsOutConsole();

private:
	void* _WTSGetActiveConsoleSessionId;

	DWORD  m_session;
};


extern CTSSessions TSSessions;

#endif // !defined(AFX_TSSESSIONS_H__0FE83246_9AD4_4767_82E3_3D9A8A1E0F1B__INCLUDED_)
