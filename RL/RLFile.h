#if !defined(_RL_FILE_H___26B3587BD75__INCLUDED_)
#define _RL_FILE_H___26B3587BD75__INCLUDED_

class RLFile
{
public:
	RLFile();
	~RLFile();

	void Open  (LPCSTR fileName);
	void Create(LPCSTR fileName);
	void OpenOrCreate(LPCSTR fileName);
	void Read (LPVOID  buffer, UINT len);
	void Write(LPCVOID buffer, UINT len);
	void Close();

	//inline bool IsEOF()    { return (feof(m_handle)!=0); }
	inline bool IsOpened() { return (m_handle!=NULL); }
	UINT32 GetSize();
	void   SetSeek(UINT32 offset);

private:
	FILE* m_handle;
};

#ifdef _WIN32

class RLHandle
{
public:
	RLHandle(HANDLE handle=NULL);
	virtual ~RLHandle();
	virtual void Close();

protected:
	HANDLE m_handle;
};

class RLFileWin : public RLHandle
{
public:
	RLFileWin();	

	void Attach(HANDLE handle);
	void Read (LPVOID buffer, UINT len);
	void Write(LPCVOID buffer, UINT len);

	UINT32 GetSize();

	static void GetLastWriteTime(HANDLE hFile, FILETIME* pTime);
	static void SetLastWriteTime(HANDLE hFile, FILETIME* pTime);
};

#endif // _WIN32

#endif // !defined(_RL_FILE_H___26B3587BD75__INCLUDED_)
