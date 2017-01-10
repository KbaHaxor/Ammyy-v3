#include "stdafx.h"
#include "AmmyyApp.h"
#include "DlgSettings.h"
#include "DlgPermissionList.h"
#include "DlgHttpProxy.h"
#include "DlgRDPSettings.h"
#include "DlgMain.h"
#include "DlgExternalPorts.h"
#include "DlgEncoderList.h"
#include "resource.h"
#include "Common.h"
#include <dsound.h>
#include "../target/TrMain.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


#define TRANSLATE(ctrl_id, phrase_id) ::SetDlgItemTextW(m_hWnd, ctrl_id,   rlLanguages.GetValue(phrase_id))
#define SETCHECKBOX(ctrlid, value) RLWndButton(::GetDlgItem(m_hWnd, ctrlid)).SetCheckBool(value)
#define GETCHECKBOX(ctrlid)        RLWndButton(::GetDlgItem(m_hWnd, ctrlid)).GetCheckBool()


BOOL DlgSettingsPage1::OnInitDialog()
{		
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_FILES_FOLDER), SW_HIDE);

	m_debugLog.AttachDlgItem(m_hWnd, IDC_DEBUG_LOG);
	m_debugLog.SetCheckBool(settings.m_debugLog);

	m_runAsSystem.AttachDlgItem(m_hWnd, IDC_RUN_SYSTEM_VISTA);
	m_runAsSystem.SetCheckBool(settings.m_runAsSystemOnVista);

	m_accessFiles.AttachDlgItem(m_hWnd, IDC_ACCESS_FILES);
	m_accessFiles.SetCheckBool(settings.m_impersonateFS);	

	CStringW str;
	str.Format(rlLanguages.GetValue(D_RUN_UNDER_SYSTEM), "Windows Vista/7/2003/2008");
	::SetDlgItemTextW(m_hWnd, IDC_RUN_SYSTEM_VISTA, str);
	TRANSLATE(IDC_DEBUG_LOG,	D_LOGGING_FOR_DEBUG);
	TRANSLATE(IDC_ACCESS_FILES,	D_ACCESS_FILES_CUR_USER);

	// audio
	{
		m_sPlay.m_comboBox.AttachDlgItem(m_hWnd, IDC_AUDIO_PLAY);
		m_sRecd.m_comboBox.AttachDlgItem(m_hWnd, IDC_AUDIO_RECD);
		
		m_sPlay.m_index = 0;
		m_sRecd.m_index = 0;
		m_sPlay.lpGuidSettings = &settings.m_audioDevicePlay;
		m_sRecd.lpGuidSettings = &settings.m_audioDeviceRecd;

		GUID guid = {0};

		m_sRecd.Add(&Settings::GuidOFF, "OFF");
		m_sRecd.Add(&guid,              "Default device");
		m_sPlay.Add(&guid,              "Default device");

		// this shoule be first, to avoid bug with truncated device name On Vista
		HRESULT hr1 = ::DirectSoundCaptureEnumerate( DSEnumCallback, (LPVOID)&m_sRecd);
		ASSERT(SUCCEEDED(hr1));

		HRESULT hr2 = ::DirectSoundEnumerate( DSEnumCallback, (LPVOID)&m_sPlay);
		ASSERT(SUCCEEDED(hr2));

		m_sPlay.m_comboBox.SetCurSel(m_sPlay.m_index);
		m_sRecd.m_comboBox.SetCurSel(m_sRecd.m_index);
	}

	return TRUE;
}

void DlgSettingsPage1::OnOK()
{
	settings.m_debugLog			  = m_debugLog.GetCheckBool();
	settings.m_runAsSystemOnVista = m_runAsSystem.GetCheckBool();
	settings.m_impersonateFS	  = m_accessFiles.GetCheckBool();
	m_sPlay.OnOK();
	m_sRecd.OnOK();
}

