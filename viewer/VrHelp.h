#ifndef _VTCHELP_H__INCLUDED_
#define _VTCHELP_H__INCLUDED_

class VrHelp  
{	
public:
	VrHelp();
	~VrHelp();

	void Popup(LPARAM lParam);
	BOOL TranslateMsg(MSG *pmsg);

private:
	DWORD m_dwCookie;
};

#endif // _VTCHELP_H__INCLUDED_
