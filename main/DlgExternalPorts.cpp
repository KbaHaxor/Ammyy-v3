#include "stdafx.h"
#include "DlgExternalPorts.h"
#include "resource.h"
#include "Common.h"
#include "..\target\TrListener.h"
#include "CmdPortTest.h"


DlgExternalPorts::DlgExternalPorts()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_DIRECT_IN);
}

DlgExternalPorts::~DlgExternalPorts()
{

}

BOOL DlgExternalPorts::OnInitDialog()
{
	m_list.AttachDlgItem(m_hWnd, IDC_LIST1);
	m_list.SetExtStyle(LVS_EX_FULLROWSELECT); //|LVS_EX_SUBITEMIMAGES);

	{
		LVCOLUMN lvCol;
		memset(&lvCol,0,sizeof(lvCol));

		lvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;    // Type of mask
		lvCol.fmt = LVCFMT_CENTER;		
		
		lvCol.cx=80;
		lvCol.pszText = (LPSTR)"Port";
		m_list.SendMessage(LVM_INSERTCOLUMN,0,(LPARAM)&lvCol);

		lvCol.cx=120;
		lvCol.pszText = (LPSTR)"status";
		m_list.SendMessage(LVM_INSERTCOLUMN,1,(LPARAM)&lvCol);
	}

	m_fullTest = settings.m_directInItems.GetLen()==0;

	if (m_fullTest) {
		UINT8 status = 0; // off
		
		settings.m_directInItems.AddUINT16(443);
		settings.m_directInItems.AddUINT8(status);

		settings.m_directInItems.AddUINT16(DEFAULT_INTRANET_PORT);
		settings.m_directInItems.AddUINT8(status);
	}

	FillListView();
	OnListChangedSelected(-1);

	RLWnd wndText(::GetDlgItem(m_hWnd, IDC_TEXT));
	wndText.SetTextA(
	"External TCP ports allow you increase connection speed by establishing direct TCP connections to your partner.\n"
	"If any port is OFF we recommend that you configure your firewall or router for port forwarding. "
	"\nAlso any port might be blocked by other application which may be turned off if not required "
	"(port 443 might be blocked by Internet Information Server).\n"
	);

	return TRUE;
}

BOOL DlgExternalPorts::OnEndDialog(BOOL ok)
{
	RLStream ports;

	int n = m_list.GetItemCount();
	for (int i=0; i<n; i++) 
	{
		UINT16 port = GetPort(i);

		if (port==0) continue; // should never happen

		UINT8 state = GetState(i) ? 1 : 0;

		ports.AddUINT16(port);
		ports.AddUINT8(state);
	}	

	try {
		settings.m_directInItems = ports;
		settings.Save();
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
	}

	return TRUE;
}

void DlgExternalPorts::FillListView()
{
	m_list.DeleteAllItems();

	int n = settings.m_directInItems.GetLen() / sizeof(Settings::DirectInItem);
	for (int i=0; i<n; i++) {
		Settings::DirectInItem* pItem = ((Settings::DirectInItem*)settings.m_directInItems.GetBuffer()) + i;
		this->InsertRow(i, pItem->m_port, (pItem->m_status != 0));
	}

	m_list.SetFocus();
}

int DlgExternalPorts::InsertRow(int index, UINT16 port, bool state)
{
	char temp[64];
	sprintf(temp,"%u",(int)port);

	LVITEM LvItem;
	memset(&LvItem, 0, sizeof(LVITEM));

	LvItem.mask=LVIF_TEXT|LVIF_STATE;   // Text Style
	LvItem.pszText= temp;
	LvItem.iItem=index;					// choose item  
	LvItem.iSubItem=0;					// Put in first coluom
	LvItem.iImage = -1;
	LvItem.state = 0;
	
	int i = m_list.SendMessage(LVM_INSERTITEM,0,(LPARAM)&LvItem);

	SetState(i, state);

	return i;
}

void DlgExternalPorts::SetState(int index, LPCSTR state)
{
	m_list.SetItemTextA(index, 1, state);
}

void DlgExternalPorts::SetState(int index, bool state)
{
	LPCSTR txt = (state) ? "ON" : "OFF";
	this->SetState(index, txt);
}

UINT16 DlgExternalPorts::GetPort(int index)
{
	char buffer[32];
	m_list.GetItemTextA(index, 0, buffer, sizeof(buffer));
	UINT16 port = atol(buffer);
	return port;
}