void DlgSettingsPage1::OnKeyDown(LPMSG msg)
{
	if (msg->wParam!='P')  return;
	if ((::GetKeyState(VK_LSHIFT)   & 0x8000)==0) return;
	if ((::GetKeyState(VK_LCONTROL) & 0x8000)==0) return;

	// Ctrl+Shift+P was pressed

	::SetDlgItemTextW(m_hWnd, IDC_FILES_FOLDER, (LPCWSTR)TheApp.GetRootFolderW());	
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_FILES_FOLDER), SW_SHOW);
}

INT_PTR DlgSettingsPage1::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_NOTIFY:
		{
			NMHDR* pNMHdr = (NMHDR*)lParam;

			if (pNMHdr->code==PSN_TRANSLATEACCELERATOR) 
			{
				PSHNOTIFY* p = (PSHNOTIFY*)lParam;
				if (p) {
					LPMSG msg = (LPMSG)p->lParam;
					if (msg) OnKeyDown(msg);
				}	
			}
			break;
		};
	}
	return 0;
}


void DlgSettingsPage2::OnHttpProxy()
{
	DlgHttpProxy dlg;
	dlg.DoModal(m_hWnd);
}

void DlgSettingsPage2::OnExtTCPports()
{
	DlgExternalPorts dlg;
	dlg.DoModal(m_hWnd);
}

void DlgSettingsPage3::OnBtnPermissions()
{
	if (settings.m_protectPermissions) {
		if (TrMain::GetStateTotalWAN()==InteropCommon::STATE::OPERATOR_AUTHORIZED) {
			::MessageBoxA(m_hWnd, "Ammyy Permissions are protected from remote operator access", TheApp.m_appName, MB_ICONERROR);
			return;
		}
	}

	DlgPermissionList dlg;
	dlg.DoModal(m_hWnd);
}


INT_PTR DlgSettingsPage3::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		{
			WORD wItem = LOWORD(wParam);

			if (wItem == IDC_PERMISSIONS) {
				OnBtnPermissions();
				return TRUE;			
			}
			break;
		};
	}
	return 0;
}

INT_PTR DlgSettingsPage4::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		{
			WORD wItem = LOWORD(wParam);

			if (wItem == IDC_RDP_SETTINGS) {
				DlgRDPSettings dlg;
				dlg.DoModal(m_hWnd);
				return TRUE;			
			}
			if (wItem == IDC_ENCODER_LIST) {
				DlgEncoderList dlg;
				dlg.DoModal(m_hWnd);
				return TRUE;
			}
			break;
		};
	}
	return 0;
}

BOOL DlgSettingsPage2::OnInitDialog()
{
	m_wndAllowIncomingByIP.AttachDlgItem(m_hWnd, IDC_INCOMING_LAN);
	m_wndAllowDirectTCP.AttachDlgItem   (m_hWnd, IDC_DIRECT_TCP);
	m_useWAN.AttachDlgItem(m_hWnd, IDC_USE_WAN);

	m_routerText.AttachDlgItem(m_hWnd, IDC_ROUTER_TEXT);

	m_routerType.AttachDlgItem(m_hWnd, IDC_ROUTER_TYPE);
	m_routerType.InsertString(0, "Public");
	m_routerType.InsertString(1, "Private");
	m_routerType.SetCurSel(settings.m_privateRoutersUse ? 1 : 0);

	m_useWAN.SetCheckBool(settings.m_useWAN);

	m_wndAllowIncomingByIP.SetCheckBool(settings.m_allowIncomingByIP);
	m_wndAllowDirectTCP.SetCheckBool(settings.m_allowDirectTCP);

	OnRouterTypeChanged();
	OnUseWANChanged();
	OnDirectTCPChanged();

	// set primary TCP port
	{
		RLWnd wndPort(::GetDlgItem(m_hWnd, IDC_PORT));
		char buffer[32];
		sprintf(buffer, "%u", DEFAULT_INCOME_PORT);
		wndPort.SetTextA(buffer);

	}

	return TRUE;
}


BOOL DlgSettingsPage2::OnKillActive()
{
	if (m_routerType.GetCurSel()==1) { // if private router
		CStringA router = m_routerText.GetTextA();

		if (router.Find(":")<=0) {
			::MessageBox(m_hWnd, "Invalid router", "Ammyy Admin", MB_ICONERROR);
			return FALSE;
		}
	}
	return TRUE;
}

