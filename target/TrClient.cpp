// The per-client object.  This object takes care of all per-client stuff, such as socket input and buffering of updates.

// TrClient class handles the following functions:
// - Recieves requests from the connected client and handles them
// - Handles incoming updates properly, using a TrDesktop object to keep track of screen changes


#include "stdafx.h"
#include "../main/aaProtocol.h"
#include "../main/AmmyyApp.h"
#include "../main/rsa/rsa.h"
#include "../main/Common.h"
#include "TrClient.h"
#include "TrService.h"
#include "TrMain.h"
#include "TrKeymap.h"
#include "TrDlgAccept.h"
#include "TrEncoder.h"


void TrClient::OnEventFS(LPCSTR templ, LPCSTR param1)
{
	this->SendInfo(templ, param1);
}


void TrClient::Thread01()
{
	try {
		this->SetState(InteropCommon::STATE::STARTED);

		_log2.Print(LL_INF, VTCLOG("ConnectingThread()#0"));

		if (_TrMain.m_bStopping) goto exit;

		this->Run();
	}
	catch(CStoppingException&) {
		_log2.Print(LL_INF, VTCLOG("catch CStoppingException"));
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
		this->ShowError(ex.GetDescription());

		// wait 5 secons in case of error
		for (int i=0; i<100; i++) {
			if (_TrMain.m_bStopping) break;
			::Sleep(50);
		}
	}

exit:
	//_log2.Print(LL_INF, VTCLOG("ConnectingThread()#8 exit"));

	try {
		_TrMain.RemoveClient(this); // it'll delete self
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
		this->ShowError(ex.GetDescription());
	}
}




void TrClient::DoCrypt()
{
	RLStream sendBuffer(256);
	BYTE cryptType;

	m_transport->ReadExact(&cryptType, 1);

	m_transport->m_encryptor_on = false;
	m_transport->SendByte(aaPreInitMsg_v3[0], FALSE); // sign to distinct from PING
	m_transport->m_encryptor_on = true;

	if (cryptType==Settings::Encryption::None) {
		m_transport->SetKey(NULL);
	}
	else 
	{
		BYTE key[20];
		CCommon::FillRandom(key, sizeof(key)); 
				
		if (cryptType==Settings::Encryption::DynamicKeyAndRSA)
		{
			char buffer[132];

			m_transport->ReadExact(buffer, sizeof(buffer));

			RLStream rsaKey(buffer, sizeof(buffer));
			rsaKey.SetLen(sizeof(buffer));
			
			EncryptorRSA rsa;
			rsa.ParseFromPublicKey(rsaKey);

			BYTE keyEncrypted[128]; // 128 bytes for RSA-1024

			rsa.Encrypt(RSA_PUBLIC, sizeof(key), (BYTE*)key, keyEncrypted);

			m_transport->SendExact(keyEncrypted, sizeof(keyEncrypted), FALSE);
		}
		else if (cryptType==Settings::Encryption::DynamicKey)
		{
			m_transport->SendExact(key, sizeof(key), FALSE);
		}
		else throw INVALID_PROTOCOL;
		
		m_transport->SetKey(key);
	}
}

bool TrClient::DirectConnectByTCP(TCP& tcp, UINT32 ip_v4, UINT16 port, UINT32 timeout, const char* key)
{
	SOCKET s = TCP::Connect(ip_v4, port, timeout);

	if (s==0) return false;	

	char buffer[17];

	buffer[0] = '%';
	memcpy(&buffer[1], key, 16);

	if (::send(s, buffer, sizeof(buffer), 0)!=sizeof(buffer))
		return false;

	tcp.Attach(s);

	return true;
}

bool TrClient::DirectConnectWaitTCP(const char* key, Transport* m_transport)
{
	// TODO: need to think about this while()
	while(true) {
		// IsReadyForRead(1ms) -> real time about 15 ms, but ::Sleep(1) also waits about 15ms
		if (m_transport->IsReadyForRead(1)) break;
		_TrMain.m_listener.OnTimer();
	}

	UINT8 msg = m_transport->ReadByte();

	if (msg==aaDcOK) {
		SOCKET s = 0;
		for (int i=0; i<10; i++) {
			s = _TrMain.m_listener.FindSocket(key, 16);
			if (s!=0) break;
			::Sleep(1);
			_TrMain.m_listener.OnTimer();
		}

		msg = (s!=0) ? aaDcOK : aaDcFailed;

		m_transport->SendByte(msg, TRUE);

		if (s!=0) {			
			m_transport->SetTCPSocket(s, true);
			return true; // ok
		}
	}	
	else if (msg!=aaDcFailed) throw INVALID_PROTOCOL;

	return false;
}


