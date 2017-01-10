#include "stdafx.h"
#include "DlgEncoder.h"
#include "resource.h"
#include "../main/aaDesktop.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

struct COMBOSTRING {
	LPCSTR NameString;
	int    Encoder;
};

static COMBOSTRING EncodersEnum[] = 
{
	"Raw",			aaEncoderRaw,
	"AAFC",			aaEncoderAAFC,
	"AAC",			aaEncoderAAC,
//	"VTCTi",		aaEncoderTight,
//	"RRE",			aaEncoderRRE,
//	"CoRRE",		aaEncoderCoRRE,
//	"Zlib(pure)",	aaEncoderZlib,
//	"ZlibHex(mix)",	aaEncoderZlibHex
};


DlgEncoder::DlgEncoder()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_ENCODER);
	m_showDescr = true;
}

DlgEncoder::~DlgEncoder()
{
}

BOOL DlgEncoder::OnInitDialog()
{
	SetTextA(TheApp.m_appName + " - Encoder");

	m_wndEncoder.AttachDlgItem		 (m_hWnd, IDC_ENCODING);
	m_wndColorQuality.AttachDlgItem	 (m_hWnd, IDC_COLOR_QUALITY);
	m_wndCompressLevel.AttachDlgItem (m_hWnd, IDC_COMPRESSLEVEL);
	m_wndCompressText.AttachDlgItem	 (m_hWnd, IDC_COMPRESSLEVEL_TEXT);
	m_wndCompress1.AttachDlgItem	 (m_hWnd, IDC_STATIC_FAST);
	m_wndCompress2.AttachDlgItem	 (m_hWnd, IDC_STATIC_BEST);
	m_wndJpegLevel.AttachDlgItem	 (m_hWnd, IDC_QUALITYLEVEL);
	m_wndJpegText.AttachDlgItem		 (m_hWnd, IDC_JPEG_TEXT);
	m_wndJpeg1.AttachDlgItem		 (m_hWnd, IDC_STATIC_JPEG1);
	m_wndJpeg2.AttachDlgItem		 (m_hWnd, IDC_STATIC_JPEG2);
	m_wndDescription.AttachDlgItem   (m_hWnd, IDC_DESCRIPTION);

	m_wndDescription.SetTextW(m_item.name);

	m_wndColorQuality.InsertString(0, "8 bit grayscale");
	m_wndColorQuality.InsertString(1, "8 bit");
	m_wndColorQuality.InsertString(2, "16 bit");
	m_wndColorQuality.InsertString(3, "24 bit");
	m_wndColorQuality.InsertString(4, "32 bit");
	m_wndColorQuality.SetCurSel(m_item.colorQuality);

	{
		for (int i = 0; i<COUNTOF(EncodersEnum); i++) {
			m_wndEncoder.InsertString(i, EncodersEnum[i].NameString);
			if (EncodersEnum[i].Encoder != m_item.encoder) continue;
			m_wndEncoder.SetCurSel(i);
		}
	}

	m_wndCompressLevel.SendMessage(TBM_SETRANGE, TRUE, (LPARAM) MAKELONG(1, 9));
	m_wndCompressLevel.SendMessage(TBM_SETPOS, TRUE, m_item.compressLevel);

	m_item.FixJpegLevel();
						
	m_wndJpegLevel.SendMessage(TBM_SETRANGE, TRUE, (LPARAM) MAKELONG(0, 101)); // aaJpegOFF
	m_wndJpegLevel.SendMessage(TBM_SETPOS,   TRUE, m_item.jpegQualityLevel);

	if (!m_showDescr) {
		RLWnd wnd;
		wnd.AttachDlgItem(m_hWnd, IDC_TEXT_DESCR);
		wnd.ShowWindow1(false);
		m_wndDescription.ShowWindow1(false);
	}

	this->OnChangedEncoder();


	return TRUE;
}



