#if !defined(AFX_RLEVENT_H__993DE57D_A4D8_4299_8F9A_FD048966883F__INCLUDED_)
#define AFX_RLEVENT_H__993DE57D_A4D8_4299_8F9A_FD048966883F__INCLUDED_

class RLHandleWin
{
public:
	RLHandleWin();
	~RLHandleWin();

	void Close();

	inline bool IsValid() { return (m_handle!=NULL); }
	inline operator HANDLE () { return m_handle; }

protected:
	HANDLE m_handle;
};

class RLEvent: public RLHandleWin 
{
public:
	void Create(LPCTSTR lpName=NULL, SECURITY_ATTRIBUTES* sa=NULL);
	void CreateOnly(LPCTSTR lpName=NULL);

	void Set();
	void Reset();

	bool IsSet();

	static bool IsEventExist(LPCTSTR lpName);
};

class RLMutexWin: public RLHandleWin 
{
public:
	HANDLE CreateAndOwn(LPSECURITY_ATTRIBUTES lpMutexAttributes, LPCSTR lpName);

	static bool IsExist(LPCTSTR lpName);
};

#endif // !defined(AFX_RLEVENT_H__993DE57D_A4D8_4299_8F9A_FD048966883F__INCLUDED_)
