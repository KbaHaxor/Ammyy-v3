#include "stdafx.h"
#include "HttpClient.h"
#include "HttpBasicAuthObject.h"
#include "HttpNtlmAuthObject.h"
#include "../TCP.h"
#include "../../RL/RLBase64Coder.h"

#include <errno.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif



CStringA CHttpAuthObject::MyBase64Encode(const CStringA& strIn)
{
	RLBase64Coder encoder(NULL);

	CStringA res = encoder.Encode(strIn);
	int		pad = (res.GetLength() % 4);
	if (pad > 0)
		res += CStringA('=', 4 - pad);
	return res;
}



// Constants used by CHttpClient class

const static LPCSTR szHttpHeaderEndMark  = "\r\n\r\n";

#define READ_BUFFER_SIZE			2048
#define DEFAULT_READ_SOCKET_TIMEOUT	8000		// max period (in ms) for recv() while empty reads will be repeated


// Field names related to authenticate types (direct and proxy)
const char*	CHttpClient::g_szAuthTypeFields[] = { "www-authenticate", "proxy-authenticate" };




CHttpClient::CHttpClient()
{
	m_readSocketTimeout = DEFAULT_READ_SOCKET_TIMEOUT;
	m_bAutoLogon = true;

	m_conn = INVALID_SOCKET;
	m_proxyPort = 0;
	m_dwLastError = 0;
	m_curPort = 0;
	m_lpPostData = NULL;
	m_postDataLen = 0;
	m_dwHeaderLen = m_dwBodyLen = 0;
	m_nStatus = NULL;

	for (int i=0; i<sizeof(m_authProviders)/sizeof(m_authProviders[0]); i++) 
		m_authProviders[i] = NULL;
}

CHttpClient::~CHttpClient()
{
	Close();
	
	// Free memory used by Authorization Providers
	for (int i=0; i<sizeof(m_authProviders)/sizeof(m_authProviders[0]); i++) {
		if (m_authProviders[i]) delete m_authProviders[i];
	}
}


// Wrapper for SendRequest to establish persistent connection via https-proxy.
bool CHttpClient::ConnectByHttpsProxy(LPCSTR szServer, WORD port)
{
	// extra HTTP headers for HTTP proxy
	LPCSTR szHttpProxyHeader = "Proxy-Connection: Keep-Alive\r\nPragma: no-cache\r\n";
	return (SendRequest2(szServer, NULL, port, "CONNECT", szHttpProxyHeader) && IsConnected());
}


// Send HTTP request and receive response header (wrapper). 
//bool CHttpClient::SendRequest1(LPCSTR szURL, LPCSTR szMethod, LPCSTR szExtraHeaders, LPCSTR szDataType, LPCBYTE lpData, int dataLen)
//{
//	char	szProtocol[32];
//	char	szHost[512];
//	char	szURI[4096];
//	WORD	port = 80;		// default port
//	szProtocol[0] = szHost[0] = szURI[0] = 0;
//	RLHttp::ParseURL(szURL, szProtocol, szHost, port, szURI);	// TODO: Make ParseURL more secure to protect buffer's overflow
//
//	return SendRequest2(szHost, szURI, port, szMethod, szExtraHeaders, szDataType, lpData, dataLen, szProtocol);
//}


