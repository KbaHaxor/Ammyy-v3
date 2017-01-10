#include "stdafx.h"
#include "DlgContactBook.h"
#include "DlgAddComputer.h"
#include "resource.h"
#include "AmmyyApp.h"
#include "Common.h"
#include <algorithm>
#include "ImpersonateWrapper.h"
#include "DlgMain.h"
#include "Image.h"
#include "../viewer/res/resource_fm.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

DlgContactBook::ContactBookItem DlgContactBook::m_root;
bool                            DlgContactBook::m_loaded = false;

static int const ITEM_RETURN = INT_MIN;
static UINT8  Contacts_key_v3[20] = { 0x84,0x81,0x32,0x60, 0xB2,0xFE,0xA9,0x6C, 0x63,0x0D,0x8B,0x82, 0x51,0xA6,0x45,0xEB, 0x10,0xF1,0x34,0x88 };

bool DlgContactBook::FnIdAsc(const ContactBookItem& v1, const ContactBookItem& v2)
{
	int t1 = v1.GetTypeForSorting();
	int t2 = v2.GetTypeForSorting();

	if (t1!=t2) return t1<t2;

	if (t1==1) return wcscmp(v1.name, v2.name) < 0; // folders
	if (t1==2) return (v1.id_int < v2.id_int); // ID
			   return wcscmp(v1.id, v2.id) < 0; // IP or Host
}

bool DlgContactBook::FnIdDesc(const ContactBookItem& v1, const ContactBookItem& v2)
{		
	int t1 = v1.GetTypeForSorting();
	int t2 = v2.GetTypeForSorting();

	if (t1!=t2) return t1<t2;

	if (t1==1) return wcscmp(v1.name, v2.name) < 0;
	if (t1==2) return (v1.id_int > v2.id_int);
			   return wcscmp(v1.id, v2.id) > 0;
}

bool DlgContactBook::FnNameAsc(const ContactBookItem& v1, const ContactBookItem& v2)
{		
	return wcscmp(v1.name, v2.name) < 0;
}
	
bool DlgContactBook::FnNameDesc(const ContactBookItem& v1, const ContactBookItem& v2)
{
	return wcscmp(v1.name, v2.name) > 0;
}


DlgContactBook::DlgContactBook()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_CONTACT_BOOK);
}

DlgContactBook::~DlgContactBook()
{
	if (m_hWnd!=NULL) {
		::DestroyWindow(m_hWnd);
		m_hWnd = NULL;
		this->SaveContactBook(L"");
	}
}


void DlgContactBook::OnChangeLanguage()
{
	if (m_hWnd==NULL) return; // not ready

	this->SetTextW(L"Ammyy - " + rlLanguages.GetValue(D_CONTACT_BOOK));
	
	m_wndBtnAdd   .SetToolTipTextW(rlLanguages.GetValue(D_ADD));
	m_wndBtnEdit  .SetToolTipTextW(rlLanguages.GetValue(D_EDIT));
	m_wndBtnRemove.SetToolTipTextW(rlLanguages.GetValue(D_REMOVE));
	m_wndBtnConnect.SetTextW      (rlLanguages.GetValue(D_CONNECT));
}


static GetBitmaps1(HBITMAP& hBitmap0, HBITMAP& hBitmap1, int res)
{
	hBitmap0 = (HBITMAP)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(res), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	hBitmap1 = (HBITMAP)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(res), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	RLToolTipButton::MakeBWBitmap(hBitmap0);
}

