#include "stdafx.h"
#include "AudioIn.h"
#include <process.h>
#include <dsound.h>
#include "../aaProtocol.h"
#include "../Common.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")



//const MAX_THREAD_TIMEOUT = 3000;	// Max wait time to stop thread in "soft" way. (via m_bStopThread flag). Then thread will be terminated.
const CAPTURE_BUFFER_LEN = 2000;	// Buffer length (in ms), multiply of SOUND_FRAME_LEN.
const SOUND_FRAME_LEN = 20;			// Sound frame size (in ms)


AudioIn::AudioIn()
{
	m_transport = NULL;
	m_samplesPerSecond = 0;
	m_bitsPerSample = 16;
	m_numChannels = 1;

	m_hCaptureThread = NULL;
	
	m_pDSCapture = NULL;
	m_pDSCaptureBuf = NULL;
	m_bufferSize = m_bufferPos = 0;
	m_hCaptureEvent = NULL;

	m_speexEncoder = NULL;
	m_speexFrameSize = 0;
	m_speexBuffer = NULL;
}


AudioIn::~AudioIn()
{
	if (m_hCaptureThread)
		StopSound();

	if (m_hCaptureEvent) ::CloseHandle(m_hCaptureEvent);
}


void AudioIn::StartSound(DWORD samplesPerSecond)
{
	RLMutexLock l(m_mutexStartStop);

	m_deviceGuid = settings.m_audioDeviceRecd; // need to copy it, case m_audioDeviceRecd can change to OFF until it will be used

	if (m_deviceGuid==Settings::GuidOFF) return; // is off

	_log.Print(LL_SOUND, VTCLOG("StartSound()#1"));

	if (m_hCaptureThread)
		throw RLException("AudioIn::Start: Sound Capture thread is already active");

	m_samplesPerSecond = samplesPerSecond;

	ASSERT((m_samplesPerSecond >= 1000) && (m_samplesPerSecond <= 48000));
	ASSERT((m_bitsPerSample == 8) || (m_bitsPerSample == 16));
	ASSERT((m_numChannels == 1) || (m_numChannels == 2));
	
	const DWORD	bytesPerSecond = ((m_bitsPerSample / 8) * m_numChannels) * m_samplesPerSecond;
	m_frameSize = (SOUND_FRAME_LEN * bytesPerSecond) / 1000;

	this->InitDirectSoundCapture();

	_log.Print(LL_SOUND, VTCLOG("StartSound()#2"));

	try {
		InitSpeexEncoder(SPEEX_MODEID_NB, 3, 8000);

		_log.Print(LL_SOUND, VTCLOG("StartSound()#3"));

		// Start Sound Capture thread
		m_bStopThread = FALSE;
		m_hCaptureThread = (HANDLE)_beginthreadex(NULL, 0, CaptureThreadDelegate, this, 0, NULL);
		if (m_hCaptureThread == NULL)
			throw RLException("Failed to start Sound Capture thread (errno = %d)", errno);
	}
	catch(RLException&) {
		StopSound();
		throw;
	}
}


void AudioIn::StopSound()
{
	RLMutexLock l(m_mutexStartStop);

	if (CCommon::IsThreadWorking(m_hCaptureThread))
	{
		// Capture Thread is working
		_log.WriteInfo("AudioIn::Stop(): use safe way to stop thread");
		m_bStopThread = TRUE;
		::SetEvent(m_hCaptureEvent);

		// We have to wait some time to give a chance for Capture Sound Thread to exit
		::WaitForSingleObject(m_hCaptureThread, INFINITE /*MAX_THREAD_TIMEOUT*/);

		// Hard way has been disabled by Maxim's request.
		//if (CCommon::IsThreadWorking(m_hCaptureThread))
		//{
		//	// Thread still alive, use hard way to stop thread.
		//	_log.WriteInfo(_T("AudioIn::Stop(): thread does not stopped in safe mode, use hard way"));
		//	m_pDSCaptureBuf->Stop();	// stop sound capture explicitly
		//	::TerminateThread(m_hCaptureThread, 1);
		//}

		_log.WriteInfo("AudioIn::Stop(): thread stopped");
	}

	if (m_hCaptureThread) {
		VERIFY(::CloseHandle(m_hCaptureThread)!=0);
		m_hCaptureThread = NULL;
	}

	SAFE_RELEASE(m_pDSCaptureBuf);
	SAFE_RELEASE(m_pDSCapture);
	m_bufferSize = m_bufferPos = 0;

	if (m_speexEncoder)
		DoneSpeexEncoder();
}



