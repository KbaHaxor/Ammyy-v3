#include "StdAfx.h"
#include "VrFm1.h"
#include "resource.h"
#include "res/resource_fm.h"
#include "../main/aaProtocol.h"
#include "../main/InteropCommon.h"
#include "../main/Common.h"
#include "../main/ImpersonateWrapper.h"
#include "../RL/RLFile.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// transport algorithm of File Manager here


VrFmBlockR::VrFmBlockR()
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_request.id = 0;	
}

bool VrFmBlockR::CloseFile()
{
	if (m_hFile!= INVALID_HANDLE_VALUE) {
		::CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		return true;
	}
	return false;
}

VrFmBlockR::~VrFmBlockR()
{
	CloseFile();
}


void VrFmBlockR::TrySetNewDir(LPCWSTR pathName)
{
	if (!pathName) pathName = m_currentPath;

	RLStream buffer;
 
	buffer.AddUINT8(aaFileListRequest);
	buffer.AddUINT8(0); // not recursively
	buffer.AddUINT16(0);
	CCommon::ConvToUTF8(pathName, buffer, false);

	SetLenUINT16(buffer, 2);

	ASSERT(m_request.id==0);
	m_request.id = aaFileListRequest;
	m_request.flags = 0;
	m_request.name1 = pathName;
 		
	m_transport->SendExact(buffer.GetBufferRd(), buffer.GetLen(), TRUE);

	m_pMainWnd->EnableInterface(FALSE);
}

void VrFmBlockR::CreateDirectory(LPCWSTR dirName)
{
	if (dirName == 0 || dirName[0] == 0) return;

	CStringW fullPath = m_currentPath + dirName;
	
	RLStream buffer;
	buffer.AddUINT8(aaFolderCreateRequest);
	buffer.AddUINT16(0);
	CCommon::ConvToUTF8((LPCWSTR)fullPath, buffer, false);

	SetLenUINT16(buffer, 1);

	ASSERT(m_request.id==0);
	m_request.id = aaFolderCreateRequest;
	m_request.name1 = dirName;
	m_pMainWnd->EnableInterface(FALSE);
		
	m_transport->SendExact(buffer.GetBufferRd(), buffer.GetLen(), TRUE);
}

bool VrFmBlockR::RenameItem(LPCWSTR extName, LPCWSTR newName)
{
	m_pMainWnd->EnableInterface(FALSE);

	RLStream buffer;		
	buffer.AddUINT8(aaRenameRequest);
	buffer.AddUINT16(0);
	CCommon::ConvToUTF8(m_currentPath, buffer, true);
	CCommon::ConvToUTF8(extName, buffer, true);
	CCommon::ConvToUTF8(newName, buffer, true);
		
	SetLenUINT16(buffer, 1);

	ASSERT(m_request.id==0);
	m_request.id = aaRenameRequest;
	m_request.name1 = extName;
	m_request.name2 = newName;
			
	m_transport->SendExact(buffer.GetBufferRd(), buffer.GetLen(), TRUE);
		
	return true;
}

int VrFmBlockR::PackSelectedItems(RLStream& buffer, bool onlyFolders)
{
	CCommon::ConvToUTF8(m_currentPath, buffer, true);

	int c = 0;
	for (int i=-1 ; (i = ListView_GetNextItem(m_listview, i, LVNI_SELECTED)) != -1; ) 
	{
		WCHAR name[MAX_PATH];
		wcscpy(name, m_items[i].name);
		
		TrFmFileSys::ItemType type = m_items[i].type;

		if (onlyFolders) {
			if (type != TrFmFileSys::TYPE_DIR) continue;
		}
		else {
			if (type == TrFmFileSys::TYPE_DIR) {
				wcscat(name, L"\\");
			}
			else if (type != TrFmFileSys::TYPE_FILE) 
				continue;
		}
		CCommon::ConvToUTF8(name, buffer, true);
		c++;
	}

	return c;
}


