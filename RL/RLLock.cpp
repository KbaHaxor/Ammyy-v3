#include "stdafx.h"
#include "RLLock.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#ifndef _WIN32
pthread_mutexattr_t		RLMutex::m_attrs;
bool					RLMutex::m_init = false;
#endif

RLMutex::RLMutex()
{
#ifdef _WIN32
	::InitializeCriticalSection(&m_crit);
#else
	if (!m_init) {
		m_init = true;
		::pthread_mutexattr_init(&m_attrs);
		::pthread_mutexattr_settype(&m_attrs, PTHREAD_MUTEX_RECURSIVE);
	}

	::pthread_mutex_init(&m_crit, &m_attrs);
#endif
}


RLMutex::~RLMutex()
{
#ifdef _WIN32
	::DeleteCriticalSection(&m_crit);
#else
	::pthread_mutex_destroy(&m_crit);
#endif
}


void RLMutex::lock()
{
#ifdef _WIN32
	::EnterCriticalSection(&m_crit);
#else
	::pthread_mutex_lock(&m_crit);
#endif
}


void RLMutex::unlock()
{
#ifdef _WIN32
	::LeaveCriticalSection(&m_crit);
#else
	::pthread_mutex_unlock(&m_crit);
#endif
}


bool RLMutex::trylock()
{
#ifdef _WIN32
	return (::TryEnterCriticalSection(&m_crit) != 0);
#else
	return (::pthread_mutex_trylock(&m_crit) == 0);
#endif
}


RLMutexLock::RLMutexLock(RLMutex& m): mutex(m) 
{ 
	mutex.lock();
}

RLMutexLock::~RLMutexLock()
{ 
	mutex.unlock();
}