// Send HTTP request and receive response header. 
// NOTE: In case of additional authorization required, function automatically
// will re-send request with authorization info.
// When @szMethod = NULL, "GET" request will be used.
// When @szProtocol = NULL, protocol will be chosen based on @port.
// NOTE: @szExtraHeaders (if provided), should be ended with "\r\n".
// When dataLen < 0, then "Content-length" key will not be appended.
bool CHttpClient::SendRequest2(LPCSTR szHost, LPCSTR szURI, WORD port, LPCSTR szMethod, LPCSTR szExtraHeaders, 
							   LPCSTR szDataType, LPCBYTE lpData, int dataLen, LPCSTR szProtocol)
{
	// Validate input parameters
	if (!szHost || !*szHost)
	{
		_log.WriteError("CHttpClient::SendRequest2(%s, %s, %d) error: invalid host name.\n", szHost, szURI, port);
		m_dwLastError = WSAEINVAL;
		return false;
	}

	if (!port) port = 80;

	// can we reuse connection? (if proxy is used or same pair of host and port)
//	CloseSocket();	// TODO: decide should we supports multiple requests per connection
	if (m_conn != INVALID_SOCKET)
	{
		if (m_dwLastError || 
			(m_proxyAddress.IsEmpty() && 
			 ((m_curHost.CompareNoCase(szHost) != 0)) || (m_curPort != port)))
		{
			CloseSocket();
		}
	}



	// normalize URI (any URI should starts with '/', except ones that starts with '*') 
	const char	uri_prefix = (szURI ? szURI[0] : 0);
	if ((uri_prefix == '/') || (uri_prefix == '*'))
		m_curUri = szURI;
	else
	{
		m_curUri = '/';
		m_curUri += szURI;
	}

	m_curHost = szHost;
	m_curPort = port;
	m_curProtocol = (szProtocol && *szProtocol) ? szProtocol : GetDefaultProtocol(port);
	
	m_postDataLen = (lpData ? dataLen : min(dataLen, 0));
	m_lpPostData = lpData;


	// Prepare HTTP request 
	m_request = BuildRequest(szMethod, szExtraHeaders, szDataType);

	bool isSuccess = false;
	do 
	{
		if (!ExecuteRequest())
			break;

		if (!ProcessResponse(m_bAutoLogon))
		{
			ASSERT(m_dwLastError != 0);		// we should have errno

			if (m_nStatus != -1)
				_log.WriteError("CHttpClient::SendRequest2() failed: HTTP Server returns error %s.", (LPCSTR) GetStatusLine());
			else
				_log.WriteError("CHttpClient::SendRequest2() failed: ERROR %u during authorization.", m_dwLastError);
			CloseSocket();
			break;
		}

		isSuccess  = true;
	} 
	while (0);

	// Invalidate PostData buffer
	m_lpPostData = NULL;
	m_postDataLen = 0;
	return isSuccess;
}


// close current session
void CHttpClient::Close()
{
	CloseSocket();

	m_dwLastError = 0;
	m_curHost.Empty();
	m_curUri.Empty();
	m_curProtocol.Empty();
	m_curPort = 0;

	m_request.Empty();
	m_lpPostData = NULL;
	m_postDataLen = 0;

	m_answer.Free();
	m_dwHeaderLen = m_dwBodyLen = 0;
	m_nStatus = -1;

	m_headerMap.clear();
}


// (re-)execute current request (with optional changes in request header)
bool CHttpClient::ExecuteRequest(LPCSTR lpszExtraHeaders)
{
	do
	{
		RestartRequest();
		if (!ConnectSocket())
			break;

		// Send HTTP request (and optional body) to server
		bool success;
		if (!lpszExtraHeaders)
			success = WriteSocket((LPCBYTE) (LPCSTR) m_request, m_request.GetLength());
		else
		{
			// extends current request with additional headers (insert headers before last "\r\n")
			CStringA newRequest = m_request;
			newRequest.Insert(newRequest.GetLength() - 2, lpszExtraHeaders);
			success = WriteSocket((LPCBYTE) (LPCSTR) newRequest, newRequest.GetLength());
		}

		if (success && (m_postDataLen > 0))
			success = WriteSocket(m_lpPostData, m_postDataLen);

		if (!success)
		{
			_log.WriteError("CHttpClient::ExecuteRequest() failed: ERROR %u while sending data to socket.", m_dwLastError);
			break;
		}

		// Read HTTP response and body
		if (!ReadHttpResponse() || !ReadHttpBody())
		{
			_log.WriteError("CHttpClient::ExecuteRequest() failed: ERROR %u while receiving data from socket.", m_dwLastError);
			break;
		}


		// Lets be polite, close connection if server ask this.
		if (GetConnectionValue().CompareNoCase("close") == 0)
			CloseSocket();

		return true;
	} 
	while (0);

	// Some error happened
	CloseSocket();
	ASSERT(m_dwLastError != 0);		// do we miss something?
	return false;
}




// Write data to socket (function blocks execution while all data sent).
bool CHttpClient::WriteSocket(LPCBYTE lpData, int len)
{
	ASSERT(m_conn != INVALID_SOCKET);
	int res = ::send(m_conn, (const char*) lpData, len, 0);
	m_dwLastError = ::WSAGetLastError();
	return (res == len);
}