void TrClient::DoDirectConnect()
{
	while(true) 
	{
		UINT8 msg = ReadByte();

		if (msg==aaDcExit) return;

		if (msg==aaDcPing) {
			SendByte(aaDcPingReply, TRUE);
			continue;
		}

		if (msg!=aaDcWaitTCP && msg!=aaDcConnect) throw INVALID_PROTOCOL;
		
		char key[16];
		m_transport->ReadExact(key, sizeof(key));

		if (msg==aaDcConnect) {
			char buffer[10];
			m_transport->ReadExact(buffer, sizeof(buffer));
			UINT32 timeout = *((UINT32*)&buffer[0]);
			UINT32 ip      = *((UINT32*)&buffer[4]);
			UINT16 port    = *((UINT16*)&buffer[8]);

			TCP tcp;
			UINT8 reply = (DirectConnectByTCP(tcp, ip, port, timeout, key)) ? aaDcOK : aaDcFailed;
			SendByte(reply, TRUE);

			if (reply==aaDcOK) {
				msg = ReadByte();
				if (msg==aaDcOK) {
					// good, let's change transport
					m_transport->SetTCPSocket(tcp.Detach(), true);
					return;
				}
				else if (msg!=aaDcFailed) throw INVALID_PROTOCOL;
			}
		}
		else // aaDcWaitTCP
		{
			UINT16 port;
			m_transport->ReadExact(&port, sizeof(port));

			TrListenerWrapper listener(port);

			UINT8 reply = (listener.InOpened()) ? aaDcOK : aaDcFailed;

			SendByte(reply, TRUE);
			
			if (reply==aaDcOK) {		
				if (DirectConnectWaitTCP(key, m_transport)) return; // ok
			}
		}
	}
}

void TrClient::DoSpeedTest()
{
	while(true) 
	{
		UINT8 msg = ReadByte();

		if (msg==aaStExit) return;

		if (msg==aaStPing) {
			SendByte(aaStPingReply, TRUE);
			continue;
		}

		if (msg==aaStDownload) {
			const int len = sizeof(double) + sizeof(UINT32);
			RLStream buffer(len);
			m_transport->ReadExact(buffer.GetBuffer(), len);
			buffer.SetLen(len);

			double test_time;
			buffer.GetRaw(&test_time, sizeof(double));

			UINT32 block_size = buffer.GetUINT32();

			buffer.SetMinCapasity(block_size);
			char* pBuffer = (char*)buffer.GetBuffer();

			pBuffer[0] = aaStDownloadData;
			for (int i=1; i<block_size; i++) pBuffer[i] = (char)i;

			RLTimer t1;

			while(true) {	
				m_transport->SendExact(pBuffer, block_size, TRUE);
				double time = t1.GetElapsedSeconds();
				if (time>test_time)
					break;
			}

			this->SendByte(aaStDownloadFinished, TRUE);
		}
		else if (msg==aaStUpload) 
		{
			RLTimer t1;
			RLStream buffer(4);
			m_transport->ReadExact(buffer.GetBuffer(), 4);
			buffer.SetLen(4);
			
			UINT32 block_size = buffer.GetUINT32();

			buffer.SetMinCapasity(block_size);
			char* pBuffer = (char*)buffer.GetBuffer();

			int loops = 0;

			while(true) {	
				UINT8 b = this->ReadByte();

				if (loops==0) {
					t1.Start();
				}
				else {
					if (b==aaStUploadFinished) break;
				}
				if (b!=aaStUploadData) throw INVALID_PROTOCOL;
				m_transport->ReadExact(pBuffer, block_size-1);

				loops++;
			}

			double time = t1.GetElapsedSeconds();
			m_transport->SendExact(&time, sizeof(time), TRUE);
		}
		else 
			throw INVALID_PROTOCOL;
	}
}


