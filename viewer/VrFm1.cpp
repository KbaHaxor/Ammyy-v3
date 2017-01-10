#include "StdAfx.h"
#include <algorithm>
#include "VrFm1.h"
#include "res/resource_fm.h"
#include "../main/aaProtocol.h"
#include "../main/DlgMain.h"
#include "../main/AmmyyApp.h"
#include "../main/ImpersonateWrapper.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


// transport-less algorithm of File Manager here

#define BUTTON_WIDTH 28
#define BTN_CANCEL 1501


//VrFmIconizer VrFmMainWindow::m_fmIconizer;

/*
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

bool GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, DWORD dwFlags, CStringW &strName)
{
	bool bSuccess = true;
	STRRET str;
	
	if (lpsf->GetDisplayNameOf(lpi,dwFlags, &str) == NOERROR) {
		LPWSTR lpStr;
		StrRetToStrW(&str, lpi, &lpStr);
		strName = lpStr;
		CoTaskMemFree(lpStr);
	}
	else
		bSuccess = false;
	
	return bSuccess;
}
*/


namespace FmComparators
{
	bool FmNameLess(const VrFmItem& i1, const VrFmItem& i2)
	{
		if (i1.type != i2.type) return (i2.type == TrFmFileSys::TYPE_FILE);
		return wcsicmp(i1.name, i2.name) < 0;
	}
	
	bool FmNameGreater(const VrFmItem& i1, const VrFmItem& i2)
	{
		if (i1.type != i2.type) return (i2.type == TrFmFileSys::TYPE_FILE);
		return wcsicmp(i1.name, i2.name) > 0;
	}

	bool FmSizeLess(const VrFmItem& i1, const VrFmItem& i2)
	{
		if (i1.type != i2.type) return (i2.type == TrFmFileSys::TYPE_FILE);
		return i1.size < i2.size;
	}
	
	bool FmSizeGreater(const VrFmItem& i1, const VrFmItem& i2)
	{
		if (i1.type != i2.type) return (i2.type == TrFmFileSys::TYPE_FILE);
		return i1.size > i2.size;
	}

	bool FmDateTimeLess(const VrFmItem& i1, const VrFmItem& i2)
	{
		if (i1.type != i2.type) return (i2.type == TrFmFileSys::TYPE_FILE);
		return ::CompareFileTime(&i1.time, &i2.time) < 0;
	}
	
	bool FmDateTimeGreater(const VrFmItem& i1, const VrFmItem& i2)
	{
		if (i1.type != i2.type) return (i2.type == TrFmFileSys::TYPE_FILE);
		return ::CompareFileTime(&i1.time, &i2.time) > 0;
	}
}

CStringW VrFmItem::GetReadbleSize() const
{			
	if (this->size == 0) return L"";

	wchar_t buff[32];
	_i64tow(this->size, buff, 10);

	int len = wcslen(buff);

	CStringW str;
	LPWSTR pBufDst1 = str.GetBuffer(64); // pre-allocate memory
	LPWSTR pBufDst = pBufDst1;
	LPWSTR pBufSrc = buff;

	while(len>0) {
		int i2 = (len % 3);
		if (i2==0) i2=3;
		memcpy(pBufDst, pBufSrc, i2*sizeof(WCHAR));
		pBufDst += i2;
		pBufSrc += i2;
		len -= i2;

		if (len>0) {
			*pBufDst++ = ' '; // add group delimiter
		}
	}
	*pBufDst = 0; // null terminated string
	str.ReleaseBuffer(pBufDst-pBufDst1);

 	//str += L"Kb";
	return str;
}

CStringW VrFmItem::GetReadbleTime() const
{
	SYSTEMTIME stUTC, stLocal;

	::FileTimeToSystemTime(&this->time, &stUTC);
	::SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
			
	wchar_t buff[48];

	wsprintfW(buff, L"%02d.%02d.%d  %02d:%02d", 
		stLocal.wDay, stLocal.wMonth, stLocal.wYear, stLocal.wHour, stLocal.wMinute);
			
	return buff;
}

CStringW VrFmItem::GetReadableAttrs() const
{
	CStringW str;

	if (this->attrs & FILE_ATTRIBUTE_READONLY) str += L"R";
	if (this->attrs & FILE_ATTRIBUTE_HIDDEN)   str += L"H";
	if (this->attrs & FILE_ATTRIBUTE_SYSTEM)   str += L"S";
	if (this->attrs & FILE_ATTRIBUTE_ARCHIVE)  str += L"A";
	return str;
}

//--------------------------------------------------------------------------------------------------------


VrFmIconizer::VrFmIconizer()
{
	m_smallList = NULL;
	m_counter = 0;
}

VrFmIconizer::~VrFmIconizer()
{
}

int VrFmIconizer::AddIcon2(HMODULE hLib, LPCSTR lpIconName)
{
	HICON hIcon = ::LoadIcon(hLib, lpIconName);
	if (hIcon==NULL) return -1;
	int i = ImageList_AddIcon(m_smallList, hIcon);
	::DestroyIcon(hIcon);
	return i;
}

void VrFmIconizer::AddRef()
{
	if (m_counter++ == 0) {
		WCHAR sysDir[MAX_PATH];

		::GetSystemDirectoryW(sysDir, MAX_PATH);

		m_icons[0] = AddIcon(sysDir, true);
		for (int i=1; i<sizeof(m_icons)/sizeof(m_icons[0]); i++) m_icons[i] = -1;

		HMODULE hLib = ::LoadLibraryA("shell32.dll");
		
		if (hLib) {
			m_icons[1] = AddIcon2(hLib, "#235");// documents
			m_icons[2] = AddIcon2(hLib, "#35");	// desktop
			m_icons[3] = AddIcon2(hLib, "#9");	// hard drive
			m_icons[4] = AddIcon2(hLib, "#12");	// cd-rom drive
			m_icons[5] = AddIcon2(hLib, "#13");	// ram drive
			m_icons[6] = AddIcon2(hLib, "#10");	// network drive
			m_icons[7] = AddIcon2(hLib, "#7");	// floppy 3.5"			
			m_icons[8] = AddIcon2(hLib, "#8");	// other removeable media, 8=233=305

			/*
			m_icons[i++] = AddIcon2(hLib, "#11");	// disconnected network drive
			m_icons[i++] = AddIcon2(hLib, "#230");	// zip drive
			m_icons[i++] = -1;
			m_icons[i++] = AddIcon2(hLib, "#234");	// no disk drive disabled
			m_icons[i++] = AddIcon2(hLib, "#258");	// network drive red thing
			m_icons[i++] = AddIcon2(hLib, "#275");	// drive use (piechart)
			m_icons[i++] = AddIcon2(hLib, "#291");	// dvd drive
			m_icons[i++] = AddIcon2(hLib, "#300");	// cassete drive
			m_icons[i++] = AddIcon2(hLib, "#301");	// smaller cassete drive
			m_icons[i++] = AddIcon2(hLib, "#312");	// jazz drive
			m_icons[i++] = AddIcon2(hLib, "#313");	// zip drive, same as before
			m_icons[i++] = AddIcon2(hLib, "#1003");	// drive ok (webview)
			*/
			
			::FreeLibrary(hLib);
		}
		else {
			_log.WriteError("LoadLibrary(shell32.dll) Error=%d", ::GetLastError());
			for (int i=1; i<sizeof(m_icons)/sizeof(m_icons[0]); i++) m_icons[i] = -1;
		}
	}
}

