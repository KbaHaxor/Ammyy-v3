#ifndef __HTTP_BASIC_AUTH_OBJECT_H__
#define __HTTP_BASIC_AUTH_OBJECT_H__

#include "HttpClient.h"


// Declaration of the class CHttpBasicAuthObject: provider for HTTP Basic Authorization.
class CHttpBasicAuthObject : public CHttpAuthObject
{
public:
	virtual bool Logon(CHttpClient& conn, const CStringA& authType, const CStringA& userName, const CStringA& password, bool bProxy);

protected:
	bool ParseAuthParams(const CStringA& authType, CStringA& realm);
};



#endif // !defined(__HTTP_BASIC_AUTH_OBJECT_H__)

