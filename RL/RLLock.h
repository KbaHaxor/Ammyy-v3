#if !defined(_RLLOCK_H___26B65807BD75__INCLUDED_)
#define _RLLOCK_H___26B65807BD75__INCLUDED_

#ifndef _WIN32
#include <pthread.h>
#endif

class RLMutex
{
public:
	RLMutex();
	~RLMutex();

	void lock();
	void unlock();
	bool trylock();

private:
#ifdef _WIN32
	CRITICAL_SECTION m_crit;	
#else
	pthread_mutex_t m_crit;
	static pthread_mutexattr_t m_attrs;
	static bool m_init;
#endif
};


class RLMutexLock
{
public:
	RLMutexLock(RLMutex& m);
	~RLMutexLock();

private:
	RLMutex& mutex;
};



#endif // !defined(_RLLOCK_H___26B65807BD75__INCLUDED_)
