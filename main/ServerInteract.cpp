#include "stdafx.h"
#include "ServerInteract.h"
#include "AmmyyApp.h"
#include "CmdInit.h"
#include "Common.h"
#include "Service.h"
#include "../RL/RLEvent.h"
#include "resource.h"
#include "../common/unzip/Unzip.h"
#include "ImpersonateWrapper.h"
#include "../target/TrMain.h"
#include "aaProtocol.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


CServerInteract::CServerInteract()
{
	m_bNeedComputerId = true;
}

void CServerInteract::SendInitCmd()
{
	m_updating = false;

	_log.WriteInfo("CServerInteract::SendInitCmd()#1");

	CmdInit cmd;
	{
		if (TheApp.m_ID==0)
			cmd.m_strComputerId = settings.GetComputerId(); // if it's first query
		else
			cmd.m_strComputerId.Format("%u", TheApp.m_ID);

		ImpersonateWrapper impersonate; // error 12015 occur if not used this when have proxy with authorization
		cmd.m_bService				= TheApp.m_CmgArgs.noGUI;
		cmd.m_bNeedComputerId		= m_bNeedComputerId;
		cmd.m_timeLastUpdateTrying	= settings.m_timeLastUpdateTrying;
		cmd.Send();
	}

	if (cmd.m_strStatus == "UPDATE") {
		_log.WriteInfo("CServerInteract::SendInitCmd()#3 updating to new version");
		
		m_updating = true;

		settings.m_timeLastUpdateTrying = cmd.m_timeLastUpdateTrying;
		settings.Save();
		//settings.Load(); // maximp I don't know why it was

		UpdateToNewVersion(cmd.m_strUpdateURL);
	}
	else {
		_log.WriteInfo("CServerInteract::SendInitCmd()#4");
		
		TheApp.m_ID = atol(cmd.m_strComputerId);
		settings.m_publicRouters = cmd.m_strRouters;
		settings.SetHardware(cmd.m_strComputerId, cmd.m_strLicenseType);

		//fill files
		/*
		{
			CStringA str = cmd.m_strFiles;

			for (int i1=0;;)
			{
				CStringA str1 = CCommon::GetSubString(str, "\n\n", i1);

				if (str1.IsEmpty()) break;

				int i2 = 0;

				CStringA key = CCommon::GetSubString(str1, "\n", i2);

				if (i2>str1.GetLength()) i2 = str1.GetLength();	// in case of no data

				CStringA data = str1.Mid(i2);

				if (data.IsEmpty())	// in case of no data
					throw RLException("No entries for '%s' from server", key);

				if      (key=="VIEWER:") m_ViewerFiles = data;
				else if (key=="TARGET:") m_TargetFiles = data;
			}
		}
		*/
	}

	_log.WriteInfo("CServerInteract::SendInitCmd()#6");
}


void CServerInteract::DownloadFile(LPCSTR url, LPCWSTR fileName)
{
	CCommon::DeleteFileIfExistW(fileName);

	CStringA shortFileName = CCommon::GetShortFileName(CStringA(fileName));
						
	m_downloader.m_fileName = fileName;
	m_downloader.m_url = url;
	m_downloader.Start();

	while(m_downloader.IsWorking()) 
	{
		int percents = (int)(m_downloader.m_done*100);
		double speed = m_downloader.m_speed/1024;

		CStringA msg;
		msg.Format("Downloading '%s' %d%%, %.1f Kbytes/sec", shortFileName, percents, speed);

		OnDownloadFileStatus(msg, percents);

		::Sleep(100);
	}

	OnDownloadFileStatus("", 0);
}

void CServerInteract::CopyResource(HANDLE hModuleOut, LPCSTR lpType, LPCSTR lpName)
{
	LPCVOID lpData;
	DWORD dwSize = RLResource::Load(NULL, lpType, lpName, 0, &lpData);
	if (::UpdateResourceA( hModuleOut, lpType, lpName, 0, (LPVOID)lpData, dwSize) ==FALSE)
		throw RLException("UpdateResource() error=%d", ::GetLastError());
}