void VrFmIconizer::Release()
{
 	if (--m_counter == 0) {
		m_mSmallIds.clear();
		::ImageList_Destroy(m_smallList);
		m_smallList = NULL;
 	}
}

int VrFmIconizer::AddIcon(LPCWSTR pszPath, bool init)
{
	SHFILEINFOW shFileInfo = {0};
	int flags = (init) ? SHGFI_SYSICONINDEX : SHGFI_USEFILEATTRIBUTES;
	DWORD dwFileAttr = 0; //(init) ? 0 : FILE_ATTRIBUTE_NORMAL;

	HIMAGELIST hSystemList = (HIMAGELIST)::SHGetFileInfoW(pszPath, dwFileAttr, &shFileInfo, sizeof(shFileInfo), SHGFI_SMALLICON | SHGFI_ICON | flags);

	if (init) {
		int cx, cy;
		ImageList_GetIconSize(hSystemList, &cx, &cy);
		ASSERT(m_smallList==NULL);
		ASSERT(cx>0);
		ASSERT(cy>0);
		m_smallList = ::ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK, 0, 10);
	}

	int newId = ImageList_AddIcon(m_smallList, shFileInfo.hIcon);
	::DestroyIcon(shFileInfo.hIcon);
	
	return newId;
}

int VrFmIconizer::GetIconId(VrFmItem& item, bool remote)
{
	if (item.type == TrFmFileSys::TYPE_DISK && remote) 
	{
		if (item.attrs == DRIVE_FIXED)		return m_icons[3];
		if (item.attrs == DRIVE_CDROM)		return m_icons[4];
		if (item.attrs == DRIVE_RAMDISK)	return m_icons[5];
		if (item.attrs == DRIVE_REMOTE)	return m_icons[6];
		if (item.attrs == DRIVE_REMOVABLE) {
			return (item.name[0] == 'A' || item.name[0] == 'B') ? m_icons[7] : m_icons[8];
		}
		else
			return -1; // no icons for remote disks
	}

	if (item.type == TrFmFileSys::TYPE_DIR)    return m_icons[0];
	if (item.type == TrFmFileSys::TYPE_LINK_1) return m_icons[1];
	if (item.type == TrFmFileSys::TYPE_LINK_0) return m_icons[2];	

	CStringW key;

	switch(item.type)
	{
		case TrFmFileSys::TYPE_DISK:	key = item.name; break;
		case TrFmFileSys::TYPE_FILE:	key = TrFmFileSys::GetFileExt(item.name); break;

		default:
			return -1; // TYPE_DOTS or incorrect type, so no icon
	}
	
	IconIdMap::const_iterator itr = m_mSmallIds.find(key);
	if (itr != m_mSmallIds.end()) return itr->second;

	int newId = AddIcon(key, false);
	m_mSmallIds.insert(std::make_pair(key, newId));
	
	return newId;
}



// ---------------------------------------------------------------------------------


void VrFmEditDirBox::Create(HWND parentHWnd, UINT nId, HFONT hFont)
{
	const DWORD type = WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL;	
	m_hWnd = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", type, 0, 0, 0, 0, parentHWnd, (HMENU)nId, 0, 0);

	::SendMessageW(m_hWnd, WM_SETFONT, (WPARAM) hFont, 0);

	SubclassWindow(true);
}

LRESULT VrFmEditDirBox::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_KEYDOWN:
			{
				if (wParam == VK_RETURN) {
					m_pBlock->OnDirBoxEnterKeyDown();
				}
				break;
			}
	}

	return RLWndEx::WindowProc(hwnd, msg, wParam, lParam);
}


// ---------------------------------------------------------------------------------

VrFmListView::VrFmListView()
{
	m_lastSelectedIndex = -1;
	m_selectedCount = 0;
	for (int i=0; i<3; ++i) m_sortOrder[i] = true;
}

void VrFmListView::Create(HWND hwndParent)
{
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LVS_REPORT | LVS_SHAREIMAGELISTS | LVS_EDITLABELS; //| LVS_SHOWSELALWAYS;
	
	m_hWnd = ::CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", dwStyle, 0, 0, 0, 0, hwndParent, 0, 0, 0);

	m_hWndHeader = (HWND)::SendMessage(m_hWnd, LVM_GETHEADER, 0, 0);

	SetExtStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);

	ListView_SetImageList(m_hWnd, m_pBlock->m_pMainWnd->m_fmIconizer.m_smallList, LVSIL_SMALL);

	this->InsertColumn("Name", 0, 200);
	this->InsertColumn("Size", 1, 80);
	this->InsertColumn("Last time", 2, 110);
	this->InsertColumn("Attributes", 3, 80, true);
}

void VrFmListView::InsertColumn(LPCTSTR text, int index, int width, bool textLeft)
{
	LVCOLUMN lvc; 
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
	lvc.fmt = textLeft ? LVCFMT_LEFT : LVCFMT_RIGHT;
	lvc.iSubItem = index;
	lvc.pszText = (LPTSTR)text;
	lvc.cchTextMax = 10;
	lvc.cx = width;
	lvc.iOrder = index;
	ListView_InsertColumn(m_hWnd, index, &lvc);
}


void VrFmListView::OnRenaming()
{
	if (this->IsAllowedEditing()) {
		this->SetFocus();
		ListView_EditLabel(m_hWnd, m_lastSelectedIndex);
	}
}

void VrFmListView::SortItems2(int columnId)
{
	if (columnId < 0 || columnId >= 3) return;

	bool order = m_sortOrder[columnId] = !m_sortOrder[columnId];

	VrFmBlock::FmCompFunction function = NULL;

	switch (columnId) {
		case 0:	{ function = (order) ? FmComparators::FmNameLess : FmComparators::FmNameGreater; break; }
		case 1: { function = (order) ? FmComparators::FmSizeLess : FmComparators::FmSizeGreater; break;	}
		case 2: { function = (order) ? FmComparators::FmDateTimeLess : FmComparators::FmDateTimeGreater; break; }
	}

	if (function) {
		m_pBlock->SortItems(function);
		this->UpdateContent();
	}
}


bool VrFmListView::IsAllowedDirCreation() const
{
	return (m_pBlock->m_currentPath[0] != 0); // if not main page with disks
}

bool VrFmListView::IsAllowedDeletion() const
{
	if (m_pBlock->m_currentPath[0] == 0) return false; // if main page with disks

	if (m_selectedCount>1) return true;
	if (m_selectedCount<1) return false;

	return (m_lastSelectedIndex>0); // if not first element ".." was selected
}


bool VrFmListView::IsAllowedEditing() const
{
	if (m_pBlock->m_currentPath[0] == 0) return false; // if main page with disks

	if (m_selectedCount != 1) return false;  // if more that 1 elements were selected
		
	return (m_lastSelectedIndex>0); // if not first element ".." was selected
}