void VrFmBlockR::DeleteSelectedItems()
{	
	RLStream buffer;
	buffer.AddUINT8(aaDeleteRequest);
	buffer.AddUINT32(0);

	PackSelectedItems(buffer, false);
		
	SetLenUINT32(buffer, 1);

	ASSERT(m_request.id==0);
	m_request.id = aaDeleteRequest;
	m_pMainWnd->EnableInterface(FALSE);

	m_transport->SendExact(buffer.GetBufferRd(), buffer.GetLen(), TRUE);
}


void VrFmBlockR::OnBtnUpload()
{
	if (!m_pMainWnd->IsCopyAllow()) return;

	VrFmBlockL* pBlockL = &m_pMainWnd->m_L;
	VrFmListView* pListView = &pBlockL->m_listview;

	HWND hWnd = (HWND)pBlockL->m_listview;

	if (!pListView->IsAllowedDeletion()) return;

	ASSERT(m_copy->m_fileList.size()==0);
				
	m_pMainWnd->EnableInterface(FALSE);

	for (int i = -1 ; (i = ListView_GetNextItem(hWnd, i, LVNI_SELECTED)) != -1; ) 
	{
		CStringW name = pBlockL->m_items[i].name;
		UINT8    type = pBlockL->m_items[i].type;
		if (type == TrFmFileSys::TYPE_DIR) {
			CStringW relPath = name + '\\';
			DWORD dwError = 0;
			CStringW errorPath;
			TrFmFileSys::SearchRecursively(pBlockL->m_currentPath+relPath, m_copy->m_fileList, true, relPath, dwError, errorPath);
			if (dwError>0) {
				this->ShowMsgBoxError(L"Error %d while lookup local folder %s", dwError, errorPath);
				m_pMainWnd->EnableInterface(TRUE);
				return;				
			};
		}
		else if (type == TrFmFileSys::TYPE_FILE) {
			m_copy->PushBack(name, pBlockL->m_items[i].size);
		}
	}

	m_copy->OnInit(true);
}

void VrFmBlockR::OnBtnDnload()
{
	if (!m_pMainWnd->IsCopyAllow()) return;
	if (!m_listview.IsAllowedDeletion()) return;

	m_pMainWnd->EnableInterface(FALSE);

	ASSERT(m_copy->m_fileList.size()==0);

	// adding only files !
	//
	for (int i=-1 ; (i = ListView_GetNextItem(m_listview, i, LVNI_SELECTED)) != -1; ) {
		TrFmFileSys::ItemType type = m_items[i].type;

		if (type != TrFmFileSys::TYPE_FILE) continue;

		m_copy->PushBack(m_items[i].name, m_items[i].size);
	}

	m_copy->OnInit(false);
}


void VrFmBlockR::StartDnload()
{
	RLStream buffer;
	buffer.AddUINT8(aaFileListRequest);
	buffer.AddUINT8(1); // recursively
	buffer.AddUINT32(0);

	if (PackSelectedItems(buffer, true)>0) {
		SetLenUINT32(buffer, 2);

		ASSERT(m_request.id==0);
		m_request.id = aaFileListRequest;
		m_request.flags = 1;		

		m_transport->SendExact(buffer.GetBufferRd(), buffer.GetLen(), TRUE);
	}
	else
		DnloadNextFile();
}


void VrFmBlockR::DnloadNextFile()
{
	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for accessing network disks, CreateRecurseDir(), CreateFileW()

begin:
	CloseFile(); // close previous file

	if (m_copy->TryNext()) return;

	CStringW fullFileName = m_pMainWnd->m_L.m_currentPath + m_copy->fileName;

	DWORD dwError = TrFmFileSys::CreateRecurseDir(fullFileName);

	if (dwError)
	{
		this->ShowMsgBoxError(L"Error %d while create local folder %d", dwError, (LPCWSTR)fullFileName);
		goto begin;
	}

	if (TrFmFileSys::IsFolder(m_copy->fileName)) goto begin;

	ASSERT(m_hFile==INVALID_HANDLE_VALUE);
	m_hFile = ::CreateFileW(fullFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);

	if (m_hFile==INVALID_HANDLE_VALUE) {
		this->ShowMsgBoxError(L"Error %d while opening local file '%s'", ::GetLastError(), fullFileName);
		goto begin;
	}

	LARGE_INTEGER size1;
	if (::GetFileSizeEx(m_hFile, &size1)==0) {
		this->ShowMsgBoxError(L"Error %d GetFileSizeEx() for local file '%s'", ::GetLastError(), fullFileName);
		goto begin;
	}

	if (!SendDnloadRequest(size1.QuadPart, size1.QuadPart>0)) goto begin;
}



