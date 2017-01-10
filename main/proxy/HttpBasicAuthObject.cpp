#include "stdafx.h"
#include "HttpBasicAuthObject.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


// Implementation of the class CHttpBasicAuthObject: provider for HTTP Basic Authorization.
bool CHttpBasicAuthObject::Logon(CHttpClient& conn, const CStringA& authType, 
								 const CStringA& userName, const CStringA& password, bool bProxy)
{
	CStringA	realm;
	if (!ParseAuthParams(authType, realm))
		return false;

	// Prepare extra header for HTTP authorization request
	CStringA login = userName + ":" + password;
	login = MyBase64Encode(login);	// Convert the pair "Login:Password" into Base64 format

	CStringA authInfo;
	if (bProxy)
		authInfo.Format("Proxy-Authorization: Basic %s\r\n", (LPCSTR) login);
	else
		authInfo.Format(      "Authorization: Basic %s\r\n", (LPCSTR) login);

	// Re-send request with authorization attempt
	return conn.ExecuteRequest((LPCSTR) authInfo);
}


// Helper function: parse @authType parameters returned by server.
// General format of authType: "basic x*[sp] (realm="bla-bla")
bool CHttpBasicAuthObject::ParseAuthParams(const CStringA& authType, CStringA& realm)
{
	LPCSTR	ptr = (LPCSTR) authType;
	ASSERT(strnicmp(ptr, "basic", 5) == 0);	// should be started with 'basic'

	// skip 'basic' and spaces
	ptr += 5;
	while (isspace(*ptr)) ++ptr;

	realm.Empty();

	// do we have 'realm' parameter?
	if (strnicmp(ptr, "realm", 5) == 0)
	{
		// skip 'realm' and spaces
		ptr += 5;	
		while (isspace(*ptr)) ++ptr;
		
		if (*ptr != '=')
			return false;	// expected '=' here

		// skip '=' and spaces
		ptr += 1;
		while (isspace(*ptr)) ++ptr;

		LPCSTR	end;
		if (*ptr != '\"')
			end = ptr + strlen(ptr);	// assumes whole string is a realm's parameter
		else
		{
			// skip quotas (leading and trailing)
			end = ++ptr;
			while (*end && (*end != '\"'))
			{
				if ((end[0] == '\\') && end[1])
					end += 2;	// its an escaped character, skip it
				else
					end += 1;
			}
		}

		// copy realm value into output parameter
		realm = CStringA(ptr, (end - ptr));
		// TODO: probably need to unescape "realm" value
	}

	return true;
}
