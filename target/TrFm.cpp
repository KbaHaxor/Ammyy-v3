#include "StdAfx.h"
#include "TrFm.h"
#include "../main/aaProtocol.h"
#include "../main/Common.h"
#include "../main/ImpersonateWrapper.h"
#include "TrMain.h"
#include "../RL/RLFile.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

class TrFmLookUp: public TrFmLookUpBase
{
public:
	TrFmLookUp(RLStream& bufferOut, LPCWSTR path): m_bufferOut(bufferOut) { m_path = path; Do(); }

	void OnBegin()
	{
		if (m_dwError==0) {
			m_bufferOut.AddUINT8(0); //no error
		}
		else {
			m_bufferOut.AddUINT8(1); // error
			m_bufferOut.AddUINT32(m_dwError);
		}
	}

	void OnAdd(WIN32_FIND_DATAW& fd)
	{
		bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		UINT8 fType = isDir ? TrFmFileSys::TYPE_DIR : TrFmFileSys::TYPE_FILE;
		m_bufferOut.AddUINT8(fType);
		m_bufferOut.AddRaw(&fd.ftLastWriteTime, sizeof(FILETIME));
		m_bufferOut.AddUINT8(fd.dwFileAttributes & 0xff);

		if (!isDir) {			
			TrFmFileSys::AddFileSize(m_bufferOut, GetFileSize64(fd));
		}
				
		CCommon::ConvToUTF8(fd.cFileName, m_bufferOut, true);
	}

	void OnBeginDrives(DWORD drives)
	{
		m_bufferOut.AddUINT8(TrFmFileSys::TYPE_DISK);
		m_bufferOut.AddUINT32(drives);
	};

	void OnAddDrive(LPCWSTR path)
	{
		UINT type = ::GetDriveTypeW(path);
		m_bufferOut.AddUINT8(type);
	}

	void OnEndDrives()
	{
		CStringW links[2];
		TrFmFileSys::FillLinks(links);

		for (int i=0; i<2; i++) {
			if (links[i].GetLength()==0) continue;
			m_bufferOut.AddUINT8(TrFmFileSys::TYPE_LINK_0+i);
			CCommon::ConvToUTF8(links[i], m_bufferOut, true);
		}
	}


private:
	RLStream& m_bufferOut;
};


TrFM::TrFM()
{
	m_hFile = INVALID_HANDLE_VALUE;
}

TrFM::~TrFM()
{
	CloseFile();
}