void VrFmListView::UpdateContent()
{
	ListView_DeleteAllItems(m_hWnd);
	LVITEMW lvItem;
	lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE; 
	lvItem.state = 0;
	lvItem.stateMask = 0;

	bool remote = (&m_pBlock->m_pMainWnd->m_R == m_pBlock);
	
	int count = m_pBlock->m_items.size();

	for (int i=0; i<count; i++) {
		VrFmItem& item = m_pBlock->m_items[i];

		const bool isDir = (item.type == TrFmFileSys::TYPE_DIR);
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		lvItem.pszText = L"";
		lvItem.iImage = m_pBlock->m_pMainWnd->m_fmIconizer.GetIconId(item, remote);
		ListView_InsertItem(m_hWnd, &lvItem);
		
		this->SetItemTextW(i, 0, item.name);
		
		if (item.type == TrFmFileSys::TYPE_FILE) {
			this->SetItemTextW(i, 1, item.GetReadbleSize());
		}
		
		if (item.type == TrFmFileSys::TYPE_FILE || item.type == TrFmFileSys::TYPE_DIR) {
			this->SetItemTextW(i, 2, item.GetReadbleTime());
			this->SetItemTextW(i, 3, item.GetReadableAttrs());
		}
	}
}

void VrFmListView::SelectPrevPath(const CStringW& prevPath)
{
	int index = 0;
	int len1 = wcslen(m_pBlock->m_currentPath);
	int len2 = prevPath.GetLength();
	int len_name = (len1==0) ? len2 : len2-len1-1;

	if (len_name>0) {
		if (wcsncmp(prevPath, m_pBlock->m_currentPath, len1)==0) 
		{
			CStringW name = prevPath.Mid(len1, len_name);
			m_pBlock->SetSelected(name);
			return;
		}
	}

	this->SetSelected(index);
}

void VrFmListView::SetSelected(int index)
{
	this->SendMessage(LVM_SETSELECTIONMARK, 0, index);

	LV_ITEM lvItem;
	lvItem.iItem = index;
	lvItem.iSubItem = 0;
	lvItem.stateMask = lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;
	lvItem.pszText = NULL;
				
	this->SendMessage(LVM_SETITEMSTATE,  index, (LPARAM) &lvItem);
	this->SendMessage(LVM_ENSUREVISIBLE, index, (LPARAM)FALSE);
}


// ---------------------------------------------------------------------------------


void VrFmStatusBar::Create(HWND parentHWnd)
{
	DWORD type = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
	DWORD exStyles = 0;
	
	m_hWnd = ::CreateWindowExW(exStyles, STATUSCLASSNAMEW, L"", type, 0, 0, 0, 0, parentHWnd, 0, 0, 0);
}

void VrFmStatusBar::SetPartWidths(const int width1, const int width2)
{
	int parts[2];
	parts[0] = width1;
	parts[1] = width2 + width1;	
	::SendMessage(m_hWnd, SB_SETPARTS, (WPARAM)2, (LPARAM)&parts);
	UpdateWindow();
}

void VrFmStatusBar::SetTextForPart(const int iPart, LPCWSTR text)
{
	::SendMessageW(m_hWnd, SB_SETTEXTW, (WPARAM)iPart, (LPARAM)text);
}

// ---------------------------------------------------------------------------------

VrFmCreateDirDlg::VrFmCreateDirDlg()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_NEWDIR_DLG);
}

VrFmCreateDirDlg::~VrFmCreateDirDlg()
{

}

BOOL VrFmCreateDirDlg::OnInitDialog()
{
	return TRUE;
}

BOOL VrFmCreateDirDlg::OnEndDialog(BOOL ok)
{
	RLWnd textWnd(::GetDlgItem(m_hWnd, IDC_DIR_NAME));

	m_dirName = textWnd.GetTextW();;
	if (ok) {
		if (m_dirName.GetLength() == 0)
			return FALSE;
		return TRUE;
	}
	else
		return TRUE;
}

// ---------------------------------------------------------------------------------

void VrFmCopy::OnInit(bool upload)
{
	m_upload = upload;
	m_activated = false;
	m_canceling = false;
	m_fileAll.end = 0;
	m_fileAll.pos = 0;
	m_fileOne.end = 0;
	m_fileOne.pos = 0;
	m_cntFilesTotal = 0;
	m_cntFilesCopied = 0;
	m_skipped = 0;
	fileName = "";

	for (TrFmFileSys::FileList::iterator it=m_fileList.begin() ; it != m_fileList.end(); it++ ) {
		m_fileAll.end += (*it).size;
		m_cntFilesTotal++;
	}

	m_pBlockR->m_dlgConfirm.Reset(upload);

	m_pMainWnd->ShowCopyControls(true);

	OnTimerInternal();

	::SetTimer(*m_pMainWnd, 1, 250, 0);
	m_timer1.Start();
}

bool VrFmCopy::TryNext()
{
	if (m_canceling || m_fileList.empty()) {
		Finish();
		return true;
	}

	m_cntFilesCopied++;
	m_fileAll.pos += m_fileOne.end; // add previous file size
	m_fileOne.pos = 0;
	m_fileOne.end = m_fileList.front().size;
	fileName      = m_fileList.front().name;

	m_fileList.pop_front();

	return false;
}


void VrFmCopy::Finish()
{
	m_fileList.clear();

	::KillTimer(*m_pMainWnd, 1);

	m_pMainWnd->ShowCopyControls(false);

	if (m_upload) {
		m_pMainWnd->m_R.TrySetNewDir();
	}
	else {
		m_pMainWnd->m_L.TrySetNewDir();
		m_pMainWnd->EnableInterface(TRUE);
	}
}

void VrFmCopy::OnCancel()
{
	if (!m_canceling) {
		CStringA str;
		str.Format("Do you want to cancel %s ?", (m_upload) ? "uploading" : "downloading");
		if (::MessageBox(*m_pMainWnd, str, "Ammyy Admin", MB_YESNO | MB_ICONQUESTION)== IDYES) {
			m_canceling = true; // user want to cancel
		}
	}
}

void VrFmCopy::OnTimer()
{
	if (!m_activated) {
		 m_activated = true;

		if (m_upload) 
			m_pBlockR->UploadNextFile();
		else
			m_pBlockR->StartDnload();
	}
	this->OnTimerInternal();
}

void VrFmCopy::OnTimerInternal()
{
	UINT64 pos = m_fileAll.pos + m_fileOne.pos;
	UINT64 transferred = pos - m_skipped;

	double sec = m_timer1.GetElapsedSeconds();
	double speed   = (double)((double)transferred)/sec;
	double speedKb = speed/1024;

	double f1 = Progress64::GetComplete(m_fileOne.pos, m_fileOne.end);
	double f2 = Progress64::GetComplete(          pos, m_fileAll.end);

	CStringA estimated;

	if (speed>0) {
		UINT64 estSeconds = (m_fileAll.end - pos) / speed; // estimated seconds

		int hour   =  estSeconds/3600;
		int minute = (estSeconds%3600)/60;
		int second =  estSeconds%60;

		if (hour>0) {
			estimated.Format("%u:%0.2u:%0.2u", hour, minute, second);
		}
		else if (minute>0) {
			estimated.Format("%u:%0.2u", minute, second);
		}
		else {
			estimated.Format("%u", second);
		}
		estimated = ", " + estimated + " s";
	}

	m_progressOne.SetPos(65535*f1);
	m_progressAll.SetPos(65535*f2);

	CStringW str;
	str.Format(L"%d of %d - %s", m_cntFilesCopied, m_cntFilesTotal, fileName);
	m_wndStatic1.SetTextW(str);	
	m_wndStatic1.Invalidate();
	m_wndStatic1.UpdateWindow();

	LPCSTR quotes = (m_upload) ? ">>>>" : "<<<<";
	LPCSTR action = (m_upload) ? "Upload" : "Download";

	char buffer[64];	
	sprintf(buffer, "%5.3f %%%s", (double)(f1*100), (LPCSTR)estimated);
	m_progressOne.SetTextA(buffer);
	sprintf(buffer, "%s %s %5.3f %% speed %5.1f Kb/s %s", quotes, action, (double)(f2*100), speedKb, quotes);
	m_progressAll.SetTextA(buffer);
}


