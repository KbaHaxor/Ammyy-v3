#include "stdafx.h"
#include "AudioOut.h"
#include <process.h>
#include <stdlib.h>
#include <algorithm>
#include "../aaProtocol.h"
#include "../Common.h"
#include <dsound.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

const SOUND_BUFFER_LEN = 2000;		// Buffer length (in ms)
//const MAX_THREAD_TIMEOUT = 3000;	// Max wait time to stop thread in "soft" way. (via m_bStopThread flag). Then thread will be terminated.


AudioOut::AudioOut()
{	
	m_transport = NULL;
	m_hOwnerWnd = NULL;

	m_hSoundThread = NULL;
	m_bStopThread = FALSE;

	m_samplesPerSecond = 16000;
	m_bitsPerSample = 16;
	m_numChannels	= 1;

	m_pDSoundMng = NULL;
	m_pDSoundBuf = NULL;
	m_bufferSize = m_nextWritePos = 0;

	m_speexDecoder = NULL;
	m_speexFrameSize = 0;
	m_speexBuffer = NULL;
	m_frameSizeBytes = 0;
	m_autoStart = false;
}

AudioOut::~AudioOut()
{
	if (m_hSoundThread)
		StopSound();
}

BOOL AudioOut::StartSound()
{	
	try 
	{
		if (m_hSoundThread) 
			throw RLException("AudioOut::StartSound() Sound Playback thread is already active");

		InitDirectSound();

		InitSpeexDecoder(SPEEX_MODEID_NB, TRUE);


		if ((HANDLE)m_eventNewData==NULL)
			m_eventNewData.Create();

		// Start Sound Playback thread
		m_bStopThread = FALSE;
		m_hSoundThread = (HANDLE)_beginthreadex(NULL, 0, SoundThreadProcStatic, this, 0, NULL);
		if (m_hSoundThread == 0)
			throw RLException("Failed to start Sound Playback thread (errno = %d)", errno);

		return TRUE;
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
		StopSound();
		return FALSE;
	}	
}


void AudioOut::StopSound()
{
	if (CCommon::IsThreadWorking(m_hSoundThread))
	{
		// Sound Playback thread is working
		_log.WriteInfo("AudioOut::Stop(): use safe way to stop thread");
		m_bStopThread = TRUE;
		m_eventNewData.Set();
		
		// We have to wait some time to give a chance for Capture Sound Thread to exit
		::WaitForSingleObject(m_hSoundThread, INFINITE /*MAX_THREAD_TIMEOUT*/);

		// Hard way has been disabled by Maxim's request.				
		//if (CCommon::IsThreadWorking(m_hSoundThread))
		//{
		//	// Thread still alive, use hard way to stop thread.
		//	_log.WriteInfo("AudioOut::Stop(): thread does not stopped in safe mode, use hard way");			
		//	m_pDSoundBuf->Stop();	// stop sound playback explicitly
		//	::TerminateThread(m_hSoundThread, 1);
		//}
		
		_log.WriteInfo("AudioOut::Stop(): thread stopped");
	}

	// clears m_bufferEncoded array
	{		
		RLMutexLock l(m_bufferEncodedLock);
		while (!m_bufferEncoded.empty())
		{
			delete m_bufferEncoded.front();
			m_bufferEncoded.pop();
		}
	}


	if (m_hSoundThread) {
		VERIFY(::CloseHandle(m_hSoundThread)!=0);
		m_hSoundThread = NULL;
	}
	
	m_bStopThread = FALSE;
	m_autoStart   = false;
	
	SAFE_RELEASE(m_pDSoundBuf);
	SAFE_RELEASE(m_pDSoundMng);
	m_nextWritePos = 0;

	DoneSpeexDecoder();
}



