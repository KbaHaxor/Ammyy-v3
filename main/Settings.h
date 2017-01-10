#if !defined(AFX_SETTINGS_H__7B1FD126_8D85_4F2D_A316_8A10CDA54036__INCLUDED_)
#define AFX_SETTINGS_H__7B1FD126_8D85_4F2D_A316_8A10CDA54036__INCLUDED_

#include "../RL/RLLock.h"
#include <vector>
#include "Common.h"
#include "../common/Common2.h"
#include "DlgEncoderList.h"

#define DEFAULT_INCOME_PORT   5931
#define DEFAULT_INTRANET_PORT 5932

class SettingsHardware
{
public:
	CStringA GetComputerId() { return m_computerId; }
	void SetHardware(LPCSTR computerId, const CStringA& newLicenseType);

protected:
	SettingsHardware() {};
	void LoadHardware_v3();

	CStringA m_licenseType; // "" - free, "P" - pro

private:
	void SaveHardwareInternal(RLStream& stream, LPCWSTR name, bool replace);
	void SaveHardware_v2(FILETIME& t1);
	void LoadHardware_v2();
	bool IsSameComputer(RLStream& stream);
	void SaveHardware();

	CStringA m_computerId; // added v2.12
	static LPCSTR REGISTRY_STORE;
	static LPCWSTR FileHardwareName_v2;
	static LPCWSTR FileHardwareName_v3;
};


	class Permission
	{
	public:
		enum Bits
		{
			ViewScreen    = 1,   // 2<<0
			RemoteControl = 2,   // 2<<1
			ClipboardOut  = 4,   // 2<<2
			ClipboardIn   = 8,   // 2<<3
			FileManager   = 16,  // 2<<4
			AudioChat     = 32,	 // 2<<5
			RDPsession    = 64,  // 2<<6
		};

		Permission() { m_id = m_values = 0; };
		void ClearAll() { m_values = 0; }
		void GrandAll() { m_values = 0x7F; }
		void Set(UINT mask)          { m_values|=mask; }
		void Set(UINT mask, bool on) { m_values|=mask; if (!on) m_values^=mask; }
		bool Get(UINT mask) { return (m_values & mask) ? true : false; }
		bool IsClear() { return (m_values==0); }

		bool IsSameKey(const Permission& v) const
		{
			if (m_id!=v.m_id) return false;
			return (m_id!=0 || (m_password==v.m_password));
		}


		friend class Permissions;
	//private:
		UINT32		m_values;
		UINT32		m_id; // computer id, 0 - mean "ANY"
		RLMD5		m_password;		
	};


class Settings : public SettingsHardware
{
public:
	class Permissions
	{
	public:
		void Set(const Permission& permission);
		int  TryToFind(Permission& permission);

		void Load_v2_1(RLStream& stream);
		void Load(RLStream& stream, bool v3);
		void Save(RLStream& stream);
		void Sort();
	
	private:
		static bool Comparator(const Permission& a, const Permission& b);

	private:
		RLMutex  m_lock;
		std::vector<Permission> m_items;

		friend class DlgPermissionList;
	};

	Permissions m_permissions;

	class Customization
	{
	public:
		void ApplyIfNeeded();

		bool	 m_enabled;
		CStringA m_url;
	} m_customization;


	#pragma pack(push, 1)
	struct DirectInItem
	{
		UINT16 m_port;
		UINT8  m_status; // 0-off, 1-on
	};
	#pragma pack(pop)


public:
	Settings();

	        void Load();
	virtual void Save();

	static LPCSTR GetVersionSTR();
	static UINT32 GetVersionINT();
	static CStringA ConvertVersionINT2Str(UINT32 versionINT);

	CStringA GetRouters();

	LPCSTR GetLisenceString()
	{
		if (m_licenseType=="C") return "Corporate";
		if (m_licenseType=="P") return "Premium";
		return "Free";
	}

	bool IsFreeLicense() { return m_licenseType.GetLength()==0; }


	// maximum autorized sessions
	int GetMaxSessionsOnTarget()
	{
		if (m_licenseType=="C") return 64;
		if (m_licenseType=="P") return 2;
		                        return 1;
	}

	int GetMaxSessionsOnViewer()
	{
		if (m_licenseType=="C") return 256;
		if (m_licenseType=="P") return 4;
		                        return 2;
	}

public:
	static GUID GuidOFF;


	WORD	m_langId;
	//--- v2.2
	bool	 m_turnOffBackground;	// remove desktop background while viewer is connected
	bool	 m_requestRouter;
	bool     m_privateRoutersUse;
	CStringA m_privateRouters;
	bool	 m_captureHints;
	bool	 m_warnFullScreen;
	
	// v2.8
	UINT16   m_operProtocol;
	CStringA m_operRemoteId;

	// v2.9
	GUID	m_audioDevicePlay;
	GUID	m_audioDeviceRecd;
	bool	m_debugLog;

	// v2.11
	bool	m_runAsSystemOnVista;
	DWORD	m_timeLastUpdateTrying;

	// v2.12
	bool	 m_protectPermissions;
	CStringA m_customizingTimeStamp;

	// v2.13
	bool	m_turnOffVisualEffects;
	bool	m_turnOffComposition;
	bool	m_impersonateFS;

	// v3.0
	INT8	 m_encryption;	// 0 -none, 1 -static key, 2 - dynamic key, 3 - RSA + dynamic key
	bool	 m_startClient;
	bool	 m_useWAN;
	bool     m_allowIncomingByIP;
	bool     m_allowDirectTCP;
	RLStream m_directInItems;
	bool	m_copyFileTime; // copy modifed (Last Write) FileTime
	std::vector<DlgEncoderList::Item> m_encoders;

	enum Encryption
	{
		None = 1,			// only for debug purpose
		DynamicKey = 2,
		DynamicKeyAndRSA = 3
	};

	// for viewer
	UINT16	m_scale; // 0 - auto
	UINT8   m_cursorLocal;
	UINT8   m_cursorRemoteRequest; 
	bool	m_viewOnly;
	bool	m_autoScrollWndMode;

	 // these variablies are NOT SAVED
	CStringA m_publicRouters;

	
private:
	bool IsValid();
	void SetDefault();
	void Load_v2_12(RLStream& stream);
	void Load_v2_13(RLStream& stream);
	void Load_v3_0 (RLStream& stream);

	CStringW GetFileName_v2();
	CStringW GetFileName_v3();

	RLMutex m_lock;
};

const int PingTime   =55; // in seconds, need to copy to each TrClent object if user can change it
const int PingTimeOut=40; // in seconds

// here for Customizer
static UINT8  Settings_key_v3[20] = { 0x5B,0x2f,0x1A,0xA7, 0x98,0x7E,0x43,0x16, 0xA6,0x05b,0x18,0x7B, 0x98,0x91,0xC0,0xD4, 0xBC,0x5A,0x95,0x73 };

#endif // !defined(AFX_SETTINGS_H__7B1FD126_8D85_4F2D_A316_8A10CDA54036__INCLUDED_)
