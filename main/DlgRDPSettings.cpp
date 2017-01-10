#include "stdafx.h"
#include "DlgRDPSettings.h"
#include "resource.h"
#include "Common.h"
#include "AmmyyApp.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// Special symbol that used as a header for Unicode text files.
static const WCHAR		UNICODE_FILE_HEADER	= 0xFEFF;

// Supported key names in RDP-file
static const LPCWSTR	csz_ScreenMode			= L"Screen Mode Id";
static const LPCWSTR	csz_DesktopWidthKey		= L"desktopwidth";
static const LPCWSTR	csz_DesktopHeightKey	= L"desktopheight";
static const LPCWSTR	csz_DesktopBppKey		= L"session bpp";
static const LPCWSTR	csz_SoundModeKey		= L"audiomode";
static const LPCWSTR	csz_DisableWallpaperKey = L"disable wallpaper";
static const LPCWSTR	csz_DisableThemesKey	= L"disable themes";
static const LPCWSTR	csz_DisableMenuAnimKey	= L"disable menu anims";
static const LPCWSTR	csz_DisableFullWndDragModeKey = L"disable full window drag";



DlgRDPSettings::DlgRDPSettings()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_RDP_SETTINGS);
}

DlgRDPSettings::~DlgRDPSettings()
{
}

CStringW DlgRDPSettings::GetFileName()
{
	return TheApp.GetRootFolderW() + L"settings.rdp";
}

BOOL DlgRDPSettings::OnInitDialog()
{
	try {
		SetDefault();

		CStringW rdpFileName = this->GetFileName();

		if (CCommon::FileIsExistW(rdpFileName)) {
			ReadRDPFile(rdpFileName);
			ImportRDPKeys();
		}

		m_wndWidth.AttachDlgItem(m_hWnd, IDC_WIDTH);
		m_wndHeight.AttachDlgItem(m_hWnd, IDC_HEIGHT);

		FillScreenSettings();
		FillSoundModeComboBox();
				
		m_wndScreenMode.AttachDlgItem(m_hWnd, IDC_SCREEN_MODE);
		m_wndScreenMode.InsertString(0, "Full-screen mode");
		m_wndScreenMode.InsertString(1, "Window mode");
		m_wndScreenMode.SetCurSel((m_screenMode==0) ? 0 : 1);
		OnScreenModeChanged(true);

		
		// Fill "Performance" section
		::SendDlgItemMessage(m_hWnd, IDC_DISABLE_WALLPAPER,		BM_SETCHECK, (WPARAM) m_disableWallpaper, 0);
		::SendDlgItemMessage(m_hWnd, IDC_DISABLE_THEMES,		BM_SETCHECK, (WPARAM) m_disableThemes, 0);
		::SendDlgItemMessage(m_hWnd, IDC_DISABLE_MENU_ANIM,		BM_SETCHECK, (WPARAM) m_disableMenuAnim, 0);
		::SendDlgItemMessage(m_hWnd, IDC_DISABLE_FULL_WND_DRAG, BM_SETCHECK, (WPARAM) m_disableFullWndDragMode, 0);
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);		
	}	

	return TRUE;
}


BOOL DlgRDPSettings::OnEndDialog(BOOL ok)
{
	if (ok) 
	{
		if (!SaveData())	// Validate data at first
			return FALSE;

		WriteRDPFile();
	}
	return TRUE;
}


INT_PTR DlgRDPSettings::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) 
	{
	case WM_COMMAND:

		if (LOWORD(wParam)==IDC_SCREEN_MODE && HIWORD(wParam)==CBN_SELCHANGE) { 
			OnScreenModeChanged(false);
			return TRUE;
		}
		
		break;
	}
	return 0;
}

void DlgRDPSettings::OnScreenModeChanged(bool init)
{
	BOOL enabled = (m_wndScreenMode.GetCurSel()==1);

	::EnableWindow(m_wndWidth,  enabled);
	::EnableWindow(m_wndHeight, enabled);

	if (enabled) {
		if (!init) m_wndWidth.SetCurSel(3);
		if (!init) m_wndHeight.SetCurSel(3);
	}
	else {
		m_wndWidth.SetTextA("");
		m_wndHeight.SetTextA("");
	}
}