void AudioOut::OnData(UINT16 length)
{
	_log.Print(LL_SOUND, VTCLOG("OnData() %u"), length);

	if (m_autoStart && m_hSoundThread==NULL) StartSound();

	RLStream* pBuffer = new RLStream(length);

	try {
		m_transport->ReadExact(pBuffer->GetBuffer(), length);
	}
	catch(RLException&) {
		delete pBuffer;
		throw;
	}

	pBuffer->SetLen(length);

	if (m_speexFrameSize!=0) {
		// TODO: fix it, we need also compressed bitrate, its a 8Kbit/sec
		ASSERT(((float)m_speexFrameSize / 8.0) == (float) length);

		{
			RLMutexLock l(m_bufferEncodedLock);
			m_bufferEncoded.push(pBuffer);
		}
		m_eventNewData.Set(); // working thread should wake up and decode data
	}
	else {
		delete pBuffer; // this object is stopped
	}
}

void AudioOut::InitDirectSound()
{
	if (m_hSoundThread)
		throw RLException("InitDirectSound: Sound Playback thread is already active");	

	SAFE_RELEASE(m_pDSoundBuf);
	SAFE_RELEASE(m_pDSoundMng);
	
	try 
	{
		HRESULT hr;

		// Create IDirectSound using the specified device
		LPCGUID lpcDeviceGuid = CCommon::GetLPGUID(&settings.m_audioDevicePlay); 
		if ( FAILED( hr = DirectSoundCreate( lpcDeviceGuid, &m_pDSoundMng, NULL )))
			throw RLException("DirectSoundCreate error: HRESULT = 0x%x", hr);		


		HWND ownerWnd = m_hOwnerWnd;

		// DirectSound doesn't work if m_hOwnerWnd not provided
		if ( FAILED( hr = m_pDSoundMng->SetCooperativeLevel(m_hOwnerWnd, DSSCL_PRIORITY)))
			throw RLException("SetCooperativeLevel() error: HRESULT = 0x%x", hr);

		// create sound buffer
		{
			ASSERT(m_pDSoundMng && !m_pDSoundBuf);
			ASSERT(m_bitsPerSample==16 || m_bitsPerSample==8);

			// Setup sound format
			WAVEFORMATEX	wfx = {0};
			wfx.wFormatTag		= WAVE_FORMAT_PCM;
			wfx.nChannels		= m_numChannels;
			wfx.nSamplesPerSec	= m_samplesPerSecond;
			wfx.wBitsPerSample	= m_bitsPerSample;
			wfx.nBlockAlign		= m_numChannels * (m_bitsPerSample / 8);
			wfx.nAvgBytesPerSec	= m_samplesPerSecond * wfx.nBlockAlign;

			DSBUFFERDESC	desc = {0};
			desc.dwSize			= sizeof(DSBUFFERDESC1);
			desc.dwFlags		= DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
			desc.dwBufferBytes  = SOUND_BUFFER_LEN * wfx.nAvgBytesPerSec / 1000;
			desc.lpwfxFormat	= &wfx;
			desc.guid3DAlgorithm= GUID_NULL;

			// create secondary buffer
			m_bufferSize = desc.dwBufferBytes;
			m_nextWritePos = 0;

			if ( FAILED( hr = m_pDSoundMng->CreateSoundBuffer( &desc, &m_pDSoundBuf, NULL )))
				throw RLException("CreateSoundBuffer error: HRESULT = 0x%x", hr);
		}
	} 
	catch(RLException&) {
		// Failed to create one of components for Sound Capture
		SAFE_RELEASE(m_pDSoundBuf);
		SAFE_RELEASE(m_pDSoundMng);
		m_bufferSize = 0;
		throw;
	}
}