// Read up to @maxLen bytes of data from socket into the buffer @lpData.
// Function blocks execution until any data received or timeout occurred or
// any error happened.
// Parameters:
//  lpData         - buffer for incoming data;
//  maxLen         - size of buffer (in bytes).
// Return value:
//   > 0  - number of received bytes;
//   ==0  - no more data, connection closed;
//   < 0  - failed to received data due to error (check m_dwLastError for details).
int CHttpClient::ReadSocket(LPBYTE lpData, int maxLen)
{
	ASSERT(m_conn != INVALID_SOCKET);
	ASSERT(lpData && (maxLen > 0));

	struct timeval	tm;
	tm.tv_sec	= m_readSocketTimeout / 1000;
	tm.tv_usec	= (m_readSocketTimeout % 1000) * 10;

	fd_set	rdset;
	FD_ZERO(&rdset);
	FD_SET(m_conn, &rdset);

	// check socket for arrived data
	int	res = select(m_conn + 1, &rdset, NULL, NULL, &tm);
	if (res > 0)	// there is something in socket
		res = ::recv(m_conn, (char*) lpData, maxLen, 0);
	else if (res == 0)
	{
		_log.WriteInfo("CHttpClient::ReadSocket(): no data received during %u ms.", m_readSocketTimeout);
		m_dwLastError = WSAETIMEDOUT;
		return 0;
	}

	m_dwLastError = (res >= 0 ? 0 : WSAGetLastError());
	return res;
}


// Returns entire status line of the last session (first line of HTTP response).
CStringA CHttpClient::GetStatusLine() const
{
	// search for the start of status line (after 1st word)
	char*	szBuf = m_answer.GetBufferRd();
	LPCSTR	szEnd = szBuf + m_answer.GetAvailForReading();

	while ((szBuf < szEnd) && !isspace(*szBuf))
		++szBuf;

	// skip spaces also
	while ((szBuf < szEnd) && isspace(*szBuf))
		++szBuf;

	// search for the end of status line
	int		len  = szEnd - szBuf;
	szEnd = (LPCSTR) memchr(szBuf, '\n', (size_t) len);
	if (szEnd)
	{
		len = (szEnd - szBuf);
		if ((len > 0) && (szBuf[len-1] == '\r'))
			--len;	// exclude also '\r'
	}

	return CStringA(szBuf, len);
}


// Function tries to locate the start of next line.
// Parameters:
//  pCurLine  - position inside the current line;
//  pBufEnd   - pointer of first byte after the end of buffer (usually points to '\0' char);
// Returns pointer to the 1st char of next line if its exists; otherwise returns NULL.
LPCSTR CHttpClient::NextLine(LPCSTR pCurLine, LPCSTR pBufEnd)
{
	int		len = (pBufEnd - pCurLine);
	if (!pCurLine || (len <= 0))
		return NULL;	// no more lines

	LPCSTR ptr = (LPCSTR) memchr(pCurLine, '\n', (size_t) len);
	if (!ptr || (ptr + 1 >= pBufEnd))
		return NULL;	// no more lines, current line is a last one

	return ptr + 1;
}


// Returns the value of a response header field
bool CHttpClient::GetHeaderValue(LPCSTR szFieldName, CStringA& strValue) const
{
	CStringA strKey(szFieldName);
	key_map_t::const_iterator	it = m_headerMap.find(strKey);
	if (it != m_headerMap.end())
	{
		strValue = it->second;
		return true;
	}
	else
	{
		strValue.Empty();
		return false;
	}
}


// get value of "Content-Length" field from HTTP Response
LONG CHttpClient::GetContentLength() const
{
	CStringA strValue;
	if (!GetHeaderValue("content-length", strValue))
		return -1;

	LONG	len = strtol((LPCSTR) strValue, NULL, 10);
	return ((errno != ERANGE) ? len : -1);
}

// get value of "Connection" field from HTTP Response (or "Proxy-Connection" field)
CStringA CHttpClient::GetConnectionValue() const
{
	CStringA res; 
	GetHeaderValue("connection", res) || GetHeaderValue("proxy-connection", res); 
	return res;
}


