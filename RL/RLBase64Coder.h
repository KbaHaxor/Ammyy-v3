#if !defined(RL_RLBASE64CODER_H__INCLUDED_)
#define RL_RLBASE64CODER_H__INCLUDED_

#include "RLStream.h"

class RLBase64Coder
{
public:
	RLBase64Coder(LPCSTR key);
	~RLBase64Coder(){};

	void     Encode(RLStream* pStreamIn, RLStream* pStreamOut);
	void	 Decode(RLStream* pStreamIn, RLStream* pStreamOut);
	CStringA Encode(CStringA strIn);
	CStringA Decode(CStringA strIn);

private:
	char m_key[64];
	RLStream m_decode_codes;
};

#endif // !defined(RL_RLBASE64CODER_H__INCLUDED_)
