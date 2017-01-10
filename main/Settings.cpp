#include "stdafx.h"
#include "Settings.h"
#include "AmmyyApp.h"
#include "Common.h"
#include "../RL/RLStream.h"
#include "../RL/RLEncryptor01.h"
#include "../RL/RLEncryptor02.h"
#include "../RL/RLHttp.h"
#include "../RL/RLRegistry.h"
#include "resource.h"
#include "RLLanguages.h"
#include <algorithm>
#include "DlgHttpProxy.h"
#include "aaProtocol.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

static LPCSTR Settings_Key_v2="aSg@f1H_hjy!";

#define VERSION32(v1, v2, v3) ((v1<<20) + (v2<<10) + v3)


GUID Settings::GuidOFF = {0xFFFFFFFF, 0xFFFF, 0xFFFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


Settings::Settings()
{
	m_debugLog = false; // need, because can be accessed before Load()
}

void Settings::SetDefault()
{
	m_langId = RLLanguages::GetUserDefaultUILanguage();

	m_turnOffVisualEffects = m_turnOffComposition = m_turnOffBackground = true;
	m_requestRouter = true;
	m_privateRouters = "";
	m_privateRoutersUse = false;
	m_captureHints = true;
	m_warnFullScreen = true;

	RLHttp::m_proxyPort = 0;
	RLHttp::m_proxyHost = "";
	RLHttp::m_proxyUsername = "";
	RLHttp::m_proxyPassword = "";

	// take setting proxy from IE
	try {
		CStringA proxyHost;
		UINT16 proxyPort = DlgHttpProxy::GetIEProxy(proxyHost);
		if (proxyPort!=0 && (!proxyHost.IsEmpty())) {
			RLHttp::m_proxyPort = proxyPort;
			RLHttp::m_proxyHost = proxyHost;
		}
	}		
	catch(RLException& ex) {
		// it's not critical error, so use no proxy in this case
		_log.WriteError(ex.GetDescription());
	}


	m_operProtocol = 1; // Desktop 256 Kb - 1Mb
	m_operRemoteId = "";

	memset(&m_audioDevicePlay, sizeof(m_audioDevicePlay), 0);
	memset(&m_audioDeviceRecd, sizeof(m_audioDeviceRecd), 0);
	m_debugLog = false;
	m_runAsSystemOnVista = true;
	m_timeLastUpdateTrying = 0;
	m_protectPermissions = true;
	m_customizingTimeStamp = "";
	m_impersonateFS = false;

	// for viewer
	m_scale = 100; // 0 - auto
	m_cursorLocal = 0;
	m_cursorRemoteRequest = aaSetPointerShape | aaSetPointerPos;
	m_viewOnly = false;
	m_autoScrollWndMode = true;
	
	m_licenseType = "";
	m_encryption = Encryption::DynamicKey;
	m_startClient  = true;
	m_useWAN       = true;
	m_allowIncomingByIP = true;
	m_allowDirectTCP    = true;
	m_copyFileTime = true;

	m_directInItems.Reset();
	
	DlgEncoderList::SetDefault(m_encoders);
}

void Settings::Customization::ApplyIfNeeded()
{
	m_enabled = false;

	try {
		// read customizing data from resource
		LPCVOID lpData;
		DWORD dwSize = RLResource::Load(NULL, "BINARY", MAKEINTRESOURCE(IDR_CUSTOMIZE), 0, &lpData);
		RLStream dataCustomizing;
		dataCustomizing.AddRaw(lpData, dwSize);

		m_enabled = dataCustomizing.GetBool();
		if (m_enabled) {
			CStringA ver     = dataCustomizing.GetString1A();
			CStringA privateRouter;

			if (ver=="3.0") {
				RLEncryptor02 enc;
				enc.SetKey(Settings_key_v3, false);
				enc.Decrypt((BYTE*)dataCustomizing.GetBufferRd(), dataCustomizing.GetAvailForReading());
			}
			
			if (ver=="2.12" || ver=="3.0")
			{
				bool v3 = (ver=="3.0");

				RLStream permissions;
				CStringA timeStamp	= dataCustomizing.GetString1A();
				privateRouter		= dataCustomizing.GetString1A();
				m_url				= dataCustomizing.GetString1A();				
									  dataCustomizing.GetStream(permissions, false);

				if (settings.m_customizingTimeStamp == timeStamp) return; // already done
				settings.m_customizingTimeStamp = timeStamp;

				if (permissions.GetLen()>4)
					settings.m_permissions.Load(permissions, v3);

				settings.m_privateRoutersUse = !privateRouter.IsEmpty();
				settings.m_privateRouters = privateRouter;
				settings.Save();
			}
			else /*(ver=="2.7")*/ {
				privateRouter   = dataCustomizing.GetString1A();
				m_url           = dataCustomizing.GetString1A();

				// do nothing it's old version, just for remember m_url
			}
		}
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}
}


// read setting from file
//
void Settings::Load()
{
	RLMutexLock l(m_lock); // Load() can be called from many threads

	LoadHardware_v3();

	CStringW fileName = GetFileName_v3();

	if (CCommon::FileIsExistW(fileName))
	{
		RLStream stream;
		stream.ReadFromFileW(fileName);

		RLEncryptor02 enc;
		enc.SetKey(Settings_key_v3, false);
		enc.Decrypt((BYTE*)stream.GetBuffer(), stream.GetLen());

		try {
			UINT32 version = stream.GetUINT32();

			if		 (version==VERSION32(3,0,0)) {
				Load_v3_0(stream);

				if (IsValid()) goto exit; //ok
			}

			// for old or incorrect version loading
			
			SetDefault();  
			Save(); // save with new format

			goto exit; //ok
		}		
		catch(RLStreamOutException&) {}
	}
	else if (CCommon::FileIsExistW(GetFileName_v2()))
	{
		RLStream stream;
		stream.ReadFromFileW(GetFileName_v2());
		RLEncryptor01::Encrypt(stream, 0, Settings_Key_v2);

		try {
			CStringA version = stream.GetString1A();

			SetDefault();  // for old or incorrect version loading

			     if (version=="2.12") Load_v2_12(stream);
			else if (version=="2.13") Load_v2_13(stream);

			if (version!="2.13") {
				m_turnOffVisualEffects = m_turnOffComposition = m_turnOffBackground;
			}

			Save(); // save with new format

			goto exit; //ok
		}
		catch(RLStreamOutException&) {}
	}

	//if we're here so we couldn't read file
	SetDefault();
	Save();

exit:
	m_customization.ApplyIfNeeded(); // read customizing data from resource and rewrite current settings
	TheApp.OnLogChange();
}


void Settings::Load_v2_12(RLStream& stream)
{
	m_langId     = stream.GetUINT16();

	m_permissions.Load(stream, false);

	m_turnOffBackground = stream.GetBool();
	m_requestRouter    = stream.GetBool();
	m_privateRouters   = stream.GetString1A();

	m_captureHints   = stream.GetBool();
	m_warnFullScreen = stream.GetBool();

	RLHttp::m_proxyPort	 = stream.GetUINT16();
	RLHttp::m_proxyHost	 = stream.GetString1A();
	RLHttp::m_proxyUsername = stream.GetString1A();
	RLHttp::m_proxyPassword = stream.GetString1A();
	
	bool operActive	= stream.GetBool();
	m_operProtocol  = stream.GetUINT16();
	m_operRemoteId  = stream.GetString1A();

	stream.GetRaw(&m_audioDevicePlay, sizeof(m_audioDevicePlay));
	stream.GetRaw(&m_audioDeviceRecd, sizeof(m_audioDeviceRecd));
	m_debugLog = stream.GetBool();

	m_timeLastUpdateTrying	= stream.GetUINT32();
	m_runAsSystemOnVista	= stream.GetBool();
	m_protectPermissions	= stream.GetBool();
	m_customizingTimeStamp	= stream.GetString1A();
}

void Settings::Load_v2_13(RLStream& stream)
{
	Load_v2_12(stream);
	
	m_turnOffVisualEffects = stream.GetBool();
	m_turnOffComposition   = stream.GetBool();
	m_impersonateFS		   = stream.GetBool();

	m_scale			= stream.GetUINT16();
	m_cursorLocal	= stream.GetUINT8();
							  stream.GetBool(); // m_requestCursorShape
							  stream.GetBool(); // m_requestCursorPosition
							  stream.GetBool(); // m_disableClipboard
	m_viewOnly				= stream.GetBool();
	m_autoScrollWndMode		= stream.GetBool();
}

void Settings::Load_v3_0(RLStream& stream)
{
	m_langId     = stream.GetUINT16();

	m_permissions.Load(stream, true);

	m_turnOffBackground = stream.GetBool();
	m_requestRouter     = stream.GetBool();
	m_privateRoutersUse = stream.GetBool();
	m_privateRouters    = stream.GetString1A();

	m_captureHints   = stream.GetBool();
	m_warnFullScreen = stream.GetBool();

	RLHttp::m_proxyPort	 = stream.GetUINT16();
	RLHttp::m_proxyHost	 = stream.GetString1A();
	RLHttp::m_proxyUsername = stream.GetString1A();
	RLHttp::m_proxyPassword = stream.GetString1A();
	
	m_operProtocol  = stream.GetUINT16();
	m_operRemoteId  = stream.GetString1A();

	stream.GetRaw(&m_audioDevicePlay, sizeof(m_audioDevicePlay));
	stream.GetRaw(&m_audioDeviceRecd, sizeof(m_audioDeviceRecd));
	m_debugLog = stream.GetBool();

	m_timeLastUpdateTrying	= stream.GetUINT32();
	m_runAsSystemOnVista	= stream.GetBool();
	m_protectPermissions	= stream.GetBool();
	m_customizingTimeStamp	= stream.GetString1A();	
	
	// v2.13
	m_turnOffVisualEffects = stream.GetBool();
	m_turnOffComposition   = stream.GetBool();
	m_impersonateFS		   = stream.GetBool();

	m_scale				   = stream.GetUINT16();
	m_cursorLocal		   = stream.GetUINT8();
	m_cursorRemoteRequest  = stream.GetUINT8();
	m_viewOnly			   = stream.GetBool();
	m_autoScrollWndMode	   = stream.GetBool();

	// v3.0
	m_encryption  = stream.GetUINT8();

	m_startClient = stream.GetBool();
	m_useWAN	  = stream.GetBool();
	m_allowIncomingByIP  = stream.GetBool();
	m_allowDirectTCP     = stream.GetBool();
	m_copyFileTime       = stream.GetBool();
	stream.GetStream(m_directInItems, true);


	// load encoders
	{
		int n = stream.GetUINT32();
		m_encoders.resize(n);
		for (int i=0; i<n; i++) 
		{
			m_encoders[i].name			   = stream.GetString1W();
			m_encoders[i].colorQuality     = stream.GetUINT8();
			m_encoders[i].encoder          = stream.GetUINT8();
			m_encoders[i].compressLevel    = stream.GetUINT8();
			m_encoders[i].jpegQualityLevel = stream.GetUINT8();
		}

		if (n==0) // normally, it's not should happen
			DlgEncoderList::SetDefault(m_encoders);
	}
}

bool Settings::IsValid()
{
	if (m_encryption!=Encryption::DynamicKey && 
		m_encryption!=Encryption::DynamicKeyAndRSA) return false;

	if (m_cursorLocal<0 || m_cursorLocal>3) return false;


	return true;
}

void Settings::Save()
{
	RLMutexLock l(m_lock);

	RLStream stream;

	stream.AddUINT32(GetVersionINT());
	stream.AddUINT16(m_langId);

	m_permissions.Save(stream);

	stream.AddBool(m_turnOffBackground);
	stream.AddBool(m_requestRouter);
	stream.AddBool(m_privateRoutersUse);
	stream.AddString1A(m_privateRouters);
	stream.AddBool(m_captureHints);
	stream.AddBool(m_warnFullScreen);

	//v2.7
	stream.AddUINT16  (RLHttp::m_proxyPort);
	stream.AddString1A(RLHttp::m_proxyHost);
	stream.AddString1A(RLHttp::m_proxyUsername);
	stream.AddString1A(RLHttp::m_proxyPassword);

	//v2.8
	stream.AddUINT16(m_operProtocol);
	stream.AddString1A(m_operRemoteId);

	//v2.9
	stream.AddRaw(&m_audioDevicePlay, sizeof(m_audioDevicePlay));
	stream.AddRaw(&m_audioDeviceRecd, sizeof(m_audioDeviceRecd));
	stream.AddBool(m_debugLog);

	//v2.11
	stream.AddUINT32(m_timeLastUpdateTrying);
	stream.AddBool(m_runAsSystemOnVista);

	//v2.12
	stream.AddBool(m_protectPermissions);
	stream.AddString1A(m_customizingTimeStamp);

	//v2.13
	stream.AddBool(m_turnOffVisualEffects);
	stream.AddBool(m_turnOffComposition);
	stream.AddBool(m_impersonateFS);

	stream.AddUINT16(m_scale);
	stream.AddUINT8(m_cursorLocal);
	stream.AddUINT8(m_cursorRemoteRequest);
	stream.AddBool(m_viewOnly);
	stream.AddBool(m_autoScrollWndMode);

	//v3.0
	stream.AddUINT8(m_encryption);
	stream.AddBool(m_startClient);
	stream.AddBool(m_useWAN);
	stream.AddBool(m_allowIncomingByIP);
	stream.AddBool(m_allowDirectTCP);
	stream.AddBool(m_copyFileTime);
	stream.AddStream(&m_directInItems, true);

	// save encoders
	{
		int n = m_encoders.size();
		stream.AddUINT32(n);
		for (int i=0; i<n; i++) 
		{
			stream.AddString1W(m_encoders[i].name);
			stream.AddUINT8(m_encoders[i].colorQuality);
			stream.AddUINT8(m_encoders[i].encoder);
			stream.AddUINT8(m_encoders[i].compressLevel);
			stream.AddUINT8(m_encoders[i].jpegQualityLevel);
		}
	}

	RLEncryptor02 enc;
	enc.SetKey(Settings_key_v3, true);
	enc.Encrypt((BYTE*)stream.GetBuffer(), stream.GetLen());

	stream.WriteToFileW(GetFileName_v3());
}


CStringW Settings::GetFileName_v3() { return TheApp.GetRootFolderW() + "settings3.bin"; }
CStringW Settings::GetFileName_v2() { return TheApp.GetRootFolderW() + "settings.bin";  }

UINT32 Settings::GetVersionINT()
{
	return VERSION32(3,0,0);
}

LPCSTR Settings::GetVersionSTR()
{
	static CStringA version_str;

	if (version_str.GetLength()==0)
		version_str = ConvertVersionINT2Str(Settings::GetVersionINT());

	return version_str;
}

CStringA Settings::ConvertVersionINT2Str(UINT32 versionINT)
{
	UINT ver1 = (versionINT >> 20);
	UINT ver2 = (versionINT >> 10) % 1024;
	UINT ver3 =  versionINT % 1024;

	CStringA versionStr;

	if (ver3!=0)
		versionStr.Format("%u.%u.%u", ver1, ver2, ver3);
	else
		versionStr.Format("%u.%u", ver1, ver2);

	return versionStr;
}



void Settings::Permissions::Set(const Permission& permission)
{
	RLMutexLock l(m_lock);

	UINT32 id = permission.m_id;
	UINT count = m_items.size();

	for (DWORD i=0; i<count; i++) 
	{
		if (m_items[i].m_id==id) {
			m_items[i] = permission; // found
			return;
		}
	}

	m_items.push_back(permission);
}

int Settings::Permissions::TryToFind(Permission& perm)
{
	UINT32		id = perm.m_id;
	Permission* pAnyPermisson = NULL;
	bool		need_password = false;

	RLMutexLock l(m_lock);

	DWORD count = m_items.size();
	for (DWORD i=0; i<count; i++)
	{
		UINT32 id2 = m_items[i].m_id;

		if (id2==0 || id2==id) {
			need_password = true;
			if (m_items[i].m_password==perm.m_password) {
				if (id2==0) {
					pAnyPermisson = &m_items[i];
				}
				else { // id2==id
					perm = m_items[i];
					return 0; // found exact
				}
			}
		}
	}

	if (pAnyPermisson) {
		perm = *pAnyPermisson;
		return 0; // return ANY record if exact is not found
	}

	return (need_password) ? 1 : 2; // not found
}


void Settings::Permissions::Save(RLStream& stream)
{
	RLMutexLock l(m_lock);

	UINT count = m_items.size();

	stream.AddUINT32(count);

	for (UINT i=0; i<count; i++) {
		stream.AddUINT32(m_items[i].m_id);
		stream.AddUINT32(m_items[i].m_values);
		stream.AddRaw   (m_items[i].m_password.hash, 16);
	}
}

void Settings::Permissions::Load_v2_1(RLStream& stream)
{
	RLMutexLock l(m_lock);

	UINT count = stream.GetUINT32();

	m_items.clear();
	m_items.reserve(count);
	
	for (UINT i=0; i<count; i++) {
		Permission item;
		item.m_id = stream.GetUINT32();
		
		UINT32 values = stream.GetUINT32();
		if (values & 1) item.Set(Permission::ViewScreen + Permission::AudioChat);
		if (values & 2) item.Set(Permission::RemoteControl + Permission::ClipboardOut + Permission::ClipboardIn + Permission::RDPsession);
		if (values & 4) item.Set(Permission::FileManager);
		
		m_items.push_back(item);
	}
}

void Settings::Permissions::Load(RLStream& stream, bool v3)
{
	RLMutexLock l(m_lock);

	UINT count = stream.GetUINT32();

	m_items.clear();
	m_items.resize(count);

	for (UINT i=0; i<count; i++)
	{
		m_items[i].m_id = stream.GetUINT32();
		m_items[i].m_values = stream.GetUINT32();

		if (v3) {
			stream.GetRaw(m_items[i].m_password.hash, 16);
		}
		else {
			CStringA password = stream.GetString1A();
			m_items[i].m_password.Calculate(password);
		}
	}
}

bool Settings::Permissions::Comparator(const Permission& a, const Permission& b)
{
	int v = (a.m_id-b.m_id);
	if (v<0) return true;
	if (v>0) return false;
	return (memcmp(a.m_password.hash, b.m_password.hash, 16)<0);
}

void Settings::Permissions::Sort()
{
	RLMutexLock l(m_lock);
	std::sort(m_items.begin(), m_items.end(), Comparator);
}

CStringA Settings::GetRouters()
{
	CStringA& routers = (m_privateRoutersUse) ? m_privateRouters : m_publicRouters;
	if (routers.IsEmpty()) throw RLException("Router is not defined");	
	return routers;
}


// __________________________________________________________________________________________

LPCSTR  SettingsHardware::REGISTRY_STORE = "SOFTWARE\\Ammyy\\Admin";
LPCWSTR SettingsHardware::FileHardwareName_v2 = L"hr";
LPCWSTR SettingsHardware::FileHardwareName_v3 = L"hr3";

#include "CmdInit.h"

void SettingsHardware::SetHardware(LPCSTR computerId, const CStringA& newLicenseType)
{
	bool save = false;
	
	if (newLicenseType != "-"  && m_licenseType != newLicenseType) {
		m_licenseType  = newLicenseType;
		save = true;
	}

	if (atol(computerId)!=0 && m_computerId != computerId) {
		m_computerId = computerId;
		save = true;
	}

	try {
		if (save) this->SaveHardware();
	}
	catch(RLException&) {}
}


void SettingsHardware::SaveHardwareInternal(RLStream& stream, LPCWSTR name, bool replace)
{
	try {	
		CStringW fileName = TheApp.GetRootFolderW() + name;
		if (replace || (!CCommon::FileIsExistW(fileName)))
			stream.WriteToFileW(fileName);
	}
	catch(RLException& ex) {
		_log.WriteError("SHdr1 %s", ex.GetDescription());
	}	
  
	{
		RLRegistry registry;
		registry.CreateOrOpenA(HKEY_CURRENT_USER, REGISTRY_STORE);
		if (replace || (!registry.IsExistW(name)))
			registry.SetBinaryValueW(name, stream);
	}

	// for users we have not permissions for writing to HKEY_LOCAL_MACHINE
	try {
		RLRegistry registry;
		registry.CreateOrOpenA(HKEY_LOCAL_MACHINE, REGISTRY_STORE);
		if (replace || (!registry.IsExistW(name)))
			registry.SetBinaryValueW(name, stream);
	}
	catch(RLException& ex) {
		_log.WriteError("HKEY_LOCAL_MACHINE %s", ex.GetDescription());
	}
}

// need to save if v2.13 will run after, cause it dwon;t send hdd info
void SettingsHardware::SaveHardware_v2(FILETIME& t1)
{
	// no need to save it, cause when we will have a valid m_computerId it won't save, cause file will be exist
	//if (m_computerId.GetLength()==0) return;

	RLStream stream(128);
	stream.AddString1A("2.13");
	stream.AddString1A(m_computerId);
	stream.AddRaw(&t1, sizeof(t1));
	
	RLEncryptor01::Encrypt(stream, 0, Settings_Key_v2);

	SaveHardwareInternal(stream, FileHardwareName_v2, true);
}


void SettingsHardware::SaveHardware()
{
	FILETIME t1 = {0};
	::GetSystemTimeAsFileTime(&t1);

	this->SaveHardware_v2(t1);

	RLStream stream(512);
	stream.AddRaw(&t1, sizeof(t1));
	stream.AddUINT32(Settings::GetVersionINT());
	stream.AddString1A(m_computerId);
	stream.AddString1A(m_licenseType);
	CmdInit::AddHDDInfo(&stream);
	CmdInit::AddMacAddresses(&stream);

	RLEncryptor02 enc;
	enc.SetKey(Settings_key_v3, true);
	enc.Encrypt((BYTE*)stream.GetBuffer(), stream.GetLen());

	SaveHardwareInternal(stream, FileHardwareName_v3, true);	
}



bool SettingsHardware::IsSameComputer(RLStream& stream1)
{
	RLStream stream2;
	CmdInit::AddHDDInfo(&stream2);

	UINT8 primary1   = stream1.GetUINT8();
	UINT8 primary2   = stream2.GetUINT8();
	UINT64 size1     = stream1.GetUINT64();
	UINT64 size2     = stream2.GetUINT64();
	CStringA firmw1  = stream1.GetString1A();
	CStringA firmw2  = stream2.GetString1A();
	CStringA model1  = stream1.GetString1A();
	CStringA model2  = stream2.GetString1A();
	CStringA serial1 = stream1.GetString1A();
	CStringA serial2 = stream2.GetString1A();

	if (size1 == size2 && firmw1==firmw2 && model1==model2 && serial1==serial2) return true; // same HDD

	stream2.Reset();
	CmdInit::AddMacAddresses(&stream2);

	int c1 = stream1.GetUINT32();
	int c2 = stream2.GetUINT32();

	char* p1 = stream1.GetBufferRd();
	char* p2 = stream2.GetBufferRd();

	if (c1*6>stream1.GetAvailForReading() || c2*6>stream2.GetAvailForReading())
		return false; // incorrect data

	for (int i1=0; i1<c1; i1++) {
	for (int i2=0; i2<c1; i2++) {
		if (memcmp(p1+i1*6, p2+i2*6, 6)==0) 
			return true; // same mac
	}
	}

	return false;
}

void SettingsHardware::LoadHardware_v3()
{
	FILETIME time = {0};

	for (int i=0; i<3; i++)
	{
		try {
			RLStream stream;
			if (i==0 || i==1) {
				RLRegistry registry;
				try {
					HKEY hkey = (i==0) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
					registry.OpenA(hkey, REGISTRY_STORE, KEY_QUERY_VALUE);
				}
				catch(RLException& ) {
					continue;
				}
				if (!registry.GetBinaryValueW(FileHardwareName_v3, stream)) continue;
			}
			else {
				// skip reading file, if we've read data from registry
				if (time.dwHighDateTime!=0 || time.dwLowDateTime!=0 || (!m_computerId.IsEmpty())) continue;
				CStringW fileName = TheApp.GetRootFolderW() + FileHardwareName_v3;
				if (!CCommon::FileIsExistW(fileName)) continue;
				stream.ReadFromFileW(fileName);
			}

			RLEncryptor02 enc;
			enc.SetKey(Settings_key_v3, false);
			enc.Decrypt((BYTE*)stream.GetBuffer(), stream.GetLen());

			FILETIME t;
			stream.GetRaw(&t, sizeof(t));

			if (::CompareFileTime(&t, &time) <= 0) continue; // skip it if HKEY_CURRENT_USER has more later data

			UINT32 version = stream.GetUINT32(); 
			m_computerId   = stream.GetString1A(); // leave it even in case of NOT same computer
			CStringA licenseType = stream.GetString1A();

			try {
				if (!IsSameComputer(stream)) continue;
			}
			catch(RLException&) {}

			m_licenseType = licenseType;
			time = t;
		}	
		catch(RLException& ex) {
			_log.WriteError("LoadHr() %d %s", __LINE__,  ex.GetDescription());
		}
	}

	if (m_computerId.GetLength()==0 && time.dwHighDateTime==0 && time.dwLowDateTime==0) {
		try {
			LoadHardware_v2();
			SaveHardware();
		}
		catch(RLException& ex) {
			_log.WriteError("LoadHr() %d %s", __LINE__,  ex.GetDescription());
		}
	}
}


void SettingsHardware::LoadHardware_v2()
{
	FILETIME time = {0};

	for (int i=0; i<3; i++)
	{
		try {
			RLStream stream;
			if (i==0 || i==1) {
				RLRegistry registry;
				try {
					HKEY hkey = (i==0) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
					registry.OpenA(hkey, REGISTRY_STORE, KEY_QUERY_VALUE);
				}
				catch(RLException& ) {
					continue;
				}
				if (!registry.GetBinaryValueW(FileHardwareName_v2, stream)) continue;
			}
			else {
				// skip reading file, if we've read data from registry
				if (time.dwHighDateTime!=0 || time.dwLowDateTime!=0 || (!m_computerId.IsEmpty())) continue;
				CStringW fileName = TheApp.GetRootFolderW() + FileHardwareName_v2;
				if (!CCommon::FileIsExistW(fileName)) continue;
				stream.ReadFromFileW(fileName);
			}

			RLEncryptor01::Encrypt(stream, 0, Settings_Key_v2);

			FILETIME t;
			CStringA version    = stream.GetString1A();
			CStringA computerId = stream.GetString1A();
			stream.GetRaw(&t, sizeof(t));

			if (::CompareFileTime(&t, &time) < 0) continue; // skip it if HKEY_CURRENT_USER has more later data

			m_computerId = computerId;
			time = t;
		}	
		catch(RLException& ex) 
		{
			_log.WriteError("LoadHr() %d %s", __LINE__,  ex.GetDescription());
		}
	}
}

