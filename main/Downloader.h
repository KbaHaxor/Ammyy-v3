#if !defined(_DOWNLOADER_H__INCLUDED_)
#define _DOWNLOADER_H__INCLUDED_


class CDownloader  
{
public:
	CDownloader();
	virtual ~CDownloader();
	void Start();
	void Stop();
	bool IsWorking();

	CStringW m_fileName;
	CStringA m_url;

	double m_done;
	double m_speed; // bytes/second

private:
	HANDLE	m_hThread;
	bool	m_stopping;
	static	DWORD WINAPI StartThreadProc(LPVOID lpParameter);
	void	StartInternal();
	void	UpdateStatus();

	DWORD	m_dwFilePosBegin;
	DWORD	m_dwFilePosCurr;
	DWORD	m_dwFilePosEnd;

	DWORD	 m_dwTickCountBegin;
	CStringA m_error;	// if error was occur this value has description of it
};

#endif // !defined(_DOWNLOADER_H__INCLUDED_)
