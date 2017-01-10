#include "stdafx.h"
#include "RLDlgTemplate.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

RLDlgTemplate::RLDlgTemplate(UINT capacity):
m_data(capacity)
{

}

RLDlgTemplate::~RLDlgTemplate()
{

}

void RLDlgTemplate::AddDlgHeader(DWORD style, DWORD dwExtendedStyle, WORD cdit, short x, short y, short cx, short cy, 
				 WORD id, WORD fontSize, LPCWSTR fontName,  LPCWSTR title)
{
	DLGTEMPLATE dt;

	dt.style = style;
    dt.dwExtendedStyle = dwExtendedStyle;
    dt.cdit = cdit;
    dt.x = x;
    dt.y = y;
    dt.cx = cx;
    dt.cy = cy;
	

	m_data.AddRaw(&dt, sizeof(dt));
	m_data.AddUINT16(0); // menu
	m_data.AddUINT16(0); // class
	this->AddLPWSTR(title);
	m_data.AddUINT16(fontSize);
	this->AddLPWSTR(fontName);	
}

void RLDlgTemplate::AddLPWSTR(LPCWSTR str)
{
	m_data.AddRaw(str, (wcslen(str)+1)*2);
}


void RLDlgTemplate::AddDlgItem(DWORD style, DWORD dwExtendedStyle, UINT32 classId, short x, short y, short cx, short cy, WORD id, LPCWSTR title)
{
	// align
	{
		UINT c = (m_data.GetLen() % 4);
		if (c>0) {
			c = 4 - c;
			UINT v = 0;
			m_data.AddRaw(&v, c); // align by DWORD
		}
	}

	DLGITEMTEMPLATE it;

	it.style = style;
    it.dwExtendedStyle = dwExtendedStyle;
    it.x  = x;
    it.y  = y;
    it.cx = cx;
    it.cy = cy;
    it.id = id;

	m_data.AddRaw(&it, sizeof(it));
	m_data.AddUINT32(classId);
	this->AddLPWSTR(title);	
	m_data.AddUINT16(0); // creation data
}
