#if !defined(AFX_AUDIOIN_H__04790728_0D5E_4A5A_87AD_9F33D6EF5440__INCLUDED_)
#define AFX_AUDIOIN_H__04790728_0D5E_4A5A_87AD_9F33D6EF5440__INCLUDED_

#include <dsound.h>
#include "speex/speex.h"
#include "../Transport.h"

class AudioIn  
{
public:
	AudioIn();
	~AudioIn();

	void StartSound(DWORD samplesPerSecond);
	void StopSound();

	// calling before close m_transport, to avoid writting message to log "transport error"
	void SetStoppingStatus() { m_bStopThread = TRUE; } 

private:
	void InitDirectSoundCapture();
	HRESULT CreateCaptureBuffer(DWORD milliseconds);
	HRESULT InitCaptureNotifications(DWORD frameLen);

	void InitSpeexEncoder(int modeID, int complexity, int dest_rate);
	void DoneSpeexEncoder();

	//void SendAudioData(LPCSTR data, int len);

	// Real entry point for the "Sound Capture" thread
	UINT CaptureThreadProc();

	// Process captured sound
	BOOL ProcessCapturedData();
	BOOL GetCapturedData();
	void ConvertFrameToSpeexFormat(LPCSTR ptr, UINT blockSize);

	// !Do not use directly! Only as a argument for _beginthread().
	// Static function-wrapper used as a start point of "Sound Capture" thread.
	// @param - pointer to its AudioIn object (this).
	// Remember: "Sound Capture" thread owned by AudioIn object, so if AudioIn
	// object is deleted, related thread will be automatically stopped also.
	static UINT __stdcall CaptureThreadDelegate(void* param);

public:
	// Next members should be changed before Start().
	DWORD						m_samplesPerSecond;
	WORD						m_bitsPerSample;
	WORD						m_numChannels;

protected:
	DWORD						m_frameSize;		// size of sound frame in bytes
	RLStream					m_rawBuf;			// Source buffer with raw sound samples
	RLStream					m_encodedBuf;		// Buffer with encoded sound samples

	LPDIRECTSOUNDCAPTURE		m_pDSCapture;
	LPDIRECTSOUNDCAPTUREBUFFER	m_pDSCaptureBuf;
	DWORD						m_bufferSize;
	DWORD						m_bufferPos;		// current position in buffer
	HANDLE						m_hCaptureEvent;	// signaled when next portion of data captured

	// Variables for Speex encoder
	void*						m_speexEncoder;
	SpeexBits					m_speexBits;
	int							m_speexFrameSize;	// Frame size for Speex encoder (in samples)
	float*						m_speexBuffer;		// Work buffer for Speex encoder.
													// Could be also void* array.
													

	// Handle of Sound Capture thread. Not NULL if thread exists.
	volatile HANDLE				m_hCaptureThread;
	// Asynchronous stop request flag. This flag used as a asynchronous stop signal 
	// for Sound Capture thread.
	volatile BOOL				m_bStopThread;


public:
	Transport*				m_transport;
	GUID					m_deviceGuid;

private:
	RLMutex					m_mutexStartStop;
};

#endif // !defined(AFX_AUDIOIN_H__04790728_0D5E_4A5A_87AD_9F33D6EF5440__INCLUDED_)
