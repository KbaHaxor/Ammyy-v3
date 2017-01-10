#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include <map>
#include <vector>


class CHttpAuthObject;


class CHttpClient
{
public:
	enum HC_ERROR
	{
		ERR_BAD_STATUS_CODE		= 0x00190100L,	// failed to find or bad status code in HTTP response
		ERR_BAD_RESPONSE		= 0x00190101L,	// failed to parse response header (bad response)
		ERR_BAD_BODY_LENGTH		= 0x00190102L,	// bad length of received HTTP Body (did not matched with "Content-Length")
		ERR_HTTP_CLIENT_ERROR	= 0x00190103L,	// HTTP Response receives 4xx status code.
		ERR_HTTP_SERVER_ERROR	= 0x00190104L,	// HTTP Response receives 5xx status code.
		ERR_HTTP_AUTH_ERROR		= 0x00190105L,	// HTTP Authorization error (system error in HttpLogin())
	};

	typedef std::vector<CStringA>	string_array_t;

public:
	CHttpClient();
	virtual ~CHttpClient();

	const SOCKET& GetSocket() const				{ return m_conn; }
	bool  IsConnected() const					{ return (m_conn != INVALID_SOCKET); }
	//DWORD GetLastError() const					{ return m_dwLastError; }

private:
	// Send HTTP request and receives response header (few versions). 
	// NOTE: In case of additional authorization required, function automatically
	// will re-send request with authorization info.
	// When @szMethod = NULL, "GET" request will be used.
	// When @szProtocol = NULL, protocol will be choosed based on @port.
	// When dataLen < 0, then "Content-length" key will not be appended.
	//bool SendRequest1(LPCSTR szURL, LPCSTR szMethod = NULL, LPCSTR szExtraHeaders = NULL,
	//				  LPCSTR szDataType = NULL, LPCBYTE lpData = NULL, int dataLen = 0);

	bool SendRequest2(LPCSTR szHost, LPCSTR szURI, WORD port = 80, LPCSTR szMethod = NULL, LPCSTR szExtraHeaders = NULL, 
					  LPCSTR szDataType = NULL, LPCBYTE lpData = NULL, int dataLen = 0, LPCSTR szProtocol = NULL);

public:
	// Wrapper for SendRequest to establish persistent connection via https proxy
	bool ConnectByHttpsProxy(LPCSTR szServer, WORD port = 443);

	// (re-)execute current request (with optional changes in request header)
	bool ExecuteRequest(LPCSTR lpszExtraHeaders = NULL);

	// Returns the value of a response header field
	bool GetHeaderValue(LPCSTR szFieldName, CStringA& strValue) const;

private:
	// Close current session
	void Close();

	// Returns status code of the last session
	LONG GetStatusCode() const					{ return m_nStatus; }
	// Returns entire status line of the last session (first line of HTTP response).
	CStringA GetStatusLine() const;

	// get value of "Content-Length" field from HTTP Response
	LONG GetContentLength() const;
	// get value of "Connection" field (or "Proxy-Connection" field) from HTTP Response.
	CStringA GetConnectionValue() const;

	// access to HTTP response header and its length (when there is an active session)
	const char* GetHeader() const				{ return (m_dwHeaderLen ? m_answer.GetBufferRd() : NULL); }
	DWORD GetHeaderLength() const				{ return ((m_dwHeaderLen > 1) ? m_dwHeaderLen - 2 : 0); }	// exclude header's separator
	// access to HTTP response body and its length (when there is an active session)
	const char* GetBody() const					{ return (m_dwBodyLen ? m_answer.GetBufferRd() + m_dwHeaderLen : NULL); }
	DWORD GetBodyLength() const					{ return m_dwBodyLen; }

	// Advanced functions - use it carefully!
	//
	bool WriteSocket(LPCBYTE lpData, int len);
	int  ReadSocket(LPBYTE lpData, int maxLen);

	// Try to do authorization to HTTP server or proxy for the current session
	// (NOTE: it could be used only inside active session - after SendRequst())).
	bool HttpLogin(bool bProxy);


	// Returns the start of next line.
	LPCSTR NextLine(LPCSTR pCurLine, LPCSTR pBufEnd);