bool VrFmBlockR::SendDnloadRequest(INT64 size, bool askconfirm)
{
	UINT8 flags = 0; // type of aaDRF
	BYTE md5Hash[16];

	if (askconfirm)
	{
		m_dlgConfirm.m_continue = (size>0);
		m_dlgConfirm.m_labelText.Format(L"%s\n\n"L"This file already exists on local computer.", m_copy->fileName);

		int ret = m_dlgConfirm.DoModal(*m_pMainWnd);

		if (ret == IDC_BTN_CONTINUE) 
		{
			DWORD dwError = TrFmFileSys::GetMD5ForFile(m_hFile, size, md5Hash);
			if (dwError) {
				this->ShowMsgBoxError(L"Error %d while getting md5 for local file %s", dwError, m_copy->fileName);
				return false;
			}
			
			flags |= aaDRF_TryToContinue;
		}
		else if (ret == IDC_BTN_OVERWRITE)
		{
			if (::SetEndOfFile(m_hFile)==0) {
				this->ShowMsgBoxError(L"Error %d while SetEndOfFile() for local file %s", ::GetLastError(), m_copy->fileName);
				return false;
			}
		}
		else {
			if (ret == IDCANCEL) m_copy->m_canceling = true;
			return false;
		}
	}

	if (settings.m_copyFileTime) flags |= aaDRF_GetTime;
		
	RLStream buffer;
	buffer.AddUINT8(aaDnloadRequest);
	buffer.AddUINT16(0);
	buffer.AddUINT8(flags);

	UINT64 offset = 0;

	if (flags & aaDRF_TryToContinue) {
		TrFmFileSys::AddFileSize(buffer, size);
		buffer.AddRaw(md5Hash, 16);
		offset = size;
	}
			
	CCommon::ConvToUTF8(m_currentPath+m_copy->fileName, buffer, true);

	SetLenUINT16(buffer, 1);
	
	ASSERT(m_request.id==0);
	m_request.id = aaDnloadRequest;
	m_request.flags = flags;
	m_request.offset = offset;

	m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);
	return true;
}



void VrFmBlockR::UploadNextFile()
{
	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for accessing network disks, CreateFileW()

begin:
	CloseFile(); // close previous file

	if (m_copy->TryNext()) return;

	CStringW fileName = m_pMainWnd->m_L.m_currentPath + m_copy->fileName;

	if (!TrFmFileSys::IsFolder(m_copy->fileName)) {
		m_hFile = ::CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY, 0);

		if (m_hFile == INVALID_HANDLE_VALUE) {
			this->ShowMsgBoxError(L"Error %d while open local file %s", ::GetLastError(), fileName);
			goto begin; // skip this file, goto next file
		}

		INT64 size = TrFmFileSys::GetFileSize(m_hFile);
		ASSERT(size>=0);
		m_copy->ReSetCurFileSize(size);
	}

	SendUploadRequest();
}


void VrFmBlockR::SendUploadRequest(UINT8 flags, UINT64 offset, BYTE* md5Hash)
{
	RLStream buffer;
	buffer.AddUINT8(aaUploadRequest);
	buffer.AddUINT16(0); // size, will be set later

	if (settings.m_copyFileTime) {
		buffer.AddUINT8(flags|aaURF_SetTime);
		FILETIME time;
		RLFileWin::GetLastWriteTime(m_hFile, &time);
		buffer.AddRaw(&time, sizeof(time));
	}
	else {
		buffer.AddUINT8(flags);
	}

	if (flags & aaURF_TryToContinue) {
		TrFmFileSys::AddFileSize(buffer, offset);
		buffer.AddRaw(md5Hash, 16);
	}
	else {		
		m_request.name1 = m_currentPath + m_copy->fileName;
		CCommon::ConvToUTF8(m_request.name1, buffer, false);
	}

	SetLenUINT16(buffer,1);

	ASSERT(m_request.id==0);
	m_request.id = aaUploadRequest;
	m_request.flags = flags;
	m_request.offset = offset;
	
	m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);
}


