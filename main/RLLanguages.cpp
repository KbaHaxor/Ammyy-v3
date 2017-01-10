#include "stdafx.h"
#include "RLLanguages.h"
#include "RLLanguages_data.h"

RLLanguages::RLLanguages()
{
	m_iLang = -1;
}

RLLanguages::~RLLanguages() {}

CStringW RLLanguages::ConvToUTF16(LPCSTR src, int codepage)
{
	CStringW str;

	int size = strlen(src);

	if (size==0) return str;
		
	int len2 = ::MultiByteToWideChar(codepage, 0, src, size, str.GetBuffer(size), size);
	if (len2 <= 0 || len2!=size)
		throw RLException("ConvToUTF16()#2 %d %d %d", ::GetLastError(), len2, size);

	str.ReleaseBuffer(size);
	return str;
}

void RLLanguages::Select(int lang)
{
	m_iLang = lang;

	for (int i=0; i<D__COUNT; i++) m_valuesA[i] = NULL;

	int codepage = Languages[lang].codepage;

	int c = Languages[lang].count;
	MessageStr* p = Languages[lang].Strings;
	for ( i=0; i<c; i++) {
		int index = p[i].key;
		m_valuesA[index] = p[i].str;
		m_valuesW[index] = ConvToUTF16(m_valuesA[index], codepage);
	}

	//other try to find in English language
	if (lang>0) {
		codepage = Languages[0].codepage;
		int c = Languages[0].count;
		MessageStr* p = Languages[0].Strings;
		for (int i=0; i<c; i++) {
			int index = p[i].key;
			if (m_valuesA[index]==NULL) {
				m_valuesA[index] = p[i].str;
				m_valuesW[index] = ConvToUTF16(m_valuesA[index], codepage);
			}
		}
	}

	for ( i=0; i<D__COUNT; i++) {
		if (m_valuesA[i] == NULL) m_valuesW[i] = ""; // fill empty
	}

}

CStringW RLLanguages::GetValue(int key)
{
	if (m_iLang<0) this->Select(0); // select English by default
	if (key<0 || key>D__COUNT) throw RLException("Incorrect key GetValue()");
	return m_valuesW[key];
}

LPCSTR RLLanguages::GetValueA(int key)
{
	if (m_iLang<0) this->Select(0); // select English by default
	if (key<0 || key>D__COUNT) throw RLException("Incorrect key GetValue()");
	return m_valuesA[key];
}

LANGID RLLanguages::GetLangByIndex(int index)
{
	return Languages[index].LangID;
}


void RLLanguages::SetDlgItemsText(HWND hDlg, UINT* dlgIds, UINT* strIds)
{
	if (m_iLang<0) this->Select(0); // select English by default	
	while(true) {
		UINT dlgId=*dlgIds++;
		if (dlgId==0) break;

		UINT strId=*strIds++;		

		LPCWSTR str = m_valuesW[strId];
		
		::SetDlgItemTextW(hDlg, dlgId, str);
	}
}

int RLLanguages::GetLangIndex(LANGID langId)
{
	int n = sizeof(Languages)/sizeof(Languages[0]);
	for (int i=0; i<n; i++) {
		if (langId==Languages[i].LangID) return i;
	}
	return 0;
}

int RLLanguages::GetCountLang()
{
	return sizeof(Languages)/sizeof(Languages[0]);
}

LPCSTR RLLanguages::GetLangName(int index)
{
	return Languages[index].LangName;
}

LANGID RLLanguages::GetUserDefaultUILanguage()
{
	WORD primaryLangID = PRIMARYLANGID(::GetUserDefaultUILanguage());

	int n = sizeof(Languages)/sizeof(Languages[0]);
	for (int i=0; i<n; i++) {
		if (primaryLangID==PRIMARYLANGID(Languages[i].LangID)) return Languages[i].LangID;
	}
	
	return Languages[0].LangID; // default if not found
}



/*
UINT16 keys[] = { IDS_GET_HELP, IDS_GIVE_HELP, IDS_STOP_HELP, IDS_YOUR_ID_STATIC, IDS_TAB1, IDS_CLIENT_ID_NOT_PROVIDED,
    IDS_CLIENT_ID_STATIC, IDS_PCNotFound, IDS_REMOTEPC_CLOSE,
    IDS_WARN_DIFF_VERSIONS, IDS_ACCEPT, IDS_REJECT, IDS_ACCEPT_CONN_TEXT, IDS_REMEMBER_MY_ANSWER, 
    IDS_ENABLE_INPUT, IDS_ENABLE_FS, IDS_ACCESSREJECTED, IDS_WAITING_AUTHORIZATION };


LPCSTR Get(int lan, int key)
{
	int c = Languages[lan].count;
	MessageStr* p = Languages[lan].Strings;

	for (int i=0; i<c; i++) {
		if (p[i].key == key) return p[i].str;
	}

	return NULL; // not found
}

bool Test()
{
	int c1 = sizeof(Languages)/sizeof(Languages[0]);

	for (int lan=0; lan<c1; lan++) {
		int c2 = sizeof(keys)/sizeof(keys[0]);

		for (int i=0; i<c2; i++) 
		{
			RLStream str;

			RLResource::LoadStrW_Simple(Languages[lan].LangID, (HINSTANCE)0x400000, keys[i], str);



			LPCSTR t = Get(lan, keys[i]);

			if (t==NULL) {
				if (str.GetLen()!=0) 
					throw RLException("#1");
			}
			else {
				CStringW t2 = CRLLanguage::ConvToUTF16(t, Languages[lan].codepage);

				int len = str.GetLen()/2 - 1;

				if (len!= t2.GetLength())
					throw RLException("#2");

				if (memcmp(t2.GetBuffer(0), str.GetBuffer(), len*2)!=0) 
					throw RLException("#3");
			}
		}
	}

	return true;
}

//bool t = Test();
*/


RLLanguages rlLanguages;

