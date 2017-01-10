#include "stdafx.h"
#include "DlgPermissionList.h"
#include "DlgOperatorPermissions.h"
#include "resource.h"
#include <algorithm>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

int DlgPermissionList::InsertComputerRow(int item, UINT32 computerID)
{
	char temp[64];
	if (computerID>0)
		sprintf(temp,"%u",computerID);
	else
		strcpy(temp, "ANY");


	LVITEM LvItem;
	memset(&LvItem, 0, sizeof(LVITEM));

	LvItem.mask=LVIF_TEXT|LVIF_STATE;   // Text Style
	LvItem.pszText= temp;
	LvItem.iItem=item;          // choose item  
	LvItem.iSubItem=0;       // Put in first coluom
	LvItem.iImage = -1;
	LvItem.state = 0;

	return m_pcList.SendMessage(LVM_INSERTITEM,0,(LPARAM)&LvItem);
}


DlgPermissionList::DlgPermissionList()
{
	m_selectedComputerIndex = -1;
	m_lpTemplateName = MAKEINTRESOURCE(IDD_PERMISSION_LIST);
}

DlgPermissionList::~DlgPermissionList()
{
}


BOOL DlgPermissionList::OnInitDialog()
{
	this->SetTextW(L"Ammyy - " + rlLanguages.GetValue(D_ACCESS_PERMISSIONS));

	::SetDlgItemTextW(m_hWnd, IDC_ADD,				rlLanguages.GetValue(D_ADD));
	::SetDlgItemTextW(m_hWnd, IDC_EDIT,				rlLanguages.GetValue(D_EDIT));
	::SetDlgItemTextW(m_hWnd, IDC_REMOVE,			rlLanguages.GetValue(D_REMOVE));
	::SetDlgItemTextW(m_hWnd, IDC_PROTECT_SETTINGS,	rlLanguages.GetValue(D_PROTECT_SETTINGS));	

	m_pcList.AttachDlgItem(m_hWnd, IDC_PC_LIST);
	m_pcList.SetExtStyle(LVS_EX_FULLROWSELECT); //|LVS_EX_SUBITEMIMAGES);

	m_panel.Create(m_hWnd, 253,7);

	m_wProtectSettings.AttachDlgItem(m_hWnd, IDC_PROTECT_SETTINGS);
#ifdef CUSTOMIZER
	::ShowWindow(m_wProtectSettings, SW_HIDE);
#else
	m_wProtectSettings.SetCheckBool(settings.m_protectPermissions);
#endif

	LVCOLUMN lvCol;
	memset(&lvCol,0,sizeof(lvCol));

	lvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;    // Type of mask
	lvCol.cx=80;
	lvCol.fmt = LVCFMT_CENTER;

	LPCSTR columnsName[] = {"computer ID", "Password", "Screen", "Files"};

	for (int i=0; i<sizeof(columnsName)/sizeof(columnsName[0]); i++) {
		lvCol.pszText = (LPSTR)columnsName[i];
		m_pcList.SendMessage(LVM_INSERTCOLUMN,i,(LPARAM)&lvCol);
	}

/*
	HIMAGELIST hImageList = ::ImageList_Create(16, 16, ILC_COLOR32+ILC_MASK, 5, 2);

	HICON hIcon = ::LoadIcon((HINSTANCE)0x400000, MAKEINTRESOURCE(IDR_MAINFRAME));

	int i1 = ::ImageList_ReplaceIcon(hImageList, -1, hIcon);
	int i2 = ::ImageList_ReplaceIcon(hImageList, -1, hIcon);

	//::SendMessage(m_pcList.m_hWnd, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)hImageList);
	::SendMessage(m_pcList.m_hWnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hImageList);
	//::SendMessage(m_pcList.m_hWnd, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)hImageList);
*/

	{
		RLMutexLock l(settings.m_permissions.m_lock);
		m_items = settings.m_permissions.m_items;
	}
	FillListView(-1);
	OnChangedSelectedComputer(-1);

	return FALSE;
}


void DlgPermissionList::FillListView(int index)
{
	m_pcList.DeleteAllItems();

	int n = m_items.size();

	for (int i=0; i<n; i++ ) {
		UINT32 id = m_items[i].m_id;

		int index = InsertComputerRow(i, id);
		FillRow(i);
	}
	if (index>=0) {
		m_pcList.SetSelectionMark(index);
		m_pcList.EnsureVisible(index);
	}
	m_pcList.SetFocus();
}

