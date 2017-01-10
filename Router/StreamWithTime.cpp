#include "stdafx.h"
#include "StreamWithTime.h"


// this class is needed for research only

StreamWithTime::StreamWithTime()
:m_timer(false)
{

}

StreamWithTime::~StreamWithTime()
{

}

void StreamWithTime::Create(LPCSTR fileName)
{
	m_f1.Create(CStringA(fileName)+".data");
	m_f2.Create(CStringA(fileName)+".time");

	m_timer.Start();
	m_last_time = 0;
	m_last_bytes = 0;
}

void StreamWithTime::Open(LPCSTR fileName)
{
	m_f1.Open(CStringA(fileName)+".data");
	m_f2.Open(CStringA(fileName)+".time");

	m_bytes = 0;
	m_time  = 0;
	m_nextItemReady = false;

	UINT size = m_f2.GetSize();
	m_f2.SetSeek(0);
	ASSERT(size%sizeof(TIMEITEM));
	m_count_items = size / sizeof(TIMEITEM);
}


void StreamWithTime::Push(LPCVOID buffer, UINT len)
{
	if (!m_f1.IsOpened()) return;

	UINT64 ms = (UINT64)(m_timer.GetElapsedSeconds()*1000);
	UINT32 elapsed = (UINT32)(ms - m_last_time);

	if (elapsed>=200) {
		TIMEITEM timeItem;
		timeItem.bytes = m_last_bytes;
		timeItem.time  = elapsed;

		m_last_time = ms;
		m_last_bytes = 0;

		m_f2.Write(&timeItem, sizeof(timeItem));
	}

	m_f1.Write(buffer, len);
	m_last_bytes += len;
}

void StreamWithTime::Close()
{
	m_f1.Close();
	m_f2.Close();
}

void StreamWithTime::Read(LPVOID buffer, UINT len)
{
	m_f1.Read(buffer, len);
	m_bytes += len;
}

UINT64 StreamWithTime::GetTime()
{
	while(true) {
		if (!m_nextItemReady && m_count_items>0) {
			m_count_items--;
			m_f2.Read(&m_nextItem, sizeof(m_nextItem));
			m_nextItemReady = true;
		}

		if (m_nextItemReady) {
			if (m_bytes>m_nextItem.bytes) {
				m_bytes -= m_nextItem.bytes;
				m_time  += m_nextItem.time;
				m_nextItemReady = false;
				continue;
			}
		}
		break;
	}

	return m_time;
}
