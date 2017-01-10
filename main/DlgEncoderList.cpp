#include "stdafx.h"
#include "DlgEncoderList.h"
#include "DlgEncoder.h"
#include "resource.h"
#include "DlgContactBook.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif



DlgEncoderList::DlgEncoderList()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_ENCODER_LIST);
}

DlgEncoderList::~DlgEncoderList()
{
}

BOOL DlgEncoderList::OnInitDialog()
{
	SetTextA(TheApp.m_appName + " - Encoder profiles");

	m_list.AttachDlgItem(m_hWnd, IDC_LIST);	

	DlgContactBook::AddButtons(m_wndBtnAdd, m_wndBtnEdit, m_wndBtnRemove, m_wndBtnUp, m_wndBtnDn, m_hWnd, 12, 12);

	m_selectedIndex=-1;

	m_items = settings.m_encoders;
	
	FillList();

	return TRUE;
}



BOOL DlgEncoderList::OnEndDialog(BOOL ok)
{
	if (!ok) return TRUE;

	settings.m_encoders = m_items;
	settings.Save();

	return TRUE;
}


void DlgEncoderList::SetDefault(std::vector<Item>& items)
{
	items.clear();
	items.push_back(DlgEncoderList::Item("speed <256 Kb",        aaPixelFormat8,  aaEncoderAAC,  6, aaJpegOFF));
	items.push_back(DlgEncoderList::Item("speed  256 Kb - 1 Mb", aaPixelFormat16, aaEncoderAAC,  6, aaJpegOFF));
	items.push_back(DlgEncoderList::Item("speed  1 Mb - 5 Mb",   aaPixelFormat24, aaEncoderAAC,  6, aaJpegOFF));

	// I think 3 Mb, but say 5 Mb to reduce traffic on public routers
	items.push_back(DlgEncoderList::Item("speed >5 Mb",          aaPixelFormat24, aaEncoderAAFC, 6, aaJpegOFF));
	items.push_back(DlgEncoderList::Item("photo JPEG 20%",       aaPixelFormat16, aaEncoderAAC,  6, 20));
}

void DlgEncoderList::FillList()
{
	m_list.DeleteAllItems();

	m_count = m_items.size();

	for (int i=0; i<m_count; i++) {
		m_list.AddString((LPCSTR)(CStringA)m_items[i].name);
	}

	if (m_selectedIndex>=0)
		m_list.SetSelectionMark(m_selectedIndex);
	
	m_list.SetFocus();

	SetButtonsState();
}

void DlgEncoderList::SetButtonsState()
{
	::EnableWindow(m_wndBtnEdit,  (m_count>0 && m_selectedIndex>=0));
	::EnableWindow(m_wndBtnUp,    (m_count>1 && m_selectedIndex>0));
	::EnableWindow(m_wndBtnDn,    (m_count>1 && m_selectedIndex>=0 && m_selectedIndex<m_count-1));
	::EnableWindow(m_wndBtnRemove,(m_count>0 && m_selectedIndex>=0));
}


INT_PTR DlgEncoderList::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{		
	if (msg==WM_DRAWITEM) 
	{
		//return this->OnDrawItem((DRAWITEMSTRUCT*)lParam);
		DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
		     
		     if (dis->hwndItem==m_wndBtnAdd   ) m_wndBtnAdd   .DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnEdit  ) m_wndBtnEdit  .DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnRemove) m_wndBtnRemove.DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnUp    ) m_wndBtnUp    .DrawItem(dis);
		else if (dis->hwndItem==m_wndBtnDn    ) m_wndBtnDn    .DrawItem(dis);
		return TRUE;
	}

	switch (msg) 
	{
	case WM_COMMAND:
		{
			WORD wItem = LOWORD(wParam);
			WORD msg1  = HIWORD(wParam);

			if (wItem==IDC_ADD)		{ OnAdd();		 return TRUE; }
			if (wItem==IDC_EDIT)	{ OnEdit();		 return TRUE; }
			if (wItem==IDC_REMOVE)	{ OnDelete();	 return TRUE; }
			if (wItem==IDC_UP)		{ OnUp();		 return TRUE; }
			if (wItem==IDC_DOWN)	{ OnDown();		 return TRUE; }
			if (wItem==IDC_SET_DEFAULT)
			{
				SetDefault(m_items);
				m_selectedIndex = -1;
				FillList();
			}
									
			if (LBN_SELCHANGE == msg1) {
				int index = m_list.GetSelectionMark();
				if (m_selectedIndex != index) {
					m_selectedIndex  = index;
					SetButtonsState();
				}
			}
			else if (LBN_DBLCLK == msg1) {
				OnEdit();
			}

		}
		break;
	}
	return 0;
}

void DlgEncoderList::OnUp()
{
	if (m_count>1 && m_selectedIndex>0)
	{
		std::swap(m_items[m_selectedIndex], m_items[m_selectedIndex-1]);
		m_selectedIndex--;
		FillList();
	}
}

void DlgEncoderList::OnDown()
{
	if (m_count>1 && m_selectedIndex>=0 && m_selectedIndex<m_count-1)
	{
		std::swap(m_items[m_selectedIndex], m_items[m_selectedIndex+1]);
		m_selectedIndex++;
		FillList();
	}
}

bool DlgEncoderList::CheckNewItem(const Item& item, int index_exclude)
{
	for (int i=0; i<m_count; i++) {
		if (i==index_exclude) continue;
		if (m_items[i].name == item.name) {
			LPCSTR msg = "Profile with such name already exists";
			::MessageBox(m_hWnd, msg, TheApp.m_appName, MB_OK|MB_ICONWARNING);
			return false;
		}
		if (m_items[i].IsSameSettings(item)) {
			LPCSTR msg = "Profile with such settings already exists";
			::MessageBox(m_hWnd, msg, TheApp.m_appName, MB_OK|MB_ICONWARNING);
			return false;
		}
	}

	return true;
}

void DlgEncoderList::OnAdd()
{	
	DlgEncoder dlg;
	if (dlg.DoModal(m_hWnd) != IDOK) return;

	if (!CheckNewItem(dlg.m_item, -1)) return;

	m_items.push_back(dlg.m_item);

	m_selectedIndex = m_count; // select last one
	
	FillList();	
}

void DlgEncoderList::OnEdit()
{
	if (m_selectedIndex<0) return;

	int i = m_selectedIndex;

	DlgEncoder dlg;
	dlg.m_item = m_items[i];
	if (dlg.DoModal(m_hWnd) != IDOK) return;

	if (!CheckNewItem(dlg.m_item, i)) return;

	m_items[i] = dlg.m_item;
	FillList();
}

void DlgEncoderList::OnDelete()
{
	if (m_count>0 && m_selectedIndex>=0)
	{
		if (m_count==1) {
			LPCSTR msg = "At least 1 encoder profile should exist";
			::MessageBox(m_hWnd, msg, TheApp.m_appName, MB_OK|MB_ICONWARNING);
			return;
		}

		m_items.erase(&m_items[m_selectedIndex]);
		m_selectedIndex = -1;
		FillList();
	}
}



