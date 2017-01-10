#include "stdafx.h"
#include "TrDesktopUtils.h"
#include "TrMain.h"
#include "TrService.h"
#include "../main/ImpersonateWrapper.h"
#include "../main/AmmyyApp.h"
#include "../main/DynamicFn.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

typedef HRESULT (WINAPI *__DwmEnableComposition) (UINT uCompositionAction);

#define DWM_EC_DISABLECOMPOSITION         0
#define DWM_EC_ENABLECOMPOSITION          1


TrDesktopUtils::TrDesktopUtils()
{
	m_hDwmapi = NULL;
	ResetAll(); // TODO: no need it
}


TrDesktopUtils::~TrDesktopUtils()
{
	if (m_hDwmapi) {
		if (::FreeLibrary(m_hDwmapi)==0)
			_log.WriteError("FreeLibrary() error=%d", ::GetLastError());
	}
}


bool TrDesktopUtils::SystemParametersInfoMy(UINT uiAction, UINT uiParam, PVOID pvParam)
{
	if (::SystemParametersInfoW(uiAction, uiParam, pvParam, 0)!=0) 
		return true;
	_log2.Print(LL_ERR, VTCLOG("SystemParametersInfo(%X) error=%d"), uiAction, ::GetLastError());
	return false;
}


void TrDesktopUtils::ResetAll()
{
	m_wallpaper = false;   // Apply1
	m_composition = false; // Apply3
	for (int i=0; i<COUNTOF(m_effects); i++) m_effects[i] = false; // Apply2
	//m_activeDesktop = false;
}


bool TrDesktopUtils::IsNeedRestore()
{
	if (m_wallpaper) return true;
	if (m_composition) return true;
	for (int i=0; i<COUNTOF(m_effects); i++) { if (m_effects[i]) return true; }

	return false;
}


void TrDesktopUtils::ApplyAll()
{
	ImpersonateWrapper impersonate;

	ResetAll();
	Apply1();
	Apply2();
	Apply3();

	// refresh desktop, but waiting not more than 2 seconds
	if (::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETDESKWALLPAPER, 0, SMTO_ABORTIFHUNG, 2000, NULL)==0)
		_log2.Print(LL_ERR, VTCLOG("WARN: SendMessageTimeout() error=%d"), ::GetLastError());

	_log2.Print(LL_INF, VTCLOG("ApplyAll()"));
}



void TrDesktopUtils::RestoreAll()
{
	if (IsNeedRestore())
	{
		ImpersonateWrapper impersonate;

		//CoInitialize(NULL);
		//RestoreActiveDesktop();
		//CoUninitialize();

		Restore1();
		Restore2();
		Restore3();
	}
}



void TrDesktopUtils::Apply1()
{
	if (!settings.m_turnOffBackground) return;	// no need by settings

	// SystemParametersInfoMy(SPI_GETDESKWALLPAPER, MAX_PATH, m_path);

	// Tell all applications that there is no wallpaper, Note that this doesn't change the wallpaper registry setting!
	if (SystemParametersInfoMy(SPI_SETDESKWALLPAPER, 0, "")) {	// SPIF_SENDCHANGE
		//CoInitialize(NULL);
		//KillActiveDesktop();
		//CoUninitialize();
		m_wallpaper = true;
	}
}

void TrDesktopUtils::Restore1()
{
	//restore to default wallpaper
	if (m_wallpaper) {
		m_wallpaper = false;
		SystemParametersInfoMy(SPI_SETDESKWALLPAPER, 0, NULL);		// SPIF_SENDCHANGE
	}
}

//_________________________________________________________________________________________________________________


#define SPI_SETDROPSHADOW          0x1025
#define SPI_SETCLIENTAREAANIMATION 0x1043