void DlgContactBook::AddButtons(RLToolTipButton& m_wndBtnAdd, RLToolTipButton&m_wndBtnEdit, 
								RLToolTipButton& m_wndBtnRemove, RLToolTipButton& m_wndBtnUp, 
								RLToolTipButton&m_wndBtnDn, HWND m_hWnd, int x, int y)
{
	int cx = 28+3;

	Image img;
	HBITMAP hBitmap0, hBitmap1;

	img.FromResource(contactbook_add);
	hBitmap1 = img.GetBitmap();
	m_wndBtnAdd.Create(x, y, 28, 28, m_hWnd, IDC_ADD);
	x+=cx;
	m_wndBtnAdd.InitImage2(hBitmap1, hBitmap1, true, false);
	m_wndBtnAdd.SetToolTipTextW(rlLanguages.GetValue(D_ADD));

	GetBitmaps1(hBitmap0, hBitmap1, IDB_RENAME);
	m_wndBtnEdit.Create(x, y, 28, 28, m_hWnd, IDC_EDIT);
	x+=cx;
	m_wndBtnEdit.InitImage2(hBitmap1, hBitmap0, true, true);
	m_wndBtnEdit.SetToolTipTextW(rlLanguages.GetValue(D_EDIT));

	GetBitmaps1(hBitmap0, hBitmap1, IDB_DELETE);
	m_wndBtnRemove.Create(x, y, 28, 28, m_hWnd, IDC_REMOVE);
	x+=cx;
	m_wndBtnRemove.InitImage2(hBitmap1, hBitmap0, true, true);
	m_wndBtnRemove.SetToolTipTextW(rlLanguages.GetValue(D_REMOVE));

	img.FromResource(contactbook_up);
	hBitmap1 = img.GetBitmap();
	img.MakeBW();
	hBitmap0 = img.GetBitmap();
	m_wndBtnUp.Create(x, y, 28, 14, m_hWnd, IDC_UP);
	m_wndBtnUp.InitImage2(hBitmap1, hBitmap0, true, true);
	m_wndBtnUp.SetToolTipTextW(CStringW("Up"));

	img.FromResource(contactbook_up);
	img.FlipVertical();
	hBitmap1 = img.GetBitmap();
	img.MakeBW();
	hBitmap0 = img.GetBitmap();
	m_wndBtnDn.Create(x, y+14, 28, 14, m_hWnd, IDC_DOWN);
	m_wndBtnDn.InitImage2(hBitmap1, hBitmap0, true, true);
	m_wndBtnDn.SetToolTipTextW(CStringW("Down"));
}


BOOL DlgContactBook::OnInitDialog()
{
	m_wndBtnUp.AttachDlgItem(m_hWnd, IDC_UP);
	m_wndBtnDn.AttachDlgItem(m_hWnd, IDC_DOWN);
	m_wndBtnConnect.AttachDlgItem(m_hWnd, IDC_CONNECT);
	
	m_pcList.AttachDlgItem(m_hWnd, IDC_LIST);
	m_pcList.SetExtStyle(LVS_EX_FULLROWSELECT); //|LVS_EX_SUBITEMIMAGES);

	{
		LVCOLUMN lvCol;
		memset(&lvCol,0,sizeof(lvCol));

		lvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;    // Type of mask
		
		lvCol.fmt = LVCFMT_CENTER;
		lvCol.pszText = "";
		lvCol.cx=100;
		m_pcList.SendMessage(LVM_INSERTCOLUMN,0,(LPARAM)&lvCol);

		lvCol.fmt = LVCFMT_LEFT;
		lvCol.pszText = "ID / IP";
		lvCol.cx=130;
		m_pcList.SendMessage(LVM_INSERTCOLUMN,1,(LPARAM)&lvCol);
	}

	m_sortColumn = -1;
	m_selectedIndex = -1;
	m_connectToID   = "";

	m_curr = &m_root;

	HIMAGELIST imageList = ::ImageList_Create(16, 16, ILC_COLORDDB, 2, 0);

	for (int i=0; i<4; i++) {
		//CString path;
		//path.Format("ammyy_%u.bmp", i);
		//path = CCommon::GetPathA((CStringA)CCommon::GetModuleFileNameW(NULL)) + path;
		//HBITMAP bitmap = (HBITMAP)::LoadImage(0, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		HBITMAP bitmap = (HBITMAP)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(IDB_CONTACKBOOK_0+i), IMAGE_BITMAP, 0, 0, 0);
		::ImageList_Add(imageList, bitmap, NULL);
	}
	ListView_SetImageList(m_pcList, imageList, LVSIL_SMALL); // list_view will delete "imageList" 

	AddButtons(m_wndBtnAdd, m_wndBtnEdit, m_wndBtnRemove, m_wndBtnUp, m_wndBtnDn, m_hWnd, 12, 12);
	

	HBITMAP hBitmap0 = (HBITMAP)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(IDB_IMPORT), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	m_wndBtnImport.Create(0, 0, 28, 28, m_hWnd, IDC_IMPORT);
	m_wndBtnImport.InitImage2(hBitmap0, hBitmap0, true, false);
	m_wndBtnImport.SetToolTipTextW(CStringW("Import"));

	        hBitmap0 = (HBITMAP)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(IDB_EXPORT), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	m_wndBtnExport.Create(0, 0, 28, 28, m_hWnd, IDC_EXPORT);
	m_wndBtnExport.InitImage2(hBitmap0, hBitmap0, true, false);
	m_wndBtnExport.SetToolTipTextW(CStringW("Export"));



	try {
		LoadContactBook();
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
	}
	FillList();

	OnChangeLanguage();


	{
		RECT rect1;
		::GetWindowRect((HWND)*m_pDlgMain, &rect1);
		
		rect1.left = rect1.right;
		rect1.right += 330;

		SIZE s1;
		s1.cx = rect1.right  - rect1.left;
		s1.cy = rect1.bottom - rect1.top;

		RECT rect2;
		RLWnd::GetWorkArea(&rect2);
		if (IsRectInside(rect2, rect1))
			this->SetWindowPos1(rect1.left, rect1.top, s1.cx, s1.cy);
		else
			this->SetWindowSize(s1.cx, s1.cy);
		
		SIZE s = this->GetClientSize();

		this->OnSize(s.cx, s.cy);
	}

	return TRUE;
}


