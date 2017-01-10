#include "stdafx.h"
#include "RLTimer.h"


// Unix    timer: 2^31 seconds = 67 years, 1970+67 = 2037 year
// Windows timer: 2^32 seconds * (2^32/m_frequency (3579545 in my laptop)) = 162966 years, it's from windows started
 

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#ifndef _WIN32
#include <sys/time.h>
#endif

__int64 RLTimer::m_frequency = RLTimer::Init();

__int64 RLTimer::Init()
{
#ifdef _WIN32
	VERIFY(::QueryPerformanceFrequency((LARGE_INTEGER*)&m_frequency)!=0);
#else
	m_frequency = 1000000;
#endif
	return m_frequency;	
}

inline void RLTimer::GetTicks(__int64& ticks)
{
#ifdef _WIN32
	VERIFY(::QueryPerformanceCounter((LARGE_INTEGER*)&ticks)!=0);
#else
	struct timeval tv;
	VERIFY(::gettimeofday(&tv, NULL) == 0);
	ticks = ((__int64)tv.tv_sec)*1000000 + tv.tv_usec;	
#endif
}

RLTimer::RLTimer(bool start)
{ 
	if (start) Start(); else m_tStart = 0;
}

void RLTimer::Start()
{
	//HANDLE hCurThread = ::GetCurrentThread(); 
    //DWORD_PTR dwOldMask = ::SetThreadAffinityMask(hCurThread, 1); 

	GetTicks(m_tStart);

	//::SetThreadAffinityMask(hCurThread, dwOldMask);
}

void RLTimer::Stop()
{
	GetTicks(m_tTime);
	m_tTime -= m_tStart;
}


double RLTimer::GetElapsedSeconds()
{
	Stop();
	return ((double)m_tTime/m_frequency);
}

bool RLTimer::IsStarted()
{
	return (m_tStart!=0);
}

bool RLTimer::IsElapsedOrNotStarted(int ms)
{
	if (m_tStart==0) return true; // not started
	Stop();
	return (((m_tTime*1000)/m_frequency)>=ms);
}


double RLTimer::GetTime() //return count miliseconds 
{
	return ((double)(m_tTime*1000)/m_frequency);
}