bool VrFmBlockR::SendUploadFileData(CStringW& errorMsg)
{
	RLStream buffer(6+TrFmFileSys::BLOCK_SIZE);

	DWORD dwRead = 0;

	if (!m_copy->m_canceling)
	{
		int size = TrFmFileSys::GetNextBlockSize(m_copy->m_fileOne.end, m_copy->m_fileOne.pos);

		if (size == TrFmFileSys::BLOCK_SIZE) {
			buffer.AddUINT8(aaUploadData);
		}
		else {
			buffer.AddUINT8(aaUploadDataLast);
			buffer.AddUINT8(m_copy->m_canceling); // flag
			TrFmFileSys::AddFileSize(buffer, size);
		}

		DWORD dwError = TrFmFileSys::ReadFile(m_hFile, buffer.GetBufferWr(), size);

		if (dwError) {
			errorMsg.Format(L"Error %d while reading local file '%s'", dwError, m_copy->fileName);
		}
		else {
			dwRead = size;
			buffer.AddRaw(NULL, size);
			m_copy->m_fileOne.pos += size;
		}
	}

	bool close = (dwRead!=TrFmFileSys::BLOCK_SIZE);

	if (close) 
	{
		if (dwRead==0) {
			buffer.Reset();
			buffer.AddUINT8(aaUploadDataLast);
			buffer.AddUINT8(m_copy->m_canceling);  // flag
			TrFmFileSys::AddFileSize(buffer, 0);   // size
		}
		m_uploadReady = false;
	}

	m_dwUnAckDateMsg++;
	m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);

	return !close;
}




void VrFmBlockR::OnAaFileListRequest1()
{
	UINT32 size;
	m_transport->ReadExact(&size, 4);

	RLStream buffer(size);
	m_transport->ReadExact(buffer.GetBuffer(), size);
	buffer.SetLen(size);

	UINT8 flag = buffer.GetUINT8();

	m_reply.error = (flag) ? buffer.GetUINT32() : 0;
	
	if (m_request.flags)
	{
		if (flag==0) {
			while (buffer.GetAvailForReading()>0)
			{
				CStringW name = CCommon::ConvToUTF16(buffer.GetString1A());
				UINT64   size = TrFmFileSys::GetFileSize(buffer);
				m_copy->PushBack(name, size);
				m_copy->m_fileAll.end += size;
				m_copy->m_cntFilesTotal++;
			}
		}
		else {
			m_reply.name1 = CCommon::ConvToUTF16(buffer.GetString0A());
		}
	}	
 	else 	
	{
		if (flag==0)
		{		
			m_items.clear();

			if (m_request.name1[0] != 0) {
				m_items.push_back(VrFmItem(L"..", TrFmFileSys::TYPE_DOTS, 0, NULL, 0));
			}
				
			while (buffer.GetAvailForReading()>0) 
			{
				UINT8	 fAttrs = 0;
				UINT8	 fType = buffer.GetUINT8();
				ASSERT(fType == TrFmFileSys::TYPE_DIR   || fType == TrFmFileSys::TYPE_FILE || fType == TrFmFileSys::TYPE_DISK || 
					   fType == TrFmFileSys::TYPE_LINK_0 || fType == TrFmFileSys::TYPE_LINK_1);

				if (fType == TrFmFileSys::TYPE_DISK) {
					DWORD drives = buffer.GetUINT32();

					WCHAR path[] = L"*:\\";

					for (int i=0; i<26; i++) {
						if (((drives >> i) & 1) == 0) continue;
						path[0] = 'A' + i;
						fAttrs = buffer.GetUINT8(); // type of disk
						m_items.push_back(VrFmItem(path, TrFmFileSys::TYPE_DISK, 0, NULL, fAttrs));
					}
					continue;
				}

				UINT64	 fSize  = 0;
				FILETIME fTime = {0,0};

				if (fType == TrFmFileSys::TYPE_DIR || fType == TrFmFileSys::TYPE_FILE) {
					buffer.GetRaw(&fTime, sizeof(FILETIME));
					fAttrs = buffer.GetUINT8();
									
					if (fType == TrFmFileSys::TYPE_FILE) fSize = TrFmFileSys::GetFileSize(buffer);
				}

				CStringW fName = CCommon::ConvToUTF16(buffer.GetString1A());

				if		(fType == TrFmFileSys::TYPE_LINK_0)	{ m_linkPath[0] = fName; fName = L"Remote Desktop";  }
				else if (fType == TrFmFileSys::TYPE_LINK_1) { m_linkPath[1] = fName; fName = L"Remote Documents";}
			
				m_items.push_back(VrFmItem(fName, (TrFmFileSys::ItemType)fType, fSize, &fTime, fAttrs));
			}
		}		
	}
	::PostMessage(*m_pMainWnd, WM_SERVER_REPLY, aaFileListRequest, 0);
}