bool DlgContactBook::CheckIfEntryExist(const DlgContactBook::ContactBookItem& item, int exclude)
{
	int count = m_curr->children.size();
	bool folder = item.IsFolder();

	for (int i=0; i<count; i++) {
		if (i==exclude) continue;
		const ContactBookItem& item1 = m_curr->children[i];
		if (folder) {			
			if (item1.IsFolder() && item1.name==item.name) {
				CStringA txt = "Folder with such name already exists";
				::MessageBox(m_hWnd, txt, "Ammyy Admin", MB_ICONWARNING);
				return true;
			}
		}
		else {
			if (item1.id==item.id) {
				CStringA txt;
				txt.Format("Computer with ID=%s already exists in the contact book", (LPCSTR)(CStringA)item.id);
				::MessageBox(m_hWnd, txt, "Ammyy Admin", MB_ICONWARNING);
				return true;
			}
		}
	}
	return false;
}

CStringW DlgContactBook::FindNameByIDInternal(UINT32 id, ContactBookItem& item)
{
	int count = item.children.size();
	for (int i=0; i<count; i++) 
	{
		ContactBookItem& item1 = item.children[i];
		if (item1.IsFolder()) {
			CStringW str = FindNameByIDInternal(id, item1);
			if (str.GetLength()>0) return str;
		}
		else {
			if (DlgMain::ConvertID(item1.id)==id) return item1.name;
		}
	}
	return ""; // not found

}

CStringW DlgContactBook::FindNameByID(UINT32 id)
{
	try {
		if (!m_loaded) LoadContactBook();
	}
	catch(RLException& ) {}
	return FindNameByIDInternal(id, m_root);
}

void DlgContactBook::AddItem(LPCWSTR id, LPCWSTR alias, int image)
{
	LVITEMW LvItem = {0};
	LvItem.mask=LVIF_TEXT|LVIF_STATE|LVIF_IMAGE;   // Text Style
	LvItem.cchTextMax = 256;	// Max size of test
	LvItem.iItem=0; //index;          // choose item  
	LvItem.iSubItem=0;      
	LvItem.pszText = (LPWSTR)(LPCWSTR)alias; // Text to display (can be from a char variable)
	LvItem.iImage = image; //-1;
	LvItem.state = 0;

	int index = m_pcList.SendMessage(LVM_INSERTITEMW,0,(LPARAM)&LvItem);

	m_pcList.SetItemTextW(index, 1, id);
}

