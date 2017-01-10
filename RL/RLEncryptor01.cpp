#include "stdafx.h"
#include "RLEncryptor01.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


RLEncryptor01::RLEncryptor01()
{	
	m_pKey  = NULL;
}

RLEncryptor01::~RLEncryptor01()
{
	if (m_pKey!=NULL) delete[] m_pKey;
}

void RLEncryptor01::Init(LPCSTR pKey, int keylen=-1)
{
	ASSERT(pKey!=NULL);

	if (keylen<0)
		keylen = strlen(pKey);

	ASSERT(keylen>0);

	if (m_pKey!=NULL) delete[] m_pKey;
	
	m_pCurrPos = m_pKey = new char[keylen+1];
	
	memcpy(m_pKey, pKey, keylen);
	m_pKey[keylen] = 0; // null terminated key
}


void RLEncryptor01::Reset()
{
	m_pCurrPos = m_pKey;
}


void RLEncryptor01::Encrypt(void* pData, DWORD dwDataLen)
{
	if (pData!=NULL) {	
		m_pCurrPos = Encrypt(pData, dwDataLen, m_pCurrPos-m_pKey, m_pKey);
	}
	else {
		int keylen = strlen(m_pKey);
		m_pCurrPos = ((m_pCurrPos-m_pKey + dwDataLen) % keylen) + m_pKey;
	}
}


void RLEncryptor01::EncryptPeek(void* pData, DWORD dwDataLen)
{
	Encrypt(pData, dwDataLen, m_pCurrPos-m_pKey, m_pKey);
}



LPCSTR RLEncryptor01::Encrypt(void* pData, DWORD dwDataLen, UINT64 offset, LPCSTR pKey)
{
	ASSERT(pKey!=NULL);

	int keylen = strlen(pKey);

	LPCSTR pCurrPos = pKey + (offset%keylen);

	char*  pData1 = (char*)pData;
	char*  pData1_end = pData1 + dwDataLen;

	while (pData1<pData1_end)
	{
		char c_key = *pCurrPos++;
		if (c_key==0) {
			pCurrPos = pKey;
			c_key = *pCurrPos++;
		}
		*pData1++ ^= c_key;
	}

	return pCurrPos;
}

void RLEncryptor01::Encrypt(RLStream& stream, UINT64 offset, LPCSTR pKey)
{
	Encrypt(stream.GetBuffer(), stream.GetLen(), offset, pKey);
}

