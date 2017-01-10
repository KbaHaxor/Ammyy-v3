#ifndef _RL_HTTP_H_INCLUDED_
#define _RL_HTTP_H_INCLUDED_

#include <windows.h>
#include <wininet.h>
#include "RLStream.h"

class RLHttp
{
public:
	RLHttp();
	~RLHttp();

	bool  Get(LPCSTR lpszURL);
	bool  Post(LPCSTR lpszURL, RLStream* pOptionalData);
	void  CloseRequest();	

	bool  ReadData(RLStream& streamOut);
	bool  ReadDataOneTime(LPVOID pBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead);
	DWORD GetContentLength();
	DWORD GetStatusCode();

	inline DWORD GetError()    const { return m_dwLastError; }
	inline bool  IsConnected() const { return m_hRequest!=NULL; }

public:
	static void ParseURL(const TCHAR* szURL, TCHAR* szProtocol, TCHAR* szAddress, WORD &nPort, TCHAR* szURI);

private:	
	void  OnError();
	void  CloseInternet();
	void  CloseConnection();
	void  SetOption(DWORD dwOption, DWORD dwValue);
	void  SetOption(DWORD dwOption, LPVOID data, DWORD dataSize);
	bool  OpenConnection(LPCSTR lpszServerName, INTERNET_PORT nServerPort);
	bool  SendRequest(LPCSTR lpszVerb,LPCSTR lpszAction,RLStream* pOptionalData);	

	HINTERNET m_hInternet;
	HINTERNET m_hConnection;
	HINTERNET m_hRequest;

	DWORD     m_dwLastError;

public:
	CStringA  m_szHeader;
	LPCSTR    m_lpszReferer;

	static bool		m_proxyIE;		// take IE proxy settings
	static WORD		m_proxyPort;	// if (m_proxyPort==0 and !m_proxyIE) - no proxy
	static CStringA	m_proxyHost;	
	static CStringA	m_proxyUsername;
	static CStringA	m_proxyPassword;
};

#endif // _RL_HTTP_H_INCLUDED_