bool TrClient::DoAuthorization()
{
	m_prm.m_password.SetEmpty(); // TODO: no need it, but for evidence...
	m_prm.ClearAll();			 // TODO: no need it, but for evidence...

	int ret = settings.m_permissions.TryToFind(m_prm);

	if (ret==2) // not found and no need password request
	{
		if (!TheApp.m_CmgArgs.noGUI) {
			TrDlgAccept dlg;
			dlg.m_prm.m_id = m_prm.m_id;
			dlg.m_transport = this->m_transport;
			dlg.DoModal();

			m_prm = dlg.m_prm;

			if (dlg.m_remember) {
				settings.m_permissions.Set(m_prm);
				settings.Save();
			}
		}
	}
	else if (ret==1) // found but need password
	{
		for (int i=0; i<3; i++) {
			BYTE b = aaAuthorizationNeedPassword;
			m_transport->SendExact(&b, sizeof(b), TRUE); // sending password request

			//read password
			UINT8 flag;
			m_transport->ReadExact(&flag, sizeof(flag));

			//cancelled by user and no need to send terminate msg
			if (flag==0xFF) return false;			
			
			m_transport->ReadExact(m_prm.m_password.hash, 16);
						
			if (settings.m_permissions.TryToFind(m_prm)==0) break;
		}
	}


	if (!_TrMain.IsAllowedNewAuthorized())
	{
		aaTerminateMsg msg;
		msg.type = aaError;
		msg.code = Router_ComputerIsBusy;
		m_transport->SendExact(&msg, sizeof(msg), TRUE);
		return false;
	}

	// send permissions
	char buffer[5];

	(*(UINT8 *)(&buffer[0])) = aaAuthorizationOK;
	(*(UINT32*)(&buffer[1])) = m_prm.m_values;
	
	m_transport->SendExact(buffer, sizeof(buffer), TRUE);
	
	m_prmRemoteControl = m_prm.Get(Permission::RemoteControl);
	m_prmFileManager   = m_prm.Get(Permission::FileManager);
	m_prmAudioChat	   = m_prm.Get(Permission::AudioChat);

	return !m_prm.IsClear();
}


void TrClient::GetReadyDirectInPort(RLStream& ports)
{
	int c = settings.m_directInItems.GetLen() / sizeof(Settings::DirectInItem);
	for (int i=0; i<c; i++) {
		Settings::DirectInItem* pItem = ((Settings::DirectInItem*)settings.m_directInItems.GetBuffer()) + i;
		if (pItem->m_status != 0) {
			ports.AddUINT16(pItem->m_port);
		}
	}
}

// send computer name
//
void TrClient::SendOtherInfo()
{
	RLStream stream(512);

	// fill OS_type, OS_version, computer_name and build_datetime
	{
		stream.AddUINT16(0);

		CCommon::AddOSTypeAndVersion(stream);
		
		CStringA computerName = CCommon::GetComputerName();
		computerName += '\n'; // this delimeter for compatiable with v3.0 Beta
		computerName += CCommon::GetBuildDateTime();
		stream.AddString1A(computerName);

		int len = stream.GetLen()-2;
		if (len>65535) throw RLException("SendOtherInfo()#1");
		*((UINT16*)stream.GetBuffer()) = len;
	}

	// send direct connect info if we have Router connection
	if (!m_transport->m_direct)
	{
		UINT8 flags = 0;
		if (settings.m_allowDirectTCP) flags |= 1;
		//if (settings.m_allowDirectUDP) flags |= 2;
		if (RLHttp::m_proxyPort!=0) flags |= 4; // if HTTP proxy used
		stream.AddUINT8(flags);
		
		if ((flags&3)!=0) {
			RLStream internal_ips;
			TCP::GetIPaddresses(internal_ips);
			stream.AddUINT32(m_external_ip);
			stream.AddUINT16(internal_ips.GetLen());
			stream.AddRaw(internal_ips.GetBuffer(), internal_ips.GetLen());
			
			RLStream directInPorts;
			TrClient::GetReadyDirectInPort(directInPorts);
			stream.AddUINT16(TrListener::GetIntranetPort()); // intranet TCP port
			stream.AddUINT16(directInPorts.GetLen());
			stream.AddRaw(directInPorts.GetBuffer(), directInPorts.GetLen());
		}
	}

	m_transport->SendExact(stream.GetBuffer(), stream.GetLen(), TRUE);
}