void VrFmBlockR::OnAaFileListRequest2()
{
	m_request.id = 0;

	if (m_request.flags)
	{
		if (m_reply.error) {
			this->ShowMsgBoxError(L"Error %d while lookup remote folder %s", m_reply.error, (LPCWSTR)m_reply.name1);
			m_copy->m_canceling = true;
		}

		DnloadNextFile();
	}
	else {
		if (m_reply.error) {
			this->ShowMsgBoxError(L"Error %d while lookup remote folder '%s'", m_reply.error, (LPCWSTR)m_request.name1);
		}
		else {
			CStringW prevPath = m_currentPath;
			m_currentPath = m_request.name1;
			SortItems(m_currentComp);
			m_listview.UpdateContent();
			m_listview.SelectPrevPath(prevPath);
		}

		m_dirBox.SetTextW((m_currentPath[0] == 0) ? L"Remote Computer" : m_currentPath);
		m_pMainWnd->EnableInterface(TRUE);
	}
}


void VrFmBlockR::OnAaDnloadRequest1()
{
	m_request.id=0;

	m_transport->ReadExact(&m_reply.error, 4);

	if (m_reply.error) {
		::PostMessage(*m_pMainWnd, WM_SERVER_REPLY, aaDnloadRequest, 0);
	}
	else {
		INT64 size = TrFmFileSys::GetFileSize(m_transport);
		m_copy->ReSetCurFileSize(size);
		m_copy->StartFile(m_request.offset);

		if (m_request.flags & aaDRF_GetTime) {
			FILETIME time;
			m_transport->ReadExact(&time, sizeof(time));
			RLFileWin::SetLastWriteTime(m_hFile, &time);
		}
	}
}

void VrFmBlockR::OnAaDnloadRequest2()
{
	if (m_reply.error==ERROR_CRC) {
		if (SendDnloadRequest(0, true)) return;
	}
	else {
		this->ShowMsgBoxError(L"Error %d while opening remote file %s", m_reply.error, (LPCWSTR)m_copy->fileName);
	}
	DnloadNextFile();
}	


void VrFmBlockR::OnAaUploadDataAck1()
{
	DWORD error = 0; // we can't use m_reply.error

	UINT8 flags;
	m_transport->ReadExact(&flags, 1);

	ASSERT(flags==0 || flags==1);

	if (flags) m_transport->ReadExact(&error, 4);

	m_dwUnAckDateMsg--;
	ASSERT(m_dwUnAckDateMsg>=0);

	if (error>0) {
		if (m_uploadDataWasError) return; // already was error
		m_uploadDataWasError = true;
		m_uploadReady = false;
		m_reply.errorMsg2.Format(L"Error %d while writting remote file %s", error, (LPCWSTR)m_request.name1);
		goto exit;
	}
	else if (m_uploadReady) {
		SendUploadFileData(m_reply.errorMsg2);
		if (m_reply.errorMsg2[0]) goto exit;
	}

	if (m_dwUnAckDateMsg!=0) return;

exit:
	::PostMessage(*m_pMainWnd, WM_SERVER_REPLY, aaUploadDataAck, 0);
}

