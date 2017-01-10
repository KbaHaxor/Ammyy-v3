#if !defined(_TR_IMPERSONATE_WRAPPER_H__B0C6E78F8E36__INCLUDED_)
#define _TR_IMPERSONATE_WRAPPER_H__B0C6E78F8E36__INCLUDED_

class ImpersonateWrapper
{
public:
	ImpersonateWrapper();
	ImpersonateWrapper(bool b);
	~ImpersonateWrapper();

private:
	bool m_ok;
};

class LoadUserProfileWrapper
{
public:
	LoadUserProfileWrapper();
	~LoadUserProfileWrapper();

	HKEY GetProfile() 
	{ return (m_hProfile) ? (HKEY)m_hProfile : HKEY_CURRENT_USER; }

private:
	HANDLE m_hProfile;
	HANDLE m_hToken;
};

#endif // !defined(_TR_IMPERSONATE_WRAPPER_H__B0C6E78F8E36__INCLUDED_)
