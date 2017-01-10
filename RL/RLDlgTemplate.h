#if !defined(AFX_RLDLGTEMPLATE_H__0C5401F6_5E1C_4325_A396_E5D0768CC32A__INCLUDED_)
#define AFX_RLDLGTEMPLATE_H__0C5401F6_5E1C_4325_A396_E5D0768CC32A__INCLUDED_

#include "RLStream.h"

class RLDlgTemplate  
{
public:
	RLDlgTemplate(UINT capacity);
	~RLDlgTemplate();

	void AddDlgHeader(DWORD style, DWORD dwExtendedStyle, WORD cdit, short x, short y, short cx, short cy, 
				 WORD id, WORD fontSize, LPCWSTR fontName,  LPCWSTR title);

	void AddDlgItem(DWORD style, DWORD dwExtendedStyle, UINT32 classId, short x, short y, short cx, short cy, WORD id, LPCWSTR title);

	LPCDLGTEMPLATE GetTemplate() { return (LPCDLGTEMPLATE)m_data.GetBuffer(); }

private:
	void AddLPWSTR(LPCWSTR str);

private:
	RLStream m_data;
};

#endif // !defined(AFX_RLDLGTEMPLATE_H__0C5401F6_5E1C_4325_A396_E5D0768CC32A__INCLUDED_)
