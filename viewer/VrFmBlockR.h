#if !defined(_VR_FM_BLOCK_R_15B00769F118__INCLUDED_)
#define _VR_FM_BLOCK_R_15B00769F118__INCLUDED_

#include "../target/TrFmFileSys.h"

class VrFmConfirmDlg : public RLDlgBase
{
public:
	int  DoModal(HWND hWndParent);
	void Reset(bool upload);

	CStringW m_labelText;
	bool	 m_continue;

private:
	BOOL OnInitDialog();
	INT_PTR WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	bool	m_upload;
	bool	m_skipAll;
	bool	m_overwriteAll;
};

class VrFmBlockR: public VrFmBlock, public TrFmFileSys
{
	friend class VrFmCopy;

public:
	VrFmBlockR();
	~VrFmBlockR();

	void TrySetNewDir(LPCWSTR pathName=NULL);
	void CreateDirectory(LPCWSTR dirName);
	void DeleteSelectedItems();
	bool RenameItem(LPCWSTR extName, LPCWSTR newName);
	void StartDnload();
	void UploadNextFile();
	void DnloadNextFile();
	
	void OnBtnUpload();
	void OnBtnDnload();

	bool OnAaMsg(UINT8 msgType);
	void OnAaFmReply();
	void OnAaUploadDataAck1();
	void OnAaDnloadData1(bool last);
	void OnAaUploadRequest1();
	void OnAaUploadRequest2();
	void OnAaFileListRequest1();
	void OnAaFileListRequest2();	
	void OnAaUploadDataAck2();
	void OnAaDnloadRequest1();
	void OnAaDnloadRequest2();
	void OnAaDnloadData2();
	void OnAaFolderCreateRequest2();
	void OnAaRenameRequest2();
	void OnAaDeleteRequest2();

private:
	void SendUploadRequest(UINT8 flags=0, UINT64 offset=0, BYTE* md5Hash=NULL);
	bool CloseFile();
	bool SendUploadFileData(CStringW& errorMsg);
	void SendDnLoadCancel();
	bool SendDnloadRequest(INT64 size, bool askconfirm);
	int  PackSelectedItems(RLStream& buffer, bool onlyFolders);

private:
	struct CmdRequest {
		UINT8		id;
		CStringW	name1;	// aaRenameRequest - ext, aaFileListRequest, aaFolderCreateRequest, aaUploadRequest
		CStringW	name2;	// aaRenameRequest - new
		UINT8		flags;	// aaDnloadRequest, aaUploadRequest, aaFileListRequest			
		UINT64		offset; // aaDnloadRequest
	} m_request; // last request

	struct CmdReply {
		UINT32 error;
		INT64 size;
		BYTE   md5Hash[16];	// aaUploadReply
		CStringW	name1;	// aaDeleteRequest, aaFileListRequest - name of error path
		UINT16 count;		// aaDeleteRequest - count of succesed queries
		bool   last;        // aaDnloadData
		bool   remoteok;    // aaDnloadData
		bool		uploadNextFile;
		CStringW	errorMsg1;
		CStringW	errorMsg2;
	} m_reply; // last reply from target

	HANDLE		m_hFile;	// current download or upload file
	bool		m_uploadReady;
	bool		m_uploadDataWasError;
	VrFmConfirmDlg	m_dlgConfirm;
	volatile LONG m_dwUnAckDateMsg; // for upload

public:
	Transport* m_transport; 
	VrFmCopy*  m_copy;
};



#endif // !defined(_VR_FM_BLOCK_R_15B00769F118__INCLUDED_)
