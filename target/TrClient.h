#if (!defined(_VTC_CLIENT_H__INCLUDED))
#define _VTC_CLIENT_H__INCLUDED

class TrClient;

typedef SHORT TrClientId;

#include "../main/aaProtocol.h"
#include "../main/TransportT.h"
#include "../main/sound/AudioOut.h"
#include "../main/sound/AudioIn.h"
#include "../common/omnithread/omnithread.h"
#include "TrDesktop.h"
#include "TrFm.h"
#include "TrMain.h"


class TrClient : public TrFM
{
	friend class TrDesktop;	
	friend class TrMain; // for m_rdp;
	friend struct TrListener::SocketItem;

public:
	static void GetReadyDirectInPort(RLStream& ports);
	static bool DirectConnectByTCP(TCP& tcp, UINT32 ip_v4, UINT16 port, UINT32 timeout, const char* key);
	static bool DirectConnectWaitTCP(const char* key, Transport* m_transport);

public:
	TrClient();
	~TrClient();
	
	// The server uses this to close the client object, causing the client thread to fail, which in turn deletes the client object
	void KillClient(UINT16 reason);

private:
	// Update handling functions
	void SendClipboardText(LPCSTR text);
	void OnTransportError(TransportException& ex, LPCSTR where);
	virtual void OnEventFS(LPCSTR templ, LPCSTR param1=NULL);
	virtual void Thread01();
	void Run();	// The main thread function
	void DoCrypt();
	bool DoAuthorization();
	void DoDirectConnect();
	void DoSpeedTest();
	void SendOtherInfo();
	inline void DoRDP();
	inline void OnAaSetEncoder();
	inline void OnAaDesktopOFF();
	inline void OnAaSetPointer();
	inline void OnAaScreenUpdateRequest();
	inline void OnAaScreenUpdateCommit();
	inline void OnAaKeyEvent();
	inline void OnAaPointerEvent();
	inline void OnAaCutText();
	inline void OnAaSound();
	CStringA ReadString(UINT32 len);
	void ClearKeyState(BYTE key);


private:
	// Per-client settings
	//bool			m_prmFileManager;	 in TrFM
	bool			m_prmAudioChat;
	bool			m_prmRemoteControl;
	Permission		m_prm;
	TrClientId		m_id;
	//Transport*	m_transport; it has in TrFm
	omni_mutex		m_transport_close_lock;
	INT8			m_ptrevent_buttonMask; // User input information
	omni_mutex		m_regionLock;	
	bool			m_send_cursor_shape;
	bool			m_send_cursor_position;
	volatile LONG	m_dwUnAckFileDateMsg;	// send file target -> viewer
	volatile bool	m_killing;
	bool			m_rdp;
	bool			m_LAN;
	TrDesktop*		m_desktop;
	AudioOut		m_audioOu;
	AudioIn			m_audioIn;
	TrDesktopSelector	m_desktopSelector;
};

#endif