CStringA DlgSettingsPage2::GetRouterText()
{
	CStringA src = m_routerText.GetTextA();
	src.Replace("\r\n", "|");
	
	CStringA dst;
	dst.GetBuffer(src.GetLength());

	for (int i=0;true;) {
		int j = src.Find("|", i);

		CStringA line = (j<0) ? src.Mid(i) : src.Mid(i, j-i);

		line.TrimRight();
		line.TrimLeft();
		if (!line.IsEmpty()) {
			dst += line + "|";
		}
		if (j<0) break;
		i = j+1;
	}

	return dst;
}

void DlgSettingsPage2::OnOK()
{
	settings.m_useWAN = m_useWAN.GetCheckBool();
	settings.m_allowIncomingByIP = m_wndAllowIncomingByIP.GetCheckBool();
	settings.m_allowDirectTCP    = m_wndAllowDirectTCP.GetCheckBool();

	settings.m_privateRoutersUse = (m_routerType.GetCurSel()==1);

	if (settings.m_privateRoutersUse)
		settings.m_privateRouters = this->GetRouterText();
}

void DlgSettingsPage2::OnRouterTypeChanged()
{	
	int index = m_routerType.GetCurSel();

	m_routerText.SetReadOnly(index==0);

	CStringA str = (index==0) ? settings.m_publicRouters : settings.m_privateRouters;
	str.Replace("|", "\r\n");

	m_routerText.SetTextA(str);
}

void DlgSettingsPage2::OnUseWANChanged()
{
	RLWnd wndRouter(::GetDlgItem(m_hWnd, IDC_ROUTER_STATIC));
	RLWnd wndHttpProxy(::GetDlgItem(m_hWnd, IDC_HTTP_PROXY));
	RLWnd wndExtTCPports(::GetDlgItem(m_hWnd, IDC_EXTERNAL_TCP_PORTS));
	RLWnd wndUseDirectTCP(::GetDlgItem(m_hWnd, IDC_DIRECT_TCP));

	int v = (m_useWAN.GetCheckBool()) ? SW_SHOW : SW_HIDE;
	::ShowWindow(m_routerType, v);
	::ShowWindow(m_routerText, v);
	::ShowWindow(wndRouter,    v);
	::ShowWindow(wndHttpProxy, v);
	::ShowWindow(wndHttpProxy, v);
	::ShowWindow(wndExtTCPports, v);
	::ShowWindow(wndUseDirectTCP, v);

}

void DlgSettingsPage2::OnDirectTCPChanged()
{
	RLWndButton wndUseDirectTCP(::GetDlgItem(m_hWnd, IDC_DIRECT_TCP));
	RLWnd       wndExtTCPports (::GetDlgItem(m_hWnd, IDC_EXTERNAL_TCP_PORTS));

	int v = (wndUseDirectTCP.GetCheckBool()) ? SW_SHOW : SW_HIDE;

	::ShowWindow(wndExtTCPports, v);
}


INT_PTR DlgSettingsPage2::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		{
			WORD wItem = LOWORD(wParam);

			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				if (wItem==IDC_ROUTER_TYPE)  { OnRouterTypeChanged(); return TRUE; }
			}
			
			if (HIWORD(wParam)==BN_CLICKED) {
				if (wItem==IDC_USE_WAN)				{ OnUseWANChanged();      return TRUE; }
				if (wItem==IDC_HTTP_PROXY)			{ OnHttpProxy();          return TRUE; }
				if (wItem==IDC_EXTERNAL_TCP_PORTS)	{ OnExtTCPports(); 		   return TRUE; }
				if (wItem==IDC_DIRECT_TCP)			{ OnDirectTCPChanged();   return TRUE; }
			}
			
			break;
		};
	}
	return 0;
}

