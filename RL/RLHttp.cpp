//
// * according to HTTP Proxy authorization specs, connect should be opened with
//   keep-alive option. Otherwise, proxy authorization will not work.
//   In our case situation even worse. When we try to connect to proxy 
//   with authorization but without 'keep-alive' flag, WinInet function 
//   HttpSendRequestA block execution till timeout reached.
//   It could be bug in WinINet API or lack of config options (need to figure out).
//   SOLUTION: force to set 'keep-alive' flag when proxy enabled.
//
// * Sequence for proxy authorization currently performed in 2 passes.
//   At first, usual request is sent (without any authorization info).
//   Server answers with '401' or '407' status code and provide the list of
//   supported authorization methods with hash-keys (whenever its requires).
//   Then, client encrypt login information with received 'hash-key' and resent
//   request again.
//   Unfortunately, its possible to send authorization request in 1 pass only
//   for basic authorization (thats is not safe and disabled by default in ISA).
//   So, probably now there is an only one way to do authorization: 2 passes.
//
// * According to WinINet documentation, its possible to set options
//   INTERNET_OPTION_PROXY_USERNAME and INTERNET_OPTION_PROXY_PASSWORD for the
//   any type of HINTERNET handlers. In this case, best way is root handler 
//   received from InternetOpen(). But, unfortunately InternetSetOption() returns
//   error INCORRECT_HANDLE_TYPE in this case, so this options set per session.


#include "stdafx.h"
#include "RLHttp.h"

#include <tchar.h>

#pragma comment(lib,"wininet")

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


bool	 RLHttp::m_proxyIE   = false;
WORD	 RLHttp::m_proxyPort = 0;
CStringA RLHttp::m_proxyHost;	
CStringA RLHttp::m_proxyUsername;
CStringA RLHttp::m_proxyPassword;


RLHttp::RLHttp()
: m_hInternet(NULL),
  m_hConnection(NULL),
  m_hRequest(NULL),
  m_dwLastError(0)
{
	m_lpszReferer = NULL;
	m_szHeader =  "Content-Type: application/x-www-form-urlencoded\r\n"
				/*"Accept-Language:ru\r\n"
				 /"Accept-Encoding:gzip, deflate"*/ ;
}

RLHttp::~RLHttp()
{
 	CloseInternet();
}

void RLHttp::OnError()
{
	m_dwLastError = ::GetLastError();
}


void RLHttp::CloseInternet()
{
	CloseConnection();
	
	if (m_hInternet != NULL) {
		::InternetCloseHandle(m_hInternet);
		m_hInternet = NULL;
	}
}

void RLHttp::SetOption(DWORD dwOption, DWORD dwValue)
{
	if (::InternetSetOption(m_hConnection, dwOption, &dwValue, sizeof(dwValue))==FALSE) {
		_log.WriteError("RLHttp::SetOption()#1 option=%d error=%d", dwOption, ::GetLastError());
	}
}

void RLHttp::SetOption(DWORD dwOption, LPVOID data, DWORD dataSize)
{
	if (::InternetSetOption(m_hConnection, dwOption, data, dataSize)==FALSE) {
		_log.WriteError("RLHttp::SetOption()#2 option=%d error=%d", dwOption, ::GetLastError());
	}

}

bool RLHttp::OpenConnection(LPCSTR lpszServerName, INTERNET_PORT nServerPort)
{

	if (m_hInternet==NULL) 
	{
		// Depend on settings, choose right way to open connection
		if (m_proxyIE) {				// use IE proxy settings
			m_hInternet = ::InternetOpenA( NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		}
		else if (!m_proxyPort) {		// Do not use proxy
			m_hInternet = ::InternetOpenA( NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
		}
		else {							// Use manually specified proxy
			CStringA proxyList;
			proxyList.Format("%s:%d", (LPCSTR)m_proxyHost, (int)m_proxyPort);

			m_hInternet = ::InternetOpenA( NULL, INTERNET_OPEN_TYPE_PROXY, (LPCSTR)proxyList, NULL, 0);
		}		

		if (m_hInternet==NULL) {
			OnError();
			return false;
		}
	}


	if (m_hConnection!=NULL) return true;

	m_hConnection = ::InternetConnectA(m_hInternet, lpszServerName, nServerPort, NULL, NULL,  INTERNET_SERVICE_HTTP, 0,1u);

	if (m_hConnection==NULL) {
		OnError();
		return false;
	}

	// Setup authorization info for manual proxy
	if ((m_proxyIE || m_proxyPort) && !m_proxyUsername.IsEmpty())
	{
		SetOption(INTERNET_OPTION_PROXY_USERNAME, (LPVOID) (LPCSTR)m_proxyUsername, m_proxyUsername.GetLength());

		if (!m_proxyPassword.IsEmpty())
			SetOption(INTERNET_OPTION_PROXY_PASSWORD, (LPVOID) (LPCSTR)m_proxyPassword, m_proxyPassword.GetLength());
	}

	// set timeouts
	SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 60*1000);
	SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 60*1000);
	SetOption(INTERNET_OPTION_SEND_TIMEOUT,    60*1000);		

	return true;
}

void RLHttp::CloseConnection()
{
	CloseRequest();

	if (m_hConnection != NULL) {
		::InternetCloseHandle(m_hConnection);
		m_hConnection = NULL;
	}
}


void RLHttp::CloseRequest()
{
	if (m_hRequest !=NULL) {
		::InternetCloseHandle(m_hRequest);
		m_hRequest = NULL;
	}
}


