#if !defined(_STREAM_WIT_TIME_H_50167DA223FE__INCLUDED_)
#define _STREAM_WIT_TIME_H_50167DA223FE__INCLUDED_

#include "../RL/RLFile.h"

class StreamWithTime
{
public:
	// reader & writter
	StreamWithTime();
	~StreamWithTime();
	void Close();

	// reader
	void Open  (LPCSTR fileName);
	void Read(LPVOID  buffer, UINT len);
	UINT64 GetTime();
	
	// writter
	void Create(LPCSTR fileName);
	void Push(LPCVOID buffer, UINT len);

private:
	#pragma pack(push, 1)
	struct TIMEITEM {
		UINT32 time;
		UINT32 bytes;
	};
	#pragma pack(pop)

	// both reader & writer
	RLFile		m_f1;
	RLFile		m_f2;

	// writer
	RLTimer   m_timer;
	UINT64	  m_last_time;  // in ms
	UINT32	  m_last_bytes;

	//reader
	UINT64	  m_bytes;
	UINT64    m_time;    // in ms
	TIMEITEM  m_nextItem;
	bool	  m_nextItemReady;
	UINT	  m_count_items;
};

#endif // !defined(_STREAM_WIT_TIME_H_50167DA223FE__INCLUDED_)