// Function scans text buffer @szResponse (HTTP Header) for the status code.
// Status code passed in the first text line of HTTP Header and has next format:
//   "message DDD ...".
LONG CHttpClient::ParseStatusCode(LPCSTR szResponse, DWORD len)
{
	if (!szResponse || !len)
		return -1;

	LPCSTR	szEnd = szResponse + len;

	// Status code passed in a next format: "message DDD ...", 
	// so lets search for first 'space' symbol.
	while ((szResponse < szEnd) && !isspace(*szResponse))
		++szResponse;

	// skip spaces also
	while ((szResponse < szEnd) && isspace(*szResponse))
		++szResponse;

	if ((szResponse >= szEnd) || !isdigit(*szResponse))
		return -1;	// status code is not found

	char*	tokenEnd;
	LONG	code = strtol(szResponse, &tokenEnd, 10);
	if ((errno == ERANGE) || (tokenEnd == szResponse))
		return -1;	// bad status code

	ASSERT(tokenEnd < szEnd);
	return code;
}


// return default protocol for specified port
LPCSTR CHttpClient::GetDefaultProtocol(DWORD port)
{
	const static char*	c_ftp	= "ftp";
	const static char*	c_https	= "https";
	const static char*	c_http	= "http";

	switch (port)
	{
	case 21:	return c_ftp;
	case 443:	return c_https;
	default:	return c_http;
	}
}


// (Re) Establish connection
//
bool CHttpClient::ConnectSocket()
{
	if (m_conn != INVALID_SOCKET)
	{
		// can we reuse the current connection?
		CStringA val = GetConnectionValue();
		if (val.IsEmpty() || (val.CompareNoCase("keep-alive") == 0))
			return true;	// re-use current connection

		if (val.CompareNoCase("close") != 0)
			_log.WriteError("CHttpClient::ConnectSocket(): key 'Connection' has unexpected value '%s'", (LPCSTR) val);

		CloseSocket();
	}

	m_dwLastError = 0;

	LPCSTR host = (m_proxyAddress.IsEmpty()) ? m_curHost : m_proxyAddress;
	WORD   port = (m_proxyAddress.IsEmpty()) ? m_curPort : m_proxyPort;

	m_conn = TCP::Connect2(host, port, m_dwLastError);

	if (m_conn == INVALID_SOCKET)
	{
		return false;
	}

	return true;
}


// reset current request
void CHttpClient::RestartRequest()
{
	m_dwHeaderLen = m_dwBodyLen = 0;
	m_nStatus = -1;
	m_dwLastError = 0;
	m_answer.Reset();
	m_headerMap.clear();
}


// Build string buffer with a HTTP request.
CStringA CHttpClient::BuildRequest(LPCSTR szMethod, LPCSTR szExtraHeaders, LPCSTR szDataType)
{	
	ASSERT(!m_curHost.IsEmpty());

	if (!szMethod || !*szMethod)
		szMethod = "GET";	// default HTTP request

	CStringA strHost;
	strHost.Format("%s:%d", (LPCSTR) m_curHost, m_curPort);


	CStringA tempS;
	CStringA res;
	if (stricmp(szMethod, "CONNECT") == 0)	// "CONNECT" method. Special format
	{
		res.Format("%s %s HTTP/1.1\r\nHost: %s\r\n", 
					szMethod, (LPCSTR) strHost, (LPCSTR) strHost);
	}
	else if (!m_proxyAddress.IsEmpty())	// request through the proxy
	{
		res.Format("%s %s://%s%s HTTP/1.1\r\nHost: %s\r\n", 
					szMethod, (LPCSTR) m_curProtocol, (LPCSTR) strHost, (LPCSTR) m_curUri, (LPCSTR) strHost);
	}
	else	// direct request
	{
		res.Format("%s %s HTTP/1.1\r\nHost: %s\r\n", 
					szMethod, (LPCSTR) m_curUri, (LPCSTR) strHost);
	}

	if (m_postDataLen >= 0)
	{
		tempS.Format("Content-Length: %d\r\n", m_postDataLen);
		res += tempS;
	}

	if (szDataType)
	{
		res += "Content-Type: ";
		res += szDataType;
		res += "\r\n";
	}

	if (szExtraHeaders)
	{
		res += szExtraHeaders;	
		if (res.Find("\r\n", res.GetLength() - 2) == -1)	// szExtraHeaders missed "\r\n"
			res += "\r\n";
	}


	res += "\r\n";	// mark end of request
	return res;
}