void DlgRDPSettings::SetDefault()
{
	// Collect current display settings as a reference	
	//HDC	hDC = ::GetDC(NULL);
	//int	cx = ::GetDeviceCaps(hDC, HORZRES);
	//int	cy = ::GetDeviceCaps(hDC, VERTRES);
	//int	bpp = ::GetDeviceCaps(hDC, BITSPIXEL);
	//::ReleaseDC(NULL, hDC);

	m_screenMode = 0;
	m_desktopWidth	= 0; //cx
	m_desktopHeight	= 0; //cy
	m_sessionBpp	= 16;
	m_soundMode		= SM_playLocal;

	m_disableWallpaper = m_disableThemes = m_disableMenuAnim = m_disableFullWndDragMode = true;
}


// Validate all fields of dialog and then read data from those fields into related class members.
BOOL DlgRDPSettings::SaveData()
{
	BOOL	bTranslated = FALSE;

	m_screenMode = (m_wndScreenMode.GetCurSel()==0) ? 0 : 1;

	m_desktopWidth = ::GetDlgItemInt(m_hWnd, IDC_WIDTH, &bTranslated, FALSE);
	if (!bTranslated) m_desktopWidth = 0;

	m_desktopHeight = ::GetDlgItemInt(m_hWnd, IDC_HEIGHT, &bTranslated, FALSE);
	if (!bTranslated) m_desktopHeight = 0;

	// Validate "Color"
	//
	HWND	hColorCB = ::GetDlgItem(m_hWnd, IDC_BPP);
	LRESULT	idx = ::SendMessage(hColorCB, CB_GETCURSEL, 0, 0);
	UINT	newBpp = ::SendMessage(hColorCB, CB_GETITEMDATA, (WPARAM) idx, 0);
	if ((idx == -1) || (newBpp == CB_ERR))
		newBpp = 0;	// Something wrong! Use default value

	// Validate "Sound Mode"
	//
	HWND	hSoundCB = ::GetDlgItem(m_hWnd, IDC_SOUND_MODE);
	idx = ::SendMessage(hSoundCB, CB_GETCURSEL, 0, 0);
	UINT	newSoundMode = ::SendMessage(hSoundCB, CB_GETITEMDATA, (WPARAM) idx, 0);
	if ((idx == -1) || (newSoundMode < 0) || (newSoundMode >= SM_COUNT))
		newSoundMode = SM_playLocal;	// Something wrong! Use default value


	// Now save values into class members
	m_sessionBpp = newBpp;
	m_soundMode = (SoundMode) newSoundMode;
	m_disableWallpaper		 = (::SendDlgItemMessage(m_hWnd, IDC_DISABLE_WALLPAPER, BM_GETCHECK, 0, 0)		== BST_CHECKED);
	m_disableThemes			 = (::SendDlgItemMessage(m_hWnd, IDC_DISABLE_THEMES, BM_GETCHECK, 0, 0)			== BST_CHECKED);
	m_disableMenuAnim		 = (::SendDlgItemMessage(m_hWnd, IDC_DISABLE_MENU_ANIM, BM_GETCHECK, 0, 0)		== BST_CHECKED);
	m_disableFullWndDragMode = (::SendDlgItemMessage(m_hWnd, IDC_DISABLE_FULL_WND_DRAG, BM_GETCHECK, 0, 0)	== BST_CHECKED);

	return TRUE;
}

void DlgRDPSettings::ReadRDPFile(LPCWSTR fileName)
{
	RLStream stream;
	stream.ReadFromFileW(fileName);

	CStringW file = stream.GetString0W();

	m_rdpFile.clear();
	m_rdpFile.reserve(1024);

	int		 lineStart = (file[0] == UNICODE_FILE_HEADER) ? 1 : 0;
	int		 lineEnd;
	int		 nextLine;
	CStringW line;
	do
	{
		// skip leading spaces
		while (iswspace(file[lineStart])) 
			++lineStart;

		nextLine = file.Find('\n', lineStart) + 1;

		// skip trailing spaces
		lineEnd = (nextLine ? nextLine : file.GetLength()) - 1;
		while ((lineStart < lineEnd) && iswspace(file[lineEnd-1]))
			--lineEnd;

		line = file.Mid(lineStart, (lineEnd - lineStart));
		if (!line.IsEmpty())
			m_rdpFile.push_back(line);

		lineStart = nextLine;
	}
	while (nextLine);
}


