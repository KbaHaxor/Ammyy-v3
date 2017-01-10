#include <stdafx.h>
#include "HttpNtlmAuthObject.h"
#include "../../RL/RLBase64Coder.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma comment(lib, "SECUR32.LIB")

//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)


// Implementation of the class CHttpNTLMAuthObject: provider for HTTP NTLM Authorization.
CHttpNTLMAuthObject::CHttpNTLMAuthObject()
{
	SecInvalidateHandle(&m_hCred);
	SecInvalidateHandle(&m_hSecurityContext);
	m_maxTokenSize = 0;
}


CHttpNTLMAuthObject::~CHttpNTLMAuthObject()
{
	Reset();
}


bool CHttpNTLMAuthObject::Logon(CHttpClient& conn, const CStringA&, const CStringA& userName, const CStringA& password, bool bProxy)
{
	if (!Prepare(userName, password)) return false;

	CStringA token;

	// stage 1 of NTLM authentication
	if (!GetAuthRequestToken(token) || 
		!SendAuthToken(conn, bProxy, token))
	{
		Reset();
		return false;
	}

	// stage 2 of NTLM authentication (get challenge response from server and process it)
	CStringA challengeToken;
	if (!GetAuthResponseToken(conn, bProxy, token) || 
		!ProcessResponseToken(token, challengeToken))
	{
		Reset();
		return false;
	}

	// stage 3 of NTLM authentication (send final challenge key to server)
	bool bSuccess = SendAuthToken(conn, bProxy, challengeToken);

	Reset();
	return bSuccess;
}


// Get credentials handle and size of authenticate tokens
bool CHttpNTLMAuthObject::Prepare(const CStringA& userName, const CStringA& password)
{
	// get credentials handle on the NTLM security package
	PSecPkgInfo		packageInfo = NULL;
	SECURITY_STATUS	res = ::QuerySecurityPackageInfo("NTLM", &packageInfo);
	if (res != SEC_E_OK)
	{
		_log.WriteError("ERROR in CHttpNTLMAuthObject::Prepare(): QuerySecurityPackageInfo() failed (result = 0x%x)", res);
		return false;
	}

	const DWORD	tokenSize = packageInfo->cbMaxToken;
	if (tokenSize > 512*1024)
	{
		_log.WriteError("ERROR in CHttpNTLMAuthObject::Prepare(): invalid token size (size = %u)", tokenSize);
		return false;
	}

	SEC_WINNT_AUTH_IDENTITY_EX	ai = { 0 };
	ai.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
	ai.Length = sizeof(ai);
	ai.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
	ai.User = (unsigned char*) (LPCSTR) userName;
	ai.UserLength = userName.GetLength();
	ai.Password = (unsigned char*) (LPCSTR) password;
	ai.PasswordLength = password.GetLength();

	// try to get credentials handle
	TimeStamp	ts;
	res = ::AcquireCredentialsHandle(0, packageInfo->Name, SECPKG_CRED_OUTBOUND, NULL, (void*) &ai, NULL, NULL, &m_hCred, &ts);

	::FreeContextBuffer(packageInfo);

	if (res != SEC_E_OK)
	{
		_log.WriteError("ERROR in CHttpNTLMAuthObject::Prepare(): AcquireCredentialsHandle() failed (result = 0x%x)", res);
		return false;
	}

	m_maxTokenSize = tokenSize;
	return true;
}


// get NTLM auth request token (Type1 message)
bool CHttpNTLMAuthObject::GetAuthRequestToken(CStringA& reqToken)
{
	// Setup SecBufferDesc for reqToken
	SecBuffer		outBuf = { 0 };
	outBuf.BufferType = SECBUFFER_TOKEN;
	outBuf.cbBuffer = m_maxTokenSize;
	outBuf.pvBuffer = (void*) reqToken.GetBuffer(m_maxTokenSize);

	SecBufferDesc	outBufDesc = { 0 };
	outBufDesc.ulVersion = SECBUFFER_VERSION;
	outBufDesc.cBuffers = 1;
	outBufDesc.pBuffers = &outBuf;

	TimeStamp		ts;
	SECURITY_STATUS	res;
	DWORD			contextAttrib = 0;
	res = ::InitializeSecurityContext(&m_hCred, 0, NULL, ISC_REQ_CONNECTION, 0, 0, NULL, 0,
									&m_hSecurityContext, &outBufDesc, &contextAttrib, &ts);
	
	if ( (res == SEC_I_COMPLETE_NEEDED) || (res == SEC_I_COMPLETE_AND_CONTINUE) )	// in progress
		res = CompleteAuthToken(&m_hSecurityContext, &outBufDesc);

	if (IS_ERROR(res))
	{
		_log.WriteError("ERROR in GetAuthRequestToken::GetAuthRequestToken(): InitializeSecurityContext() failed (result = 0x%x)", res);
		reqToken.ReleaseBuffer(0);
		return false;
	}

	reqToken.ReleaseBuffer(outBuf.cbBuffer);	// NOTE: this is binary buffer, not ASCIIZ-string
	return true;
}