static UINT EffectsUI[] = {   
//===SPI_SETUIEFFECTS - on (Win7)
SPI_SETMENUANIMATION,		   //"Fade or slide menus into view",		"Ёффекты затухани€ или скольжени€ при обращении к меню"		(Win7)
SPI_SETTOOLTIPANIMATION,	   // "Fade or slide ToolTips into view",	"Ёффекты затухани€ или скольжени€ при по€влении подсказок"	(Win7)
SPI_SETSELECTIONFADE,		   // "Fade out menu items after clicking"	(Win7) // I don't know what it
SPI_SETCURSORSHADOW,		   // "Show shadows under mouse pointer"	(Win7)
SPI_SETDROPSHADOW,			   // "Show shadows under windows",			"ќтображать тени, отбрасываемые окнами" (Win7)
SPI_SETCOMBOBOXANIMATION,	   // "Slide open combo boxes",				"—кольжение при раскрытии списков" (Win7)
SPI_SETLISTBOXSMOOTHSCROLLING, // "Smooth-scroll list boxes" (Win7)

SPI_SETANIMATION,			  // "Animate windows when minimizing and maximizing",	"јнимаци€ окон при свертывании и развертывании" (Win7)
SPI_SETCLIENTAREAANIMATION,	  // "Animate controls and elements inside windows",	"јнимированные элементы управлени€ и элементы внутри окна" (Win7), не знаю дл€ чего...
//SPI_SETGRADIENTCAPTIONS		  - no effect on Win7
//SPI_SETFLATMENU				  - no effect on Win7
};

void TrDesktopUtils::Apply2()
{	
	if (!settings.m_turnOffVisualEffects) return;
	for (int i=0; i<sizeof(EffectsUI)/sizeof(EffectsUI[0]); i++) 
	{	
		UINT uiAction = EffectsUI[i];

		if (uiAction==SPI_SETANIMATION)
		{
			ANIMATIONINFO ai;
			ai.cbSize = sizeof(ANIMATIONINFO);
			ai.iMinAnimate = 0;

			if (!SystemParametersInfoMy(uiAction-1, sizeof(ANIMATIONINFO), &ai)) continue;
			if (ai.iMinAnimate == 0) continue; // not set
			ai.iMinAnimate = 0;
			if (!SystemParametersInfoMy(uiAction,   sizeof(ANIMATIONINFO), &ai)) continue;
		}
		else {
			if (uiAction==SPI_SETCLIENTAREAANIMATION && (TheApp.m_dwOSVersion < 0x60000))
				continue; // not supported on Win 2000/XP/2003

			BOOL v;
			if (!SystemParametersInfoMy(uiAction-1, 0, (void*)&v))  continue;
			if (v==FALSE) continue; // not set
			if (!SystemParametersInfoMy(uiAction,   0, (void*)FALSE)) continue;
		}
		m_effects[i] = true;
	}
}

void TrDesktopUtils::Restore2()
{	
	for (int i=0; i<sizeof(EffectsUI)/sizeof(EffectsUI[0]); i++) 
	{
		if (!m_effects[i]) continue; // was not set
		m_effects[i] = false;

		if (EffectsUI[i]==SPI_SETANIMATION) 
		{
			ANIMATIONINFO ai;
			ai.cbSize = sizeof(ANIMATIONINFO);
			ai.iMinAnimate = 1;

			SystemParametersInfoMy(EffectsUI[i], sizeof(ANIMATIONINFO), &ai);
		}
		else {
			SystemParametersInfoMy(EffectsUI[i], 0, (void*)TRUE);
		}
	}
}


//________________________________________________________________________________________________________________


void TrDesktopUtils::Apply3()
{	
	if (!settings.m_turnOffComposition) return;
	if (TheApp.m_dwOSVersion < 0x60000) return; // if early than Vista
	
	try {
		if (m_hDwmapi==NULL) {
			m_hDwmapi = ::LoadLibraryA("Dwmapi.dll");
			if (m_hDwmapi==NULL)
				throw RLException("Error %d while load Dwmapi", ::GetLastError());
		}

		__DwmEnableComposition _DwmEnableComposition = (__DwmEnableComposition)DynamicFnBase::GetProcAddr(m_hDwmapi, "DwmEnableComposition");
		HRESULT hr = (*_DwmEnableComposition)(DWM_EC_DISABLECOMPOSITION);
		if (SUCCEEDED(hr))
			m_composition = true;
		else
			throw RLException("DwmEnableComposition(0) error=%X", hr);
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}	
}

void TrDesktopUtils::Restore3()
{
	if (!m_composition) return; // no need to restore
	m_composition = false;
	try 
	{
		if (m_hDwmapi==NULL) 
			throw RLException("ERROR in Restore1()#1");

		__DwmEnableComposition _DwmEnableComposition = (__DwmEnableComposition)DynamicFnBase::GetProcAddr(m_hDwmapi, "DwmEnableComposition");
		HRESULT hr = (*_DwmEnableComposition)(DWM_EC_ENABLECOMPOSITION);
		if (FAILED(hr))
			throw RLException("DwmEnableComposition(1) error=%X", hr);
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}	
}