// Keep reading data from the socket till we've received entire HTTP Request.
// After that, function will parse HTTP Request.
// Function returns true if HTTP Request received and parsed; otherwise - returns false.
bool CHttpClient::ReadHttpResponse()
{
	ASSERT(m_conn != INVALID_SOCKET);

	BYTE	buf[READ_BUFFER_SIZE];

	// Reads data from socket till we receives end-of-header mark
	m_answer.SetMinCapasity(READ_BUFFER_SIZE);	// usually, its a well-enough size
	while (1)
	{
		int cnt = ReadSocket(buf, sizeof(buf));
		if (cnt <= 0)
			return false;

		m_answer.SetMinExtraCapasity(cnt + 1);	// buf + '\0' terminator
		m_answer.AddRaw(buf, cnt);
		m_answer.GetBufferWr()[0] = '\0';

		// do we have a '\r\n\r\n' sequence in arrived data?
		char*	szBuf = m_answer.GetBufferRd();
		DWORD	len = m_answer.GetAvailForReading();


		// NOTE: according to recommendations from "HTTP Specs #4.5.6.1" we should
		// skip leading empty lines (to avoid treating it as an end of header)
		char*	pos = szBuf;
		while (isspace(*pos)) ++pos;
		
		pos = strstr(szBuf, szHttpHeaderEndMark);
		if (pos)
		{
			m_dwHeaderLen = (pos - szBuf) + strlen(szHttpHeaderEndMark);
			m_dwBodyLen = len - m_dwHeaderLen;

			// Got entire HTTP Response. Parse it...
			m_nStatus = ParseStatusCode(szBuf, m_dwHeaderLen);
			if (m_nStatus == -1)
			{
				_log.WriteError("CHttpClient::ReadHttpResponse(): failed to find status code in the HTTP response...\n");
				m_dwLastError = ERR_BAD_STATUS_CODE;
				return false;
			}
			else if (m_nStatus == 100)	// HTTP response "CONTINUE"
			{
				// Skip current header and continue waiting for the response
				m_answer.GetRaw(NULL, m_dwHeaderLen);
				m_dwHeaderLen = 0;
				m_dwBodyLen = 0;
				m_headerMap.clear();
				continue;
			}

			ParseResponseHeader(szBuf, m_dwHeaderLen - 2);	// do not include last "\r\n" (header delimiter)
			break;
		}
	}

	return true;
}


// Reads the HTTP body into @m_answer buffer
bool CHttpClient::ReadHttpBody()
{
	ASSERT(m_conn != INVALID_SOCKET);

	LONG	dataSize = GetContentLength();	// try to get size of HTTP body
	LONG	needToRead;
	if (dataSize >= 0)
		needToRead = dataSize - m_dwBodyLen;
	else
	{
		// body size is unknown. For "CONNECT" request method, assumes no body.
		needToRead = (IsCurrentMethod("CONNECT") ? 0 : 0x7fffffff);
	}

	BYTE		buf[READ_BUFFER_SIZE];
	const int	chunkSize = sizeof(buf);

	while (needToRead > 0)
	{
		int	cnt = ReadSocket(buf, min(chunkSize, (int) needToRead));
		if (cnt <= 0)	// no more data or any error
			break;

		m_answer.AddRaw(buf, cnt);
		needToRead -= cnt;
	}

	// if we know content length, make sure that its matched with received amount
	if ((dataSize >= 0) && (needToRead > 0))
	{
		m_dwLastError = ERR_BAD_BODY_LENGTH;	// some data has not been received
		return false;
	}

	// Update m_answer and terminates buffer with '\0' (for easy processing)
	m_answer.SetMinExtraCapasity(1);
	m_answer.GetBufferWr()[0] = 0;
	m_dwBodyLen = m_answer.GetLen() - m_dwHeaderLen;
	return true;
}


// Parse entire HTTP response.
// NOTE: szResponse should be point to the start of HTTP response header.
void CHttpClient::ParseResponseHeader(LPCSTR szResponse, DWORD len)
{
	// skip "Status code" line (should be parsed before)
	LPCSTR	end = szResponse + len;
	LPCSTR	ptr = NextLine( szResponse, end);

	while (ptr && (ptr < end))
	{
		ptr = ParseHeaderField(ptr, end);
	}
}


