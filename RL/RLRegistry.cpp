#include "stdafx.h"
#include "RLRegistry.h"

#pragma comment(lib, "Advapi32.lib")


RLRegistry::RLRegistry() 
{
	m_hKey = NULL;
}

RLRegistry::~RLRegistry()
{
	Close();
}

void RLRegistry::Close()
{
	if (m_hKey) {
		::RegCloseKey(m_hKey);
		m_hKey = 0;
	}
}

void RLRegistry::OpenA(HKEY hKey, LPCSTR lpSubKey, REGSAM samDesired)
{
	Close();

	LONG ret = ::RegOpenKeyExA(hKey,  lpSubKey, 0, samDesired, &m_hKey);
	if (ret != ERROR_SUCCESS)
		throw RLException("RegOpenKeyExA() error=%u", ret);
}

void RLRegistry::CreateOrOpenA(HKEY hKey,LPCSTR lpSubKey)
{
	Close();

	DWORD lpdwDisposition;
	LONG ret = ::RegCreateKeyExA(hKey, lpSubKey, 0, NULL, 0, KEY_READ | KEY_WRITE,
							NULL, &m_hKey, &lpdwDisposition); 

	if (ret!=ERROR_SUCCESS)
		throw RLException("RegCreateKeyExA() error=%u", ret);
}

bool RLRegistry::GetDWORDValueA(LPCSTR lpValueName, DWORD& value)
{
	DWORD dwSize = sizeof(value);
	LONG ret = ::RegQueryValueExA(m_hKey, lpValueName, NULL, NULL, (LPBYTE)&value, &dwSize);

	if (ret==ERROR_SUCCESS) {
		return true;
	}
	if (ret==ERROR_FILE_NOT_FOUND) {
		return false;
	}
	else
		throw RLException("RegQueryValueExA() error=%u", ret);
}

bool RLRegistry::GetStringValueA(LPCSTR lpValueName, CStringA& value)
{
	DWORD dwSize = MAX_PATH * sizeof(char);
	LONG ret = ::RegQueryValueExA(m_hKey, lpValueName, NULL, NULL, (LPBYTE)value.GetBuffer(MAX_PATH), &dwSize);

	if (ret==ERROR_SUCCESS) {
		value.ReleaseBuffer(dwSize/sizeof(char));
		return true;
	}
	if (ret==ERROR_FILE_NOT_FOUND) {
		value = "";
		return false;
	}
	else
		throw RLException("RegQueryValueExA() error=%u", ret);
}

bool RLRegistry::GetStringValueW(LPCWSTR lpValueName, CStringW& value)
{
	DWORD dwSize = MAX_PATH * sizeof(WCHAR);
	LONG ret = ::RegQueryValueExW(m_hKey, lpValueName, NULL, NULL, (LPBYTE)value.GetBuffer(MAX_PATH), &dwSize);

	if (ret==ERROR_SUCCESS) {
		value.ReleaseBuffer(dwSize/sizeof(WCHAR));
		return true;
	}
	if (ret==ERROR_FILE_NOT_FOUND) {
		value = L"";
		return false;
	}
	else
		throw RLException("RegQueryValueExW() error=%u", ret);
}

bool RLRegistry::IsExistW(LPCWSTR name)
{
	ULONG size = 0; 
	LONG ret = ::RegQueryValueExW(m_hKey, name, 0, 0, NULL, &size);

	if (ret==ERROR_FILE_NOT_FOUND) return false;

	if (ret==ERROR_SUCCESS) return true;

	throw RLException("IsExistW() error=%u", ret);
}

bool RLRegistry::GetBinaryValueW(LPCWSTR name, RLStream& value)
{
	ULONG size = 0; 
	LONG ret = ::RegQueryValueExW(m_hKey, name, 0, 0, NULL, &size);

	if (ret==ERROR_FILE_NOT_FOUND) return false;

	if (ret==ERROR_SUCCESS) {
		value.SetMinExtraCapasity(size);

		ret = ::RegQueryValueExW(m_hKey, name, 0, 0, (BYTE*)value.GetBufferWr(), &size);

		if (ret==ERROR_SUCCESS) {
			ASSERT(size<1000000);
			value.SetLen(value.GetLen()+size);
			return true;
		}
	}

	throw RLException("RegQueryValueExW() error=%u", ret);
}

void RLRegistry::SetBinaryValueW(LPCWSTR name, const RLStream& value)
{
	LONG ret = ::RegSetValueExW(m_hKey, name, 0, REG_BINARY, (LPBYTE)value.GetBuffer(), value.GetLen());
	if (ret!=ERROR_SUCCESS) throw RLException("RegSetValueExW(REG_BINARY) error=%u", ret);
}


void RLRegistry::SetStringValueA(LPCSTR lpValueName,  const CStringA& value)
{
	LONG ret = ::RegSetValueExA(m_hKey, lpValueName, 0, REG_SZ, (LPBYTE)(LPCSTR)value, value.GetLength());
	if (ret!=ERROR_SUCCESS) throw RLException("RegSetValueExA() error=%u", ret);
}

void RLRegistry::SetStringValueW(LPCWSTR lpValueName, const CStringW& value)
{
	LONG ret = ::RegSetValueExW(m_hKey, lpValueName, 0, REG_SZ, (LPBYTE)(LPCWSTR)value, value.GetLength()*sizeof(WCHAR));
	if (ret!=ERROR_SUCCESS) throw RLException("RegSetValueExW() error=%u", ret);
}

void RLRegistry::DeleteValueA(LPCSTR lpValueName)
{
	LONG ret = ::RegDeleteValueA(m_hKey, lpValueName);
	if (ret!=ERROR_SUCCESS && ret!=ERROR_FILE_NOT_FOUND) throw RLException("RegDeleteValueA() error=%u", ret);
}

void RLRegistry::DeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
{
	LONG ret = ::RegDeleteKeyA(hKey, lpSubKey);
	if (ret!=ERROR_SUCCESS && ret!=ERROR_FILE_NOT_FOUND) throw RLException("RegDeleteKeyA() error=%u", ret);
}

void RLRegistry::DeleteValueW(LPCWSTR lpValueName)
{
	LONG ret = ::RegDeleteValueW(m_hKey, lpValueName);
	if (ret!=ERROR_SUCCESS && ret!=ERROR_FILE_NOT_FOUND) throw RLException("RegDeleteValueW() error=%u", ret);
}