void VrFmBlockR::OnAaUploadDataAck2()
{
	if (m_reply.errorMsg2[0]) {
		this->ShowMsgBoxError(m_reply.errorMsg2);
		m_reply.errorMsg2 = ""; // reset it to prevent double show
	}

	if (m_dwUnAckDateMsg==0) UploadNextFile();
}

void VrFmBlockR::SendDnLoadCancel()
{
	CloseFile();

	RLStream buffer;
	buffer.AddUINT8(aaDnloadRequest);
	buffer.AddUINT16(1); // size
	buffer.AddUINT8(aaDRF_Cancel);

	m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);
}

void VrFmBlockR::OnAaDnloadData1(bool last)
{
	UINT64 size = TrFmFileSys::BLOCK_SIZE;

	DWORD error = 0;
	bool remoteok = false;

	char buffer[TrFmFileSys::BLOCK_SIZE];

	if (last) {
		UINT8 flags = 0;
		m_transport->ReadExact(&flags, 1);

		if (flags) {
			m_transport->ReadExact(&error, 4);
			size = 0;
		}
		else {
			size = TrFmFileSys::GetFileSize(m_transport);
		}
	}

	if (size>0)
	{
		ASSERT(size<=TrFmFileSys::BLOCK_SIZE);
		m_transport->ReadExact(buffer, size);
	}


	// skip last messages after cancel request or internal error
	if (m_hFile!=INVALID_HANDLE_VALUE) {
		if (m_copy->m_canceling) {
			SendDnLoadCancel();			
		}
		else if (error==0) {
			remoteok = true;
			error = TrFmFileSys::WriteFile(m_hFile, buffer, size);
			if (error) 
				SendDnLoadCancel();
			else
				m_copy->m_fileOne.pos += size;
		}
	}
	else {
		error = 0; // file already closed, so do not show error message anymore for this file
	}

	if (error || last) {
		CloseFile();
		m_reply.error = error;
		m_reply.last  = last;
		m_reply.remoteok = remoteok;
		::PostMessage(*m_pMainWnd, WM_SERVER_REPLY, aaDnloadData, 0);
	}
	else {
		if (m_hFile!=INVALID_HANDLE_VALUE) { // no need to send ack if file closed
			UINT8 msg = aaDnloadDataAck;
			m_transport->SendExact(&msg, 1, TRUE);
		}
	}
}

void VrFmBlockR::OnAaDnloadData2()
{
	if (m_reply.error) {
		if (m_reply.remoteok)
			this->ShowMsgBoxError(L"Error %d while writting local file %s", m_reply.error, (LPCWSTR)m_copy->fileName);
		else
			this->ShowMsgBoxError(L"Error %d while reading remote file %s", m_reply.error, (LPCWSTR)(m_currentPath + m_copy->fileName));
	}

	if (m_reply.last) DnloadNextFile();
}

bool VrFmBlockR::OnAaMsg(UINT8 msgType)
{
	if ((HWND)*m_pMainWnd==NULL) return false; // we're not expected any FM messages, so exception will generate later

	switch(msgType) {
		case aaFmReply:			{ OnAaFmReply();			return true; }
		case aaUploadDataAck:	{ OnAaUploadDataAck1();		return true; }
		case aaDnloadData:		{ OnAaDnloadData1(false);	return true; }
		case aaDnloadDataLast:	{ OnAaDnloadData1(true);	return true; }
		default:
			return false;
	}
}