void TrClient::ClearKeyState(BYTE key)
{
	// This routine is used by the client handler to clear the CAPSLOCK, NUMLOCK and SCROLL-LOCK states.
	BYTE keyState[256];

	GetKeyboardState((LPBYTE)&keyState);

	if(keyState[key] & 1)
	{		
		keybd_event(key, 0, KEYEVENTF_EXTENDEDKEY, 0);						// Simulate the key being pressed
		keybd_event(key, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);	// Simulate it being release
	}
}


void TrClient::Run()
{
	HDESK hDesktop = NULL;

	if (!m_LAN) this->ConnectToRouter();		

	_log2.Print(LL_INF, VTCLOG("TrClient::Run()#1 id=%d"), (int)m_id);

	m_audioIn.m_transport = this->m_transport;
	m_audioOu.m_transport = this->m_transport;
	m_audioOu.m_hOwnerWnd = _TrMain.m_hwnd;

	// To avoid people connecting and then halting the connection, set a timeout
	//m_transport->SetTimeout(30000); // maximp : I think it's no need
		
	try {		
		{
			UINT32 myId, remoteId, version;
			InteropCommon::ReadInitMsg(m_LAN, myId, remoteId, version);
			m_prm.m_id = myId; // need for Authorization()
			_log2.Print(LL_INF, VTCLOG("ReadInitMsg() remoteId=%u"), remoteId);
		}

		this->SetState(InteropCommon::STATE::OPERATOR_CONNECTED);

		// TODO: may be do it only for service ???
		try {
			settings.Load();
		}
		catch(RLException& ex) 
		{
			_log.WriteError(ex.GetDescription());
		}

		if (m_LAN) {
			this->SendInitMsg(m_transport->GetSocket());
		}

		DoCrypt();
		if (!DoAuthorization()) return;


		this->SetState(InteropCommon::STATE::OPERATOR_AUTHORIZED);

		SendOtherInfo();

		bool desktopInit = false;
		bool waitingPingReply = false;

		hDesktop = TrDesktopSelector::GetThreadDesktop();
		
		while (true) // MAIN LOOP
		{
			UINT8 msg_type = this->ReadByteWithPinging(waitingPingReply); // send ping request

			if (msg_type==aaError) {
				aaTerminateMsg msg;
				m_transport->ReadExact(((char *) &msg)+1, sizeof(msg)-1);
				_log2.Print(LL_INF, VTCLOG("get close message with code=%d"), (int)msg.code);
				m_killing = true; m_audioIn.SetStoppingStatus();
				break;
			}

			if (msg_type==aaRDP) {
				this->DoRDP();
				break;
			}
			if (msg_type==aaDirectConnect) {
				this->DoDirectConnect();
				continue;
			}
			if (msg_type==aaSpeedTest) {
				this->DoSpeedTest();
				break;
			}

			// aaSetEncoder, can be removed from this block, but I leave it, cause test only on WinXP
			if (msg_type==aaKeyEvent || msg_type==aaPointerEvent || 
				msg_type==aaCutText  || msg_type==aaSetEncoder)
			{
				if (!desktopInit) {
					desktopInit = true;
					m_desktopSelector.SetNameOfDesktop(NULL);
				}

				// need it only for events which related with desktop
				// to ensure that we're running in the correct desktop
				m_desktopSelector.SetInputDesktop();
			}			

			// What to do is determined by the message id
			switch(msg_type)
			{
				case aaSetEncoder:			OnAaSetEncoder();			break;
				case aaDesktopOFF:			OnAaDesktopOFF();			break;
				case aaSetPointer:			OnAaSetPointer();			break;
				case aaScreenUpdateRequest:	OnAaScreenUpdateRequest();	break;
				case aaScreenUpdateCommit:	OnAaScreenUpdateCommit();	break;
				case aaKeyEvent:			OnAaKeyEvent();				break;
				case aaPointerEvent:		OnAaPointerEvent();			break;
				case aaCutText:		        OnAaCutText();		        break;
				
				case aaFileListRequest:		OnAaFileListRequest();		break;
				case aaFolderCreateRequest:	OnAaFolderCreateRequest();	break;
				case aaRenameRequest:		OnAaRenameRequest();		break;
				case aaDeleteRequest:		OnAaDeleteRequest();		break;
				case aaDnloadRequest:		OnAaDnloadRequest();		break;
				case aaUploadRequest:		OnAaUploadRequest();		break;
				case aaUploadData:			OnAaUploadData(false);		break;
				case aaUploadDataLast:		OnAaUploadData(true);		break;
				case aaDnloadDataAck:		OnAaDnloadDataAck();		break;

				case aaSound:				OnAaSound();			break;
				case aaNop:							continue; // do nothing

				default:
					throw RLException("invalid message received : %u", (int)msg_type);
			}
		}
	}
	catch(TransportException& ex) {
		this->OnTransportError(ex, "TrClient::Run()#239");
	}
	catch(RLException& ex) {
		_log.WriteError("TrClient::Run()#1 '%s'", ex.GetDescription());
		//_log.WriteError(ex.GetDescription());
	}

	// recover to original state, to call CloseDesktopHandle without errors
	if (hDesktop!=NULL) {
		if (::SetThreadDesktop(hDesktop)==0)
			_log.WriteError("ERROR SetThreadDesktop(%u) %u", hDesktop, ::GetLastError());
	}
	
	_log2.Print(LL_INF, VTCLOG("TrClient::Run()#end id=%d "), (int)m_id);
}


