#include "stdafx.h"
#include "ReTranslator.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

const char SPECIAL = (char)0x8E;
const UINT16 SPECIAL_TERMINATE = 0x0100 + (UINT8)SPECIAL;
const UINT16 SPECIAL_DIRECT    = 0x0200 + (UINT8)SPECIAL;

const int MAX_READ = 16384;
const int MAX_SEND = 16384;



CReTranslator::CReTranslator()
{
	s1 = NULL;
	s2 = NULL;
}

CReTranslator::~CReTranslator()
{
	CloseChildSocket();
}


void CReTranslator::DecodeS1(int dataLen)
{
	LPSTR pData = queue1.GetBufferWr();
	LPSTR pDataOut   = pData;

	if (m_s1_smart)
	{
		LPSTR pDataIn	 = pData;	
		LPSTR pDataInEnd = pData + dataLen;
		

		while (pDataIn<pDataInEnd) {
			char c = *pDataIn++;
			if (m_decode_state_normal) {
				if (c==SPECIAL) 
					m_decode_state_normal = false;
				else
					*pDataOut++ = c;
			}
			else {
				if (c==SPECIAL) {
					*pDataOut++ = c;
					m_decode_state_normal = true;
				}
				else {
					if (c==1) {
						m_s1_opened = false;
						ASSERT(pDataIn==pDataInEnd); // shoule be last char if it's terminate cmd
					} 
					else if (c==2) { //just copy	the rest
						m_s1_smart = false;
						_log.WriteInfo("CReTranslator::Decode()#1 remote child use direct transfer now");
						int count = pDataInEnd - pDataIn;
						if (count>0) {
							memcpy(pDataOut, pDataIn, count);
							pDataOut += count;
							//pDataIn  += count;
						}
					}
					else
						ASSERT(false); // not valid command

					
					break;
				}
			}
		}
	}
	else {
		pDataOut = pData + dataLen;
	}

	int count = pDataOut - pData;
	queue1.AddRaw(NULL, count);
	m_bytes_s1 += count;
}


void CReTranslator::EncodeS2(LPCSTR pDataIn, int dataInLen)
{
	if (!m_s2_smart) {
		queue2.AddRaw(pDataIn, dataInLen);
	}
	else {
		queue2.SetMinExtraCapasity(dataInLen*2); // for worse case need twice memory
		LPSTR pDataOutOrg = queue2.GetBufferWr();
		LPSTR pDataOut = pDataOutOrg;

		LPCSTR pDataInEnd = pDataIn + dataInLen;

		while (pDataIn<pDataInEnd) {
			char c = *pDataIn++;
			*pDataOut++ = c;
			if (c==SPECIAL) *pDataOut++ = c;
		}

		int count = pDataOut - pDataOutOrg;
		queue2.AddRaw(NULL, count);
	}
}


void CReTranslator::CloseChildSocket()
{
	if (s2) {
		BOOL val = 0; // to send RST
		if (::setsockopt(s2, SOL_SOCKET, SO_DONTLINGER, (LPCSTR)&val, sizeof(val))!=0) {
			_log.WriteError("setsockopt(SO_DONTLINGER) error=%d", ::WSAGetLastError());
		}

		::closesocket(s2);
		s2 = NULL;

		queue2.AddUINT16(SPECIAL_TERMINATE); // send solid socket command that socket was closed
	}
}



// 1 - critical error with solid socket (s1)
// 2 - child socket was closed or remote child socket sent command for closing
// 3 - ready for direct transfer