void VrFmCopy::ReSetCurFileSize(INT64 size)
{
	// if file size was changed, make correction
	if (m_fileOne.end!=size)
	{
		m_fileAll.end += size - m_fileOne.end;
		m_fileOne.end = size;
	}
}

// ---------------------------------------------------------------------------------

VrFmBlock::VrFmBlock()
{
	m_listview.m_pBlock = m_dirBox.m_pBlock = this;
	m_currentComp = FmComparators::FmNameLess;
}



void VrFmBlock::OnItemClicked(int itemIndex)
{
	if (m_items[itemIndex].type == TrFmFileSys::TYPE_FILE) {
		if (this->IsBlockL()) {
			CStringW dir = m_currentPath;
			CStringW file = m_items[itemIndex].name;
			::ShellExecuteW(NULL, L"open", file, NULL, dir, SW_SHOW);
		}
		return;
	}

	CStringW newPath;
	LPCWSTR lpPath = m_currentPath;

	if (lpPath[0] != 0 && itemIndex==0) {			
		for (int i = wcslen(lpPath) - 1; --i >= 0; ) {
			if (lpPath[i] == L'\\') {
				wcsncpy(newPath.GetBuffer(i+1), lpPath, i+1);
				newPath.ReleaseBuffer(i+1);
				break;
			}
		}
	}
	else {
		const VrFmItem& ii = m_items[itemIndex];

		if (ii.type == TrFmFileSys::TYPE_FILE) return; // do nothing for files

		if      (ii.type == TrFmFileSys::TYPE_LINK_0) { newPath = m_linkPath[0]; }
		else if (ii.type == TrFmFileSys::TYPE_LINK_1) { newPath = m_linkPath[1]; }
		else if (ii.type == TrFmFileSys::TYPE_DISK)   { newPath = ii.name; }
		else {
			newPath = m_currentPath + ii.name + '\\'; 
		}
	}

	this->TrySetNewDir(newPath);
}



void VrFmBlock::OnListViewItemChanged(LPNMLISTVIEW pnmv)
{
	if ((pnmv->uChanged & LVIF_STATE) == 0) return;

	if ((pnmv->uNewState & LVIS_SELECTED) != 0) {
		m_listview.m_lastSelectedIndex = pnmv->iItem;
	}

	m_listview.m_selectedCount = m_listview.GetSelectedCount();

	this->SetButtonsState();
	this->SetStatusBarText();
}

void VrFmBlock::SetButtonsState()
{
	//IDB_REFRESH, IDB_TRANSMIT, IDB_RENAME, IDB_NEWDIR, IDB_DELETE

	BOOL b1 = (m_listview.IsAllowedDeletion())    ? TRUE : FALSE;
	BOOL b2 = (m_listview.IsAllowedEditing())     ? TRUE : FALSE;
	BOOL b3 = (m_listview.IsAllowedDirCreation()) ? TRUE : FALSE;

	::EnableWindow((HWND)m_btns[1], b1);
	::EnableWindow((HWND)m_btns[4], b1);
	::EnableWindow((HWND)m_btns[2], b2);
	::EnableWindow((HWND)m_btns[3], b3);			
}

void VrFmBlock::SetStatusBarText()
{
	CStringW str;

	int count = m_listview.m_selectedCount;

	if (count==0) {
		str.Format(L"%u objects", m_listview.GetItemCount());
	}
	else if (count==1) {
		const VrFmItem& ii = m_items[m_listview.m_lastSelectedIndex];

		if (ii.type == TrFmFileSys::TYPE_FILE) {
			str.Format(L"Name: %s Size: %s Changed: ", (LPCWSTR)ii.name, (LPCWSTR)ii.GetReadbleSize(), (LPCWSTR)ii.GetReadbleTime());
		}
	}

	if (str.GetLength()==0) {
		str.Format(L"%u object(s) selected", count);
	}
	
	int iPart = (&m_pMainWnd->m_L == this) ? 0 : 1;

	m_pMainWnd->m_wndStatusBar.SetTextForPart(iPart, str);
}


CStringW VrFmBlock::GetDeletionText()
{
	CStringW str;
	
	int cnt = m_listview.m_selectedCount;
	
	if (cnt > 1) {
		str.Format(L"Are you sure you want to delete %d objects?", cnt);
	}
	else {
		VrFmItem& item = m_items[m_listview.m_lastSelectedIndex];
		
		if (item.type == TrFmFileSys::TYPE_DIR)
			str = L"Are you sure you want to delete this folder with all contents?";
		else {
			str.Format(L"Are you sure you want to delete this file?\n(%s)", (LPCWSTR)(m_currentPath+item.name));
		}
	}

	return str;
}

void VrFmBlock::OnDirBoxEnterKeyDown()
{
	CStringW str = m_dirBox.GetTextW();

	int len = str.GetLength();
	if (len>0) {
		TrFmFileSys::ThisFolder(str);
	}

	this->TrySetNewDir(str);
	m_listview.SetFocus();
}

void VrFmBlock::OnKeyDown(WORD wVKey)
{
	if (wVKey==VK_RETURN) {  // if pressed Enter
		if (m_listview.m_selectedCount!=1) return;
		this->OnItemClicked(m_listview.m_lastSelectedIndex);
	}
	else if (wVKey==VK_BACK) {	
		if (m_currentPath[0] == 0) return;
		this->OnItemClicked(0);
	}
	else if (wVKey==VK_F2) {
		this->m_listview.OnRenaming();
	}

	// for future
	// else if (wVKey==VK_INSERT) {}
}

void VrFmBlock::ShowMsgBoxError(LPCWSTR lpszFormat, ...)
{
	CStringW str;

	va_list argList;
	va_start(argList, lpszFormat);
	str.FormatV(lpszFormat, argList);
	va_end(argList);

	HWND hwnd = *m_pMainWnd;

	::MessageBoxW(hwnd, str, L"Ammyy Admin", MB_ICONERROR);
}


void VrFmBlock::SetSelected(LPCWSTR itemname)
{
	int i=m_items.size()-1;
	for (; i>=0; i--) {
		if (wcsicmp(m_items[i].name, itemname) == 0) {
			m_listview.SetSelected(i);
			return; // found;
		}
	}

	m_listview.SetSelected(0); // not found
}