// Parse "WWW-Authenticate" or "Proxy-Authenticate" fields of HTTP Response
// and collect all authorization types supported by server.
// Collected items appended to @list.
// NOTE: each entry of @list contains string in format: [auth_scheme] x*(space) [auth_scheme_params]
void CHttpClient::ParseAuthTypes(bool bProxy, string_array_t& list)
{
	CStringA str;
	if (GetHeaderValue(g_szAuthTypeFields[bProxy ? 1 : 0], str))
	{
		int		len = str.GetLength();
		int		pos = 0;
		while (pos < len)
		{
			// Find end of current value (here used ",\n" sequence as a separator)
			int	next = str.Find(",\n", pos);
			if (next == -1)
				next = len;		// last value

			list.push_back(str.Mid(pos, next - pos));
			pos = next + 2;
		}
	}
}


// Does additional processing based on received response. 
// Called after receiving whole HTTP Response.
// If bTryLogon is true and m_nStatusCode is 401/407, HttpLogon() will be called.
bool CHttpClient::ProcessResponse(bool bTryLogon)
{
	switch (m_nStatus / 100)
	{
	case 1:							// 1xx - informational
	case 2:							// 2xx - success
	case 3:							// 3xx - redirection
		return true;	// no additional action required.

	case 4:		// 4xx - client error
		if (bTryLogon)
		{
			if (m_nStatus == 401) return HttpLogin(false);
			if (m_nStatus == 407) return HttpLogin(true);
		}

		m_dwLastError = ERR_HTTP_CLIENT_ERROR;
		return false;	// its a http-error

	case 5:							// 5xx - server error
		m_dwLastError = ERR_HTTP_SERVER_ERROR;
		return false;

	default:						// unexpected error
		_log.WriteError("CHttpClient::ProcessResponse(): unexpected status code in the HTTP response... (%d)", m_nStatus);
		m_dwLastError = ERR_BAD_STATUS_CODE;
		return false;
	}
}


// Parse one field of HTTP Response Header at a time. If field found and parsed,
// function adds field into @m_headerMap. 
// Then functions return pointer to the next line of header or NULL - if no more lines.
LPCSTR CHttpClient::ParseHeaderField(LPCSTR pStart, LPCSTR pBufEnd)
{
	if (!pStart || (pStart >= pBufEnd))
		return NULL;	// no more data to parse

	LPCSTR	nextLine = (LPCSTR) memchr(pStart, '\n', (pBufEnd - pStart));
	nextLine = (nextLine ? nextLine + 1 : pBufEnd);

	LPCSTR	nextToken = (LPCSTR) memchr(pStart, ':', (nextLine - pStart));
	if (!nextToken || (nextToken == pStart))	// there is no fields in current line
	{
		int	len  = (nextLine - pStart);
		if ((len > 0) && (pStart[len-1] == '\r'))
			--len;

		if (len > 0)
		{
			CStringA strLine(pStart, len);
			_log.WriteInfo("CHttpClient::ParseHeaderField(): failed to find field in line '%s'", (LPCSTR) strLine);
		}
		return nextLine;
	}

	// Extract field name (everything before ':')
	CStringA fieldName(pStart, (nextToken - pStart));


	// And field value (it could be multi-line string)
	CStringA fieldValue;

	++nextToken;	// skip ':'
	while (1)
	{
		// remove leading/trailing spaces from the current line
		while ((nextToken < nextLine) && isspace(*nextToken))
			++nextToken;

		LPCSTR	lastToken = nextLine - 1;
		while ((lastToken >= nextToken) && isspace(*lastToken))
			--lastToken;

		// append substring to fieldValue
		int	len = (lastToken - nextToken + 1);
		if (len > 0)
		{
			int		fieldLen = fieldValue.GetLength();
			LPSTR	lpBuf = fieldValue.GetBuffer(fieldLen + len);
			memcpy(lpBuf + fieldLen, nextToken, len);
			fieldValue.ReleaseBuffer(fieldLen + len);
		}

		// continue if next line also the part of current value (started from space or tab)?
		if ((nextLine < pBufEnd) && ((*nextLine == ' ') || (*nextLine == '\t')))
		{
			fieldValue += ' ';
			nextToken = nextLine + 1;

			nextLine = (LPCSTR) memchr(nextToken, '\n', (pBufEnd - nextToken));
			nextLine = (nextLine ? nextLine + 1 : pBufEnd);
			continue;	// process next line also
		}

		break;	// field processed!
	}

	// add/append field to the m_headerMap
	key_map_t::iterator	it = m_headerMap.find(fieldName);
	if (it == m_headerMap.end())
		m_headerMap.insert(string_pair_t(fieldName, fieldValue));
	else
	{
		// TECHNICAL NOTE: custom implementation of field merging.
		// According to rfc2068 4.2 we can merge field values with the same name
		// by using comma as a separator. But, there is a one issue. If field's
		// value contain ','(like dates or some Proxy-Authenticate values),
		// it will be hard to split different values later because ',' is not 
		// unique separator anymore. 
		// So, ",\n" sequence is chosen as a value separator because our parser
		// already removes '\n' inside values.
		CStringA&	val = it->second;
		val += ",\n";
		val += fieldValue;
	}

//	TRACE2("Key %s  - value: %s\n", fieldName, fieldValue);

	return nextLine;
}


