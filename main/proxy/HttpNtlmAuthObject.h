#ifndef __HTTP_NTLM_AUTH_OBJECT_H__
#define __HTTP_NTLM_AUTH_OBJECT_H__

#include "HttpClient.h"
#define SECURITY_WIN32
#include <security.h>


// Declaration of the class CHttpNTLMAuthObject: provider for HTTP NTLM Authorization.
class CHttpNTLMAuthObject : public CHttpAuthObject
{
public:
	CHttpNTLMAuthObject();
	virtual ~CHttpNTLMAuthObject();

	virtual bool Logon(CHttpClient& conn, const CStringA& authType, 
					   const CStringA& userName, const CStringA& password, bool bProxy);

protected:
	void Reset();
	bool Prepare(const CStringA& userName, const CStringA& password);
	bool GetAuthRequestToken(CStringA& reqToken);
	bool GetAuthResponseToken(CHttpClient& conn, bool bProxy, CStringA& token);
	bool ProcessResponseToken(const CStringA& srcToken, CStringA& challengeToken);
	// send auth token to the server (via CHttpClient object)
	bool SendAuthToken(CHttpClient& conn, bool bProxy, const CStringA& token);

protected:
	CredHandle	m_hCred;
	CtxtHandle	m_hSecurityContext;
	DWORD		m_maxTokenSize;			// max token size (in bytes)
};



#endif // !defined(__HTTP_NTLM_AUTH_OBJECT_H__)