void CServerInteract::UpdateResources(LPCWSTR fileName)
{
	HANDLE hModuleOut = NULL;
	BOOL fDiscard = TRUE;

	try {

		hModuleOut = ::BeginUpdateResourceW(fileName, FALSE );

		if (hModuleOut==NULL) 
			throw RLException("BeginUpdateResource() error=%d", ::GetLastError());

		CopyResource(hModuleOut, "BINARY",		MAKEINTRESOURCE(IDR_CUSTOMIZE));
		CopyResource(hModuleOut, RT_ICON,		MAKEINTRESOURCE(1));
		CopyResource(hModuleOut, RT_ICON,		MAKEINTRESOURCE(2));
		CopyResource(hModuleOut, RT_ICON,		MAKEINTRESOURCE(3));
		CopyResource(hModuleOut, RT_GROUP_ICON,	MAKEINTRESOURCE(IDR_MAINFRAME));
		
		fDiscard = FALSE;

	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}

	if (hModuleOut!=NULL) {
		if (::EndUpdateResourceW( hModuleOut, fDiscard )==FALSE) {
			_log.WriteError("EndUpdateResource() error=%d", ::GetLastError());
		}
	}
}

void CServerInteract::UpdateToNewVersion(LPCSTR url)
{
	CStringW rootFolder = TheApp.GetRootFolderW();
	CCommon::CheckOrCreateDirectoryW(rootFolder+ "_tmp");

	bool bExe;
	{
		CStringA ext = CCommon::GetFileNameExtension(CStringA(url));
		ext.MakeLower();
		bExe = (ext=="exe");
	}
	
	CStringW fileNameExe =           rootFolder + "_tmp\\AMMYY_Admin.exe";
	CStringW fileName    = (!bExe) ? rootFolder + "_tmp\\AMMYY_Admin.bin" : fileNameExe;

	DownloadFile(url, fileName);

	if (!bExe) {
		CCommon::DeleteFileIfExistW(fileNameExe);
		CUnzip::UnzipFirstFile(fileName, fileNameExe);
		CCommon::DeleteFileW(fileName);
		fileName = fileNameExe;
	}
	
	// update resources if we need it
	if (settings.m_customization.m_enabled)
	{
		UpdateResources(fileName);
	}

	CStringW exeFileName = CCommon::GetModuleFileNameW(NULL);		// current exe filename


	RLEvent event;

	if (TheApp.m_CmgArgs.noGUI)	{
		_log.WriteInfo("CServerInteract::UpdateToNewVersion()#10 stopping service");
		event.CreateOnly(AMMYY_SERVICE_CLOSE_SELF_EVENT);	// say service not waiting while this process is finished
		ServiceManager.RunCmd((HWND)-1, 2); // stop service, it should block until service it stopped
	}

	//remove current exe to temp, because we can't delete it, because it's loaded now
	{
		CStringW newFileName;
		newFileName.Format(L"%sAmmyy_%X.tmp", (LPCWSTR)CCommon::GetTempPathW(), ::GetTickCount());

		CCommon::DeleteFileIfExistW(newFileName);
		try {
			CCommon::MoveFileW(exeFileName, newFileName);
		}
		catch(RLException& ex) {
			_log.WriteError(ex.GetDescription());
		}
				
		if (CCommon::FileIsExistW(exeFileName)) {
			newFileName = exeFileName + "_bak";	// in case temp folder on other disk

			CCommon::DeleteFileIfExistW(newFileName);
			CCommon::MoveFileW(exeFileName, newFileName);

			if (CCommon::FileIsExistW(exeFileName))
				throw RLException("Couldn't remove file '%s'", (LPCSTR)(CStringA)exeFileName);
		}
	}

	//rename uploaded exe to current name
	CCommon::MoveFileW(fileName, exeFileName);

	_TrMain.Stop(true, aaCloseSession);

	if (TheApp.m_CmgArgs.noGUI)	{
		_log.WriteInfo("CServerInteract::UpdateToNewVersion()#20 starting service");
		event.Close();
		ServiceManager.RunCmd((HWND)-1, 1); // start service
	}
}