// Function checks, if specific HTTP method is used now.
bool CHttpClient::IsCurrentMethod(LPCSTR szMethod)
{
	// just make sure that m_request starts with specified "method"
	size_t len = (szMethod ? strlen(szMethod) : 0);
	LPCSTR szRequest = (LPCSTR) m_request;
	return (len && (strnicmp(szMethod, szRequest, len) == 0) && isspace(szRequest[len]));
}


// Try to get authorization to HTTP server or proxy.
// Function collect the list of authorization types from Http Response Header 
// and try to login via supported authorization methods.
bool CHttpClient::HttpLogin(bool bProxy)
{
	m_dwLastError = 0;	// will be used later to recognize type of error

	if (m_request.IsEmpty() || m_userName.IsEmpty() || m_password.IsEmpty())
	{
		if (m_request.IsEmpty())
			_log.WriteError("CHttpClient::HttpLogin(): wrong call. There is no active session.");
		else
			_log.WriteError("CHttpClient::HttpLogin(): username and password must be not empty.");

		m_dwLastError = ERR_HTTP_AUTH_ERROR;
		return false;
	}

	// Collect the list of supported authenticate types
	string_array_t	authTypesList;
	ParseAuthTypes(bProxy, authTypesList);

	size_t count = authTypesList.size();
	for (size_t i = 0; i < count; ++i)
	{
		CStringA& authStringFull = authTypesList[i];
		CStringA authType = authStringFull;
		int i1  = authType.Find(" ");
		if (i1>=0) authType = authType.Mid(0, i1);
		authType.MakeLower();

		CHttpAuthObject* pAuthProvider = NULL;

		if      (authType == "basic") {
			if (!m_authProviders[0]) m_authProviders[0] = new CHttpBasicAuthObject;
			pAuthProvider = m_authProviders[0];
		}
		else if (authType == "ntlm")  {
			if (!m_authProviders[1]) m_authProviders[1] = new CHttpNTLMAuthObject;
			pAuthProvider = m_authProviders[1];
		}

		// if specific authorization provider is found			
		if (pAuthProvider)
		{
			//if (!m_bKeepConectionWhileAuthenticating)
				CloseSocket();

			if (pAuthProvider->Logon(*this, authStringFull, m_userName, m_password, bProxy))
			{
				if (ProcessResponse(false)) return true;	// authorization succeeded!
				
				if ((m_nStatus != 401) && (m_nStatus != 407)) break; // Its a not an authorization error, stop attempts!
			}

			// Stop attempts if authorization failed due to any socket error 
			// (make no sense to continue in this case)
			if ((m_dwLastError >= WSABASEERR) && (m_dwLastError < WSABASEERR + 2000))
				break;
		}
		m_dwLastError = 0;
	}

	// If server reject authorization and it was not socket error, treat it as a ERR_HTTP_AUTH_ERROR
	if (m_dwLastError == 0)
		m_dwLastError = ERR_HTTP_AUTH_ERROR;
	return false;
}