void VrFmBlock::SortItems(FmCompFunction compFunc)
{
	int start = 0;

	if (m_items.size()>0) {
		if (m_items[0].type == TrFmFileSys::TYPE_DOTS) start=1;
	}

	std::sort(m_items.begin()+start, m_items.end(), compFunc);

	m_currentComp = compFunc;
}


//_________________________________________________________________________________________________________


void VrFmBlockL::TrySetNewDir(LPCWSTR path)
{
	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks, LoadItemsLocaly()->VrFmLookUp()

	if (!path) path = m_currentPath;

	if (this->LoadItemsLocaly(path)) {
		CStringW prevPath = m_currentPath;
		m_currentPath = path;
		m_listview.SelectPrevPath(prevPath);
	}
			
	m_dirBox.SetTextW((m_currentPath[0] == 0) ? L"My Computer" : m_currentPath);
}

VrFmBlockL::VrFmBlockL()
{
	TrFmFileSys::FillLinks(m_linkPath);
}


class VrFmLookUp: public TrFmLookUpBase
{
public:
	VrFmLookUp(VrFmBlock::ItemVector& items, LPCWSTR path): m_items(items) {  m_path = path; Do(); }

	void OnBegin()
	{
		if (m_dwError== 0) {
			m_items.clear();
			if (m_path[0]!=0) {
				m_items.push_back(VrFmItem(L"..", TrFmFileSys::TYPE_DOTS, 0, NULL, 0));
			}
		}
		else 
			::MessageBeep(MB_ICONEXCLAMATION);
	}
	
	void OnAdd(WIN32_FIND_DATAW& fd)
	{
		TrFmFileSys::ItemType type = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TrFmFileSys::TYPE_DIR : TrFmFileSys::TYPE_FILE;
		m_items.push_back(VrFmItem(fd.cFileName, type, GetFileSize64(fd), &fd.ftLastWriteTime, fd.dwFileAttributes));
	}

	void OnAddDrive(LPCWSTR path)
	{
		m_items.push_back(VrFmItem(path, TrFmFileSys::TYPE_DISK, 0, NULL, 0));
	}

	void OnEndDrives()
	{
		m_items.push_back(VrFmItem(L"My Desktop",   TrFmFileSys::TYPE_LINK_0, 0, NULL, 0));
		m_items.push_back(VrFmItem(L"My Documents", TrFmFileSys::TYPE_LINK_1, 0, NULL, 0));
	}

private:
	VrFmBlock::ItemVector& m_items;
};


bool VrFmBlockL::LoadItemsLocaly(LPCWSTR path)
{
	VrFmLookUp lookup(m_items, path);
	if (lookup.m_dwError>0) return false;

	SortItems(m_currentComp);
	m_listview.UpdateContent();
	return true;
}


void VrFmBlockL::CreateDirectory(LPCWSTR dirName)
{
	if (dirName == 0 || dirName[0] == 0) return;

	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks, CreateDirectoryW(), LoadItemsLocaly()
	
	CStringW fullPath = m_currentPath + dirName;
	if (!::CreateDirectoryW(fullPath, 0)) {
		this->ShowMsgBoxError(L"ERROR %d while creating local folder '%s'", ::GetLastError(), (LPCWSTR)fullPath);
	}

	this->LoadItemsLocaly(m_currentPath);
	this->SetSelected(dirName);
}

bool VrFmBlockL::RenameItem(LPCWSTR extName, LPCWSTR newName)
{
	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks, MoveFileW()

	if (!::MoveFileW(m_currentPath + extName,m_currentPath + newName)) {
		this->ShowMsgBoxError(L"Error %d while local renaming '%s' to '%s'", ::GetLastError(), extName, newName);
		return false;
	}
	
	// fast version, need to resort
	//m_items[m_listview.m_lastSelectedIndex].SetName(newName);
	//return true; // for listview change name by self
	
	this->LoadItemsLocaly(m_currentPath);
	this->SetSelected(newName);
	return false;
}


void VrFmBlockL::DeleteSelectedItems()
{
	HWND hWnd = (HWND)m_listview;

	ImpersonateWrapper impersonate(settings.m_impersonateFS); // for network disks, DeleteDir(), DeleteFileW()

	int first_index = -1;

	for (int i = -1 ; (i = ListView_GetNextItem(hWnd, i, LVNI_SELECTED)) != -1; ) {
		if (first_index<0) first_index = i;
		CStringW fullPath = m_currentPath + m_items[i].name;
		int type = m_items[i].type;

		DWORD dwError = 0;
			
		if      (type == TrFmFileSys::TYPE_DIR)  {
			CStringW path;
			dwError = TrFmFileSys::DeleteDir(fullPath + '\\', path);
		}
		else if (type == TrFmFileSys::TYPE_FILE) {
			if (!::DeleteFileW(fullPath)) dwError = ::GetLastError();
		}
		else continue;
			
		if (dwError==0) {
			ListView_DeleteItem(hWnd, i);
			m_items.erase (m_items.begin()+i);
			i--;
		}
		else {
			this->ShowMsgBoxError(L"ERROR %d while local deleting '%s'", dwError, (LPCWSTR)fullPath);
			break;
		}
	}

	if (first_index>0) {
		if (first_index>=m_items.size()) first_index = m_items.size()-1;
		m_listview.SetSelected(first_index);
	}
}



// ---------------------------------------------------------------------------------

VrFmMainWindow::VrFmMainWindow()
{	
	m_wndMain = 0;
	m_R.m_pMainWnd = this;
	m_L.m_pMainWnd = this;
	m_copy.m_pBlockR = &m_R;
	m_copy.m_pMainWnd = this;
	m_copy.m_visible = false;
	m_R.m_copy = &m_copy;

	for (int i=0; i<COUNTOF(m_hBitmaps); i++) m_hBitmaps[i] = NULL;
}

VrFmMainWindow::~VrFmMainWindow()
{
	if (m_hWnd) 
	{
		//if ((HWND)m_R.m_dlgCopy) {
			//m_R.m_dlgCopy.cancel = true;
		//	::SendMessage(m_R.m_dlgCopy, WM_COMMAND, IDOK, 0);
		//}

		::DestroyWindow(m_hWnd);
		m_fmIconizer.Release();
		::DestroyCursor(m_curNormal);
		::DestroyCursor(m_curSized);
	}

	for (int i=0; i<COUNTOF(m_hBitmaps); i++) {
		if (m_hBitmaps[i]!=NULL) ::DeleteObject(m_hBitmaps[i]);
	}
}

