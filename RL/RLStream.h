#if !defined(RL_STREAM_H_INCLUDED_)
#define RL_STREAM_H_INCLUDED_

class RLStream  
{
public:
	RLStream();
	RLStream(DWORD dwCapasity);
	RLStream(void* buffer, DWORD dwCapasity);
	virtual ~RLStream();
	
	void AddRaw(const void* pData, DWORD dwLen);
	void AddUINT8(UINT8 data);
	void AddUINT16(UINT16 data);
	void AddUINT24(UINT32 data);
	void AddUINT32(UINT32 data);
	void AddUINT64(UINT64 data);
	void AddDateTime(SYSTEMTIME t);
	void AddBool(bool b);
#ifdef __CSTRING_A_H__INCLUDED__
	void AddString0A(const CStringA& str);
	void AddString1A(const CStringA& str);
	void AddString2A(const CStringA& str);
#endif
#ifdef __CSTRING_W_H__INCLUDED__	
	void AddString0W(const CStringW& str);
	void AddString1W(const CStringW& str);
#endif
	void AddStream(const RLStream *pStream, bool len);

	void    GetRaw(void* pData, DWORD dwLen);
	UINT8   GetUINT8();
	UINT16  GetUINT16();
	UINT32  GetUINT24();
	UINT32  GetUINT32();
	UINT64  GetUINT64();
	void    GetDateTime(SYSTEMTIME& t);
	bool    GetBool();
#ifdef __CSTRING_A_H__INCLUDED__
	CStringA GetString0A();
	CStringA GetString1A();
	CStringA GetString2A();
#endif
#ifdef __CSTRING_W_H__INCLUDED__
	CStringW GetString0W();
	CStringW GetString1W();
#endif

	void    GetStream(RLStream& stream, bool len);
	
	virtual void SetMinCapasity(DWORD dwMinCapasity);

	void inline SetMinExtraCapasity(DWORD dwMinExtraCapasity) {
		SetMinCapasity(dwMinExtraCapasity+m_posWrite);
	}

	void CutTillRead();

	virtual void Reset();
	virtual void Free();
public:
	void ReadFromFile (LPCSTR  fileName);
	void WriteToFile  (LPCSTR fileName);

#ifdef _WIN32
public:
	void WriteToFileW (LPCWSTR fileName);
	void ReadFromFileW(LPCWSTR fileName);
private:
	void WriteToFileWin32 (HANDLE hFile, LPCSTR fileName);
	void ReadFromFileWin32(HANDLE hFile, LPCSTR fileName);
#endif

public:
	virtual void* GetBuffer1(int len);
	virtual void* GetBuffer() const;
	virtual char* GetBufferWr() const;
	virtual char* GetBufferRd() const;
	inline DWORD  GetLen()     const { return m_posWrite; }
	inline DWORD  GetReadPos() const { return m_posRead;  }
	inline void   SetReadPos(DWORD pos) { m_posRead = pos; }
	inline DWORD  GetAvailForReading() const { return m_posWrite - m_posRead; }
	inline DWORD  GetAvailForWriting() const { return m_dwCapacity - m_posWrite; }
	virtual void  SetLen(DWORD dwLen);
	inline DWORD  GetCapacity() { return m_dwCapacity; }

	const RLStream& operator=(const RLStream& src);

private:
	char* m_pData;	
	DWORD m_dwCapacity;
	DWORD m_posWrite;		// datalen for writer
	DWORD m_posRead;		// pointer of reader
	bool  m_needToFree;
};


#include "RLException.h"

class RLStreamOutException: public RLException
{
public:
	RLStreamOutException(const char* templ, ...)
	{
		va_list ap;
		va_start(ap, templ);
		m_description.Format(templ, ap);
		va_end(ap);		
	}
};

#endif // !defined(RL_STREAM_H_INCLUDED_)