void VrFmBlockR::OnAaFmReply()
{
	if (m_request.id==aaFolderCreateRequest) 
	{
		m_transport->ReadExact(&m_reply.error, 4);
		::PostMessage(*m_pMainWnd, WM_SERVER_REPLY, aaFolderCreateRequest, 0);
	} 
	else if (m_request.id==aaRenameRequest) 
	{
		m_transport->ReadExact(&m_reply.error, 4);
		::PostMessage(*m_pMainWnd, WM_SERVER_REPLY, aaRenameRequest, 0);
	}	
	else if (m_request.id==aaDeleteRequest)
	{
		m_transport->ReadExact(&m_reply.error, 4);

		m_reply.count =0xFFFF; //max value;

		if (m_reply.error)
		{
			UINT16 len;

			m_transport->ReadExact(&m_reply.count, 2);
			m_transport->ReadExact(&len, 2);

			RLStream buffer(len);
			m_transport->ReadExact(buffer.GetBuffer(), len);
			buffer.SetLen(len);

			m_reply.name1 = CCommon::ConvToUTF16(buffer.GetString0A());
		}

		::PostMessage(*m_pMainWnd, WM_SERVER_REPLY, aaDeleteRequest, 0);
	}
	else if (m_request.id==aaFileListRequest)
	{
		OnAaFileListRequest1();
	}
	else if (m_request.id==aaUploadRequest) 
	{
		OnAaUploadRequest1();
	}
	else if (m_request.id==aaDnloadRequest) 
	{
		OnAaDnloadRequest1();
	}
	else
		throw INVALID_PROTOCOL;
}

void VrFmBlockR::OnAaRenameRequest2()
{
	if (m_reply.error==0) {
		m_items[m_listview.m_lastSelectedIndex].name = m_request.name2;
	}
	else {
		this->ShowMsgBoxError(L"Error %d while remote rename '%s' to '%s'", m_reply.error, (LPCWSTR)m_request.name1, (LPCWSTR)m_request.name2);
		m_listview.UpdateContent();
	}

	m_request.id = 0;
	m_pMainWnd->EnableInterface(TRUE);
}

void VrFmBlockR::OnAaFolderCreateRequest2()
{
	m_request.id = 0;
	if (m_reply.error==0) {
		TrySetNewDir();
	}
	else {
		this->ShowMsgBoxError(L"Error %d while creating remote folder '%s'", m_reply.error, (LPCWSTR)m_request.name1);
		m_pMainWnd->EnableInterface(TRUE);
	}
}


void VrFmBlockR::OnAaDeleteRequest2()
{
	if (m_reply.error)
		this->ShowMsgBoxError(L"Error %d while remote deleting '%s'", m_reply.error, (LPCWSTR)m_reply.name1);	

	int c = 0;
	for (int i = -1 ; (i = ListView_GetNextItem(m_listview, i, LVNI_SELECTED)) != -1; ) {
		if (c>=m_reply.count) break;
		ListView_DeleteItem(m_listview, i);
		m_items.erase(m_items.begin()+i);
		i--;
	}

	m_request.id = 0;
	m_listview.UpdateContent();
	m_pMainWnd->EnableInterface(TRUE);
}



/* TODO: may be usefull code
		if (errCode != 0) {
			char szBuf[80]; 
			LPVOID lpMsgBuf;
			
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				errCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf,
				0, NULL );
			
			wsprintf(szBuf, "%s", lpMsgBuf); 

			::MessageBox(0, szBuf, "error", MB_OK);

			LocalFree(lpMsgBuf);
		}
*/


void VrFmBlockR::OnAaUploadRequest1()
{
	m_transport->ReadExact(&m_reply.error, 4);

	if (m_reply.error == ERROR_FILE_EXISTS && m_request.flags==0) {			
		m_reply.size = TrFmFileSys::GetFileSize(m_transport);
	}

	m_request.id = 0;

	if (m_reply.error == 0) 
	{
		m_reply.errorMsg1 = "";
		m_reply.uploadNextFile = true;

		if (TrFmFileSys::IsFolder(m_copy->fileName)) {
			goto exit;
		}

		LARGE_INTEGER offset;
		offset.QuadPart = m_request.offset;
		if (::SetFilePointerEx(m_hFile, offset, NULL, FILE_BEGIN)==0)
		{
			m_reply.errorMsg1.Format(L"Error %d SetFilePointerEx() for local file %s", ::GetLastError(),
				(LPCWSTR)(m_pMainWnd->m_L.m_currentPath + m_copy->fileName));
			goto exit;
		}

		m_copy->StartFile(m_request.offset);		
		m_dwUnAckDateMsg = 0;
		m_uploadReady = true;
		m_uploadDataWasError = false;
		m_reply.uploadNextFile = false;

		for (int i=0; i<TrFmFileSys::INIT_BLOCKS ; i++) {
			// anycase we sent aaUploadData(Last), when get reply we call UploadNextFile
			if (!SendUploadFileData(m_reply.errorMsg1)) break;
		}

		if (m_reply.errorMsg1[0]==0) return; // no error to show, so no need to call other thread
	}


exit:
	::PostMessage(*m_pMainWnd, WM_SERVER_REPLY, aaUploadRequest, 0);
}