void DlgPermissionList::FillRow(int index)
{
	//LPCWSTR pass = (m_items[index].m_password.IsEmpty()==FALSE) ? L"\x25CF\x25CF\x25CF" : L"";
	//m_pcList.SetItemTextW(index, 0+1, pass);

	bool emptyPassword = m_items[index].m_password.IsEmpty();

	SetPermission(index, 0, !emptyPassword);
	SetPermission(index, 1, m_items[index].Get(Permission::ViewScreen));
	SetPermission(index, 2, m_items[index].Get(Permission::FileManager));
}


BOOL DlgPermissionList::OnEndDialog(BOOL ok)
{
	if (ok) {
		settings.m_protectPermissions = m_wProtectSettings.GetCheckBool();

		std::sort(m_items.begin(), m_items.end(), Settings::Permissions::Comparator);	
		{
			RLMutexLock l(settings.m_permissions.m_lock);
			settings.m_permissions.m_items = m_items;
		}
		settings.Save();
	}
	return TRUE;
}

int DlgPermissionList::FindKey(const Permission& v, int* where)
{
	int index = -1;
	DWORD computerID = v.m_id;

	int n = m_items.size();	 // m_items is sorted !
	for (int i=0; i<n; i++) 
	{
		if (v.IsSameKey(m_items[i])) {
			return i; // found
		}
		else if (m_items[i].m_id>computerID) {
			break;
		}
	}

	if (where) *where = i; // hint where to insert to avoid sorting
	return -1; // not found;
}

int DlgPermissionList::GetSelectedItem()
{
	//if (m_pcList.GetSelectedCount()==0) return -1;
	//return m_pcList.GetSelectionMark();
	return m_selectedComputerIndex;
}



void DlgPermissionList::OnBtnAdd()
{
	DlgOperatorPermissions dlg(false);
	dlg.m_permission.GrandAll();
	if (dlg.DoModal(m_hWnd) != IDOK) return;	

	int where;
	int index = FindKey(dlg.m_permission, &where);

	// not found, so adding
	if (index==-1) {
		m_items.insert(m_items.begin()+where, dlg.m_permission);
		index = where;
	}
	else
		m_items[index] = dlg.m_permission; // if found than replace

	FillListView(index);
	//OnChangedSelectedComputer(index);
}

void DlgPermissionList::OnBtnEdit()
{
	int index = GetSelectedItem();
	if (index<0) return;

	DlgOperatorPermissions dlg(true);
	dlg.m_permission = m_items[index];
	if (dlg.DoModal(m_hWnd) != IDOK) return;

	bool keychanged = !m_items[index].IsSameKey(dlg.m_permission);

	if (keychanged) {
		int index_duplicate = FindKey(dlg.m_permission, NULL);
		if (index_duplicate>=0) {
			::MessageBox(m_hWnd, "Entry with new ID and password is already existed, but it will be replaced", "Ammyy Admin", MB_ICONWARNING);
			m_items.erase(m_items.begin()+index_duplicate);
			if (index_duplicate<index) index--;
		}
	}

	m_items[index] = dlg.m_permission;

	if (keychanged) {
		std::sort(m_items.begin(), m_items.end(), Settings::Permissions::Comparator);
		index = FindKey(dlg.m_permission, NULL); // find new location of our entry
	}

	FillListView(index);
	OnChangedSelectedComputer(index);
}

void DlgPermissionList::OnBtnRemove()
{
	int index = GetSelectedItem();
	if (index<0) return;

	m_items.erase (m_items.begin()+index);
	m_pcList.DeleteItem(index);
	m_pcList.SetSelectionMark(-1);
	OnChangedSelectedComputer(-1);
}


BOOL DlgPermissionList::OnItemChanged1(NMLISTVIEW* pNMListView)
{
	int index = pNMListView->iItem;

	if (pNMListView->uNewState & LVIS_SELECTED != 0) {
		if (m_selectedComputerIndex!=index) {
			OnChangedSelectedComputer(index);
		}
	}
	else {
		if (m_selectedComputerIndex==index) {
			OnChangedSelectedComputer(-1);
		}
	}

	return TRUE;
}


void DlgPermissionList::SetPermission(int iComputer, int iPermission, bool value)
{
	m_pcList.SetItemTextW(iComputer, iPermission+1, value ? L"\x25CF" : L"");
}