void DlgContactBook::FillList()
{
	m_pcList.DeleteAllItems();

	m_count = m_curr->children.size();

	for (int i=m_count-1; i>=0; i--) // back order
	{
		ContactBookItem& item = m_curr->children[i];
		int image = 1; // for folder		
		if (!item.IsFolder()) image = (DlgMain::ConvertID(item.id)>0) ? 2 : 3;
		AddItem(item.id, item.name, image);
	}

	if (!IsRoot()) {
		AddItem(L"", L"", 0);
	}

	if (m_selectedIndex>=0) {
		int add = (IsRoot()) ? 0 : 1;
		m_pcList.SetSelectionMark(m_selectedIndex+add);
	}
	else if (m_selectedIndex==ITEM_RETURN) {
		m_pcList.SetSelectionMark(0);
	}

	m_pcList.SetFocus();

	SetButtonsState();
}


void DlgContactBook::OnAdd()
{
	DlgAddComputer2 dlg(false);
	if (dlg.DoModal(m_hWnd) != IDOK) return;

	ContactBookItem item;
	item.id    = dlg.m_computerID;
	item.name  = dlg.m_alias;
	item.descr = dlg.m_description;

	if (CheckIfEntryExist(item, -1)) return;

	m_curr->children.push_back(item);

	m_selectedIndex = m_count; // select last one
	
	FillList();
}


void DlgContactBook::OnEdit()
{
	if (m_selectedIndex<0) return; // ITEM_RETURN or not_selected

	int i = m_selectedIndex;

	ASSERT(i>=0);
	ASSERT(i<m_count);

	ContactBookItem& item = m_curr->children[i];

	DlgAddComputer2 dlg(true);
	dlg.m_computerID = item.id;
	dlg.m_alias      = item.name;
	dlg.m_description= item.descr;
	
	if (dlg.DoModal(m_hWnd) != IDOK) return;

	ContactBookItem item1;
	item1.id	= dlg.m_computerID;
	item1.name	= dlg.m_alias;
	item1.descr = dlg.m_description;

	ASSERT(item1.IsFolder()==item.IsFolder()); // no way to change folder flag, cause children can be with folder

	if (CheckIfEntryExist(item1, i)) return;

	item.id	   = item1.id;
	item.name  = item1.name;
	item.descr = item1.descr;

	if (!IsRoot()) i++;

	m_pcList.SetItemTextW(i, 0, item.name);
	m_pcList.SetItemTextW(i, 1, item.id);
}

void DlgContactBook::SetButtonsState()
{
	::EnableWindow(m_wndBtnEdit,   (m_count>0 && m_selectedIndex>=0));
	::EnableWindow(m_wndBtnUp,     (m_count>1 && m_selectedIndex>0));
	::EnableWindow(m_wndBtnDn,     (m_count>1 && m_selectedIndex>=0 && m_selectedIndex<m_count-1));
	::EnableWindow(m_wndBtnRemove, (m_count>0 && m_selectedIndex>=0));
	::EnableWindow(m_wndBtnConnect,(m_count>0 && m_selectedIndex>=0));
}


void DlgContactBook::OnUp()
{
	if (m_count>1 && m_selectedIndex>0)
	{
		std::swap(m_curr->children[m_selectedIndex], m_curr->children[m_selectedIndex-1]);
		m_selectedIndex--;
		FillList();
	}
}

void DlgContactBook::OnDown()
{
	if (m_count>1 && m_selectedIndex>=0 && m_selectedIndex<m_count-1)
	{
		std::swap(m_curr->children[m_selectedIndex], m_curr->children[m_selectedIndex+1]);
		m_selectedIndex++;
		FillList();
	}
}