void VrFmMainWindow::Create(const CStringW& caption)
{
	m_splitterCatched = false;
	m_splitterWidth = 5;
	m_lastSplitterX = 0;
	m_layoutKoef = 0.5;
	m_cx = m_cy = m_cy2 = 0;	
	m_sizedCursor = false;

	const LPCWSTR FM_WINDOW_CLASSNAME  = L"AMMYY_VRFM";	

	ASSERT(DlgMain::m_pObject!=NULL);

	WNDCLASSEXW wcex;
	wcex.cbSize         = sizeof (wcex);
	wcex.style          = 0;
	wcex.lpfnWndProc    = (WNDPROC)RLWndEx::WindowProcStatic;
	wcex.lpszClassName  = FM_WINDOW_CLASSNAME;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 32;
	wcex.hInstance      = TheApp.m_hInstance;
	wcex.hIcon          = DlgMain::m_pObject->m_hIconBig;
	wcex.hIconSm        = DlgMain::m_pObject->m_hIconSmall;
	wcex.hCursor        = ::LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = _GUICommon.m_brush[3]; // (HBRUSH)COLOR_WINDOW;
	wcex.lpszMenuName   = 0;
	
	::RegisterClassExW(&wcex);
	
	DWORD exStyle = WS_EX_APPWINDOW | WS_EX_NOPARENTNOTIFY;
	DWORD type = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	
	m_hWnd = ::CreateWindowExW(exStyle, FM_WINDOW_CLASSNAME, L"", type, 0, 0, 0, 0, 0, 0, TheApp.m_hInstance, 0);

	::SetWindowLong(m_hWnd, GWL_USERDATA, reinterpret_cast<LONG>(this));

	m_fmIconizer.AddRef();
	m_curNormal =::LoadCursor(NULL, IDC_ARROW);
	m_curSized  =::LoadCursor(NULL, IDC_SIZEWE);

	static LPCSTR btnTooltips[5] = {
		"Refresh file list - Ctrl+R",
		"Copy selected object(s) - F5", 
		"Rename selected object - F2",
		"Create new folder - Ctrl+N",
		"Delete selected object(s) - Del"
	};

	static const int btnIDBs[5] = {IDB_REFRESH, IDB_TRANSMIT, IDB_RENAME, IDB_NEWDIR, IDB_DELETE};
	static const int btnIDs [5] = {BTNID_REFRESH, BTNID_SEND, BTNID_RENAME, BTNID_CREATEFOLDER, BTNID_DELETE};
	for (int i = 0; i < 5; ++i) {
		HBITMAP hBitmap0 = (HBITMAP)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(btnIDBs[i]), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		HBITMAP hBitmap1 = (HBITMAP)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(btnIDBs[i]), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		RLToolTipButton::MakeBWBitmap(hBitmap1);

		CStringW toolTipText = btnTooltips[i];

		RLToolTipButton* pButton = &m_L.m_btns[i];		
		pButton->Create(0, 0, BUTTON_WIDTH, BUTTON_WIDTH, m_hWnd, btnIDs[i]);
		pButton->SetToolTipTextW(toolTipText);
		pButton->InitImage2(hBitmap0, hBitmap1, false, false);

		pButton = &m_R.m_btns[i];
		pButton->Create(0, 0, BUTTON_WIDTH, BUTTON_WIDTH, m_hWnd, btnIDs[i]+1);
		pButton->SetToolTipTextW(toolTipText);
		pButton->InitImage2(hBitmap0, hBitmap1, false, false);

		// save them to delete on destructor
		m_hBitmaps[i*2+0] = hBitmap0;
		m_hBitmaps[i*2+1] = hBitmap1;
	}
	
	m_wndStatusBar.Create(m_hWnd);

	m_L.m_listview.Create(m_hWnd);
	m_R.m_listview.Create(m_hWnd);

	m_L.m_dirBox.Create(m_hWnd, EDITID_LOCAL,  m_L.m_listview.GetFont());
	m_R.m_dirBox.Create(m_hWnd, EDITID_REMOTE, m_R.m_listview.GetFont());

	// TODO: just for testing
	//m_L.m_currentPath = L"C:\\2\\"; //\L"C:\\1\\";
	//m_R.m_currentPath = L"C:\\1\\";
		
	m_L.TrySetNewDir();
	m_R.TrySetNewDir();

	m_L.SetStatusBarText();
	m_R.SetStatusBarText();

	//m_L.m_listview.SetFocus();
	//m_L.m_listview.SetSelectionMark(-1);

	m_R.SetButtonsState();
	m_L.SetButtonsState();

	// Centre and make 75% of full screen
	{
		const int scrCX = ::GetSystemMetrics(SM_CXSCREEN);
		const int scrCY = ::GetSystemMetrics(SM_CYSCREEN);
		int cx = scrCX*3/4;
		int cy = scrCY*3/4;
		int x = (scrCX - cx) / 2;
		int y = (scrCY - cy) / 2;
		::SetWindowPos(m_hWnd, 0, x, y, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	}

	CStringW str = caption + " - File Explorer";
	::SetWindowTextW(m_hWnd, str);

	// create copy dialog controls
	{	
		CTextProgressCtrl* pWnds[] = { &m_copy.m_progressOne, &m_copy.m_progressAll };

		// create progress bars
		for (int i=0; i<COUNTOF(pWnds); i++) 
		{
			CTextProgressCtrl& wnd = *pWnds[i];
			
			DWORD style = WS_CHILD;
			wnd.CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"", style, 0, 0, 0, 0, m_hWnd, (HMENU)12, 0, 0);
			wnd.m_dwTextStyle = DT_CENTER | DT_VCENTER | DT_SINGLELINE;		
			wnd.SetRange32(0, 65535);
			wnd.m_useParentFont = false;
			wnd.SubclassWindow();
		}

		HFONT hFont = m_L.m_listview.GetFont();		
		
		m_copy.m_btnCancel.CreateWindowExW(0, WC_BUTTONW, L"Cancel", WS_CHILD, 0, 0, 0, 0, m_hWnd, (HMENU)BTN_CANCEL, 0, 0);
		m_copy.m_wndStatic1.CreateWindowExW(WS_EX_TRANSPARENT, WC_STATICW, L"", WS_CHILD, 0, 0, 0, 0, m_hWnd, (HMENU)0, 0, 0);
		m_copy.m_wndStatic1.SetFont(hFont, 0);
	}
}

void VrFmMainWindow::EnableInterface(BOOL enable)
{ 	
	::EnableWindow((HWND)m_L.m_listview, enable);
	::EnableWindow((HWND)m_R.m_listview, enable);
 	::EnableWindow((HWND)m_L.m_dirBox, enable);
 	::EnableWindow((HWND)m_R.m_dirBox, enable);

	if (enable) {
		::EnableWindow((HWND)m_L.m_btns[0], TRUE);
		::EnableWindow((HWND)m_R.m_btns[0], TRUE);
		m_L.SetButtonsState();
		m_R.SetButtonsState();
		m_R.m_listview.SetFocus();
	}
	else {
 		for (int i = 0; i < 5; ++i) {
 			::EnableWindow((HWND)m_L.m_btns[i], FALSE);
 			::EnableWindow((HWND)m_R.m_btns[i], FALSE);
 		}
	}
}

bool VrFmMainWindow::IsCopyAllow()
{
	return (m_R.m_currentPath[0] && m_L.m_currentPath[0]); // left & right panels should be not root
}

void VrFmMainWindow::ShowCopyControls(bool visible)
{	
	m_copy.m_visible = visible;

	OnSize(m_cx, m_cy);

	m_copy.m_progressAll.ShowWindow1(visible);
	m_copy.m_progressOne.ShowWindow1(visible);
	m_copy.m_wndStatic1.ShowWindow1(visible);
	m_copy.m_btnCancel.ShowWindow1(visible);
}