bool DlgExternalPorts::GetState(int index)
{
	char buffer[64];
	m_list.GetItemTextA(index, 1, buffer, sizeof(buffer));
	return (strcmp(buffer, "ON")==0);
}

int DlgExternalPorts::GetSelectedItem()
{
	if (m_list.GetSelectedCount()==0) return -1;
	return m_list.GetSelectionMark();
}

void DlgExternalPorts::OnBtnCheck()
{
	int index = GetSelectedItem();
	if (index<0) return;
	UINT16 port = GetPort(index);
	if (port==0) return;

	char key[16];
	CCommon::FillRandom(key, 16);

	bool ok = false;

	try {
		TrListenerWrapper listener(port);

		if (!listener.InOpened())
			throw RLException("Couldn't open local port");

		SetState(index, "checking");
		m_list.UpdateWindow();


		CmdPortTest cmd;
		cmd.m_port = port;

		for (int i=0; i<16; i++) cmd.m_key[i] = 'A'+i;

		cmd.Send();

		ok = (cmd.m_result==0);
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONEXCLAMATION);
	}

	SetState(index, ok);

	//bool b = GetState(index);
	//SetState(index, !b);
	m_list.SetFocus();
}

void DlgExternalPorts::OnBtnAdd()
{
	try {
		RLWnd wnd;
		wnd.AttachDlgItem(m_hWnd, IDC_PORT);
		CStringA txt = wnd.GetTextA();

		int port = atol((LPCSTR)txt);

		if (port<1 || port>65535)
			throw RLException("Port should be number in range 1-65535");

		int n = m_list.GetItemCount();
		for (int i=0; i<n; i++) 
		{
			UINT16 port1 = GetPort(i);
			if (port1==port)
				throw RLException("This port already exists");

			if (port1>port) {
				break;
			}			
		}

		int index = InsertRow(i, port, false);
		if (index>=0) {
			m_list.SetSelectionMark(index);
			m_list.EnsureVisible(index);
		}
		m_list.SetFocus();
		OnBtnCheck();
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
	}
}

void DlgExternalPorts::OnBtnRemove()
{
	int index = GetSelectedItem();
	if (index<0) return;
	
	m_list.DeleteItem(index);
	m_list.SetSelectionMark(-1);
	OnListChangedSelected(-1);
}

void DlgExternalPorts::OnListChangedSelected(int index)
{
	HWND m_wndBtnCheck  = ::GetDlgItem(m_hWnd, IDC_CHECK);
	HWND m_wndBtnRemove = ::GetDlgItem(m_hWnd, IDC_REMOVE);

	BOOL enable = (index<0) ? FALSE : TRUE;

	::EnableWindow(m_wndBtnCheck,  enable);
	::EnableWindow(m_wndBtnRemove, enable);
}

BOOL DlgExternalPorts::OnListItemChanged(NMLISTVIEW* pNMListView)
{
	int index = GetSelectedItem();
	OnListChangedSelected(index);
	return TRUE;
}

void DlgExternalPorts::OnActivated()
{
	if (m_fullTest) {
		m_fullTest = false;

		int n = m_list.GetItemCount();
		for (int i=0; i<n; i++) {
			this->SetState(i, "checking");
		}
		for (    i=0; i<n; i++) {
			m_list.SetSelectionMark(i);
			m_list.EnsureVisible(i);
			m_list.SetFocus();
			OnBtnCheck();
		}
	}	
}


INT_PTR DlgExternalPorts::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) 
	{	
	case WM_COMMAND:
		{
			WORD wItem = LOWORD(wParam);		
			
			if (wItem==IDC_CHECK)	{ this->OnBtnCheck();  }
			if (wItem==IDC_REMOVE)	{ this->OnBtnRemove(); }
			if (wItem==IDC_ADD)		{ this->OnBtnAdd(); }
		}
		break;	
	case WM_NOTIFY:
		{
			NMHDR* pNMHdr = (NMHDR*)lParam;

			if (pNMHdr->code==LVN_ITEMCHANGED) {
				if (pNMHdr->idFrom==IDC_LIST1)
					return this->OnListItemChanged((NMLISTVIEW*)lParam);
			}
			break;
		};
	case WM_WINDOWPOSCHANGED:
		{
			WINDOWPOS* pWindowPos = (WINDOWPOS*)lParam;

			if (pWindowPos && (pWindowPos->flags & SWP_SHOWWINDOW)) {
				OnActivated();
			}
		}
	}
	return 0;
}