BOOL DlgEncoder::OnEndDialog(BOOL ok)
{
	if (!ok) return TRUE;

	m_item.name	   = m_wndDescription.GetTextW();
	m_item.name.TrimLeft();
	m_item.name.TrimRight();
	m_item.encoder = EncodersEnum[m_wndEncoder.GetCurSel()].Encoder;
	m_item.colorQuality			= m_wndColorQuality.GetCurSel();
	m_item.compressLevel			= m_wndCompressLevel.SendMessage(TBM_GETPOS , 0, 0);
	m_item.jpegQualityLevel		= m_wndJpegLevel.SendMessage(TBM_GETPOS , 0, 0);
	m_item.FixJpegLevel();

	if (m_showDescr && m_item.name.IsEmpty()) {
		LPCSTR msg = "Description can't be empty!";
		::MessageBox(m_hWnd, msg, TheApp.m_appName, MB_OK|MB_ICONWARNING);
		return FALSE;
	}

	m_item.FixJpegLevel();

	return TRUE;
}

void DlgEncoder::OnChangedJpegLevel()
{
	DWORD dwPos = m_wndJpegLevel.SendMessage(TBM_GETPOS, 0, 0);
	CStringA txt;
	if (dwPos<=100)
		txt.Format("quality=%u", dwPos);
	else
		txt = "off";
	txt = "JPEG compression : " + txt;
	m_wndJpegText.SetTextA(txt);
}

void DlgEncoder::OnChangedCompressionLevel()
{
	DWORD dwPos = m_wndCompressLevel.SendMessage(TBM_GETPOS, 0, 0);
	CStringA txt;
	txt.Format("Compression level:  %u", dwPos);
	m_wndCompressText.SetTextA(txt);
}


void DlgEncoder::OnScroll(HWND hwnd)
{	
	if (hwnd == (HWND)m_wndCompressLevel)  { this->OnChangedCompressionLevel(); }
	else if (hwnd == (HWND)m_wndJpegLevel) { this->OnChangedJpegLevel(); }
}

void DlgEncoder::OnChangedEncoder()
{
	int i = m_wndEncoder.GetCurSel();

	bool allowJPEG     = false;
	bool allowCompress = false;
	
	switch (EncodersEnum[i].Encoder) {
		case aaEncoderAAC:
		//case aaEncoderTight:
			allowCompress = true;
			allowJPEG = (m_wndColorQuality.GetCurSel()!=0); // if bbp==16 || bpp==24
			break;
		//case aaEncoderZlib:
		//case aaEncoderZlibHex:
		//	allowCompress = true;
		//	break;
		case aaEncoderRaw:
		//case aaEncoderRRE:
		//case aaEncoderCoRRE:		
		case aaEncoderAAFC:
			break;
	}

	m_wndCompressLevel.ShowWindow1(allowCompress);
	m_wndCompressText. ShowWindow1(allowCompress);
	m_wndCompress1.	   ShowWindow1(allowCompress);
	m_wndCompress2.    ShowWindow1(allowCompress);

	m_wndJpeg1.    ShowWindow1(allowJPEG);
	m_wndJpeg2.    ShowWindow1(allowJPEG);
	m_wndJpegText. ShowWindow1(allowJPEG);
	m_wndJpegLevel.ShowWindow1(allowJPEG);

	if (allowCompress) {
		OnChangedCompressionLevel();
	}

	if (allowJPEG) {
		OnChangedJpegLevel();
	}
}




INT_PTR DlgEncoder::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) 
	{
	case WM_HSCROLL: {	this->OnScroll((HWND)lParam); return 0; }
	case WM_COMMAND:
		{
			WORD wItem = LOWORD(wParam);
			WORD msg1  = HIWORD(wParam);
									
			if (BN_CLICKED  == HIWORD(wParam)) {
				//if ((HWND)lParam==(HWND)m_wndFolder) { 
					//OnFolderCheckBoxClicked();
				//	return TRUE;
				//}
			}

			if (wItem==IDC_COLOR_QUALITY) {
				if (msg1==CBN_SELCHANGE) { OnChangedEncoder(); }
				return 0;
			}
			else if (wItem==IDC_ENCODING) {
				if (msg1==CBN_SELCHANGE) { OnChangedEncoder(); }
				return 0;
			}
		}
		break;
	}
	return 0;
}


