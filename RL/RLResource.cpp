#include "stdafx.h"
#include "RLResource.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


RLResource::RLResource()
{
	m_hInstance = (HINSTANCE)0x400000;
}

RLResource::~RLResource()
{

}

DWORD RLResource::Load(HMODULE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLanguage, LPCVOID* ppData)
{
	HRSRC hRes = ::FindResourceEx(hModule, lpType, lpName, wLanguage);

	if (hRes==NULL) 
		throw RLException("FindResourceEx() error=%d lpName=0x%X", ::GetLastError(), lpName);

	DWORD dwSize = ::SizeofResource(hModule, hRes);

	if (dwSize==0)
		throw RLException("SizeofResource() error=%d", ::GetLastError());

	HGLOBAL hGlobal = ::LoadResource(hModule, hRes);

	if (hGlobal==NULL)
		throw RLException("LoadResource() error=%d", ::GetLastError());

	LPVOID pData = ::LockResource(hGlobal);

	if (pData==NULL)
		throw RLException("LockResource() error=%d", ::GetLastError());

	*ppData = pData;

	return dwSize;
}


/*
int RLResource::GetLangIndex(LANGID langId)
{
	int n = sizeof(LangIDs)/sizeof(LangIDs[0]);
	for (int i=0; i<n; i++) {
		if (langId==LangIDs[i]) return i;
	}
	return 0;
}
*/


/*
CStringA RLResource::LoadStr(UINT id)
{
	RLStream string;
	this->LoadStr(m_hInstance, id, string);
	return CStringA((LPCSTR)string.GetBuffer());
}

void RLResource::LoadStr(HINSTANCE hInstance, UINT id, RLStream& string)
{
	RLStream stringW;

	if (!LoadStrW_Simple(m_hLang, hInstance, id, stringW)) {
		if (m_hLang!=m_hLangDefault) {
			LoadStrW_Simple(m_hLangDefault, hInstance, id, stringW);
		}
	}

	string.Reset();

	// if not found in the current and default languages
	if (stringW.GetLen()==0) 
	{
		string.SetMinCapasity(1);
		string.SetLen(1);
		*((char*)string.GetBuffer()) = 0;
	}
	else {
		int strLen = (stringW.GetLen()/2)-1;
		string.SetMinCapasity(strLen+1);
		string.SetLen(strLen+1);
		LPSTR pBuffer = (LPSTR)string.GetBuffer();
		int len = ::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)stringW.GetBuffer(), strLen, pBuffer, strLen+1, NULL, NULL);
		pBuffer[len] = 0;
	}
}
*/

/*
void RLResource::LoadStrW(HINSTANCE hInstance, UINT id, RLStream& string)
{
	if (!LoadStrW_Simple(m_hLang, hInstance, id, string)) {
		if (m_hLang!=m_hLangDefault) {
			LoadStrW_Simple(m_hLangDefault, hInstance, id, string);
		}
	}

	// if not found in the current and default languages
	if (string.GetLen()==0) 
	{
		string.SetMinCapasity(2);
		string.SetLen(2);
		*((WCHAR*)string.GetBuffer()) = 0;
	}
}



bool RLResource::LoadStrW_Simple(WORD hLang, HINSTANCE hInstance, UINT id, RLStream& string)
{
	string.Reset();

	//get the offset of the the block within the string table
	DWORD blockId = (id >> 4) + 1;

	//get the offset of the item within the block
	DWORD nitemId = id % 0x10;

	//get handle to the beginning of the block of 16 strings, where each string begins with its size
	HRSRC hRes = ::FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE(blockId), hLang);
	if (hRes)
	{
		HGLOBAL hGlo = ::LoadResource(hInstance, hRes);
		LPCWSTR pStr = (LPCWSTR)::LockResource(hGlo);
		DWORD nStr = 0;
		DWORD dwsize = ::SizeofResource(hInstance, hRes);
		LPCWSTR pStrEnd = pStr+dwsize;
		while(pStr < pStrEnd)
		{
			DWORD strLen = (DWORD)*pStr++;

			if (nStr == nitemId) 
			{
				if (strLen != 0) 
				{
					int countBytes = (strLen+1)*sizeof(WCHAR);						
					string.SetMinCapasity(countBytes);
					string.SetLen(countBytes);
					LPWSTR pBuffer = (LPWSTR)string.GetBuffer();
					memcpy(pBuffer, pStr, strLen*sizeof(WCHAR));
					pBuffer[strLen] = 0;
				}
				break;
			}
			
			pStr += strLen;
			nStr++;
		}
	}

	return (string.GetLen()!=0);
}
*/