void DlgContactBook::OnDelete()
{
	if (m_count>0 && m_selectedIndex>=0)
	{
		if (m_curr->children[m_selectedIndex].IsFolder())
		{
			LPCSTR msg = "Do you want to delete folder with all children items ?";
			if (::MessageBox(m_hWnd, msg, "Ammyy Admin", MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES) return;
		}

		m_curr->children.erase(&m_curr->children[m_selectedIndex]);
		m_selectedIndex = -1;
		FillList();
	}
}

void DlgContactBook::OnDoubleClick()
{
	if (m_selectedIndex>=0) {
		ContactBookItem& item = m_curr->children[m_selectedIndex];
		if (item.IsFolder()) {
			m_parents.push_back(m_curr);
			m_curr = &item;			
			m_selectedIndex = ITEM_RETURN;
			FillList();
		}
		else {
			OnConnect();
		}
	}
	else if (m_selectedIndex==ITEM_RETURN) {
		ASSERT(m_parents.size()>0);
		ContactBookItem* old = m_curr;
		m_curr = (ContactBookItem*)m_parents.back();		
		m_parents.pop_back();
		m_selectedIndex = -1;
		int count = m_curr->children.size();
		for (int i=0; i<count; i++) {
			if (&m_curr->children[i]==old) {
				m_selectedIndex = i;
				break;
			}
		}
		FillList();
	}
}


void DlgContactBook::OnConnect()
{
	if (m_count>0 && m_selectedIndex>=0)
	{
		m_connectToID = m_curr->children[m_selectedIndex].id;
		//::ShowWindow(m_hWnd, SW_HIDE);
		m_pDlgMain->m_wndRemoteIdEdit.SetTextW(m_connectToID);
		m_pDlgMain->OnBtnConnect();

		//::PostMessage(m_hWnd, WM_CLOSE,0,0); // it calls OnEndDialog and save data
	}
}


BOOL DlgContactBook::OnItemChanged(NMLISTVIEW* pNMListView)
{
	if (pNMListView->uNewState & LVIS_SELECTED) {
		int index = pNMListView->iItem;

		if (!IsRoot()) {
			if (index==0)
				index = ITEM_RETURN;
			else
				index--;
		}

		if (m_selectedIndex!=index) {
			m_selectedIndex = index;
			SetButtonsState();
		}
	}
	return TRUE;
}

void DlgContactBook::OnSize(int cx, int cy)
{
	int y1 = cy-34;
	int cx1 = cx-11-11;
	m_pcList.SetWindowPos1(11, 52, cx1, cy-52-41);
	m_wndBtnConnect.SetWindowPos1(11, y1, cx1, 23);
	m_wndBtnImport .SetWindowPos2(cx-11-28-28-6, 12);
	m_wndBtnExport .SetWindowPos2(cx-11-28, 12);

	SIZE s = m_pcList.GetClientSize();
	
	LVCOLUMN lvCol = {0};
	lvCol.mask=LVCF_WIDTH;    // Type of mask	
	m_pcList.SendMessage(LVM_GETCOLUMN,1,(LPARAM)&lvCol);

	lvCol.mask=LVCF_WIDTH;    // Type of mask	
	lvCol.cx=max(s.cx - lvCol.cx,0);
	m_pcList.SendMessage(LVM_SETCOLUMN,0,(LPARAM)&lvCol);

	//m_wndBtnConnect.Invalidate();
	//m_wndBtnExport.Invalidate();
	//m_wndBtnImport.Invalidate();
}



INT_PTR DlgContactBook::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg==WM_DRAWITEM) 
	{
		//return this->OnDrawItem((DRAWITEMSTRUCT*)lParam);
		DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
		     
		     if (dis->hwndItem==m_wndBtnAdd    ) m_wndBtnAdd   .DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnEdit   ) m_wndBtnEdit  .DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnRemove ) m_wndBtnRemove.DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnUp     ) m_wndBtnUp    .DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnDn     ) m_wndBtnDn    .DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnImport ) m_wndBtnImport.DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnExport ) m_wndBtnExport.DrawItem(dis);
		return TRUE;
	}

	switch(msg)
	{
	case WM_CLOSE:
	{
		::ShowWindow(m_hWnd, SW_HIDE);
		return TRUE;
	}

	case WM_SIZE:	{ OnSize(LOWORD(lParam), HIWORD(lParam)); return TRUE; }
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO mmi = (LPMINMAXINFO)lParam;
		mmi->ptMinTrackSize.x = 280;
		mmi->ptMinTrackSize.y = 200;
		return 0;
	}

	case WM_ERASEBKGND:
	{
		RECT rect;
		this->GetClientRect(&rect);
		RLWnd::FillSolidRect((HDC)wParam, &rect, RGB(233,233,233));
		return TRUE;
	}

	case WM_COMMAND:
	{
		WORD wItem = LOWORD(wParam);

		if (wItem==IDC_ADD)		{ OnAdd();		 return TRUE; }
		if (wItem==IDC_EDIT)	{ OnEdit();		 return TRUE; }
		if (wItem==IDC_REMOVE)	{ OnDelete();	 return TRUE; }
		if (wItem==IDC_UP)		{ OnUp();		 return TRUE; }
		if (wItem==IDC_DOWN)	{ OnDown();		 return TRUE; }
		if (wItem==IDC_CONNECT) { OnConnect();	 return TRUE; }
		if (wItem==IDC_EXPORT)  { OnBtnExportImport(true);  return TRUE; }
		if (wItem==IDC_IMPORT)  { OnBtnExportImport(false); return TRUE; }

		break;
	}

	case WM_NOTIFY:
	{
		NMHDR* pNMHdr = (NMHDR*)lParam;

		if (pNMHdr->code == HDN_ITEMCLICKA) {
			LPNMHEADER phdr = (LPNMHEADER)lParam;
			if (phdr->iButton == 0) this->SortItems(phdr->iItem);					
			return 0;
		}

		if (pNMHdr->idFrom==IDC_LIST) {
			if (pNMHdr->code==LVN_ITEMCHANGED) return this->OnItemChanged((NMLISTVIEW*)lParam);			
			if (pNMHdr->code==NM_DBLCLK || pNMHdr->code==NM_RETURN) { this->OnDoubleClick(); return 0; }
		}
		break;
	}

	}
	return 0;
}