void AudioOut::InitSpeexDecoder(int modeID, int isEnhanced)
{
	ASSERT(m_speexDecoder == NULL);

	SpeexMode*	mode = speex_lib_get_mode(modeID);
	if (!mode)
		mode = &speex_nb_mode;	// default mode

	// Initialize Speex decoder
	m_speexDecoder = speex_decoder_init(mode);
	if (!m_speexDecoder)
		throw RLException("speex_decoder_init error");

	// setup decoder
	speex_decoder_ctl(m_speexDecoder, SPEEX_GET_FRAME_SIZE, &m_speexFrameSize);
	speex_decoder_ctl(m_speexDecoder, SPEEX_SET_ENH, &isEnhanced);

	speex_bits_init(&m_speexBits);
	m_speexBuffer = new float[m_speexFrameSize * m_numChannels];
	m_frameSizeBytes = m_speexFrameSize * (m_bitsPerSample / 8) * m_numChannels;
}


void AudioOut::DoneSpeexDecoder()
{
	// Release encoder resources
	if (m_speexDecoder)
	{
		speex_decoder_destroy(m_speexDecoder);
		m_speexDecoder = NULL;
		speex_bits_destroy(&m_speexBits);
		m_speexFrameSize = 0;
		delete m_speexBuffer;
	}
}


// Entry point for the "Sound Playback" thread
UINT __stdcall AudioOut::SoundThreadProcStatic(void* param)
{
	ASSERT(param != NULL);
	
	_log.WriteInfo("Sound Playback Thread is started (param = 0x%x)", param);
	
	UINT exitCode = ((AudioOut*) param)->SoundThreadProc();
	
	_log.WriteInfo("Sound Playback Thread exiting (exitCode = %u).", exitCode);
	return exitCode;
}


// Real entry point for the "Sound Playback" thread.
UINT AudioOut::SoundThreadProc()
{
	HRESULT		hr;
	
	// Start sound playback
	const UINT	framePeriod = m_speexFrameSize * 1000 / m_samplesPerSecond;

	DWORD		startTime = GetTickCount();	// Capturing start timestamp
	bool playing = false;

	while (!m_bStopThread)
	{
		DWORD	res = ::WaitForSingleObject( (HANDLE)m_eventNewData, framePeriod / 4);
		if (res == WAIT_OBJECT_0)
		{
			if (!ProcessData())
			{
				// TODO: Notify somehow main window about early exiting...
				break;
			}
		}

		if (FAILED(hr = PlayData(playing)))
		{
			if (hr != DSERR_BUFFERLOST) break;		// stop playback
			if (!RestoreSoundBuffer()) continue;	// repeat again
		}

		if (!playing)
		{
			_log.Print(LL_SOUND, VTCLOG("Starting..."));

			if ( SUCCEEDED( hr = m_pDSoundBuf->Play(0, 0, DSBPLAY_LOOPING) ) )
				playing = true;
			else
			{
				if (hr == DSERR_BUFFERLOST)
					RestoreSoundBuffer();
				else
				{
					_log.WriteError("Play error: HRESULT = 0x%x", hr);
					break;
				}
			}
		}
	}
	
	// If we've decompressed audio, lets finish playback
	while (m_bufferRaw.HasData())
	{
		if (FAILED( PlayData(playing) )) break;
	}

	
	if ( FAILED(hr = m_pDSoundBuf->Stop() ) )
		_log.WriteError("IDirectSoundBuffer::Stop() error: HRESULT = 0x%x", hr);

	
	return 0;
}


BOOL AudioOut::ProcessData()
{
	BOOL		bSuccess = TRUE;
	RLStream*	list[20];
	BOOL		hasMoreData;

	do
	{
		// Process up to 20 blocks at once
		size_t	size;
		{
			RLMutexLock l(m_bufferEncodedLock);
			size = m_bufferEncoded.size();
			size = min(size, COUNTOF(list));
			
			for (size_t i = 0; i < size; ++i)
			{
				list[i] = m_bufferEncoded.front();
				m_bufferEncoded.pop();
			}

			hasMoreData = m_bufferEncoded.size();
		}

		for (size_t i = 0; i < size; ++i)
		{
			// prepare for decoding
			speex_bits_read_from(&m_speexBits, (char*) list[i]->GetBuffer(), list[i]->GetLen());
			
			// decode frame
			int res = speex_decode(m_speexDecoder, &m_speexBits, m_speexBuffer);
			if (res != 0)
			{
				_log.WriteError("ERROR in speex_decode: returned value %d", res);
				bSuccess = FALSE;
			}
			else
			{
				AddSpeexFrame();
			}
			
			
			delete list[i];
		}
	}
	while (hasMoreData);

	return bSuccess;
}