void DlgRDPSettings::WriteRDPFile()
{
	try {

		// export RDP class members into m_rdpFile
		SetRdpIntValue(csz_ScreenMode,			m_screenMode);
		SetRdpIntValue(csz_DesktopWidthKey,		m_desktopWidth);
		SetRdpIntValue(csz_DesktopHeightKey,	m_desktopHeight);
		SetRdpIntValue(csz_DesktopBppKey,		m_sessionBpp);
		SetRdpIntValue(csz_SoundModeKey,		m_soundMode);
		SetRdpIntValue(csz_DisableWallpaperKey,			m_disableWallpaper);
		SetRdpIntValue(csz_DisableThemesKey,			m_disableThemes);
		SetRdpIntValue(csz_DisableMenuAnimKey,			m_disableMenuAnim);
		SetRdpIntValue(csz_DisableFullWndDragModeKey,	m_disableFullWndDragMode);


		RLStream stream;
		stream.SetMinCapasity(2048);

		stream.AddUINT16(UNICODE_FILE_HEADER);

		CStringW	eol(L"\r\n");
		size_t	size = m_rdpFile.size();
		for (size_t i = 0; i < size; ++i)
		{
			stream.AddString0W(m_rdpFile[i]);
			stream.AddString0W(eol);
		}

		stream.WriteToFileW(this->GetFileName());
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);		
	}
}



// Import supported keys from m_rdpFile
void DlgRDPSettings::ImportRDPKeys()
{
	int	val;

	if (GetRdpIntValue(csz_ScreenMode, val)) {
		m_screenMode  = (UINT32)abs(val);
		if (m_screenMode>1) m_screenMode=0;
	}

	if (GetRdpIntValue(csz_DesktopWidthKey, val))		m_desktopWidth  = (UINT32)abs(val);
	if (GetRdpIntValue(csz_DesktopHeightKey, val))		m_desktopHeight = (UINT32)abs(val);
	if (GetRdpIntValue(csz_DesktopBppKey, val))			m_sessionBpp	= (UINT32) val;

	if (GetRdpIntValue(csz_SoundModeKey, val) && (val >= 0) && (val < SM_COUNT))
		m_soundMode = (SoundMode) val;

	if (GetRdpIntValue(csz_DisableWallpaperKey, val))		m_disableWallpaper		 = (val != 0);
	if (GetRdpIntValue(csz_DisableThemesKey, val))			m_disableThemes			 = (val != 0);
	if (GetRdpIntValue(csz_DisableMenuAnimKey, val))		m_disableMenuAnim		 = (val != 0);
	if (GetRdpIntValue(csz_DisableFullWndDragModeKey, val)) m_disableFullWndDragMode = (val != 0);
}


BOOL DlgRDPSettings::GetRdpValue(LPCWSTR wszKey, CStringW& value, CStringW& type)
{
	int		keyLen = ::wcslen(wszKey);
	size_t	size = m_rdpFile.size();
	for (size_t i = 0; i < size; ++i)
	{
		LPCWSTR	ptr = (LPCWSTR) m_rdpFile[i];
		LPCWSTR	token;
		UINT	tokenLen;

		ptr = NextToken(ptr, token, tokenLen, ':');
		if (ptr && (tokenLen == keyLen) && (::wcsncmp(wszKey, token, keyLen) == 0))	// key found!
		{
			ptr = NextToken(ptr, token, tokenLen, ':');		// search for type
			if (ptr)	// type is found
			{
				type = CStringW(token, tokenLen);
				ptr = NextToken(ptr, token, tokenLen, ':');	// search for value
				value = (ptr ? CStringW(token, tokenLen) : CStringW());
				return TRUE; 
			}
		}
	}

	type.Empty();
	value.Empty();
	return FALSE;	// key not found
}


BOOL DlgRDPSettings::GetRdpIntValue(LPCWSTR wszKey, int& val)
{
	CStringW	type;
	CStringW	res;
	if (!GetRdpValue(wszKey, res, type))
		return FALSE;

	ASSERT(type == L"i");
	val = _wtoi((LPCWSTR) res);
	return TRUE;
}