BOOL DlgContactBook::OnEndDialog(BOOL ok)
{
	return FALSE; // do not close if user press Enter or Esc

	/*
	if (ok) {
		OnDoubleClick();
		return FALSE; // do not close if user press Enter
	}

	SaveContactBook(L"");

	return TRUE;
	*/
}

void DlgContactBook::LoadContactBook_v2(LPCWSTR fileName)
{
	RLStream stream;
	stream.ReadFromFileW(fileName);

	CStringA version = stream.GetString1A();

	UINT32 count = stream.GetUINT32();

	if (count>256*1024)
		throw RLException("Bad format of contact file, count=%X", count);
		
	m_root.children.resize(count);
	for (UINT32 i=0; i<count; i++) {
		UINT32 id = stream.GetUINT32();
		m_root.children[i].id.Format(L"%u", id);
		m_root.children[i].name = stream.GetString1W();
	}
}

void DlgContactBook::LoadContactBook_v3(LPCWSTR fileName)
{
	RLStream stream;
	stream.ReadFromFileW(fileName);

	RLEncryptor02 enc;
	enc.SetKey(Contacts_key_v3, false);
	enc.Decrypt((BYTE*)stream.GetBuffer(), stream.GetLen());
		
	CStringA version = stream.GetString1A();

	UINT32 count = stream.GetUINT32();

	if (count>256*1024)
		throw RLException("Bad format of contact file, count=%X", count);
		
	m_root.children.resize(count);
	for (int i=0; i<count; i++)
		LoadOneItem(stream, m_root.children[i]);
}


void DlgContactBook::LoadContactBook()
{
	try {
		m_loaded = true;
		CStringW fileName = TheApp.GetRootFolderW() + "contacts3.bin";

		m_root.children.clear();

		if (!CCommon::FileIsExistW(fileName)) {
			fileName = TheApp.GetRootFolderW() + "contacts.bin";
			if (CCommon::FileIsExistW(fileName)) {
				LoadContactBook_v2(fileName);
			}
		}
		else
			LoadContactBook_v3(fileName);
	}
	catch(...) {
		m_root.children.clear();
		throw;
	}
}

void DlgContactBook::LoadOneItem(RLStream& stream, DlgContactBook::ContactBookItem& item)
{
	item.id   = stream.GetString1W();
	item.name = stream.GetString1W();
	item.descr = stream.GetString1W();

	if (item.IsFolder()) {
		int count = stream.GetUINT32();
		item.children.resize(count);
		for (int i=0; i<count; i++) LoadOneItem(stream, item.children[i]);
	}
}


void DlgContactBook::SaveOneItem(RLStream& stream, const DlgContactBook::ContactBookItem& item)
{
	stream.AddString1W(item.id);
	stream.AddString1W(item.name);
	stream.AddString1W(item.descr);

	if (item.IsFolder()) {
		int count = item.children.size();
		stream.AddUINT32(count);
		for (int i=0; i<count; i++) SaveOneItem(stream, item.children[i]);
	}
}

