#if !defined(RL_LOG_H_INCLUDED_)
#define RL_LOG_H_INCLUDED_

#include "RLLock.h"

class RLLog
{
	// methods
public:
	RLLog();
	~RLLog();

#ifdef _WIN32	
	void InitW(const WCHAR* fileName);
	void Init(HMODULE hModule);
	LPCWSTR GetFileName();
#else
	LPCSTR  GetFileName();
#endif
	void InitA(const char*  fileName);
	void WriteMsg(const char* templ, ...);
	void WriteMsgV(const char*  pTempl, va_list arg);
	void WriteMsgV(const WCHAR* pTempl, va_list arg);
	void OpenFile();
	void CloseFile();
	
	void WriteRawData(const void* pData, DWORD len);

	BOOL m_silent;
	bool m_exactTime;
	bool m_console;

protected:
	void OnError(LPCSTR error);

	// variables
private:
	RLMutex	m_csWrite;
	bool	m_initialized;	

#ifdef _WIN32
	HANDLE	m_hFile;
	WCHAR	m_fileName[MAX_PATH];
#else
	FILE*	m_hFile;
	char	m_fileName[280];
#endif

};


class RLLogEx: public RLLog
{
	// methods
public:
	RLLogEx();
	inline int  GetLogLevel() { return m_level; }
	void SetLogLevel(DWORD level);
	void WriteError(const char* templ, ...);
	void WriteInfo (const char* templ, ...);

	//inline DWORD GetLevel() { return m_level; }
	
private:
	void WriteMsgV_withPrefix(const char* pPrefix, const char*  pTempl, va_list arg);

	// variables
protected:
	DWORD  m_level;
};

#endif // !defined(RL_LOG_H_INCLUDED_)
