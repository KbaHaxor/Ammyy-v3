#include "stdAfx.h"
#include "TransportT.h"
#include "Common.h"
#include "ReTranslator.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


void TransportTCP1::SendLock()
{
	m_send_lock.lock();
}

void TransportTCP1::SendUnlock()
{
	m_send_lock.unlock();
}


/*
void TransportTCP1::SendExact(void *buff, UINT32 bufflen, BOOL block)
{
	if (m_encryptor_on) m_encryptor.Encrypt((UINT8*)buff, bufflen);

	struct fd_set write_fds;
	struct timeval tm;
	int count;

		do {
			FD_ZERO(&write_fds);
			FD_SET((SOCKET)m_socket, &write_fds);
			tm.tv_sec = 1;
			tm.tv_usec = 0;
			count = select(m_socket + 1, NULL, &write_fds, NULL, &tm);
		} while (count == 0);
		if (count < 0 || count > 1)
			throw TransportException("ERROR in TransportTCP1::SendExact()#1 %d", count);	// "socket error in select()"


	int bytes = ::send(m_socket, (char*)buff, bufflen, 0);
	if (bytes != bufflen) {
		throw TransportException("ERROR %d %d in TransportTCP1::SendFromQueue()#1", ::GetLastError(), bytes);
	}
}
*/


void TransportTCP1::SendExact(void *buff, UINT32 bufflen, BOOL block)
{
	struct fd_set write_fds;
	struct timeval tm;
	int count;

	RLMutexLock l(m_send_lock);

	// Put the data into the queue
	if (bufflen>0) 
		SendQueued(buff, bufflen);
		m_bytesSent += bufflen;

	if (!block) return;

	while (m_send_queue.GetDataLen()>0) {
		// Wait until some data can be sent
		do {
			FD_ZERO(&write_fds);
			FD_SET((SOCKET)m_socket, &write_fds);
			tm.tv_sec = 1;
			tm.tv_usec = 0;
			count = select(m_socket + 1, NULL, &write_fds, NULL, &tm);
		} while (count == 0);
		if (count < 0 || count > 1)
			throw TransportException("ERROR in TransportTCP1::SendExact()#1 %d", count);	// "socket error in select()"
		
		// Actually send some data
		if (FD_ISSET((SOCKET)m_socket, &write_fds))
			SendFromQueue();
    }	
}


void TransportTCP1::SendQueued(const void *buff, const UINT bufflen)
{
	char* pBufferSrc = (char*)buff;

	for (UINT count=bufflen; count>0;)
	{
		char* pBuffer = NULL;
		DWORD c = m_send_queue.PushDataEmpty(count, &pBuffer);

		memcpy(pBuffer, pBufferSrc, c);

		if (m_encryptor_on) m_encryptor.Encrypt((UINT8*)pBuffer, c);

		count      -= c;
		pBufferSrc += c;
	}

	//_log.WriteInfo("SendQueued() %d %d", bufflen, m_queue.GetDataLen());
}



void TransportTCP1::SendFromQueue()
{
	// Maximum data size to send at once
	int portion_size = m_send_queue.GetDataLen();
	if (portion_size > 32768) portion_size = 32768;
	
	if (portion_size==0) return; // no data for sending

	m_buffer.SetMinCapasity(portion_size);
	char* pBuffer = (char*)m_buffer.GetBuffer();

	m_send_queue.PeekData(pBuffer, portion_size);

	// Try to send some data
	int bytes = ::send(m_socket, pBuffer, portion_size, 0);
	if (bytes > 0) {
		m_send_queue.ReadSkip(bytes);
	}
	else {
		DWORD dwError = ::WSAGetLastError();

		if (bytes == 0 || dwError != WSAEWOULDBLOCK)
			throw TransportException("ERROR %d %d in TransportTCP1::SendFromQueue()#1", dwError, bytes);
	}
}



void TransportTCP1::ReadExact(void* buf, UINT32 len)
{
	if (len==0) return;

	SOCKET socket = m_socket; // copy it, in case this object was closed and deleted while we're waiting
	char*  pBuffer = (char*)buf;
	UINT32 currlen = len;

	struct timeval tm;
	tm.tv_sec = 0;
	tm.tv_usec = 50;

	while(true)
	{
		struct fd_set read_fds;
		struct fd_set write_fds;

		// Wait until some data can be read or sent
		while(true) {
			FD_ZERO(&read_fds);
			FD_SET(socket, &read_fds);
			FD_ZERO(&write_fds);
			if (m_send_queue.GetDataLen()>0)
				FD_SET(socket, &write_fds);
			int count = select(socket + 1, &read_fds, &write_fds, NULL, &tm);
			if (count < 0 || count > 2)
				throw TransportException("ERROR in TransportTCP1::ReadExact()#1 %d", count);	// "socket error in select()"
			
			if (count>0) break;
		};

		if (FD_ISSET(socket, &read_fds)) {
			// Try to read some data in
			int bytes = ::recv(socket, pBuffer, currlen, 0);
			if (bytes > 0) {
				// Adjust the buffer position and size
				pBuffer += bytes;
				currlen -= bytes;
				
				if (currlen==0) break; //we've got what we need
			} else {
				DWORD dwError = ::WSAGetLastError();

				if (bytes == 0 || dwError != WSAEWOULDBLOCK)
					throw TransportException("ERROR %d %d in TransportTCP1::ReadExact()#2", dwError, bytes);
			}
		}
		if (FD_ISSET(m_socket, &write_fds)) {
			// Try to send some data, while waiting income data
			if (m_send_lock.trylock()) {
				try {
					SendFromQueue();
				}
				catch(RLException&) {
					m_send_lock.unlock();
					throw;
				}
				m_send_lock.unlock();
			}
		}		
    }

	m_bytesRead += len;

	if (m_decryptor_on) m_decryptor.Decrypt((UINT8*)buf, len);
}


// just read without sending
//
void TransportTCP2::ReadExact(void *buf, UINT32 len)
{
	if (len==0) return;

	SOCKET socket = m_socket; // copy it, in case this object was closed and deleted while we're waiting
	char*  pBuffer = (char*)buf;
	UINT32 currlen = len;

	while(true)
	{		
		struct fd_set read_fds;

		// Wait until some data can be read or sent
		{
			FD_ZERO(&read_fds);
			FD_SET(socket, &read_fds);
			int count = select(socket + 1, &read_fds, NULL, NULL, NULL);
			if (count <= 0 || count > 2)
				throw TransportException("ERROR %d in TransportTCP2::ReadExact()#1 %d", ::GetLastError(), count);
		}

		if (FD_ISSET(socket, &read_fds)) {
			// Try to read some data in
			int bytes = ::recv(socket, pBuffer, currlen, 0);
			if (bytes > 0) {
				// Adjust the buffer position and size
				pBuffer += bytes;
				currlen -= bytes;
				
				if (currlen==0) break; //we've got what we need
			} else {
				DWORD dwError = ::WSAGetLastError();

				if (bytes == 0 || dwError != WSAEWOULDBLOCK)
					throw TransportException("ERROR %d %d in TransportTCP2::ReadExact()#2", dwError, bytes);
			}
		}
    }

	m_bytesRead += len;

	if (m_decryptor_on) m_decryptor.Decrypt((UINT8*)buf, len);
}