void DlgRDPSettings::SetRdpIntValue(LPCWSTR wszKey, int val)
{
	LPCWSTR type = L"i";

	WCHAR		buf[64];
	_ultow(val,	buf, 10);

	CStringW	newValue = CStringW(wszKey) + L':' + type + L':' + buf;

	int			keyLen = ::wcslen(wszKey);
	size_t		size = m_rdpFile.size();
	for (size_t i = 0; i < size; ++i)
	{
		LPCWSTR	ptr = (LPCWSTR) m_rdpFile[i];
		LPCWSTR	token;
		UINT	tokenLen;

		ptr = NextToken(ptr, token, tokenLen, ':');
		if (ptr && (tokenLen == keyLen) && (::wcsncmp(wszKey, token, keyLen) == 0))	// key found!
		{
			// set new value of this key
			m_rdpFile[i] = newValue;
			return;
		}
	}

	// Key not found, append it to m_rdpFile
	m_rdpFile.push_back(newValue);
}

CStringA DlgRDPSettings::ConevrtIntToString(int val)
{
	if (val==0) return "Default";

	CStringA buffer;
	buffer.Format("%u", val);
	return buffer;
}


void DlgRDPSettings::FillScreenSettings()
{
	// setup width
	{
		int values[] = {0, 640, 800, 1024, 1280, 1600};
		for (int i=0; i<COUNTOF(values); i++){
			m_wndWidth.InsertString(i, ConevrtIntToString(values[i])); 
		}
		m_wndWidth.SetTextA(ConevrtIntToString(m_desktopWidth));
	}

	// setup height
	{
		int values[] = {0, 480, 600, 768, 1024, 1200};
		for (int i=0; i<COUNTOF(values); i++){
			m_wndHeight.InsertString(i, ConevrtIntToString(values[i]));
		}
		m_wndHeight.SetTextA(ConevrtIntToString(m_desktopHeight));
	}

	// Re-fill color mode combo-box
	{
		HWND	hColorCB = ::GetDlgItem(m_hWnd, IDC_BPP);
		
		// List of color modes (constant). Keep same order!
		static const int	COLOR_MODES[] = { 8, 15, 16, 24, 32 };	
		LPCSTR	colorModes[] = { "256 Colors (8 bit)",
								 "High Color (15 bit)",
								 "High Color (16 bit)",
								 "True Color (24 bit)",
								 "Highest Quality (32 bit)"
								};

		int	curItem = 0;
		for (int i = 0; i < COUNTOF(COLOR_MODES); ++i)
		{
			int	index = ::SendMessage(hColorCB, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)colorModes[i]);
			::SendMessage(hColorCB, CB_SETITEMDATA, index, COLOR_MODES[i]);

			if (COLOR_MODES[i] == m_sessionBpp) curItem=i;
		}
		
		::SendMessage(hColorCB, CB_SETCURSEL, (WPARAM)curItem, 0);
	}
}



void DlgRDPSettings::FillSoundModeComboBox()
{
	RLWnd wnd;
	wnd.AttachDlgItem(m_hWnd, IDC_SOUND_MODE);

	LPCSTR modeList[] = { "Bring to this computer","Leave at remote computer","Do not play" };

	wnd.SendMessage(CB_RESETCONTENT, 0, 0);
	int curSel = 0;
	for (int i = 0; i < SM_COUNT; ++i)
	{		
		int	index = wnd.SendMessage(CB_ADDSTRING, 0, (LPARAM) (LPCSTR)modeList[i]);
		wnd.SendMessage(CB_SETITEMDATA, index, i);

		if (m_soundMode==modeList[i]) {
			curSel = i;
		}
	}

	wnd.SendMessage(CB_SETCURSEL, curSel, 0);
}



// Find next token and move pointer forward. Tokens separated by ':' sign.
// Function returns NULL if no more tokens found.
LPCWSTR DlgRDPSettings::NextToken(LPCWSTR ptr, LPCWSTR& token, UINT& len, WCHAR tokSep)
{
	ASSERT(ptr && tokSep);

	while (iswspace(*ptr))	// remove leading spaces
		++ptr;

	if (!*ptr)
		return NULL;	// no more tokens

	
	LPCWSTR	end;
	LPCWSTR	next = ::wcschr(ptr, tokSep);
	if (next)
		end = (next++) - 1;
	else
	{
		next = ptr + ::wcslen(ptr);
		end = next - 1;
	}

	while ((ptr < end) && iswspace(*end))	// remove trailing spaces
		--end;

	token = ptr;
	len = (end - ptr + 1);
	return next;
}
