#if !defined(_RL_ENCRYPTOR01_H_CF952F2DC56A__INCLUDED_)
#define _RL_ENCRYPTOR01_H_CF952F2DC56A__INCLUDED_

class RLEncryptor01
{
public:
	RLEncryptor01();
	virtual ~RLEncryptor01();

	void Init(LPCSTR pKey, int keylen);
	void Reset();
	void Encrypt    (void* pData, DWORD dwDataLen);
	void EncryptPeek(void* pData, DWORD dwDataLen);
	
	static LPCSTR Encrypt(void* pData, DWORD dwDataLen, UINT64 offset, LPCSTR pKey);
	static void   Encrypt(RLStream& stream, UINT64 offset,          LPCSTR pKey);

private:
	LPSTR  m_pKey;
	LPCSTR m_pCurrPos;
};

#endif // !defined(_RL_ENCRYPTOR01_H_CF952F2DC56A__INCLUDED_)
