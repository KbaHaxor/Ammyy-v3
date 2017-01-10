#if !defined(AFX_DLGRDPSETTINGS_H__A89096BC_192C_4677_B889_F62120825362__INCLUDED_)
#define AFX_DLGRDPSETTINGS_H__A89096BC_192C_4677_B889_F62120825362__INCLUDED_

#include "../RL/RLWnd.h"
#include <vector>

class DlgRDPSettings : public RLDlgBase 
{
public:
	DlgRDPSettings();
	virtual ~DlgRDPSettings();

	void CheckRDPFile();

	static CStringW GetFileName();

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnEndDialog(BOOL ok);
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Get value and type from "m_rdpFile"
	BOOL GetRdpValue(LPCWSTR wszKey, CStringW& value, CStringW& type);
	BOOL GetRdpIntValue(LPCWSTR wszKey, int& val);

	// Set/add key into  "m_rdpFile"
	void SetRdpIntValue(LPCWSTR wszKey, int val);

	void ImportRDPKeys();	// import RDP class members from m_rdpFile
	
	void FillScreenSettings();
	void FillSoundModeComboBox();
	BOOL SaveData();

	// Find next token and move pointer forward. Tokens separated by ':' sign.
	// Function returns NULL if no more tokens found.
	static LPCWSTR	 NextToken(LPCWSTR ptr, LPCWSTR& token, UINT& len, WCHAR tokSep = L':');

private:
	static CStringA ConevrtIntToString(int val);
	void SetDefault();
	void ReadRDPFile(LPCWSTR fileName);
	void WriteRDPFile();
	void OnScreenModeChanged(bool init);

public:
	enum SoundMode
	{
		SM_playLocal = 0,
		SM_playRemote = 1,
		SM_playNone = 2,
		SM_COUNT
	};

private:
	RLWndComboBox m_wndScreenMode, m_wndWidth, m_wndHeight;

	UINT32  m_screenMode;
	UINT32  m_desktopHeight;
	UINT32  m_desktopWidth;
	UINT32  m_sessionBpp;
	SoundMode m_soundMode;
	bool	m_disableWallpaper;
	bool	m_disableThemes;
	bool	m_disableMenuAnim;
	bool	m_disableFullWndDragMode;

	std::vector<CStringW>	m_rdpFile;	// Full content of RDP file	
};

#endif // !defined(AFX_DLGRDPSETTINGS_H__A89096BC_192C_4677_B889_F62120825362__INCLUDED_)
