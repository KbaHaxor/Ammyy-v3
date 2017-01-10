#if !defined(_RLHTTP_2_H__INCLUDED_)
#define _RLHTTP_2_H__INCLUDED_

class RLHttp2  
{
public:
	RLHttp2();
	~RLHttp2();

	void Get (LPCSTR url);
	void Post(LPCSTR url, RLStream* pOptionalData);

	static void SetKeepAlive(SOCKET s,  int keepidle=60, int keepintvl=3); // in seconds

private:	
	void	SendRequest(LPCSTR queryType, LPCSTR url, RLStream* pOptionalData);
	bool	ParseReply(const CStringA& bufferIn);
	void	ParseURL(LPCSTR szURL, LPSTR szProtocol, LPSTR szHost, WORD &nPort, LPSTR szURI);
	void	CloseConnection();
	void	ConnectByTCP(LPCSTR host, int port);
	UINT32	ResolveHostname_IPv4(LPCSTR hostname);	
	void 	SetTimeOut(SOCKET s, int seconds);

	int GetDigit(LPCSTR p, int count, UINT& digit);

public:
			CStringA m_szHeader;
	static	CStringA m_assignIP;

	RLStream 	m_reply;
	CStringA  	m_replyStatus;

private:
	SOCKET 		m_s;
};

#endif // !defined(_RLHTTP_2_H__INCLUDED_)
