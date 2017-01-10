#if !defined(RL_HARDWARE_MAC_674831__INCLUDED_)
#define RL_HARDWARE_MAC_674831__INCLUDED_

#ifdef _WIN32
#include <IPHlpApi.h>
#include <NtDDNdis.h>
#include <SetupAPI.h>
#include <RegStr.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "Advapi32.lib")
//#pragma comment(lib, "Iphlpapi.lib")
#endif


class RLHardwareMac
{
	#ifdef _WIN32
	static bool GetRealMacAddress(LPCSTR adapterName, BYTE* address)
	{
		char	name[512];
		wsprintf(name, "\\\\.\\%s", adapterName);

		HANDLE hFile = ::CreateFileA(name, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			// error 2 here if device is disable
			_log.WriteInfo("GetAddress()#1 ERROR=%d while CreateFile()", ::GetLastError());
			return false;
		}

		bool ret = true;
		
		ULONG	oidCode = OID_802_3_PERMANENT_ADDRESS;
		BYTE	macAddr[32];
		DWORD	writtenBytes = 0;
		if (::DeviceIoControl(hFile, IOCTL_NDIS_QUERY_GLOBAL_STATS, &oidCode, sizeof(oidCode), &macAddr, sizeof(macAddr), 
								&writtenBytes, NULL)!=0)
		{
			ASSERT(writtenBytes == 6);
			memcpy(address, macAddr, 6);
		}
		else {
			_log.WriteInfo("GetAddress()#2 ERROR=%d while DeviceIoControl()", ::GetLastError());
			ret = false;
		}

		::CloseHandle(hFile);
		return ret;
	}