void TrClient::OnAaSound()
{
	if (!m_prmAudioChat) throw RLException("AudioChat is forbidden");

	UINT16 count;

	m_transport->ReadExact(&count, sizeof(count));

	if (count==aaSoundClose) {
		_log.Print(LL_SOUND, VTCLOG("OnAaSound()#1 aaSoundClose"));

		m_audioOu.StopSound();
		m_audioIn.StopSound();
	}else if (count==aaSoundInit) {
		DWORD samplesPerSecond;

		m_transport->ReadExact(&samplesPerSecond, sizeof(samplesPerSecond));

		m_audioIn.StartSound(samplesPerSecond);
		m_audioOu.PreStart(samplesPerSecond);
	}
	else {
		m_audioOu.OnData(count);
	}
}


void TrClient::DoRDP()
{
	if (!m_prm.Get(Permission::RDPsession)) throw RLException("RDP is forbidden");

	m_rdp = true;

	UINT16 error = aaErrorNone; //mean ok

	try {		
		this->ConnectToRDPserver();
	}
	catch(RLException& ex) {
		_log2.Print(LL_ERR, VTCLOG("%s"), ex.GetDescription());
		error = aaErrorRDPServer;
	}
		
	m_transport->SendExact(&error, sizeof(error), TRUE);
	if (error == aaErrorNone) {
		this->ReTranslate();
	}
}



void TrClient::OnAaSetEncoder()
{
	if (!m_prm.Get(Permission::ViewScreen)) throw RLException("Desktop is forbidden");

	aaSetEncoderMsg msg;
	m_transport->ReadExact(((char *) &msg)+1, sizeof(msg)-1);

	omni_mutex_lock l(m_regionLock);

	bool off = (m_desktop==NULL);

	if (off) // DesktopTurnOn
	{
		m_desktop = new TrDesktop(this);
		if (m_desktop == NULL) throw RLException("ERROR new TrDesktop()");
		
		UINT16 errorCode = m_desktop->InitDesktop();

		if (errorCode!=0) {
			this->KillClient(errorCode);
			return;
		}

		// Clear the CapsLock and NumLock keys
		if (this->m_prmRemoteControl)
		{
			ClearKeyState(VK_CAPITAL);
			// *** JNW - removed because people complain it's wrong
			//ClearKeyState(VK_NUMLOCK);
			ClearKeyState(VK_SCROLL);
			TrKeymap::ClearShiftKeys();
		}
	}

	memcpy(&m_desktop->m_aaSetEncoderMsg, &msg, sizeof(msg));
	m_desktop->m_aaSetEncoderMsg.type = 1; // mark as ready

	// send screen update as soon as possible, but only when it's started
	if (off) 
		m_desktop->m_eventUpdate.Set();
}

void TrClient::OnAaDesktopOFF()
{
	if (m_desktop == NULL) return;
	
	_log2.Print(LL_INF, VTCLOG("deleting desktop object"));
	delete m_desktop;
	m_desktop = NULL;	
}