void AudioOut::AddSpeexFrame()
{
	int		size = m_speexFrameSize * m_numChannels * (m_bitsPerSample / 8);
	float*	input = m_speexBuffer;

	if (m_bitsPerSample == 16)
	{
		while (size > 0)
		{
			short*	output;
			DWORD	chunkLen = m_bufferRaw.PushDataEmpty(size, (char**) &output);
			if (chunkLen % sizeof(short))
			{
				ASSERT(!"Internal error. Buffer should be 2-bytes aligned");
			}

			for (int cnt = chunkLen / sizeof(short); cnt > 0; --cnt)
			{
				float	val = *(input++);
				int		res;
				if (val > 32767.f)
					res = 32767;
				else if (val < -32768.f)
					res = -32768;
				else
					res = (short) (val + 0.5f);

				*(output++) = res;
			}

			size -= chunkLen;
		}
	}
	else
	{
		ASSERT(m_bitsPerSample == 8);
		while (size > 0)
		{
			char*	output;
			DWORD	chunkLen = m_bufferRaw.PushDataEmpty(size, &output);
			
			for (int cnt = chunkLen; cnt > 0; --cnt)
			{
				float	val = *(input++);
				int		res;
				if (val > 32767.f)
					res = 32767;
				else if (val < -32768.f)
					res = -32768;
				else
					res = (short) (val + 0.5f);

				*(output++) = (char) (((res ^ 0x8000)) >> 8);
			}
			
			size -= chunkLen;
		}
	}
}


inline void AudioOut::FillSilence(LPBYTE buffer, int count, int blank)
{
	memset(buffer, blank, count);

	// noise for testing
	//for (int i=0; i<count; i++) ((char*)buffer)[i] = rand()/128;

	_log.Print(LL_SOUND, VTCLOG("FillSilence %X %u %u"), buffer, buffer, count);
}