/*
void TrDesktopUtils::KillActiveDesktop()
{
	_log2.Print(LL_INF, VTCLOG("KillActiveDesktop"));

	// Contact Active Desktop if possible
	HRESULT result;
	IActiveDesktop* active_desktop = 0;
	result = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, 
		(void**)&active_desktop);
	if (result != S_OK) {
		_log2.Print(LL_ERR, VTCLOG("unable to access Active Desktop object:%x"), result);
		return;
	}

	// Get Active Desktop options
	COMPONENTSOPT options;
	options.dwSize = sizeof(options);
	result = active_desktop->GetDesktopItemOptions(&options, 0);
	if (result != S_OK) {
		_log2.Print(LL_ERR, VTCLOG("unable to fetch Active Desktop options:%x"), result);
		active_desktop->Release();
		return;
	}

	// Disable if currently active
	m_activeDesktop = (options.fActiveDesktop != 0);
	if (options.fActiveDesktop) {
		_log2.Print(LL_INF, VTCLOG("attempting to disable Active Desktop"));
		options.fActiveDesktop = FALSE;
		options.fEnableComponents = FALSE;
		result = active_desktop->SetDesktopItemOptions(&options, 0);
		if (result != S_OK) {
			_log2.Print(LL_ERR, VTCLOG("unable to disable Active Desktop:%x"), result);
			active_desktop->Release();
			return;
		}
	} else {
		_log2.Print(LL_INF, VTCLOG("Active Desktop not enabled - ignoring"));
	}

	active_desktop->ApplyChanges(AD_APPLY_REFRESH);
	active_desktop->Release();
}


void TrDesktopUtils::RestoreActiveDesktop()
{
	// Contact Active Desktop if possible
	HRESULT result;
	IActiveDesktop* active_desktop = 0;
	result = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, 
		(void**)&active_desktop);
	if (result != S_OK) {
		_log2.Print(LL_ERR, VTCLOG("unable to access Active Desktop object:%x"), result);
		return;
	}

	// Get Active Desktop options
	COMPONENTSOPT options;
	options.dwSize = sizeof(options);
	result = active_desktop->GetDesktopItemOptions(&options, 0);
	if (result != S_OK) {
		_log2.Print(LL_ERR, VTCLOG("unable to fetch Active Desktop options:%x"), result);
		active_desktop->Release();
		return;
	}

	// Re-enable if previously disabled
	if (m_activeDesktop) {
		m_activeDesktop = false;
		_log2.Print(LL_INF, VTCLOG("attempting to re-enable Active Desktop"));
		options.fActiveDesktop = TRUE;
		result = active_desktop->SetDesktopItemOptions(&options, 0);
		if (result != S_OK) {
			_log2.Print(LL_ERR, VTCLOG("unable to re-enable Active Desktop:%x"), result);
			active_desktop->Release();
			return;
		}
	}

	active_desktop->ApplyChanges(AD_APPLY_REFRESH);
	active_desktop->Release();
}
*/


// ______________________________________________________________________________________________________


int TrDesktopOptimizer::Find(LPCSTR name, bool add)
{
	int count = m_items.size();
	for (int i=0; i<count; i++) {
		if (m_items[i].name == name) return i;
	}

	if (add) {
		m_items.resize(count+1);
		m_items[count].count = 0;
		m_items[count].name  = name;
		return count;
	}

	throw RLException("Not found desktop '%s'", name);
}

void TrDesktopOptimizer::OnDesktopOpen(LPCSTR name)
{
	bool apply = (name!="Winlogon"); // no need to delete wallpaper for Winlogon desktop

	RLMutexLock l(m_mutex);

	int i = this->Find(name, true);
	if (m_items[i].count==0 && apply) m_items[i].item.ApplyAll(); // do Apply only in first time
	m_items[i].count++;
}


void TrDesktopOptimizer::OnDesktopClose(LPCSTR name)
{
	RLMutexLock l(m_mutex);
	
	int i = this->Find(name, false);
	m_items[i].count--;
	if (m_items[i].count==0) {
		m_items[i].item.RestoreAll();
		m_items.erase(m_items.begin()+i);
	}
}

void TrDesktopOptimizer::OnDesktopReset(LPCSTR name)
{
	RLMutexLock l(m_mutex);
	
	int i = this->Find(name, false);
	m_items[i].item.ResetAll();
}