void DlgSettingsPage1::CDeviceList::Add(LPGUID lpGuid, LPCSTR lpDescription)
{
	m_comboBox.InsertString(-1, lpDescription);
	m_guids.AddRaw(lpGuid, sizeof(GUID));

	if (*lpGuid==*lpGuidSettings) {
		m_index = (m_guids.GetLen() / sizeof(GUID)) - 1;
	}
}

void DlgSettingsPage1::CDeviceList::OnOK()
{
	int index = m_comboBox.GetCurSel();
	LPCGUID lpGuid = ((LPCGUID)m_guids.GetBuffer()) + index;
	*lpGuidSettings = *lpGuid;
}

BOOL CALLBACK DlgSettingsPage1::DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
{
	// skip default device
	if (lpGuid) {
		((CDeviceList*)lpContext)->Add(lpGuid, lpcstrDescription);
	}

	return TRUE;
}

BOOL DlgSettingsPage3::OnInitDialog()
{
	SETCHECKBOX(IDC_START_CLIENT,		settings.m_startClient);
	SETCHECKBOX(IDC_CAPTURE_HINTS,		settings.m_captureHints);
	SETCHECKBOX(IDC_OFF_BACKGROUND,		settings.m_turnOffBackground);
	SETCHECKBOX(IDC_OFF_VISUAL_EFFECTS, settings.m_turnOffVisualEffects);
	SETCHECKBOX(IDC_OFF_COMPOSITION,    settings.m_turnOffComposition);

	TRANSLATE( IDC_START_CLIENT,		D_START_CLIENT);
	TRANSLATE( IDC_CAPTURE_HINTS,		D_SHOW_HINTS);
	TRANSLATE( IDC_OFF_BACKGROUND,		D_OFF_DESKTOP_BACKGROUND);
	TRANSLATE( IDC_OFF_VISUAL_EFFECTS,	D_OFF_DESKTOP_EFFECTS);
	TRANSLATE( IDC_OFF_COMPOSITION,		D_OFF_DESKTOP_COMPOSITION);
	TRANSLATE( IDC_PERMISSIONS,			D_ACCESS_PERMISSIONS);
	
	return TRUE;
}

void DlgSettingsPage3::OnOK()
{
	settings.m_startClient			= GETCHECKBOX(IDC_START_CLIENT);
	settings.m_captureHints			= GETCHECKBOX(IDC_CAPTURE_HINTS);
	settings.m_turnOffBackground	= GETCHECKBOX(IDC_OFF_BACKGROUND);
	settings.m_turnOffVisualEffects = GETCHECKBOX(IDC_OFF_VISUAL_EFFECTS);
	settings.m_turnOffComposition	= GETCHECKBOX(IDC_OFF_COMPOSITION);
}



BOOL DlgSettingsPage4::OnInitDialog()
{
	TRANSLATE(IDC_REQUEST_ROUTER,	D_REQUEST_ROUTER);
	TRANSLATE(IDC_WARN_FULL_SCREEN, D_WARN_FULL_SCREEN);
	::SetDlgItemTextA(m_hWnd, IDC_COPY_FILE_TIME, "Copy date/time of files");

	SETCHECKBOX(IDC_REQUEST_ROUTER,	  settings.m_requestRouter);
	SETCHECKBOX(IDC_WARN_FULL_SCREEN, settings.m_warnFullScreen);
	SETCHECKBOX(IDC_COPY_FILE_TIME,   settings.m_copyFileTime);

	m_encryption.AttachDlgItem(m_hWnd, IDC_ENCRYPTION);
	m_encryption.InsertString(-1, "AES-256");
	m_encryption.InsertString(-1, "AES-256 + RSA-1024");
	m_encryption.SetCurSel(settings.m_encryption-2);

	return TRUE;
}

void DlgSettingsPage4::OnOK()
{
	settings.m_requestRouter      = GETCHECKBOX(IDC_REQUEST_ROUTER);
	settings.m_warnFullScreen     = GETCHECKBOX(IDC_WARN_FULL_SCREEN);
	settings.m_copyFileTime		  = GETCHECKBOX(IDC_COPY_FILE_TIME);
	settings.m_encryption		  = m_encryption.GetCurSel() + 2;
}