void VrFmBlockR::OnAaUploadRequest2()
{	
	if (m_reply.error == 0) 
	{
		if (m_reply.errorMsg1[0]) this->ShowMsgBoxError(m_reply.errorMsg1);

		if (!m_reply.uploadNextFile) return;
	}
	else if ((m_reply.error == ERROR_FILE_EXISTS && m_request.flags==0) ||
			 (m_reply.error == ERROR_CRC         && m_request.flags==2))
	{
		m_dlgConfirm.m_continue = (m_request.flags==0 && m_copy->m_fileOne.end>0 && m_copy->m_fileOne.end>=m_reply.size);
		m_dlgConfirm.m_labelText.Format(L"%s\n\n"L"This file already exists on remote computer.", m_copy->fileName);
		
		int ret = m_dlgConfirm.DoModal(*m_pMainWnd);

		if (ret == IDC_BTN_CONTINUE) 
		{
			BYTE md5Hash[16];

			DWORD dwError = TrFmFileSys::GetMD5ForFile(m_hFile, m_reply.size, md5Hash);
			if (dwError) {
				this->ShowMsgBoxError(L"Error %d while ReadFile() for local file %s", dwError,
									(LPCWSTR)(m_pMainWnd->m_L.m_currentPath + m_copy->fileName));
				UploadNextFile();
			}
			else {
				SendUploadRequest(aaURF_TryToContinue, m_reply.size, md5Hash);
			}
			return;
		}
		else if (ret == IDC_BTN_OVERWRITE) {
			SendUploadRequest(aaURF_Overwrite, 0);
			return;
		}
		else {
			if (ret == IDCANCEL) m_copy->m_canceling = true;
		}
	}
	else
	{
		this->ShowMsgBoxError(L"Error %d while opening remote file %s", m_reply.error, (LPCWSTR)m_request.name1);
	}

	UploadNextFile(); // skip this file, go to the next file
}


//-------------------------------------------------------------------------------------------------------

void VrFmConfirmDlg::Reset(bool upload)
{
	m_upload = upload;
	m_skipAll = false;
	m_overwriteAll = false;
}

int VrFmConfirmDlg::DoModal(HWND hWndParent)
{
	if (m_overwriteAll) return IDC_BTN_OVERWRITE;
	if (m_skipAll)		return IDC_BTN_SKIP;

	m_lpTemplateName = MAKEINTRESOURCE(IDD_CONFIRM_DLG);

	int ret = RLDlgBase::DoModal(hWndParent);

	if (ret==IDC_BTN_OVERWRITE_ALL) {
		m_overwriteAll = true;
		return IDC_BTN_OVERWRITE;
	}
	else if (ret==IDC_BTN_SKIP_ALL) {
		m_skipAll = true;
		return IDC_BTN_SKIP;
	}
	else
		return ret;
}

BOOL VrFmConfirmDlg::OnInitDialog()
{
	HWND hwndLabel = ::GetDlgItem(m_hWnd, IDC_STRING);
	::SetWindowTextW(hwndLabel, m_labelText);

	HWND hwndBtnContinue = ::GetDlgItem(m_hWnd, IDC_BTN_CONTINUE);

	CStringA what = m_upload ? "uploading" :"downloading";

	this->SetTextA("Ammyy Admin - " + what);
	::SetWindowTextA(hwndBtnContinue, "Try to continue " + what);

	::EnableWindow(hwndBtnContinue, m_continue ? TRUE : FALSE);

	return TRUE;
}

INT_PTR VrFmConfirmDlg::WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	if(uMsg == WM_COMMAND)
	{
		::EndDialog(hwndDlg, LOWORD(wParam));
	}

	return 0;
}