	static LONG ParseStatusCode(LPCSTR szResponse, DWORD len);
	static LPCSTR GetDefaultProtocol(DWORD port);	// return default protocol for specified port

private:
	bool ConnectSocket();
	CStringA BuildRequest(LPCSTR szMethod, LPCSTR szExtraHeaders, LPCSTR szDataType);

	void RestartRequest();

	bool ReadHttpResponse();
	bool ReadHttpBody();
	void ParseResponseHeader(LPCSTR szResponse, DWORD len);
	bool ProcessResponse(bool bTryLogon = false);

	LPCSTR ParseHeaderField(LPCSTR pStart, LPCSTR pBufEnd);
	// get the list of authenticate types supported by server
	void ParseAuthTypes(bool bProxy, string_array_t& list);

	bool IsCurrentMethod(LPCSTR szMethod);

	// close current HTTP connection
	void CloseSocket()
	{
		if (m_conn != INVALID_SOCKET)
		{
			::closesocket(m_conn);
			m_conn = INVALID_SOCKET;
		}
	}

public:
	SOCKET Detach()
	{
		SOCKET s = m_conn;
		m_conn = INVALID_SOCKET;
		return s;
	}

public:
	// Proxy info - if m_proxyAddress is empty, there is no proxy.
	CStringA		m_proxyAddress;
	WORD			m_proxyPort;
	CStringA		m_userName;
	CStringA		m_password;

	DWORD			m_readSocketTimeout;	// read socket timeout (in ms)
	bool			m_bAutoLogon;			// when true (default) and server returns 401/407 code 
											// (need authorization), CHttpClient automatically
											// will try to do authorization via HttpLogin().


	// Field names related to authenticate types (direct and proxy)
	const static char*	g_szAuthTypeFields[];

	DWORD			m_dwLastError;	// last error, could be one of WSAxxx error codes or one of errors from HC_ERROR enum.


protected:
	SOCKET			m_conn;

	// Components of the current request (cached from last "SendRequest" call)
	// (used when need to resend request).
	CStringA		m_curHost;
	CStringA		m_curUri;			// url part after "host" and "port"
	CStringA		m_curProtocol;		// Protocol: "http", "ftp", etc.
	WORD			m_curPort;

	// next members only make sense only while SendRequest() works.
	CStringA		m_request;			// current HTTP request
	LPCBYTE			m_lpPostData;		// post data pointer and its size from SendRequest()
	int				m_postDataLen;		// (when m_postDataLen < 0, this section is skipped).


	// Buffer with received HTTP response and related variables. 
	// Members except @m_answer make sense only after ReadHttpResponse() call.
	RLStream		m_answer;			// received HTTP response
	DWORD			m_dwHeaderLen;		// Length of raw HTTP header in @m_answer (including header delimiter '\r\n')
	DWORD			m_dwBodyLen;		// Length of received part of body 

	LONG			m_nStatus;			// Status code from HTTP response (-1 if not ready yet)


	// functor to compare case-insensitive CStringA objects
	struct lessNoCase
	{
		bool operator()(const CStringA& lStr, const CStringA& rStr) const
		{
			return (lStr.CompareNoCase(rStr) < 0);
		}
	};

	typedef std::map< const CStringA, CStringA, lessNoCase >	key_map_t;
	typedef std::pair< const CStringA, CStringA >				string_pair_t;

	key_map_t		m_headerMap;		// Map with parsed fields from HTTP response.

	// Members to support authorization
	CHttpAuthObject*	m_authProviders[2];	// Current list of registered authorization providers
};


// Abstract class used as a root for providers of various HTTP Authentication schemes
class CHttpAuthObject
{
public:
	virtual ~CHttpAuthObject()	{};
	// send authorization request to CHttpClient and receive answer.
	// returns true if authorization request has been sent and answer received.
	virtual bool Logon(CHttpClient& conn, const CStringA& authType, 
					   const CStringA& userName, const CStringA& password, bool bProxy = false) = 0;

	static CStringA MyBase64Encode(const CStringA& strIn);
};


#endif // !defined(__HTTP_CLIENT_H__)