void TrClient::OnAaSetPointer()
{
	UINT8 flags = m_transport->ReadByte();

	m_send_cursor_position = (flags & aaSetPointerPos  )!=0;
	m_send_cursor_shape    = (flags & aaSetPointerShape)!=0;
}


void TrClient::OnAaScreenUpdateRequest()
{	
	if (m_desktop==NULL) return;

	RLMutexLock l(m_desktop->m_full_refresh_lock);
	m_desktop->m_full_refresh = true;	// mark the full update
}


void TrClient::OnAaScreenUpdateCommit()
{
	aaScreenUpdateCommitMsg msg;
	m_transport->ReadExact(((char *) &msg)+1, sizeof(msg)-1);

	if (m_desktop==NULL) return;

	LONG result = 1000;

	for(int i=0; i<msg.countCommits; i++) {
		result = ::InterlockedDecrement(&m_desktop->m_dwUncommitedScreenUpdates);
	}

	if (result==0) {
		omni_mutex_lock l(m_regionLock);
		m_desktop->m_eventUpdate.Set();
	}
}



void TrClient::OnAaKeyEvent()
{
	aaKeyEventMsg msg;

	m_transport->ReadExact(((char *) &msg)+1, sizeof(msg)-1);
	
	if (this->m_prmRemoteControl && m_desktop!=NULL)
	{
		// Get the keymapper to do the work
		TrKeymap::KeyEvent(msg.key, msg.down != 0);

		//_log.WriteError("aaKeyEvent#1 %d %d", msg.key, msg.down);

		// if key down event, fire urgent check for updates
		if (msg.down!=0)
		{
			//_log.WriteError("aaKeyEvent#2 firing urgent update event %d", msg.key);
			m_desktop->m_eventRemoteKey.Set();
		}
	}
}

void TrClient::OnAaPointerEvent()
{
	aaPointerEventMsg msg;

	m_transport->ReadExact(((char *) &msg)+1, sizeof(msg)-1);

	m_counterAaPointerEvent++;
	
	if (!m_prmRemoteControl) return;
	
	DWORD flags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

	if ( (msg.buttonMask & aaButton1Mask) != (m_ptrevent_buttonMask & aaButton1Mask) )
	{
		if (GetSystemMetrics(SM_SWAPBUTTON))
			flags |= (msg.buttonMask & aaButton1Mask) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
		else
			flags |= (msg.buttonMask & aaButton1Mask) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
	}
	if ( (msg.buttonMask & aaButton2Mask) != (m_ptrevent_buttonMask & aaButton2Mask))
	{
		flags |= (msg.buttonMask & aaButton2Mask) ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
	}
	if ( (msg.buttonMask & aaButton3Mask) != (m_ptrevent_buttonMask & aaButton3Mask))
	{
		if (GetSystemMetrics(SM_SWAPBUTTON))
			flags |= (msg.buttonMask & aaButton3Mask) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
		else
			flags |= (msg.buttonMask & aaButton3Mask) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
	}

	// Treat buttons 4 and 5 presses as mouse wheel events
	DWORD wheel_movement = 0;
	if ((msg.buttonMask & aaButton4Mask) != 0 && (m_ptrevent_buttonMask & aaButton4Mask) == 0)
	{
		flags |= MOUSEEVENTF_WHEEL;
		wheel_movement = (DWORD)+120;
	}
	else if ((msg.buttonMask & aaButton5Mask) != 0 && (m_ptrevent_buttonMask & aaButton5Mask) == 0)
	{
		flags |= MOUSEEVENTF_WHEEL;
		wheel_movement = (DWORD)-120;
	}

	if (m_desktop==NULL) return;

	// Generate coordinate values

	// Remember cursor position for this client and put position relative to screen
	int x1 = m_desktop->m_cursor_pos.x = msg.x + m_desktop->m_capture.m_screenX;
	int y1 = m_desktop->m_cursor_pos.y = msg.y + m_desktop->m_capture.m_screenY;
	int dX = m_desktop->m_mouseDividerX; 
	int dY = m_desktop->m_mouseDividerY;

	if (dX>0 && dY>0) {
		long x = (x1*65535) / dX;
		long y = (y1*65535) / dY;
		::mouse_event(flags, (DWORD)x, (DWORD)y, wheel_movement, 0); // Do the pointer event
	}
	else 
		_log.WriteError("Mouse dividers not set");

	m_ptrevent_buttonMask = msg.buttonMask; // Save the old buttonMask

	m_desktop->m_pointer_event_timer.Start();
}