	static CStringA ConvertMacAddress(BYTE* mac)
	{
		char buffer[128];
		sprintf(buffer, "%02.2X-%02.2X-%02.2X-%02.2X-%02.2X-%02.2X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		return buffer;
	}
	#endif


public:
	// new algorithm:
	//	1. get real mac address if it was changed
	//	2. discard all virtuals adapter
	//	3. discard bluetooth adapter (because it can be by USB)
	//
	//  Tested:
	//	1. On desktop & notebook (should take only PCI adapter)
	//	2. Changing mac address (on Vista no way to change)
	//	3. BlueTooth adapter
	//	4. Virtual adapter
	//	5. Disable adapter
	//	6. under Windows 2000, XP & Vista with minimal Permission and UAC on
	//	7. Test as service
	//	8. under WMWare


	static void AddMacAddressesReal(RLStream* pStream)
	{
	#ifdef _WIN32
		int iWritingPos = pStream->GetLen();
		pStream->AddUINT32(0); // count of mac addresses, we'll know this at the end

		GUID	net_class;
		DWORD	req_size;
		if (!::SetupDiClassGuidsFromName("Net", &net_class, 1, &req_size)) {
			_log.WriteError("SetupDiClassGuidsFromName() ERROR=%d", ::GetLastError());
			return;
		}	
		
		HDEVINFO hDevInfo = ::SetupDiGetClassDevs(&net_class, 0, 0, DIGCF_PRESENT | DIGCF_PROFILE);
		if (hDevInfo == NULL) {
			_log.WriteError("SetupDiGetClassDevs() ERROR=%d", ::GetLastError());
			return;
		}	

		for (DWORD idx=0; ;idx++)
		{
			char buf[512];
			SP_DEVINFO_DATA spDevInfoData    = {0};
			spDevInfoData.cbSize = sizeof(spDevInfoData);

			if (!SetupDiEnumDeviceInfo(hDevInfo, idx, &spDevInfoData))
			{
				DWORD	err = ::GetLastError();
				if (err != ERROR_NO_MORE_ITEMS) _log.WriteError("SetupDiEnumDeviceInfo() ERROR=%d", err);
				break;
			}

			// Check type of this network adapter (physical or virtual).
			// Currently, we use "Enumerator" property for checking
			buf[0] = 0;
			if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &spDevInfoData, SPDRP_ENUMERATOR_NAME, NULL, (LPBYTE)buf, sizeof(buf), NULL))
			{
				_log.WriteError("SetupDiGetDeviceRegistryProperty(SPDRP_ENMERATOR_NAME) ERROR=%d", ::GetLastError());
				continue;
			}

			// "ROOT", "SW" - software adapter, discard it
			// "BTH" - bluetooth (no way to detect by USB or internal), so discard it
			// "PCI" - physical adapter

			if (stricmp(buf, "PCI")!=0) {
				_log.WriteInfo("AddAdrs_2 %u %s" , idx, buf);
				continue; // this's NOT physical adapter
			}

			//CStringA enumerator = buf;

			// Get name of registry key associated with this adapter
			if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &spDevInfoData, SPDRP_DRIVER, NULL, (LPBYTE)buf, sizeof(buf), NULL))
			{
				_log.WriteError("SetupDiGetDeviceRegistryProperty(SPDRP_DRIVER) ERROR=%d", ::GetLastError());
				continue;
			}

			char	szKey[512];
			wsprintf(szKey, "%s\\%s", REGSTR_PATH_CLASS_NT, buf);


			HKEY	hKey;
			LONG	res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey);
			if (res != ERROR_SUCCESS) {
				_log.WriteError("RegOpenKey(%s) Error=%d", szKey, res);
				continue;
			}

			DWORD	dwRealType;
			DWORD	dwSize = sizeof(buf);
			buf[0] = 0;
			res = ::RegQueryValueEx(hKey, "NetCfgInstanceId", NULL, &dwRealType, (LPBYTE)buf, &dwSize);
			if ((res != ERROR_SUCCESS) || (dwRealType != REG_SZ))
				_log.WriteError("RegQueryValueEx(\"NetCfgInstanceId\") ERROR=%d", res);
			else
			{
				BYTE mac[6];
				if (GetRealMacAddress(buf, mac)) {
					pStream->AddRaw(&mac, sizeof(mac));
					//if (_log.GetLogLevel()>=2)
					//	_log.WriteInfo("AddAdrs_2 %u %s %s %s" , idx, (LPCSTR)enumerator, buf, (LPCSTR)ConvertMacAddress(&mac[0]));
				}			
			}

			::RegCloseKey(hKey);
		}
		
		::SetupDiDestroyDeviceInfoList(hDevInfo);
		
		int count = (pStream->GetLen() - iWritingPos - sizeof(UINT32)) / 6;
		*(UINT32*)((LPCSTR)pStream->GetBuffer() + iWritingPos) = count;
	#else
		pStream->AddUINT32(0);	// zero macs on this computer, no need for Unix
	#endif
	}

	/* old version, problem if user will change macs to emulate other computer
	static void AddMacAddressesVirtual(RLStream* pStream)
	{
	#ifdef _WIN32
		ULONG dwBufLen = 0;
		DWORD ret = ::GetAdaptersInfo(NULL, &dwBufLen);

		if (ret==ERROR_NO_DATA) {
			ASSERT(dwBufLen==0);
			pStream->AddUINT32(0);	// zero macs on this computer
		}
		else {
			ASSERT(ret==ERROR_BUFFER_OVERFLOW);
			ASSERT(dwBufLen>0);

			RLStream buffer(dwBufLen);

			IP_ADAPTER_INFO* pData = (IP_ADAPTER_INFO*)buffer.GetBuffer();

			VERIFY(ERROR_SUCCESS == ::GetAdaptersInfo(pData, &dwBufLen));

			DWORD dwCountMac = dwBufLen/sizeof(IP_ADAPTER_INFO);

			pStream->AddUINT32(dwCountMac);

			for (DWORD i=0; i<dwCountMac; i++) {
				pStream->AddRaw(pData[i].Address, 6);
				if (_log.GetLogLevel()>=2) {
					_log.WriteInfo("AddAdrs_1 %u %s %s", i, pData[i].AdapterName, (LPCSTR)ConvertMacAddress(&pData[i].Address[0]));
				}
			}
		}
	#else
		pStream->AddUINT32(0);	// zero macs on this computer, no need for Unix
	#endif
	}
	*/
};

#endif // RL_HARDWARE_MAC_674831__INCLUDED_