void VrFmMainWindow::OnKeyDown(VrFmBlock* pBlock, WORD wVKey)
{
	switch(wVKey) {					
		case VK_DELETE: OnBtnDelete(pBlock); break;
		case 'R':
		case 'N':
			{
				if ((::GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
					if (wVKey=='N') 
						OnBtnCreateDir(pBlock); // Ctrl+N
					else
						OnBtnRefresh(pBlock);   // Ctrl+R

				}
				break;
			}						

		case VK_TAB:
			{
				if      (pBlock == &m_L) m_R.m_listview.SetFocus();
				else if (pBlock == &m_R) m_L.m_listview.SetFocus();
				break;
			}
		case VK_F5:
			{								
				if (pBlock == &m_L) m_R.OnBtnUpload();
				else				m_R.OnBtnDnload();				
				break;
			}
		default:
			pBlock->OnKeyDown(wVKey);
			break;
	}
}

LRESULT VrFmMainWindow::WindowProc(HWND , UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_NOTIFY:
		{
			LPNMHDR nmhdr = (LPNMHDR)lParam;

			VrFmBlock* pBlock = NULL;
			if      (nmhdr->hwndFrom == m_L.m_listview || nmhdr->hwndFrom == m_L.m_listview.m_hWndHeader) pBlock=&m_L;
			else if (nmhdr->hwndFrom == m_R.m_listview || nmhdr->hwndFrom == m_R.m_listview.m_hWndHeader) pBlock=&m_R;


			if (nmhdr->code == NM_DBLCLK) {				
				int iItem = ((LPNMITEMACTIVATE)lParam)->iItem;
				if (iItem >= 0 && pBlock) {
					pBlock->OnItemClicked(iItem);
				}
			}
			else if (nmhdr->code == LVN_ITEMCHANGED) {
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
				if (pBlock) pBlock->OnListViewItemChanged(pnmv);				
			}
			else if (nmhdr->code == LVN_KEYDOWN) {
				LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN)lParam;
				if (!pBlock) return 0;

				OnKeyDown(pBlock, pnkd->wVKey);
			}
			else if (nmhdr->code == LVN_BEGINLABELEDITW) {
				if (!pBlock) return 1;
				return (pBlock->m_listview.IsAllowedEditing()) ? 0 : 1;
			}
			else if (nmhdr->code == LVN_ENDLABELEDITW)
			{
				LPNMLVDISPINFOW pdi = (LPNMLVDISPINFOW)lParam;
				if (!pBlock) return 0;

				ASSERT(pdi->item.iItem==pBlock->m_listview.m_lastSelectedIndex);

				LPCWSTR extName = pBlock->m_items[pBlock->m_listview.m_lastSelectedIndex].name;
				LPCWSTR newName = pdi->item.pszText;

				if (newName==NULL || newName[0]==0) return 0;
				if (extName==NULL || extName[0]==0) return 0;
				if (wcscmp(extName, newName) == 0)  return 0;

				return pBlock->RenameItem(extName, pdi->item.pszText);
			}
			else if (nmhdr->code == HDN_ITEMCLICKW) {
				LPNMHEADER phdr = (LPNMHEADER)lParam;
				if (phdr->iButton == 0) { // left button
					if (pBlock) pBlock->m_listview.SortItems2(phdr->iItem);
				}
				return 0;
			}
			break;
		}
		case WM_CLOSE:	
		{ 
			if (m_copy.m_visible) {
				m_copy.OnCancel();
			}
			else {
				if (m_wndMain==0) {
					::ShowWindow(m_hWnd, SW_HIDE);
				}
				else {
					::PostMessage(m_wndMain, WM_CLOSE, 0, 0); // if FM only, close main window
				}
			}
			return 0; 
		}
		case WM_SIZE:		 { OnSize     (LOWORD(lParam), HIWORD(lParam)); return 0; }
		case WM_LBUTTONDOWN: { OnMouseDown(LOWORD(lParam), HIWORD(lParam), wParam); return 0; }
		case WM_LBUTTONUP:   { OnMouseUp  (LOWORD(lParam), HIWORD(lParam), wParam); return 0; }
		case WM_MOUSEMOVE:   { OnMouseMove(LOWORD(lParam), HIWORD(lParam), wParam); return 0; }
		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO mmi = (LPMINMAXINFO)lParam;
				mmi->ptMinTrackSize.x = 12 * (BUTTON_WIDTH + 5) + 20;
				mmi->ptMinTrackSize.y = 120;
				return 0;
			}
		case WM_DRAWITEM:
			{
				LPDRAWITEMSTRUCT pDrawIS = (LPDRAWITEMSTRUCT)lParam;
				for (int i = 0; i < 5; ++i) {
					if ((HWND)m_L.m_btns[i] == pDrawIS->hwndItem) { m_L.m_btns[i].DrawItem(pDrawIS); break; }
					if ((HWND)m_R.m_btns[i] == pDrawIS->hwndItem) { m_R.m_btns[i].DrawItem(pDrawIS); break; }
				}
				return 0;
			}
		case WM_COMMAND:
			{
				UINT nfCode = HIWORD(wParam);
				if (nfCode == BN_CLICKED) {
					switch (LOWORD(wParam)) {
						case BTNID_REFRESH  :		{ OnBtnRefresh(&m_L); return 0; }
						case BTNID_REFRESH+1:		{ OnBtnRefresh(&m_R); return 0; }
						case BTNID_RENAME:			{ m_L.m_listview.OnRenaming(); return 0; }
						case BTNID_RENAME+1:		{ m_R.m_listview.OnRenaming(); return 0; }
						case BTNID_CREATEFOLDER:	{ OnBtnCreateDir(&m_L); return 0; }
						case BTNID_CREATEFOLDER+1:	{ OnBtnCreateDir(&m_R); return 0; }
						case BTNID_DELETE:			{ OnBtnDelete(&m_L); 	return 0; }
						case BTNID_DELETE+1:		{ OnBtnDelete(&m_R);	return 0; }
						case BTNID_SEND:			{ m_R.OnBtnUpload();	return 0; }
						case BTNID_SEND+1:			{ m_R.OnBtnDnload();	return 0; }
						case BTN_CANCEL:			{ m_copy.OnCancel();	return 0; }
					}
				}

				break;
			}
		case WM_TIMER:
			{
				m_copy.OnTimer();
				break;
			}
		case WM_SYSCOMMAND:
			{
				/*
				if (wParam==SC_RESTORE) 
				{
					if ((HWND)m_R.m_dlgCopy) {
						// on Vista & Win7 bug here, do not restore
						::ShowWindow(m_hWnd, SW_RESTORE);
						//::ShowWindow(m_R.m_dlgCopy, SW_RESTORE);
						//::ShowOwnedPopups(m_R.m_dlgCopy, SW_RESTORE);
						//::ShowOwnedPopups(m_hWnd, SW_RESTORE);
						return 0;
					}
				}
				*/
				break;				
			}
		case WM_CTLCOLORSTATIC: 
			{
				HDC hdc = (HDC)wParam;
				HWND hwnd = (HWND)lParam;
		
				if (hwnd==(HWND)m_copy.m_wndStatic1) 
				{
					::SetBkMode(hdc, TRANSPARENT);
					return (LRESULT)_GUICommon.m_brush[3];
				}
				break;
			}
		case WM_SERVER_REPLY:
			{
				switch (wParam) {
					case aaUploadRequest :		{ m_R.OnAaUploadRequest2(); break; }
					case aaUploadDataAck :		{ m_R.OnAaUploadDataAck2(); break; }
					case aaRenameRequest:		{ m_R.OnAaRenameRequest2(); break; }
					case aaDeleteRequest:		{ m_R.OnAaDeleteRequest2(); break; }
					case aaFolderCreateRequest: { m_R.OnAaFolderCreateRequest2(); break; }
					case aaFileListRequest:		{ m_R.OnAaFileListRequest2(); break; }
					case aaDnloadData:			{ m_R.OnAaDnloadData2(); break; }
					case aaDnloadRequest:		{ m_R.OnAaDnloadRequest2(); break; }
				}
				return 0;
			}
	}

	return ::DefWindowProcW(m_hWnd, msg, wParam, lParam);
}