HRESULT AudioOut::PlayData(bool playing)
{
	// If we've free area in sound buffer, put next portion of sounds
	HRESULT hr;
	DWORD	dwPlayPos;
	DWORD	dwWritePos;
	
	if (FAILED( hr = m_pDSoundBuf->GetCurrentPosition(&dwPlayPos, &dwWritePos) ) )
	{
		_log.WriteError("GetCurrentPosition error: HRESULT = 0x%x", hr);
		return hr;
	}

	int	delta = m_nextWritePos - dwWritePos;

	if (delta<0 && delta<-(m_bufferSize/2)) delta += m_bufferSize;
	else if       (delta>+(m_bufferSize/2)) delta -= m_bufferSize;

	_log.Print(LL_SOUND, VTCLOG("PlayData() %u %u %d"), dwWritePos, m_nextWritePos, delta);

	if (delta<0) {
		m_nextWritePos = dwWritePos;

		int d = m_nextWritePos % m_frameSizeBytes;

		if (d!=0) {
			m_nextWritePos += (m_frameSizeBytes-d);
			m_nextWritePos %= m_bufferSize;
		}

		_log.Print(LL_SOUND, VTCLOG("PlayData()#3 overrun occur, m_nextWritePos=%u"), m_nextWritePos);
	}

	
	void*	lpData1; 
	void*	lpData2;
	DWORD	dwLen1;
	DWORD	dwLen2;

	int countData = m_bufferRaw.GetDataLen();
	
	if (countData > 0)	// we've data
	{
		if ((delta>=0 && delta < m_bufferSize/4) || (delta<0)) {
			countData = min(countData, m_bufferSize/4);
		}
		else
			return S_OK; // we have enough data on Sound Buffer
		
		_log.Print(LL_SOUND, VTCLOG("Push audio data %d bytes"), countData);

		hr = m_pDSoundBuf->Lock(m_nextWritePos, countData, &lpData1, &dwLen1, &lpData2, &dwLen2, 0);
		if (FAILED(hr))
		{
			_log.WriteError("Lock error: HRESULT = 0x%x", hr);
			return hr;
		}


		_log.Print(LL_SOUND, VTCLOG("PlayData()#1 %u"), countData);

		ASSERT((m_bufferRaw.GetDataLen() % m_frameSizeBytes) == 0);
		
		dwLen1 = m_bufferRaw.TakeData((char*) lpData1, dwLen1);
		if (dwLen2)
			dwLen2 = m_bufferRaw.TakeData((char*) lpData2, dwLen2);
		
		m_nextWritePos += (dwLen1 + dwLen2);
		m_nextWritePos %= m_bufferSize;

		hr = m_pDSoundBuf->Unlock(lpData1, dwLen1, lpData2, dwLen2);
		if (FAILED(hr))
		{
			_log.WriteError("Unlock error: HRESULT = 0x%x", hr);
			return hr;
		}
	}
	else
	{
		// There is no new audio data. If there is nothing to play, put blank frame
		bool needSilence = playing && (delta>=0 && delta < m_frameSizeBytes) || (delta<0);
		
		if (needSilence)
		{
			// Fill in silence next 2 frames but move forward for 1 frame only
			UINT blankAreaSize = m_frameSizeBytes*2;
			int	 blank = ((m_bitsPerSample == 8) ? 128 : 0);

			hr = m_pDSoundBuf->Lock(m_nextWritePos, blankAreaSize, &lpData1, &dwLen1, &lpData2, &dwLen2, 0);
			if (FAILED(hr))
			{
				_log.WriteError("Lock error: HRESULT = 0x%x", hr);
				return hr;
			}			
			
			dwLen1 = min(blankAreaSize, dwLen1);
			FillSilence((LPBYTE)lpData1, dwLen1, blank);

			if (dwLen2)
			{
				if (blankAreaSize <= dwLen1)
					dwLen2 = 0;
				else
				{
					dwLen2 = blankAreaSize - dwLen1;
					FillSilence((LPBYTE)lpData2, dwLen2, blank);
				}
			}

			hr = m_pDSoundBuf->Unlock(lpData1, dwLen1, lpData2, dwLen2);
			if (FAILED(hr))
			{
				_log.WriteError("Unlock error: HRESULT = 0x%x", hr);
				return hr;
			}			
			
			m_nextWritePos += m_frameSizeBytes;
			m_nextWritePos %= m_bufferSize;
		}
	}
	
	return S_OK;
}


// If current sound buffer is lost, function restores the lost buffer.
BOOL AudioOut::RestoreSoundBuffer()
{
	ASSERT(m_pDSoundBuf);

    HRESULT	hr;
    DWORD	dwStatus;

    if ( FAILED( hr = m_pDSoundBuf->GetStatus( &dwStatus ) ) )
	{
		_log.WriteError("IDirectSoundBuffer::GetStatus() error: HRESULT = 0x%x", hr);
		return FALSE;
	}	
	
	if (dwStatus & DSBSTATUS_BUFFERLOST)
    {
		_log.WriteInfo("AudioOut lost sound buffer. Try to restore...\n");

        // If app just been activated, it could take sometime to restore buffer.
		hr = m_pDSoundBuf->Restore();
		if (hr != DS_OK)
		{
			if (hr != DSERR_BUFFERLOST)
				_log.WriteError("IDirectSoundBuffer::Restore() error: HRESULT = 0x%x", hr);

			return FALSE;
		}

		// Buffer is restored. Restart it.
		m_nextWritePos = 0;
    }

	return TRUE;
}