void AudioIn::InitDirectSoundCapture()
{
	if (m_hCaptureThread)
		throw RLException("InitDirectSoundCapture: Sound Capture thread is already active");

	SAFE_RELEASE(m_pDSCaptureBuf);
	SAFE_RELEASE(m_pDSCapture);

	try {
		HRESULT hr;

		// Create IDirectSoundCapture using the capture device
		LPCGUID lpcDeviceGuid = CCommon::GetLPGUID(&settings.m_audioDeviceRecd);
		if ( FAILED( hr = ::DirectSoundCaptureCreate( lpcDeviceGuid, &m_pDSCapture, NULL)))
			throw RLException("DirectSoundCaptureCreate error: HRESULT = 0x%x", hr);		

		if ( FAILED( hr = CreateCaptureBuffer( CAPTURE_BUFFER_LEN ) ) ) {			
			throw RLException("CreateCaptureBuffer error: HRESULT = 0x%x", hr);
		}

		// Setup capture notifications (and create Event object)
		if (!m_hCaptureEvent) {		
			m_hCaptureEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
			if (!m_hCaptureEvent)
				throw RLException("CreateEvent error: 0x%x", ::GetLastError());
		}

		if (FAILED( hr = InitCaptureNotifications( SOUND_FRAME_LEN )))
			throw RLException("InitCaptureNotifications error: HRESULT = 0x%x", hr);
	}
	catch(RLException&) {

		// Failed to create one of components for Sound Capture
		SAFE_RELEASE(m_pDSCaptureBuf);
		SAFE_RELEASE(m_pDSCapture);
		m_bufferSize = 0;

		throw;
	}
}


// Wrapper for IDirectSoundCapture::CreateCaptureBuffer
HRESULT AudioIn::CreateCaptureBuffer(DWORD milliseconds)
{
	ASSERT(m_pDSCapture && !m_pDSCaptureBuf);
	ASSERT((milliseconds > 0) && !(m_bitsPerSample & 0x07));

	const WORD		unitSize = (m_bitsPerSample / 8) * m_numChannels;

	WAVEFORMATEX	wfx;
	ZeroMemory(&wfx, sizeof(wfx));
	wfx.wFormatTag		= WAVE_FORMAT_PCM;
	wfx.nChannels		= m_numChannels;
	wfx.nSamplesPerSec	= m_samplesPerSecond;
	wfx.wBitsPerSample	= m_bitsPerSample;
	wfx.nBlockAlign		= unitSize;
	wfx.nAvgBytesPerSec	= m_samplesPerSecond * unitSize;

	DSCBUFFERDESC	desc; 
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize			= sizeof(DSCBUFFERDESC1);
	desc.dwBufferBytes  = (milliseconds / SOUND_FRAME_LEN) * m_frameSize;	// round to frame size
	desc.lpwfxFormat	= &wfx;

    // Create the capture buffer
	m_bufferSize = desc.dwBufferBytes;
	m_bufferPos = 0;
	return m_pDSCapture->CreateCaptureBuffer( &desc, &m_pDSCaptureBuf, NULL );
}


HRESULT AudioIn::InitCaptureNotifications(DWORD frameLen)
{
	ASSERT(m_pDSCaptureBuf && frameLen);

	HRESULT				hr;
	LPDIRECTSOUNDNOTIFY	pDSNotify;

    // Create a notification event, for when the sound stops playing
    if ( FAILED( hr = m_pDSCaptureBuf->QueryInterface( IID_IDirectSoundNotify, (void**) &pDSNotify ) ) )
		return hr;

	const DWORD	numFrames = m_bufferSize / m_frameSize;

	ASSERT(m_frameSize * numFrames == m_bufferSize);

	DSBPOSITIONNOTIFY*	aPosNotify = new DSBPOSITIONNOTIFY[numFrames];
	if (!aPosNotify)
	{
		SAFE_RELEASE(pDSNotify);
		return E_OUTOFMEMORY;
	}

    // Setup the notification positions
	ZeroMemory(aPosNotify, sizeof(DSBPOSITIONNOTIFY) * numFrames);
	DWORD	offset = m_frameSize - 1;	// 1st position
    for (DWORD i = 0; i < numFrames; ++i)
    {
        aPosNotify[i].dwOffset = offset;
        aPosNotify[i].hEventNotify = m_hCaptureEvent;
		offset += m_frameSize;
    }

	ASSERT((offset - m_frameSize) < m_bufferSize);
    
    // Tell DirectSound when to notify us. the notification will come in the from 
    // of signaled events that are handled in WinMain()
	hr = pDSNotify->SetNotificationPositions( numFrames, aPosNotify );

	delete[] aPosNotify;
	pDSNotify->Release();
	return hr;
}