void TrClient::OnAaCutText()
{
	aaCutTextMsg msg;

	m_transport->ReadExact(((char *)&msg)+1, sizeof(msg)-1);
	
	// Allocate storage for the text
	const UINT length = msg.length;
	CStringA text = ReadString(length);

	// if permission ok and desktop is exist, update the local clipboard
	if (m_prm.Get(Permission::ClipboardIn) && m_desktop!=NULL) m_desktop->SetLocalClipboard((LPSTR)(LPCSTR)text);
}

CStringA TrClient::ReadString(UINT32 count)
{
	CStringA str;
	m_transport->ReadExact(str.GetBuffer(count), count);
	str.ReleaseBuffer(count);
	return str;
}


TrClient::TrClient()
{
	_log2.Print(LL_INF, VTCLOG("TrClient() executing..."));

	m_transport = new TransportTCP1();

	m_ptrevent_buttonMask = 0;	
	m_send_cursor_shape    = false;
	m_send_cursor_position = false;

	m_dwUnAckFileDateMsg = 0;

	m_desktop = NULL;

	TrKeymap::Init();

	m_rdp = false;
	m_id = 0;

	m_state = InteropCommon::STATE::OFF;
	m_LAN = false;
	m_killing = false;
}

TrClient::~TrClient()
{
	_log2.Print(LL_INF, VTCLOG("~TrClient() executing..."));

	// need closing here to close Desktop faster with transport error
	{
		omni_mutex_lock l(m_transport_close_lock);
		m_transport->Close(false);	// may be it was closed already
	}

	m_audioIn.StopSound(); // need to stop because it can use m_transport

	//  delete m_desktop, need do it outside m_transport_close_lock, cause can be deadlock with OnTransportError()
	OnAaDesktopOFF();

	
	// remove transport the last
	ASSERT(m_transport != NULL)
	_log2.Print(LL_INF, VTCLOG("deleting transport"));
	delete m_transport;	
}


void TrClient::KillClient(UINT16 reason)
{
	omni_mutex_lock l(m_transport_close_lock);

	m_killing = true; m_audioIn.SetStoppingStatus();

	if (m_transport->IsOpened())
	{
		bool sendTermMsg = (!m_rdp) && (m_state==InteropCommon::STATE::OPERATOR_AUTHORIZED);

		try {
			if (sendTermMsg) {
				_log2.Print(LL_INF, VTCLOG("KillClient(%d)"), (int)reason);
	
				aaTerminateMsg msg;
				msg.type = aaError;
				msg.code = reason;

				m_transport->SendExact(&msg, sizeof(msg), TRUE);
			}
		}
		catch(TransportException& ex) {
			_log.WriteError("ERROR in TrClient::Kill() '%s'", ex.GetDescription());
		}
		m_transport->Close(sendTermMsg);
	}
}


void TrClient::OnTransportError(TransportException& ex, LPCSTR where)
{
	omni_mutex_lock l(m_transport_close_lock);

	if (m_transport->IsOpened()) {
		m_transport->Close(false);

		//if (!ex.IsNormalClose())
		//	_log.WriteError("%s in %s", (LPCSTR)ex.GetDescription(), where);
		//else
		//	_log.WriteInfo ("%s in %s", (LPCSTR)ex.GetDescription(), where);
	
		if (!m_killing)
			_log.WriteError("%s in %s", (LPCSTR)ex.GetDescription(), where);
	}	
}



void TrClient::SendClipboardText(LPCSTR text)
{
	if (m_desktop==NULL) return;
	if (!m_prm.Get(Permission::ClipboardOut)) return; // Don't send the clipboard contents to a view-only client

	aaCutTextMsg message;
	message.type   = aaCutText;
	message.length = strlen(text);

	RLStream buffer(sizeof(message) + message.length);
	buffer.AddRaw(&message, sizeof(message));
	buffer.AddRaw(text, message.length);

	//_log.WriteInfo("SendClipboardText() %s", text);

	m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);
}