int CReTranslator::DoSmart()
{
	_log.WriteInfo("CReTranslator::DoSmart()#0 solid=%X, child=%X", s1, s2);

	m_bytes_s1 = 0;
	m_bytes_s2 = 0;
	m_decode_state_normal = true;
	m_s1_smart = true;
	m_s2_smart = true;
	m_s1_opened  = true;	

	RLStream queueTmp(MAX_READ);

	struct fd_set rds_read;
	struct fd_set rds_write;

	// Wait until some data can be read or sent
	while(true) {
		FD_ZERO(&rds_read);
		FD_ZERO(&rds_write);
		if (m_s1_opened) FD_SET(s1, &rds_read);
		if (s2!=NULL)    FD_SET(s2, &rds_read);
		if (queue2.GetAvailForReading()>0) FD_SET(s1, &rds_write);
		if (queue1.GetAvailForReading()>0 && s2) FD_SET(s2, &rds_write);

		int count = ::select(0, &rds_read, &rds_write, NULL, NULL);
		if (count <= 0 || count > 4) {
			_log.WriteError("DoSmart()#1 error=%d %d", ::GetLastError(), count);
			return 1;
		}

		//if (count==0) continue; // just time-out

		if (m_s1_opened && FD_ISSET(s1, &rds_read)) {
			queue1.SetMinExtraCapasity(MAX_READ);

			int bytes = SimpleRead1(s1, queue1.GetBufferWr());		

			if (bytes > 0) {
				DecodeS1(bytes);

				if (!m_s1_opened) {
					_log.WriteInfo("CReTranslator::DoSmart()#139 Remote child socket was closed");
					CloseChildSocket();
				}

			} else if (bytes < 0) {
				_log.WriteError("CReTranslator::DoSmart()#159 solid socket read error=%d", ::WSAGetLastError());
				return 1;
			}
		}

		if (s2 && FD_ISSET(s2, &rds_read)) {
			int bytes = SimpleRead1(s2, queueTmp.GetBufferWr());

			if (bytes > 0) {
				EncodeS2(queueTmp.GetBufferWr(), bytes);
				m_bytes_s2 += bytes;

			} else if (bytes < 0) {
				_log.WriteInfo("CReTranslator::DoSmart()#165 child socket was closed, error=%d", ::WSAGetLastError());
				CloseChildSocket();
			}
		}

		if (m_s2_smart && m_bytes_s1>128 && m_bytes_s2>128) {
			m_s2_smart = false;
			queue2.AddUINT16(SPECIAL_DIRECT);
			_log.WriteInfo("CReTranslator::DoSmart()#170 sent request for direct transfer");
		}


		// Try to send some data

		if (s2)
		{
			if (!SimpleSend1(s2, queue1)) {
				_log.WriteInfo("CReTranslator::DoSmart()#166 child socket was closed, error=%d", ::WSAGetLastError());
				CloseChildSocket();
			}
		}

		if (!SimpleSend1(s1, queue2)) {
			_log.WriteError("CReTranslator::DoSmart()#159 solid socket send error=%d", ::WSAGetLastError());
			return 1;
		}

		if (!m_s1_opened && s2==NULL && queue2.GetAvailForReading()==0) 
			return 2;
		if (m_s1_opened && !m_s1_smart && !m_s2_smart) return 3;
	}
}


inline int CReTranslator::SimpleRead1(SOCKET s, LPSTR pBuffer)
{
	int bytes = ::recv(s, pBuffer, MAX_READ, 0);
			
	_log.WriteInfo("CReTranslator::SimpleRead1(%X) read %d bytes",s, bytes);

	if (bytes > 0) {
		return bytes;
	} 
	else if (bytes < 0) {
		if (::WSAGetLastError() == WSAEWOULDBLOCK) return 0; // no error, but no data also, need to try later
	}
	
	return -1; // error
}

inline bool CReTranslator::SimpleRead2(SOCKET s, RLStream& queue)
{
	queue.SetMinExtraCapasity(MAX_READ);

	int bytes = ::recv(s, queue.GetBufferWr(), MAX_READ, 0);

	if (bytes > 0) {
		queue.AddRaw(NULL, bytes);
	} else if (bytes < 0) {
		DWORD dwError = ::WSAGetLastError();
		if (dwError != WSAEWOULDBLOCK) {
			if (dwError!=WSAECONNRESET && dwError!=WSAECONNABORTED)	
				_log.WriteError("CReTranslator::SimpleRead()#1 error=%d", dwError);
			return false;
		}
	} else
		return false;


	return true;
}

inline bool CReTranslator::SimpleSend1(SOCKET s, RLStream& queue)
{
	int count = queue.GetAvailForReading();
	if (count>0) 
	{	
		int bytes = ::send(s, queue.GetBufferRd(), min(MAX_SEND,count), 0);

		_log.WriteInfo("CReTranslator::SimpleSend1(%X) send %d bytes", s, bytes);

		if (bytes > 0) {
			if (bytes==count) queue.Reset();
			else              queue.GetRaw(NULL, bytes);
		} else if (bytes < 0) {
			if (::WSAGetLastError() != WSAEWOULDBLOCK) return false;
		} else
			return false;
	}

	return true;
}


void CReTranslator::DoDirect()
{
	struct fd_set rds_read;
	struct fd_set rds_write;

	// Wait until some data can be read or sent
	while(true) {
		FD_ZERO(&rds_read);
		FD_ZERO(&rds_write);
		FD_SET(s1, &rds_read);
		FD_SET(s2, &rds_read);
		if (queue2.GetAvailForReading()>0) FD_SET(s1, &rds_write);
		if (queue1.GetAvailForReading()>0) FD_SET(s2, &rds_write);

		int count = ::select(0, &rds_read, &rds_write, NULL, NULL);
		if (count <= 0 || count > 4) {
			_log.WriteError("ReTranslate()#1 error=%d %d", ::GetLastError(), count);
			return;
		}

		//if (count==0) continue; // just time-out

		if (FD_ISSET(s1, &rds_read)) {
			if (!SimpleRead2(s1, queue1)) return; 
		}

		if (FD_ISSET(s2, &rds_read)) {
			if (!SimpleRead2(s2, queue2)) return; 
		}
		
		if (!SimpleSend1(s2, queue1)) return;
		if (!SimpleSend1(s1, queue2)) return;
	}
}