/*
void AudioIn::SendAudioData(LPCSTR data, int len)
{
	_log.Print(LL_SOUND, VTCLOG("SendAudioData()#1"));

	static RLStream buffer;

	DWORD time1 = ::GetTickCount();

	while(len>0) {
		int c = min(len, 0xFFF0);
		buffer.Reset();
		buffer.SetMinCapasity(c+3);
		buffer.AddUINT8(aaSound);
		buffer.AddUINT16(c);
		buffer.AddRaw(data, c);
		BOOL block = (c>=len) ? TRUE : FALSE; // TRUE if the last message
		if (!Send(buffer, block)) return;
		len-=c;
		data+=c;
	}
}
*/


// Entry point for the "Sound Capture" thread
UINT __stdcall AudioIn::CaptureThreadDelegate(void* param)
{
	ASSERT(param != NULL);

	_log.WriteInfo("Sound Capture Thread is started (param = 0x%x, id = 0x%x)", param, GetCurrentThreadId());

	UINT exitCode = ((AudioIn*) param)->CaptureThreadProc();

	_log.WriteInfo("Sound Capture Thread exiting (exitCode = %u).", exitCode);
	return exitCode;
}


// Real entry point for the "Sound Capture" thread.
UINT AudioIn::CaptureThreadProc()
{
	UINT		exitCode = 0;
	HRESULT		hr;

	// Start sound capture
	
	if ( FAILED(hr = m_pDSCaptureBuf->Start(DSCBSTART_LOOPING) ) )
	{
		_log.WriteError("IDirectSoundCaptureBuffer::Start() error: HRESULT = 0x%x", hr);
		// TODO: Notify somehow main window about early exiting...
		return 1;
	}
	
	//DWORD		startTime = GetTickCount();	// Capturing start timestamp

	_log.Print(LL_SOUND, VTCLOG("CaptureThreadProc()#1"));

	// Preallocated some memory
	m_rawBuf.SetMinCapasity(SOUND_FRAME_LEN * 5 * m_frameSize);
	m_encodedBuf.SetMinCapasity(2048);

	while(true)
	{
		if (m_bStopThread) break;
		DWORD res = ::WaitForSingleObject( m_hCaptureEvent, INFINITE);
		if (m_bStopThread) break;

		if (res == WAIT_OBJECT_0) {
			if (!ProcessCapturedData()) break;
		}
	}


	if ( FAILED(hr = m_pDSCaptureBuf->Stop() ) )
		_log.WriteError("IDirectSoundCaptureBuffer::Stop() error: HRESULT = 0x%x", hr);

	return exitCode;
}


BOOL AudioIn::ProcessCapturedData()
{
	_log.Print(LL_SOUND, VTCLOG("ProcessCapturedData()#1"));

	ASSERT(m_pDSCaptureBuf && m_hCaptureThread);

	//DWORD tm = ::GetTickCount();

	if (!GetCapturedData())		// extract from DirectSound buffer
		return FALSE;

	// Encode captured samples by Speex encoder
	UINT	size = m_rawBuf.GetAvailForReading();
	char*	ptr  = m_rawBuf.GetBufferRd();
	UINT	blockSize = m_speexFrameSize * (m_bitsPerSample / 8) * m_numChannels;
	while (size > 0)
	{
		if (blockSize > size)
			blockSize = size;

		ConvertFrameToSpeexFormat(ptr, blockSize);

		// Encode current frame
		speex_bits_reset(&m_speexBits);
		speex_encode(m_speexEncoder, m_speexBuffer, &m_speexBits);
		
		int	nbBytes = speex_bits_write(&m_speexBits, (char*) m_speexBuffer, m_speexFrameSize * m_numChannels * sizeof(float));
		
		// Put encoded data into m_encodedBuf (with block header)
		m_encodedBuf.SetMinExtraCapasity(nbBytes + 3);
		m_encodedBuf.AddUINT8(aaSound);
		m_encodedBuf.AddUINT16(nbBytes);
		m_encodedBuf.AddRaw((const void*) m_speexBuffer, nbBytes);

		size -= blockSize;
		ptr += blockSize;
	}

	_log.Print(LL_SOUND, VTCLOG("ProcessCapturedData()#2 %u"), m_encodedBuf.GetLen());

	// Its time to send all collected data
	try {
		m_transport->SendExact(m_encodedBuf.GetBuffer(), m_encodedBuf.GetLen(), TRUE);
	}
	catch(TransportException& ex) {
		if (m_bStopThread==FALSE) _log.WriteError(ex.GetDescription());
		return FALSE;
	}

	m_rawBuf.Reset();
	m_encodedBuf.Reset();


	//tm = ::GetTickCount() - tm;

	//if (time1 > 2000) PROBLEM HERE NEED TO WRITE TO LOG
	return TRUE;
}