bool RLHttp::SendRequest(LPCSTR lpszVerb, LPCSTR lpszURL, RLStream* pOptionalData)
{	
	TCHAR	szHost[128];
	TCHAR	szProtocol[32];
	WORD	wPort;
	TCHAR	szURI[128];
	
	ParseURL(lpszURL, szProtocol, szHost, wPort, szURI);	

	CloseConnection();

	_log.WriteInfo("RLHttp::SendRequest()#3");

	if (!OpenConnection(szHost, wPort)) return false;

	LPCSTR AcceptTypes[] = {NULL}; //{ TEXT("*/*"), NULL};
	DWORD dwFlags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_COOKIES;

	if (strcmp(szProtocol, "https")==0)
		  dwFlags |= INTERNET_FLAG_SECURE|INTERNET_FLAG_IGNORE_CERT_CN_INVALID;

	// TODO: I was afraid to change it for defalut case (m_proxyIE==true) 
	if (!m_proxyUsername.IsEmpty()) //(m_proxyIE || m_proxyPort)
		dwFlags |= INTERNET_FLAG_KEEP_CONNECTION;	// Requirements for proxies with authorization

	_log.WriteInfo("RLHttp::SendRequest()#5");
		
	m_hRequest = ::HttpOpenRequestA(m_hConnection, lpszVerb, szURI, NULL, m_lpszReferer, AcceptTypes, dwFlags, NULL);

	if (m_hRequest==NULL) {
		OnError();
		return false;
	}
		
	LPVOID lpOptional		= (pOptionalData==NULL)       ? NULL : pOptionalData->GetBuffer();
	DWORD  dwOptionalLength = (pOptionalData==NULL)       ?  0   : pOptionalData->GetLen();
	DWORD  dwHeadersLength  = m_szHeader.GetLength();

	_log.WriteInfo("RLHttp::SendRequest()#8 %X %X %X %X", m_hRequest, dwHeadersLength, lpOptional, dwOptionalLength);

	if (::HttpSendRequestA(m_hRequest, (LPCSTR)m_szHeader, dwHeadersLength, lpOptional, dwOptionalLength) == FALSE)
	{				
		OnError();
		_log.WriteInfo("RLHttp::SendRequest()#8_ERROR");
		CloseRequest();
		return false;
	}	

	_log.WriteInfo("RLHttp::SendRequest()#9");

	return true;
}


DWORD RLHttp::GetContentLength()
{
	ASSERT(m_hRequest!=NULL);

	DWORD dwData = 0;
	DWORD dwDataLength = sizeof(dwData);
	DWORD dwIndex = 0;

	VERIFY(::HttpQueryInfoA(m_hRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwData, &dwDataLength, &dwIndex) != FALSE);
	
	return dwData;
}


DWORD RLHttp::GetStatusCode()
{
	ASSERT(m_hRequest!=NULL);

	DWORD dwData = 0;
	DWORD dwDataLength = sizeof(dwData);
	DWORD dwIndex = 0;

	VERIFY(::HttpQueryInfoA(m_hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwData, &dwDataLength, &dwIndex) != FALSE);
	
	return dwData;
}


bool RLHttp::Get(LPCSTR lpszURL)
{
	return SendRequest("GET",lpszURL,NULL);
}


bool RLHttp::Post(LPCSTR lpszURL,RLStream* pOptionalData)
{
	return SendRequest("POST",lpszURL,pOptionalData);
}																			   


bool RLHttp::ReadData(RLStream& stream)
{
	if (!IsConnected()) return false;

	DWORD dwBytesReadTotal = 0;
	DWORD dwSize = 4096;

	stream.Reset();
	stream.SetMinCapasity(dwSize*2);

	bool bReadOk;
	
	while(true) {
		DWORD dwBytesReadNow = 0;

		stream.SetMinExtraCapasity(dwSize);
		void* pBuffer = (char*)stream.GetBuffer() + dwBytesReadTotal;
		bReadOk = ReadDataOneTime(pBuffer, dwSize, &dwBytesReadNow);

		if (dwBytesReadNow==0 || !bReadOk) { break;}
		
		dwBytesReadTotal += dwBytesReadNow;
		stream.SetLen(dwBytesReadTotal);
	}

	return true;
}


bool RLHttp::ReadDataOneTime(LPVOID pBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead)
{
	ASSERT(m_hRequest!=NULL);
	if (::InternetReadFile(m_hRequest, pBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead) == FALSE) {
		OnError();
		return false;
	}
	return true;
}


void RLHttp::ParseURL(const TCHAR* szURL, TCHAR* szProtocol, TCHAR* szHost, WORD &nPort, TCHAR* szURI)
{
	if (_tcslen(szURL)==0) return;

	const TCHAR* pPosition = _tcsstr(szURL, _TEXT("://"));

	if (pPosition != NULL) {
		if(szProtocol){
			DWORD len = pPosition-szURL;
			_tcsncpy(szProtocol, szURL, len);
			szProtocol[len]=0;
		}
		pPosition+=3;
	}else {
		if(szProtocol) {_tcscpy(szProtocol, _T("http"));}
		pPosition=szURL;
	}

	nPort = (strcmp(szProtocol, "https")==0) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

	const TCHAR* pUriBegin = _tcsstr(pPosition, _TEXT("/"));
	if (pUriBegin==NULL) pUriBegin = szURL + _tcslen(szURL);

	const TCHAR* pHostEnd = pUriBegin;
	
	for(const TCHAR* p=pPosition; p<pHostEnd; p++){
		if(*p == _T(':')) { // find PORT

			TCHAR szPort[32]= _T("");
			_tcsncpy(szPort, p+1, pHostEnd-p-1);
			nPort=(WORD)_ttol(szPort);			

			pHostEnd = p;
			break;
		}
	}

	int count = pHostEnd-pPosition;
	_tcsncpy(szHost, pPosition, count);
	szHost[count]=0;

	_tcscpy(szURI, pUriBegin);
}
