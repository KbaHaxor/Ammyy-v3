#if !defined(_RL_REGISTRY_H__INCLUDED_)
#define _RL_REGISTRY_H__INCLUDED_

class RLRegistry
{
public:
	RLRegistry();
	~RLRegistry();

	void OpenA(HKEY hKey,LPCSTR lpSubKey, REGSAM samDesired);
	void CreateOrOpenA(HKEY hKey,LPCSTR lpSubKey);
	void Close();

	bool GetDWORDValueA (LPCSTR  lpValueName, DWORD& value);
	bool GetStringValueA(LPCSTR  lpValueName, CStringA& value);
	bool GetStringValueW(LPCWSTR lpValueName, CStringW& value);

	bool IsExistW(LPCWSTR name);
	void SetBinaryValueW(LPCWSTR name, const RLStream& value);
	bool GetBinaryValueW(LPCWSTR name, RLStream& value);

	void SetStringValueA(LPCSTR  lpValueName, const CStringA& value);
	void SetStringValueW(LPCWSTR lpValueName, const CStringW& value);
	void DeleteValueA(LPCSTR  lpValueName);
	void DeleteValueW(LPCWSTR lpValueName);

	static void DeleteKeyA(HKEY hKey, LPCSTR lpSubKey);

private:
	HKEY m_hKey;
};

#endif // _RL_REGISTRY_H__INCLUDED_