BOOL AudioIn::GetCapturedData()
{
	HRESULT hr;
	DWORD	dwReadPos;
	DWORD	dwCapturePos;

	if (FAILED( hr = m_pDSCaptureBuf->GetCurrentPosition(&dwCapturePos, &dwReadPos) ) )
	{
		_log.WriteError("GetCurrentPosition error: HRESULT = 0x%x", hr);
		return FALSE;
	}

	LONG	dataSize = dwReadPos - m_bufferPos;
	if (dataSize < 0)
		dataSize += m_bufferSize;

	// Process full frames only 
	dataSize -= (dataSize % m_frameSize);

	if (dataSize > 0)
	{
		// Lock the capture buffer down
		void*	lpCaptureData	= NULL;
		DWORD	dwCaptureLen;
		void*	lpCaptureData2  = NULL;
		DWORD	dwCaptureLen2;
		hr = m_pDSCaptureBuf->Lock( m_bufferPos, dataSize, &lpCaptureData, &dwCaptureLen,
									&lpCaptureData2, &dwCaptureLen2, 0L );
		if (FAILED(hr))
		{
			_log.WriteError("AudioIn::Lock error: HRESULT = 0x%x", hr);
			return FALSE;
		}

		ASSERT((DWORD) dataSize == (dwCaptureLen + dwCaptureLen2));

		// Copy received data into working buffer for further processing
		m_rawBuf.AddRaw(lpCaptureData, dwCaptureLen);
		if (dwCaptureLen2)
			m_rawBuf.AddRaw(lpCaptureData2, dwCaptureLen2);

		// Move the capture offset along in circular buffer
		m_bufferPos = (m_bufferPos + dwCaptureLen + dwCaptureLen2) % m_bufferSize;

		// Unlock the capture buffer
		m_pDSCaptureBuf->Unlock( lpCaptureData,  dwCaptureLen, lpCaptureData2, dwCaptureLen2 );
	}

	return TRUE;
}

// Convert chunk of data (short[] or char[]) into speex format (float[]).
void AudioIn::ConvertFrameToSpeexFormat(LPCSTR ptr, UINT blockSize)
{
	ASSERT(ptr && (blockSize <= (UINT) (m_speexFrameSize * (m_bitsPerSample / 8) * m_numChannels)));

	if (m_bitsPerSample == 16)
	{
		ASSERT((blockSize & 1) == 0);	// should be even
		short*	src = (short*) ptr;
		int		len = blockSize / 2;
		for (int i = 0; i < len; ++i)
			m_speexBuffer[i] = (float) src[i];
	}
	else
	{
		ASSERT(m_bitsPerSample == 8);
		for (UINT i = 0; i < blockSize; ++i)
			m_speexBuffer[i] = (float) ((ptr[i] << 8) ^ 0x8000);
	}
}


void AudioIn::InitSpeexEncoder(int modeID, int complexity, int dest_rate)
{
	ASSERT(m_speexEncoder == NULL);

	SpeexMode*	mode = speex_lib_get_mode(modeID);
	if (!mode)
		mode = &speex_nb_mode;	// auto-assignment

	// Initialize Speex encoder
	m_speexEncoder = speex_encoder_init(mode);
	if (!m_speexEncoder)
		throw RLException("speex_encoder_init error");

	// setup encoder
	speex_encoder_ctl(m_speexEncoder, SPEEX_GET_FRAME_SIZE, &m_speexFrameSize);
	speex_encoder_ctl(m_speexEncoder, SPEEX_SET_COMPLEXITY, &complexity);
	speex_encoder_ctl(m_speexEncoder, SPEEX_SET_BITRATE, &dest_rate);
	speex_encoder_ctl(m_speexEncoder, SPEEX_SET_SAMPLING_RATE, &m_samplesPerSecond);

	speex_bits_init(&m_speexBits);
	m_speexBuffer = new float[m_speexFrameSize * m_numChannels];
}


void AudioIn::DoneSpeexEncoder()
{
	// Release encoder resources
	if (m_speexEncoder)
	{
		speex_encoder_destroy(m_speexEncoder);
		m_speexEncoder = NULL;
		speex_bits_destroy(&m_speexBits);
		m_speexFrameSize = 0;
		delete m_speexBuffer;
	}
}
