#if !defined(_RL_TIMER_H__INCLUDED_)
#define _RL_TIMER_H__INCLUDED_

class RLTimer  
{
public:
	RLTimer(bool start=true);

	void Start();
	void Stop();
	double GetElapsedSeconds();
	double GetTime();	//return count miliseconds 

	bool IsStarted();
	bool IsElapsedOrNotStarted(int ms);

	static __int64 Init();

//#ifndef _WIN32
private:
	inline void GetTicks(__int64& ticks);
//#endif

private:
	__int64 m_tStart;
	__int64 m_tTime;
	static __int64 m_frequency;
};

#endif // !defined(_RL_TIMER_H__INCLUDED_)