void TrFM::CloseFile()
{
	if (m_hFile!= INVALID_HANDLE_VALUE) {
		::CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

void TrFM::OnAaFileListRequest()
{
	if (!m_prmFileManager) throw RLException("FileManager is forbidden");

	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks, TrFmLookUp(), SearchRecursively()

	UINT32 size = 0;
	UINT8 type; // 0 - bit recursively,	1 - recursively
	m_transport->ReadExact(&type, 1);

	if (type==0)
		m_transport->ReadExact(&size, 2);
	else
		m_transport->ReadExact(&size, 4);
	
	RLStream buffer(size);  	
	m_transport->ReadExact(buffer.GetBuffer(), size);
	buffer.SetLen(size);

	RLStream bufferOut;
	bufferOut.AddUINT8(aaFmReply);
	bufferOut.AddUINT32(0); //size


	if (type == 0) {
		CStringW path = CCommon::ConvToUTF16((LPCSTR)buffer.GetBuffer(), size);
 		
		CStringA path2 = (path[0] == 0) ? "<root disks>" : path; // need to be sure that it's exist until OnEventFS() finished
		this->OnEventFS("View folder %s", (LPCSTR)path2);

		TrFmLookUp lookup(bufferOut, path);
	}
	else if (type == 1) 
	{
		CStringW currPath = CCommon::ConvToUTF16(buffer.GetString1A());

		TrFmFileSys::FileList fList;

		DWORD dwError = 0;
		CStringW errorPath;
		
		while (buffer.GetAvailForReading()>0) 
		{
			CStringW name = CCommon::ConvToUTF16((LPCSTR)buffer.GetString1A()) + '\\';
			
			if (dwError==0) {
				TrFmFileSys::SearchRecursively(currPath+name, fList, true, name, dwError, errorPath);
			}
		}

		bufferOut.AddUINT8(dwError>0 ? 1 : 0);

		if (dwError>0) {
			bufferOut.AddUINT32(dwError);
			CCommon::ConvToUTF8(errorPath, bufferOut, false);
		}
		else {
			//UINT64 sizeTotal = 0;
			//bufferOut.AddRaw(&sizeTotal, 6);

			for (FileList::iterator it=fList.begin() ; it != fList.end(); it++ ) 
			{
				CCommon::ConvToUTF8((*it).name, bufferOut, true);
				AddFileSize(bufferOut, (*it).size);
			}

			//memcpy((char*)bufferOut.GetBuffer()+5, &sizeTotal, 6);
		}
	}

	TrFmFileSys::SetLenUINT32(bufferOut, 1);

	m_transport->SendExact(bufferOut.GetBuffer(), bufferOut.GetLen(), TRUE);
}

void TrFM::OnAaRenameRequest()
{
	if (!m_prmFileManager) throw RLException("FileManager is forbidden");

	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks, MoveFileW()

	UINT16 size = 0;	
	m_transport->ReadExact(&size, 2);
	
	RLStream buffer(size);
	m_transport->ReadExact(buffer.GetBuffer(), size);
	buffer.SetLen(size);

 	CStringW currPath = CCommon::ConvToUTF16((LPCSTR)buffer.GetString1A()/*.GetBuffer(0)*/);

	CStringW extName = currPath + CCommon::ConvToUTF16((LPCSTR)buffer.GetString1A()/*.GetBuffer(0)*/);
	CStringW newName = currPath + CCommon::ConvToUTF16((LPCSTR)buffer.GetString1A()/*.GetBuffer(0)*/);

	BOOL b = ::MoveFileW(extName,newName);
	DWORD dwError = b ? 0 : ::GetLastError();

	buffer.Reset();
	buffer.AddUINT8(aaFmReply);
	buffer.AddUINT32(dwError);

	m_transport->SendExact(buffer.GetBufferRd(), buffer.GetLen(), TRUE);
}

void TrFM::OnAaFolderCreateRequest()
{
	if (!m_prmFileManager) throw RLException("FileManager is forbidden");

	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks, CreateDirectoryW()

	UINT16 size = 0;
	m_transport->ReadExact(&size, 2);
	
	RLStream buffer(size); 	
	m_transport->ReadExact(buffer.GetBuffer(), size);
	buffer.SetLen(size);
	
	CStringW fullPath = CCommon::ConvToUTF16((LPCSTR)buffer.GetString0A(), size);

	BOOL b = ::CreateDirectoryW(fullPath, 0);
	DWORD dwError = b ? 0 : ::GetLastError();
	
	buffer.Reset();
	buffer.AddUINT8(aaFmReply);
	buffer.AddUINT32(dwError);
	
	m_transport->SendExact(buffer.GetBufferRd(), buffer.GetLen(), TRUE);
}

void TrFM::OnAaDeleteRequest()
{
	if (!m_prmFileManager) throw RLException("FileManager is forbidden");

	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks, DeleteDir(), DeleteFileW

	UINT32 size = 0;
	m_transport->ReadExact(&size, 4);
	
	RLStream buffer(size);
	m_transport->ReadExact(buffer.GetBuffer(), size);
	buffer.SetLen(size);
	
	CStringW currPath = CCommon::ConvToUTF16((LPCSTR)buffer.GetString1A());

	DWORD dwError = 0;
	CStringW errorPath;

	UINT16 count = 0;
	
	while (buffer.GetAvailForReading()>0) 
	{
		CStringW fullPath = currPath + CCommon::ConvToUTF16((LPCSTR)buffer.GetString1A());

		if (IsFolder(fullPath)) {
			dwError = TrFmFileSys::DeleteDir(fullPath, errorPath);
			if (dwError) break;
		}
		else {
			 if (!::DeleteFileW(fullPath)) {
				 dwError = ::GetLastError();
				 errorPath = fullPath;
				 break;
			 }
		}
		count++;
	}
	
 	buffer.Reset();
 	buffer.AddUINT8(aaFmReply);
	buffer.AddUINT32(dwError);	
	if (dwError!=0) {
		buffer.AddUINT16(count);
		buffer.AddUINT16(0);		
		CCommon::ConvToUTF8(errorPath, buffer, false);
		SetLenUINT16(buffer, 7);
	}
	
	m_transport->SendExact(buffer.GetBufferRd(), buffer.GetLen(), TRUE);
}


void TrFM::OnAaUploadRequest()
{
	if (!m_prmFileManager) throw RLException("FileManager is forbidden");

	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks: CreateRecurseDir(), CreateFileW(), GetFileSizeByName()

	UINT16 size;
	m_transport->ReadExact(&size, 2);

	RLStream buffer(size);	
	m_transport->ReadExact(buffer.GetBufferWr(), size);
	buffer.SetLen(size);

	UINT8 flags = buffer.GetUINT8(); // 0-normal, 1-overwrite, 2-continue

	FILETIME time;

	if (flags & aaURF_SetTime) {
		buffer.GetRaw(&time, sizeof(time));
	}
	else {
		time.dwLowDateTime  = time.dwHighDateTime = 0xFFFFFFFF;
	}

	INT64 offset;
	BYTE  md5Hash[16];

	RLStream bufferOut;
	bufferOut.AddUINT8(aaFmReply);

	bool queryNormal   = (flags & (aaURF_Overwrite | aaURF_TryToContinue))==0;  // it's first query
	bool queryContinue = (flags & aaURF_TryToContinue)!=0;						// try to continue uploading

	if (queryNormal)
	{
		m_fileName = CCommon::ConvToUTF16((LPCSTR)buffer.GetString0A());

		DWORD dwError = TrFmFileSys::CreateRecurseDir(m_fileName);

		if (dwError>0 || TrFmFileSys::IsFolder(m_fileName)) {
			bufferOut.AddUINT32(dwError);
			goto exit;
		}
	}
	else if (queryContinue) {
		offset = TrFmFileSys::GetFileSize(buffer);
		buffer.GetRaw(md5Hash, 16);
	}
		
	{	
		DWORD dwCreationFlags = (queryNormal) ? CREATE_NEW : OPEN_ALWAYS;
		DWORD dwDesiredAccess = GENERIC_WRITE;
		if (queryContinue) dwDesiredAccess|=GENERIC_READ;
		m_hFile = ::CreateFileW(m_fileName, dwDesiredAccess, FILE_SHARE_READ, 0, dwCreationFlags, FILE_ATTRIBUTE_ARCHIVE, 0);

		if (m_hFile==INVALID_HANDLE_VALUE) {
			DWORD dwError = ::GetLastError();
			bufferOut.AddUINT32(dwError);

			if (dwError == ERROR_FILE_EXISTS && queryNormal) {
				INT64 size = 0;

				try {
					size = TrFmFileSys::GetFileSizeByName(m_fileName);
				}
				catch(RLException& ) {}
				
				TrFmFileSys::AddFileSize(bufferOut, size);
			}
		}
		else {
			DWORD dwError = 0;

			this->OnEventFS("Write file %s", (LPCSTR)(CStringA)m_fileName);

			if (!queryNormal)
			{
				if (queryContinue) {
					dwError = ERROR_CRC;
					LARGE_INTEGER size;
					if (::GetFileSizeEx(m_hFile, &size)) {
						if (size.QuadPart<=offset) {
							BYTE  md5Hash_local[16];
							if (TrFmFileSys::GetMD5ForFile(m_hFile, offset, md5Hash_local)==0) {
								if (memcmp(md5Hash_local, md5Hash, 16)==0) dwError=0;
							}
						}
					}
				}

				if (dwError==0)
					if (::SetEndOfFile(m_hFile)==0) dwError = ::GetLastError();

				if (dwError>0)
					this->CloseFile();
			}
			bufferOut.AddUINT32(dwError);

			if (dwError==0)
				RLFileWin::SetLastWriteTime(m_hFile, &time);
		}
	}
	
exit:
	m_transport->SendExact(bufferOut.GetBuffer(), bufferOut.GetLen(), TRUE);
}


void TrFM::OnAaUploadData(bool last)
{
	if (!m_prmFileManager) throw RLException("FileManager is forbidden");

	int size = TrFmFileSys::BLOCK_SIZE;
	if (last) {
		UINT8 flag = m_transport->ReadByte();
		size = TrFmFileSys::GetFileSize(m_transport);
		ASSERT(size<=TrFmFileSys::BLOCK_SIZE);
	}

	if (m_hFile == INVALID_HANDLE_VALUE) return;

	char buffer[TrFmFileSys::BLOCK_SIZE];

	m_transport->ReadExact(buffer, size);
		
	DWORD dwError = TrFmFileSys::WriteFile(m_hFile, buffer, size);

	RLStream bufferOut(6);
	bufferOut.AddUINT8(aaUploadDataAck);

	if (dwError) {
		bufferOut.AddUINT8(1);
		bufferOut.AddUINT32(dwError);
	}
	else {
		bufferOut.AddUINT8(0);
	}
		
	m_transport->SendExact(bufferOut.GetBuffer(), bufferOut.GetLen(), TRUE);

	if (last || dwError) this->CloseFile(); 
}



void TrFM::OnAaDnloadRequest()
{
	if (!m_prmFileManager) throw RLException("FileManager is forbidden");

	UINT16 size;
	m_transport->ReadExact(&size, 2);
	
	RLStream buffer(size);
	m_transport->ReadExact(buffer.GetBuffer(), size);
	buffer.SetLen(size);
	
	UINT8 flags = buffer.GetUINT8();

	ASSERT(flags>=0 && flags<=7);

	if (flags & aaDRF_Cancel) {
		//dnload cancel
		if (m_hFile!=INVALID_HANDLE_VALUE) {
			m_bytesSent = m_fileSize; // to send last data with 0 size
			SendDnloadData(); // it should close file
		}
		return;
	}

	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks, CreateFileW()

	BYTE md5Hash_viewer[16];
	UINT64 offset = 0;

	if (flags & aaDRF_TryToContinue) {
		offset = TrFmFileSys::GetFileSize(buffer);
		buffer.GetRaw(md5Hash_viewer, 16);
	}
	
	CStringW path = CCommon::ConvToUTF16(buffer.GetString0A());

	DWORD dwError = 0;
	ASSERT(m_hFile==INVALID_HANDLE_VALUE);
	m_hFile = ::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY, 0);

	if (m_hFile==INVALID_HANDLE_VALUE) {
		dwError = ::GetLastError();
	}
	else {
		this->OnEventFS("Send file %s", (LPCSTR)(CStringA)path);

		m_fileSize  = TrFmFileSys::GetFileSize(m_hFile);
		m_bytesSent = 0;

		if (flags & aaDRF_TryToContinue)
		{
			dwError = ERROR_CRC;

			if (offset<=m_fileSize) {
				BYTE md5Hash[16];
				if (TrFmFileSys::GetMD5ForFile(m_hFile, offset, md5Hash)==0) {
					if (memcmp(md5Hash, md5Hash_viewer, 16)==0) dwError = 0;
				}
			}

			if (dwError==0) {
				m_bytesSent = offset;

				//LARGE_INTEGER offset1;
				//offset1.QuadPart = offset;
				//if (::SetFilePointerEx(m_hFile, offset1, NULL, FILE_BEGIN)==0) dwError = ERROR_CRC;
			}
		}
	}	

	buffer.Reset();
	buffer.AddUINT8(aaFmReply);
	buffer.AddUINT32(dwError);

	if (dwError==0) {
		TrFmFileSys::AddFileSize(buffer, m_fileSize);

		if (flags & aaDRF_GetTime) {
			FILETIME time;
			RLFileWin::GetLastWriteTime(m_hFile, &time);
			buffer.AddRaw(&time, sizeof(time));
		}
	}
	else {
		this->CloseFile();
	}

	m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);	

	for (int i=0; i<TrFmFileSys::INIT_BLOCKS; i++) {
		if (m_hFile==INVALID_HANDLE_VALUE) break;
		SendDnloadData();
	}
}

void TrFM::OnAaDnloadDataAck()
{
	if (!m_prmFileManager) throw RLException("FileManager is forbidden");

	if (m_hFile!=INVALID_HANDLE_VALUE) SendDnloadData();
}


void TrFM::SendDnloadData()
{
	RLStream buffer(8+TrFmFileSys::BLOCK_SIZE);

	int size = TrFmFileSys::GetNextBlockSize(m_fileSize, m_bytesSent);

	if (size == TrFmFileSys::BLOCK_SIZE) {
		buffer.AddUINT8(aaDnloadData);
	}
	else {
		buffer.AddUINT8(aaDnloadDataLast);
		buffer.AddUINT8(0);
		TrFmFileSys::AddFileSize(buffer, size);
	}

	DWORD dwError = TrFmFileSys::ReadFile(m_hFile, buffer.GetBufferWr(), size);

	if (dwError || size!=TrFmFileSys::BLOCK_SIZE) CloseFile();

	if (dwError) {
		buffer.Reset();
		buffer.AddUINT8(aaDnloadDataLast);
		buffer.AddUINT8(1);
		buffer.AddUINT32(dwError);
	}
	else {
		buffer.AddRaw(NULL, size);
		m_bytesSent += size;
	}	
	
	m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);
}