// get NTLM auth challenge response token (Type2 message) from received HTTP response
bool CHttpNTLMAuthObject::GetAuthResponseToken(CHttpClient& conn, bool bProxy, CStringA& token)
{
	// get response token
	CStringA	value;
	if (!conn.GetHeaderValue(CHttpClient::g_szAuthTypeFields[bProxy ? 1 : 0], value ))
	{
		_log.WriteError("ERROR in CHttpNTLMAuthObject::GetAuthResponseToken(): NTLM response not found");
		return false;
	}

	// make sure its a NTLM answer?
	if (strnicmp((LPCSTR) value, "NTLM", 4) == 0)
	{
		// extract response token and decode it from base64 format
		int	i = 4;
		while (isspace(value[i])) ++i;

		if (value[i] != 0)	// do we have any response token?
		{
			value.Delete(0, i);	// exclude response header
			RLBase64Coder encoder(NULL);
			token = encoder.Decode(value);
			return true;
		}
	}

	_log.WriteError("ERROR in CHttpNTLMAuthObject::GetAuthResponseToken(): wrong NTLM response found (%s)", (LPCSTR) value);
	return false;
}


// Process received challenge response and get NTLM auth challenge token (Type3 message)
bool CHttpNTLMAuthObject::ProcessResponseToken(const CStringA& srcToken, CStringA& challengeToken)
{
	// Setup SecBufferDesc buffers for srcToken and challengeToken 
	SecBuffer		inBuf = { 0 };
	inBuf.BufferType	= SECBUFFER_TOKEN;
	inBuf.cbBuffer		= srcToken.GetLength();
	inBuf.pvBuffer		= (void*) (LPCSTR) srcToken;

	SecBufferDesc	inBufDesc = { 0 };
	inBufDesc.ulVersion	= SECBUFFER_VERSION;
	inBufDesc.cBuffers	= 1;
	inBufDesc.pBuffers	= &inBuf;

	SecBuffer		outBuf = { 0 };
	outBuf.BufferType = SECBUFFER_TOKEN;
	outBuf.cbBuffer = m_maxTokenSize;
	outBuf.pvBuffer = (void*) challengeToken.GetBuffer(m_maxTokenSize);

	SecBufferDesc	outBufDesc = { 0 };
	outBufDesc.ulVersion = SECBUFFER_VERSION;
	outBufDesc.cBuffers = 1;
	outBufDesc.pBuffers = &outBuf;

	TimeStamp		ts;
	SECURITY_STATUS	res;
	DWORD			contextAttrib = 0;
	res = ::InitializeSecurityContext(0, &m_hSecurityContext, NULL, 0, 0, 0, &inBufDesc, 0, &m_hSecurityContext, 
										&outBufDesc, &contextAttrib, &ts);

	if (IS_ERROR(res))
	{
		_log.WriteError("ERROR in CHttpNTLMAuthObject::ProcessResponseToken(): InitializeSecurityContext() failed (result = 0x%x)", res);
		challengeToken.ReleaseBuffer(0);
		return false;
	}

	challengeToken.ReleaseBuffer(outBuf.cbBuffer);
	return true;
}


// send auth token to the server (via CHttpClient object)
bool CHttpNTLMAuthObject::SendAuthToken(CHttpClient& conn, bool bProxy, const CStringA& token)
{
	const static char*	szWWWAuthFormat		=       "Authorization: NTLM %s\r\n";
	const static char*	szProxyAuthFormat	= "Proxy-Authorization: NTLM %s\r\n";

	const char*	szFormat = (bProxy ? szProxyAuthFormat : szWWWAuthFormat);
	
	CStringA encodedToken = MyBase64Encode(token);

	CStringA extraHeader;
	extraHeader.Format(szFormat, (LPCSTR) encodedToken);

	// Re-send request with authorization token
	return conn.ExecuteRequest((LPCSTR) extraHeader);
}


// Release all used resources and re-init object
void CHttpNTLMAuthObject::Reset()
{
	if (SecIsValidHandle(&m_hCred))				::FreeCredentialHandle(&m_hCred);
	if (SecIsValidHandle(&m_hSecurityContext))	::FreeCredentialHandle(&m_hSecurityContext);

	m_maxTokenSize = 0;
}
