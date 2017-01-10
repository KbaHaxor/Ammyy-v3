#if !defined(AFX_AUDIOOUT_H__8F075B22_8CBA_4687_94DB_C8581F4FEF58__INCLUDED_)
#define AFX_AUDIOOUT_H__8F075B22_8CBA_4687_94DB_C8581F4FEF58__INCLUDED_

#include "../FastQueue.h"
#include "../../RL/RLLock.h"
#include "../../RL/RLEvent.h"
#include <queue>
#include <dsound.h>
#include "speex/speex.h"
#include "../Transport.h"

class AudioOut  
{
public:
	AudioOut();
	~AudioOut();
	void OnData(UINT16 length);	
	void StopSound();

	void PreStart(UINT samplesPerSecond)
	{
		m_samplesPerSecond = samplesPerSecond;
		m_autoStart = true;
	}

private:
	BOOL StartSound();
	inline void FillSilence(LPBYTE buffer, int count, int blank);
	void InitDirectSound();
	BOOL RestoreSoundBuffer();

	inline void InitSpeexDecoder(int modeID, int isEnhanced);
	inline void DoneSpeexDecoder();

	// Process received data
	BOOL ProcessData();
	void AddSpeexFrame();
	HRESULT PlayData(bool playing);


	// Real entry point for the "Sound Playback" thread
	UINT SoundThreadProc();


	// !Do not use directly! Only as a argument for _beginthread().
	// Static function-wrapper used as a start point of "Sound Playback" thread.
	// @param - pointer to its AudioOut object (this).
	// Remember: "Sound Playback" thread owned by AudioOut object, so if AudioOut
	// object is deleted, related thread will be automatically stopped also.
	static UINT __stdcall SoundThreadProcStatic(void* param);

public:
	Transport*				m_transport;
	HWND					m_hOwnerWnd;		// owner window (to init direct sound)

private:
	bool					m_autoStart;
	UINT					m_samplesPerSecond;
	WORD					m_bitsPerSample;	// currently, its a constants
	WORD					m_numChannels;

	LPDIRECTSOUND			m_pDSoundMng;
	LPDIRECTSOUNDBUFFER		m_pDSoundBuf;
	DWORD					m_nextWritePos;		// next write position in buffer
	DWORD					m_bufferSize;
	UINT					m_frameSizeBytes;

	CFastQueue				m_bufferRaw;
	std::queue<RLStream*>	m_bufferEncoded;
	RLMutex					m_bufferEncodedLock;
	RLEvent				    m_eventNewData;

	// Variables for Speex decoder
	void*					m_speexDecoder;
	SpeexBits				m_speexBits;
	int						m_speexFrameSize;	// Frame size for Speex decoder (in samples)
	float*					m_speexBuffer;		// Work buffer for Speex decoder (1 frame).


	// Handle of Sound Playback thread. Not NULL if thread exists.
	volatile HANDLE	m_hSoundThread;	
	// Asynchronous stop request flag. This flag used as a asynchronous stop signal 
	// for Sound Playback thread.
	volatile BOOL			m_bStopThread;
};

#endif // !defined(AFX_AUDIOOUT_H__8F075B22_8CBA_4687_94DB_C8581F4FEF58__INCLUDED_)
