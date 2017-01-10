#if !defined(_C_ROUTER_H_INCLUDED_)
#define _C_ROUTER_H_INCLUDED_

#pragma warning (disable : 4786)
#include <vector>
#include <map>
#include <set>
#include "FastQueue.h"
#include "../RL/RLLock.h"
#include "../RL/RLLog.h"
#include "../RL/RLEncryptor02.h"

#ifdef _LINUX
#include <sys/epoll.h>
#endif
#ifdef _FreeBSD
#include <sys/event.h>
#endif

//#define _ROUTER_CAPTURE_

#include "StreamWithTime.h"

class CRouter  
{
public:
	class SocketBase
	{
	public:
		void CloseSocket();

#ifdef _WIN32
		virtual void OnEvent(WORD event, WORD error) =0;
#else
		virtual void OnEvent(UINT32 events) =0;
#endif
		inline SOCKET GetSocket() const { return m_socket; }
		//inline void DeleteIfClosed() { if (m_socket==0) delete this; }
		void DeleteIfClosed(); // tmp no inline for debug

		// I think it's faster with variable
		inline bool IsListener() { return m_type<0; }

	protected:
		virtual ~SocketBase() {} // need to be to children will call this in destructor
		void AsyncSelect(bool set);

		enum SocketType
		{
			TypeListener  = -1,
			TypeUndefined =  0,
			TypeTarget    =  1,	// aaTypeTarget
			TypeViewer    =  2, // aaTypeViewer
			TypeHttpQuery =  3,
		};

		SOCKET m_socket;
		int	   m_type;	// SocketType

	private:
		friend class Sockets;
	};

	class SocketListener: public SocketBase
	{
	public:
		SocketListener(WORD port);
		virtual ~SocketListener() {}
		static void CheckPort(UINT16 port);
		void  OnAccept();
#ifdef _WIN32
		virtual void OnEvent(WORD event, WORD error);
#else
		virtual void OnEvent(UINT32 events);
#endif

	private:
		WORD m_port;
	};

	class SocketConn : public SocketBase
	{
	public:
		SocketConn(SOCKET m_socket, UINT32 ip, UINT16 portR, UINT16 postL);
		virtual ~SocketConn() {}
		void	OnClose();
		inline  void OnCloseSafe() { if (m_socket!=0) OnClose(); } 
		void	OnReceive();
		void	OnSend();
#ifdef _WIN32		
		virtual void OnEvent(WORD event, WORD error);
#else
		virtual void OnEvent(UINT32 events);
#endif

		inline bool IsLinked()    { return (m_pOtherLeg!=NULL); }
		inline bool IsTarget()    { return (m_type==TypeTarget); }
		inline bool IsViewer()    { return (m_type==TypeViewer); }
		inline bool IsHttpQuery() { return (m_type==TypeHttpQuery); }
		inline LPCSTR GetName()   { return m_name; }

	private:
		CStringA FindGetArgument(const CStringA& uri, const CStringA& name);
		inline void OnReceive_v2();
		inline void OnReceive_v3();
		       void SendReply_v3(INT8 status);
		inline void	OnReceiveUnknown(); // called when type is unknown
			   void	OnReceiveHttp();
			   void	DisConnect();
		inline void SendSimple (const void* pBuffer, const int len);
		inline  int	SendOneTime(const char* pBuffer, int len);
		inline void OnLinked();
		       void OnLinked_v3();

	public:
		SocketConn* m_pOtherLeg;

		UINT64		m_time;		// timestamp   when connected two legs
		UINT32		m_id;		// Ammyy ID of this connection		

		struct STAT {
			DWORD BytesRecv;
			DWORD RecvPackets;
			DWORD SendPackets;
			DWORD MaxRecvPacketSize;
			DWORD MaxSendPacketSize;
			DWORD MaxQueueSize;
			DWORD QueueSize; // current queue size need for GetStat1() only!!!
		} m_stat;
	
	private:
		bool		m_v3;
		char		m_name[40];
		RLStream	m_recvBuffer;
		CFastQueue	m_sendBuffer;	// buffer for sending
		UINT32		m_ip;			// remote ip
		UINT16		m_port;			// remote port
		static		char* m_pBufferStatic;

		#ifdef _ROUTER_CAPTURE_
			CStringA		m_path;
			StreamWithTime  m_fileStream;
		#endif

		friend class CRouter;
	};

	//__________________________________________________________________________________________________

	private:
		static void AddStatistics(CRouter::SocketConn::STAT& s1, CRouter::SocketConn::STAT& s2);
		static void Add_INT64(CStringA& out, INT64 val);
		static void Add_INT32(CStringA& out, INT32 val);
		CStringA GetStat1(UINT32 dwSelectedLegData);
		CStringA GetStat2();

	struct SpeedMeasure
	{
		// last values
		UINT64 m_bytesSent;
		UINT64 m_bytesRecv;
		DWORD  m_ticks;
	}m_speedMeasure;

	//__________________________________________________________________________________________________


public:
	CRouter();
	~CRouter();

	void Start();
	void Stop();	
	inline bool IsRunning() { return m_started; }

	static void ConfigCreateFile();
	       void ConfigReadFile();
		   void CheckPorts();

private:
	CStringA GetCurrentTimeUTC();
	void SendWaitingIds();

private:
	struct STAT {
		UINT64 BytesRecv;
		UINT64 BytesSent;
		UINT32 MaxRecv;	// bytes
		UINT32 MaxSent; // bytes
	} m_stat;

	
private:
	std::vector<SocketListener*>	m_Listeners;
	bool m_statWithControls;

	       int	OnNewInitiator(SocketConn* pLeg, UINT32 remoteId);
	inline void OnNewTarget(UINT32 id);
	inline int  OnNewID(UINT32 id, BYTE* pwd_md5);
 	       //void OnTimer();
	void	CloseHandles();
#ifdef _WIN32
	static	HWND CreateWorkerWindow();
	static  LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND    m_hWorkerWnd;
	#define THREAD_RETURN UINT
#else
	#define THREAD_RETURN LPVOID
	int			m_epoll;
	int			m_epoll_events_count;
#ifdef _LINUX
	epoll_event m_epoll_events[20];
#else //FreeBSD
	struct kevent	m_epoll_events[20];
#endif
#endif

	static THREAD_RETURN __stdcall WorkingThread1_static(LPVOID param);
	static THREAD_RETURN __stdcall WorkingThread2_static(LPVOID param);
	void	WorkingThread1();
	void	WorkingThread2();


	bool		m_enableNagle;
	CStringA	m_assignIP;
	CStringA	m_listenPorts;
	CStringA	m_password_console;
	RLMD5		m_password_using;
	bool		m_password_using_set;
	RLStream	m_allow_ids;

	std::vector<DWORD> m_newWaitingIDs;
	std::set<DWORD>    m_newWaitingIDs_for_sending;
	RLMutex			m_newWaitingIDs_locker;
	RLMutex			m_start_stop_locker;

	CStringA		m_startedTime; // UTC time

	bool			m_started;
	unsigned long	m_thread1;
	unsigned long	m_thread2;

private:
	bool        m_capture;
	RLLog		m_logLogins;
	//RLLog m_logNoSocket;

	RLEncryptor02 m_TemplateEncryptor02;
	RLEncryptor02 m_TemplateDecryptor02;
	RLEncryptor02 m_InstanceEncryptor02;
	RLEncryptor02 m_InstanceDecryptor02;

	friend class SocketBase;
	friend class SocketConn;
	friend class SocketListener;
};

extern CRouter Router;

#endif // !defined(_C_ROUTER_H_INCLUDED_)
