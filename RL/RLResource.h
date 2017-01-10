#if !defined(AFX_RLRESOURCE_H__INCLUDED_)
#define AFX_RLRESOURCE_H__INCLUDED_

#include "RLStream.h"

class RLResource  
{
public:
	RLResource();
	virtual ~RLResource();

	static DWORD Load(HMODULE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLanguage, LPCVOID* lpData);
	
	//CStringA LoadStr(UINT id);
	//static int    GetLangIndex(LANGID langId);	
	//virtual void LoadStr(HINSTANCE hInstance, UINT id, RLStream& string);	


	//void LoadStrW(HINSTANCE hInstance, UINT id, RLStream& string);

private:
	//static bool LoadStrW_Simple(WORD hLang, HINSTANCE hInstance, UINT id, RLStream& string);

	HINSTANCE	m_hInstance;
};

#endif // !defined(AFX_RLRESOURCE_H__INCLUDED_)
