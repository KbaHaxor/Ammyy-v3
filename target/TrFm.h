#ifndef _TR_FM_SERVER_H_
#define _TR_FM_SERVER_H_

#include "../main/InteropTarget.h"
#include "TrFmFileSys.h"

class TrFM : public TrFmFileSys, public InteropTarget
{
public:
	TrFM();
	~TrFM();

	void OnAaFileListRequest();	
	void OnAaFolderCreateRequest();
	void OnAaRenameRequest();
	void OnAaDeleteRequest();
	void OnAaUploadData(bool last);
	void OnAaUploadRequest();
	void OnAaDnloadRequest();
	void OnAaDnloadDataAck();

protected:
	virtual void OnEventFS(LPCSTR templ, LPCSTR param1=NULL) = 0;

private:
	void SendDnloadData();
	void CloseFile();
	
public:	
	bool			m_prmFileManager; // allow access to file system

private:
	HANDLE	m_hFile;		// for upload, dnload
	CStringW m_fileName;	// for upload, dnload
	INT64    m_fileSize;	//             dnload
	INT64    m_bytesSent;	//		       dnload
};

#endif // _TR_FM_SERVER_H_