void DlgPermissionList::OnChangedSelectedComputer(int index)
{
	m_selectedComputerIndex = index;

	HWND m_wndBtnEdit    = ::GetDlgItem(m_hWnd, IDC_EDIT);
	HWND m_wndBtnRemove  = ::GetDlgItem(m_hWnd, IDC_REMOVE);

	BOOL enable = (index<0) ? FALSE : TRUE;

	::EnableWindow(m_wndBtnEdit,   enable);
	::EnableWindow(m_wndBtnRemove, enable);

	for (int i=0; i<COUNTOF(m_panel.m_wnd); i++) {
		::EnableWindow(m_panel.m_wnd[i], enable);
		bool checked = (enable) ? m_items[index].Get(1<<i) : false;		
		m_panel.m_wnd[i].SetCheckBool(checked);
	}
}

void DlgPermissionList::OnPermissionCheckBoxClicked(int index)
{
	m_selectedComputerIndex = GetSelectedItem();

	bool checked = m_panel.m_wnd[index].GetCheckBool();

	m_items[m_selectedComputerIndex].Set(1<<index, checked);

	if (index==0) {
		BOOL enabled = (checked) ? TRUE : FALSE;

		for (int i=1; i<=3; i++) {
			::EnableWindow(m_panel.m_wnd[i], enabled);
			m_panel.m_wnd[i].SetCheckBool(checked);
			m_items[m_selectedComputerIndex].Set(1<<i, checked);
		}
	}
	FillRow(m_selectedComputerIndex);
}

INT_PTR DlgPermissionList::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) 
	{
	case WM_NOTIFY:
		{
			NMHDR* pNMHdr = (NMHDR*)lParam;

			if (pNMHdr->code==LVN_ITEMCHANGED) {
				if (pNMHdr->idFrom==IDC_PC_LIST)					
					return this->OnItemChanged1((NMLISTVIEW*)lParam);
			}
			else if (pNMHdr->code==NM_DBLCLK) {
				this->OnBtnEdit();
			}
			break;
		};


		// Dialog has just received a command
	case WM_COMMAND:
		{
			WORD wItem = LOWORD(wParam);
									
			if (BN_CLICKED  == HIWORD(wParam)) {
				for (int i=0; i<COUNTOF(m_panel.m_wnd); i++) {
					if ((HWND)lParam==(HWND)m_panel.m_wnd[i]) { 
						OnPermissionCheckBoxClicked(i);
						return TRUE;
					}
				}

				     if (wItem==IDC_REMOVE)	{ this->OnBtnRemove(); }
				else if (wItem==IDC_ADD)	{ this->OnBtnAdd();    }
				else if (wItem==IDC_EDIT)	{ this->OnBtnEdit();   }
			}
		}
		break;
	}
	return 0;
}


// _________________________________________________________________________________________________________

void DlgPermissionList::Panel::Create1(RLWnd& wnd, HWND m_hWndDialog, LPCSTR wName, DWORD style, int x, int y, int cx, int cy, HFONT hFont)
{
	RECT r;
	r.left = x;
	r.top = y;
	r.right = cx+x;
	r.bottom = cy+y;
	::MapDialogRect(m_hWndDialog, &r);

	x = r.left;
	y = r.top;
	cx = r.right - r.left;
	cy = r.bottom - r.top;	

	style |= WS_CHILD | WS_VISIBLE;
	wnd.CreateWindowExW(0, L"Button", (CStringW)wName, style, x, y, cx, cy, m_hWndDialog, 0, 0, 0);
	wnd.SetFont(hFont);
}


void DlgPermissionList::Panel::Create(HWND m_hWndDialog, int x, int y)
{
	HFONT hFont = RLWnd(m_hWndDialog).GetFont();

	DWORD style2 = WS_TABSTOP|BS_AUTOCHECKBOX;

	RLWnd wndGroup;

	Create1(wndGroup, m_hWndDialog, "Permissions",    BS_GROUPBOX, x,    y,106,162, hFont);  y+=21;
	Create1(m_wnd[0], m_hWndDialog, "View Screen",    style2,      x+10, y, 93, 10, hFont);  y+=16;
	Create1(m_wnd[1], m_hWndDialog, "Remote Control", style2,      x+23, y, 74, 10, hFont);  y+=16;
	Create1(m_wnd[2], m_hWndDialog, "Clipboard Out",  style2,      x+23, y, 74, 10, hFont);  y+=16;
	Create1(m_wnd[3], m_hWndDialog, "Clipboard In",   style2,      x+23, y, 74, 10, hFont);  y+=20;
	Create1(m_wnd[4], m_hWndDialog, "File Manager",   style2,      x+10, y, 93, 10, hFont);  y+=16;
	Create1(m_wnd[5], m_hWndDialog, "Audio Chat",     style2,      x+10, y, 93, 10, hFont);  y+=16;
	Create1(m_wnd[6], m_hWndDialog, "RDP session",    style2,      x+10, y, 93, 10, hFont);
}
