#include "stdafx.h"
#include "InteropViewer.h"
#include "DlgRDPSettings.h"
#include "../main/aaProtocol.h"
#include "Common.h"
#include "TransportT.h"
#include "ReTranslator.h"
#include "DynamicFn.h"
#include "../viewer/vrMain.h"
#include "rsa/rsa.h"
#include "TCP.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

typedef BOOL (WINAPI *__Wow64DisableWow64FsRedirection) (PVOID *OldValue);
typedef BOOL (WINAPI *__Wow64RevertWow64FsRedirection)  (PVOID OldValue);

DynamicFn<__Wow64DisableWow64FsRedirection> _Wow64DisableWow64FsRedirection("kernel32.dll", "Wow64DisableWow64FsRedirection");
DynamicFn<__Wow64RevertWow64FsRedirection>  _Wow64RevertWow64FsRedirection ("kernel32.dll", "Wow64RevertWow64FsRedirection");


class CWow64Wrapper
{
public:
	CWow64Wrapper() 
	{
		//_log.WriteInfo("CWow64Wrapper()++");
		if (_Wow64DisableWow64FsRedirection.isValid() && _Wow64RevertWow64FsRedirection.isValid()) {
			BOOL b = (*_Wow64DisableWow64FsRedirection)(&m_OldValue);
			_log.WriteInfo("Wow64DisableWow64FsRedirection %u %X", b, m_OldValue);
		}	
	}

	~CWow64Wrapper()
	{
		//_log.WriteInfo("CWow64Wrapper()--");
		if (_Wow64DisableWow64FsRedirection.isValid() && _Wow64RevertWow64FsRedirection.isValid()) {
			BOOL b = (*_Wow64RevertWow64FsRedirection)(m_OldValue);
			_log.WriteInfo("Wow64RevertWow64FsRedirection %u", b);
		}
	}

private:
	PVOID m_OldValue;
};




InteropViewer::InteropViewer()
{
	m_type = aaTypeViewer;
	m_pRSA = NULL;
}

InteropViewer::~InteropViewer()
{
	FreeRSA();
}

void InteropViewer::FreeRSA()
{
	if (m_pRSA==NULL) return;
	delete m_pRSA;
	m_pRSA = NULL;
}

void InteropViewer::AddToInitMsg(RLStream& buffer)
{
	m_encryption = settings.m_encryption; // we need to save it, because it can be changed within session

	// just for debbuging
	//m_encryption = Settings::None;

	if (m_encryption<Settings::DynamicKey || m_encryption>Settings::DynamicKeyAndRSA)
		throw RLException("Invalid encryption settings");

	buffer.AddUINT8(m_encryption);

	if (m_encryption==Settings::DynamicKeyAndRSA) 
	{
		if (m_pRSA==NULL) m_pRSA = new EncryptorRSA();
		m_pRSA->GenKey2();
		m_pRSA->ExportPublicKey(buffer);  // 132 bytes
	}
}


HANDLE InteropViewer::RunMstsc(WORD port, DWORD& dwProcessId)
{
	CStringW rdpFileName = DlgRDPSettings::GetFileName();

	if (CCommon::FileIsExistW(rdpFileName))
		rdpFileName = L"\"" + rdpFileName + L"\" ";
	else
		rdpFileName = "";
		

	CStringW cmdLine;
	cmdLine.Format(L"mstsc.exe %s/v:127.0.0.1:%u", (LPCWSTR)rdpFileName, (UINT)port);

	STARTUPINFOW si = {0};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;

	PROCESS_INFORMATION	pi;

	{
		CWow64Wrapper wrapper;

		if (!::CreateProcessW(NULL, cmdLine.GetBuffer(0), NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
			throw RLException("ERROR: Failed to run RDP client, error=%d", ::GetLastError());

		::CloseHandle(pi.hThread);
		//::CloseHandle(pi.hProcess);

		dwProcessId = pi.dwProcessId;
	}

	return pi.hProcess;
}


SOCKET InteropViewer::AcceptSocket(SOCKET s, DWORD timeout)
{
	struct timeval tm;
	tm.tv_sec  = timeout/1000;
	tm.tv_usec = 0;

	struct fd_set fd;
	FD_ZERO(&fd);
	FD_SET(s, &fd);

	int	res = ::select(0, &fd, 0, NULL, &tm);

	if (res==0)
		throw RLException("AcceptSocket() timeout");

	if (res>0 && FD_ISSET(s, &fd)) 
	{
		SOCKET s1 = ::accept(s, NULL, 0);
		if (s1 == INVALID_SOCKET)
			throw RLException("accept() error=%d", ::GetLastError());

		return s1;

	}
	
	throw RLException("select() error=%d", ::GetLastError());
}

void InteropViewer::RunRDPclient(HANDLE& hRDPClient, DWORD& dwProcessId, HWND hwnd, UINT msg)
{
	const WORD portFrom = 6010;
	const WORD portTill = 6100;

	SOCKET s = INVALID_SOCKET;
	hRDPClient = NULL;

	_log.WriteInfo("RunRDPclient#0");

	try {
		WORD	port;
		s = TCP::ListenFreePort(portFrom, portTill, port);

		_log.WriteInfo("RunRDPclient#1 port=%d", (DWORD)port);

		hRDPClient = RunMstsc(port, dwProcessId);

		for (int i=0; i<2; i++) // allow 2 attempts
		{
			_log.WriteInfo("RunRDPclient#2 %d", i);
			
			SOCKET s2 = AcceptSocket(s, (i==0) ? 60000 : 5000);

			TCP::SetBlockingMode(s2, false);

			CReTranslator tr;
			tr.s1 = m_transport->GetSocket();
			tr.s2 = s2;
			
			int ret = tr.DoSmart();

			_log.WriteInfo("RunRDPclient#3 %d", ret);

			if (ret==2) continue;
			if (ret==3) {
				_log.WriteInfo("ReTranslate#1 Direct Transfer STARTED------------------------------");
				::closesocket(s);
				s = INVALID_SOCKET;
				if (hwnd) ::PostMessage(hwnd, msg, 0, 0);
				tr.DoDirect();
				_log.WriteInfo("ReTranslate#2 Direct Transfer FINISHED------------------------------");
				break;
			}				

			throw RLException("CReTranslator::DoSmart() return error %u", ret);
		}	
	}
	catch(RLException&) {
		if (s != INVALID_SOCKET) ::closesocket(s);
		if (hRDPClient!=NULL) {
			::TerminateProcess(hRDPClient, 0);
			::CloseHandle(hRDPClient);
			hRDPClient = NULL;
		}
		dwProcessId = 0;
		throw;
	}

	if (s != INVALID_SOCKET) ::closesocket(s);
}