void VrFmMainWindow::OnSize(int cx, int cy)
{
	if (cx<=0 || cy<=0) return;

	m_cx = cx;
	m_cy = cy;

	::MoveWindow(m_wndStatusBar, 0, 0, cx, cy, TRUE);

	SIZE rc1 = m_wndStatusBar.GetClientSize();
	m_cy2 = m_cy - rc1.cy; // height of status bar

	if (m_copy.m_visible) m_cy2-=105;

	RecalcLayout(int((double)cx * m_layoutKoef));

	if (m_copy.m_visible) // resize copy dialog
	{
		const int CX_CANCEL = 90;
		const int CY_CANCEL = 20;
		const int CY_PROGRESS = 22;

		//int y1 = 10;
		int y1 = m_cy2 + 10;
		int y2 = y1 + CY_CANCEL + 8;
		int y3 = y2 + CY_PROGRESS + 8;

		int x1 = 2;
		int cx1 = m_cx-x1*2;
		int x2 = 5;
		int cx2 = m_cx-x2*2;
		int cy = 20;
		int x3 = m_cx-x1-CX_CANCEL;

		RLWnd::SetWindowPos(m_copy.m_btnCancel, x3, y1, CX_CANCEL,        CY_CANCEL);
		RLWnd::SetWindowPos(m_copy.m_wndStatic1,   x2, y1+4, cx2-CX_CANCEL-10, CY_CANCEL-4);
		RLWnd::SetWindowPos(m_copy.m_progressOne, x1, y2, cx1, CY_PROGRESS);
		RLWnd::SetWindowPos(m_copy.m_progressAll, x1, y3, cx1, CY_PROGRESS);
	}	
}


void VrFmMainWindow::OnMouseDown(int x, int y, UINT flags)
{
	if (m_sizedCursor && (flags & MK_LBUTTON)) {
		m_splitterCatched = true;
		::SetCapture(m_hWnd);
	}
}

void VrFmMainWindow::OnMouseUp(int x, int y, UINT flags)
{
	if (m_splitterCatched) {
		m_splitterCatched = false;
		::ReleaseCapture();
	}
}

void VrFmMainWindow::OnMouseMove(int x, int y, UINT flags)
{
	m_sizedCursor = (y>=35 && y<m_cy2 && x>=(m_lastSplitterX-2) && x<=(m_lastSplitterX+m_splitterWidth+2));

	::SetCursor( m_sizedCursor ? m_curSized : m_curNormal);

	if (m_splitterCatched) {
		POINT pt;
		::GetCursorPos(&pt);
		::ScreenToClient(m_hWnd, &pt);
		RecalcLayout(pt.x, true);
	}
}

void VrFmMainWindow::OnBtnRefresh(VrFmBlock* pBlock)
{
	pBlock->TrySetNewDir();
}

void VrFmMainWindow::OnBtnDelete(VrFmBlock* pBlock)
{
	if (pBlock->m_listview.IsAllowedDeletion()) {
		if (::MessageBoxW(m_hWnd, pBlock->GetDeletionText(), L"Delete...", MB_YESNO | MB_ICONQUESTION) == IDYES) {
			pBlock->DeleteSelectedItems();
		}
		pBlock->m_listview.SetFocus();
	}
}

void VrFmMainWindow::OnBtnCreateDir(VrFmBlock* pBlock)
{
	if (pBlock->m_listview.IsAllowedDirCreation()) {
		VrFmCreateDirDlg dlg;
		if (dlg.DoModal(m_hWnd)==IDOK) {
			pBlock->CreateDirectory(dlg.m_dirName);
		}
		pBlock->m_listview.SetFocus();
	}
}


void VrFmMainWindow::RecalcLayout(const int splitterX, const bool changeKoef)
{
	int y  = 0;
	int cy = m_cy2;

	ASSERT(m_cx>0 && cy>0);

	const UINT uFlagsDir = SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER;
	const UINT uFlagsBtn = SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE;	

	int minSpace = 5 * (BUTTON_WIDTH + 3);

	if ((splitterX - m_splitterWidth / 2) < minSpace)		m_lastSplitterX = minSpace;
	else if (splitterX + m_splitterWidth > m_cx - minSpace)	m_lastSplitterX = m_cx - m_splitterWidth - minSpace;
	else if (changeKoef)									m_lastSplitterX = splitterX - m_splitterWidth/2;
	else													m_lastSplitterX = splitterX;
	
	int halfWidth = m_lastSplitterX + m_splitterWidth/2 + 1;

	const int BORDER = 2;

	int xL  = BORDER;
	int xR  = m_lastSplitterX + m_splitterWidth;
	int cxL = m_lastSplitterX - xL;
	int cxR = m_cx - xR - BORDER-1;
	int xL_btn = xL + cxL - (BUTTON_WIDTH*5 + 3*4);
	int xR_btn = xR + cxR - (BUTTON_WIDTH*5 + 3*4);
	
	::SetWindowPos(m_L.m_dirBox,   0, xL, y+7,  xL_btn - xL - 10, 20, uFlagsDir);
	::SetWindowPos(m_R.m_dirBox,   0, xR, y+7,  xR_btn - xR - 10, 20, uFlagsDir);
	::SetWindowPos(m_L.m_listview, 0, xL, y+35, cxL, cy - 35+1,     uFlagsDir);
	::SetWindowPos(m_R.m_listview, 0, xR, y+35, cxR, cy - 35+1,     uFlagsDir);
	
	for (int i=0; i<5; ++i) {
		::SetWindowPos(m_L.m_btns[i], 0, xL_btn, y+3, 0, 0, uFlagsBtn); m_L.m_btns[i].Invalidate(TRUE);
		::SetWindowPos(m_R.m_btns[i], 0, xR_btn, y+3, 0, 0, uFlagsBtn); m_R.m_btns[i].Invalidate(TRUE);
		xL_btn += BUTTON_WIDTH + 3;
		xR_btn += BUTTON_WIDTH + 3;
	}

	cxL += BORDER + m_splitterWidth/2 + 1;

	m_wndStatusBar.SetPartWidths(cxL, m_cx-cxL);
 
	if (changeKoef)
		m_layoutKoef = (double)m_lastSplitterX / (double)m_cx;
}

