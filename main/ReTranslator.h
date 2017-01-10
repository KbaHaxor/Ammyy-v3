#if !defined(AFX_RETRANSLATOR_H__6DD4C637_2F7D_4627_998E_5524D101C4F7__INCLUDED_)
#define AFX_RETRANSLATOR_H__6DD4C637_2F7D_4627_998E_5524D101C4F7__INCLUDED_

class CReTranslator  
{
public:
	CReTranslator();
	~CReTranslator();

	int DoSmart();	
	void DoDirect();

private:
	void CloseChildSocket();
	void DecodeS1(int dataLen);
	void EncodeS2(LPCSTR pDataIn, int dataInLen);

	static inline int  SimpleRead1(SOCKET s, LPSTR pBuffer);
	static inline bool SimpleRead2(SOCKET s, RLStream& queue);
	static inline bool SimpleSend1(SOCKET s, RLStream& queue);

public:
	SOCKET s1; // solid, non-breakable channel
	SOCKET s2; // child, can be broken channel

private:	
	bool m_decode_state_normal;	

	DWORD m_bytes_s1; // bytes read from s1
	DWORD m_bytes_s2; // bytes read from s2

	RLStream queue1; // data read from s1
	RLStream queue2; // data read from s2

	bool m_s1_smart; // true while smart algorith used
	bool m_s2_smart; // true while smart algorith used
	bool m_s1_opened;  // true while remote child not closed
};

#endif // !defined(AFX_RETRANSLATOR_H__6DD4C637_2F7D_4627_998E_5524D101C4F7__INCLUDED_)