void DlgContactBook::SaveContactBook(CStringW path)
{
	if (path.IsEmpty())
		path = TheApp.GetRootFolderW() + "contacts3.bin";

	RLStream stream;

	stream.AddString1A(CStringA(settings.GetVersionSTR()));

	UINT32 count = m_root.children.size();
	stream.AddUINT32(count);
	for (int i=0; i<count; i++)
		SaveOneItem(stream, m_root.children[i]);

	try {
		RLEncryptor02 enc;
		enc.SetKey(Contacts_key_v3, true);
		enc.Encrypt((BYTE*)stream.GetBuffer(), stream.GetLen());

		stream.WriteToFileW(path);
	}
	catch(RLException& ex) {
		//if (!nogui) 
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
	}
}

void DlgContactBook::SortItems(int column)
{
	if (column<0 || column>1) return;

	if (m_sortColumn!= column) {
		m_sortColumn = column;
		m_sortOrder = true;
	}
	else
		m_sortOrder = !m_sortOrder;

	typedef bool (*CompFunction)(const ContactBookItem& i1, const ContactBookItem& i2);

	CompFunction function = NULL;

	switch (column) {
		case 0: { function = (m_sortOrder) ? FnNameAsc : FnNameDesc; break;	}
		case 1:	{ function = (m_sortOrder) ? FnIdAsc   : FnIdDesc;   break; }		
	}

	int count = m_curr->children.size();
	for (int i=0; i<count; i++) 
		m_curr->children[i].id_int = DlgMain::ConvertID(m_curr->children[i].id);	
		
	std::sort(m_curr->children.begin(), m_curr->children.end(), function);
	this->FillList();
}

#pragma comment(lib, "Comdlg32.lib")

CStringW DlgContactBook::GetInitPathExportImport()
{
	CStringW path;
	HRESULT  hr = ::SHGetFolderPathW( NULL, CSIDL_PERSONAL, NULL, 0, path.GetBuffer(MAX_PATH));
	if (FAILED(hr))
		throw RLException("SHGetFolderPathW() Error=%X", hr);
	path.ReleaseBuffer();	
	CCommon::CheckFolderTailW(path);
	return path;
}


/*
DlgContactBook::FolderWrapper::FolderWrapper()
{
	CStringW path = GetInitPathExportImport();
	::SetCurrentDirectoryW(path);

}

DlgContactBook::FolderWrapper::~FolderWrapper()
{
	::SetCurrentDirectoryW(TheApp.GetRootFolderW());
}
*/


void DlgContactBook::OnBtnExportImport(bool export)
{
	ImpersonateWrapper impersonate;

	WCHAR fileName[MAX_PATH];
	wcscpy(fileName, L"Ammyy_Contact_Book.bin");
	OPENFILENAMEW ofn = {0};

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = sizeof(fileName);
	ofn.lpstrFilter = L"bin\0*.bin\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = (export) ? L"Export contact book as" : L"Import contact book from";
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;

	CStringW path = GetInitPathExportImport();
	ofn.lpstrInitialDir = path;
	//_log.WriteError("path=%s", (LPCSTR)(CStringA)path);

	// there's bug on Win7 in Evaluated mode, in other way thinking about 20 seconds
	if (TheApp.m_dwOSVersion >= 0x60001) ofn.lpstrInitialDir = L"c:\\";

	if (export) {
		if (::GetSaveFileNameW(&ofn)==0) return;
		SaveContactBook(ofn.lpstrFile);
		return;
	}

	// import here

	if (::GetOpenFileNameW(&ofn)==0) return;

	if (m_root.children.size()>0) {
		LPCSTR msg = "Current Contact book will be erased. Do you want to continue ?";
		if (::MessageBox(m_hWnd, msg, "Ammyy Admin", MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES) return;
	}

	try {
		m_root.children.clear();
		LoadContactBook_v3(ofn.lpstrFile);
	}
	catch(RLException& ex) {
		m_root.children.clear();
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
	}
	m_selectedIndex = -1;
	m_curr = &m_root;
	m_parents.clear();
	FillList();
}
