#if !defined(AFX_SERVERINTERACT_H__A40B1ADC_5A5B_443C_B739_C1E89F8491DA__INCLUDED_)
#define AFX_SERVERINTERACT_H__A40B1ADC_5A5B_443C_B739_C1E89F8491DA__INCLUDED_

#include "Downloader.h"

class CServerInteract
{
public:
	CServerInteract();

	void SendInitCmd();

protected:		
	virtual void OnDownloadFileStatus(LPCSTR msg, int percents) {}

private:
	void DownloadFile(LPCSTR url, LPCWSTR fileName);
	void UpdateToNewVersion(LPCSTR url);
	static void CopyResource(HANDLE hModuleOut, LPCSTR lpType, LPCSTR lpName);
	static void UpdateResources(LPCWSTR fileName);

public:
	bool	m_updating;
	bool	m_bNeedComputerId;

private:
	CDownloader	m_downloader;
};

#endif // !defined(AFX_SERVERINTERACT_H__A40B1ADC_5A5B_443C_B739_C1E89F8491DA__INCLUDED_)
