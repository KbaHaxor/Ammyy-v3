#include "stdafx.h"
#include "VrDlgSpeedTest.h"
#include "../main/resource.h"
#include "../main/Common.h"
#include "../main/aaProtocol.h"
#include "../main/DlgMain.h"


VrDlgSpeedTest::VrDlgSpeedTest()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_SPEED_TEST);
}

VrDlgSpeedTest::~VrDlgSpeedTest()
{
}


BOOL VrDlgSpeedTest::OnInitDialog()
{
	m_wTestTime.AttachDlgItem(m_hWnd, IDC_TIME);
	m_wText.AttachDlgItem(m_hWnd, IDC_TEXT);
	m_wStatus.AttachDlgItem(m_hWnd, IDC_STATUS);
	
	m_wTestTime.SetTextA("1.0");
	m_wStatus.SetTextA(m_transport->GetDescription());

	return TRUE;
}


void VrDlgSpeedTest::AddText(CStringA text)
{
	m_text += text + "\r\n";
	m_wText.SetTextA(m_text);
	m_wText.SendMessage(EM_LINESCROLL, 0, 2000000); // scroll to bottom 
	m_wText.UpdateWindow();	
}

void VrDlgSpeedTest::OnBtnClear()
{
	m_text = "";
	m_wText.SetTextA(m_text);
	m_wText.UpdateWindow();
}

void VrDlgSpeedTest::OnBtnStart()
{
	double time = atof(m_wTestTime.GetTextA()); // in seconds

	if (time<0.1 || time>30) {
		::MessageBox(m_hWnd, "Time should be in range 0.1-30 seconds", "Ammyy Admin", MB_ICONWARNING);
		return;
	}

	RLWnd wBtnStart(::GetDlgItem(m_hWnd, IDC_START));
	RLWnd wBtnClear(::GetDlgItem(m_hWnd, IDC_CLEAR));

	::EnableWindow(wBtnStart, FALSE);
	::EnableWindow(wBtnClear, FALSE);

	try {
		this->DoSpeedTest(time);
	}
	catch(...) {
		::DestroyWindow(m_hWnd);
		throw;
	}
	this->AddText("");

	::EnableWindow(wBtnStart, TRUE);
	::EnableWindow(wBtnClear, TRUE);
}

HBRUSH VrDlgSpeedTest::OnCtlColor(HDC hdc, HWND hwnd) 
{ 
	::SetBkMode(hdc, TRANSPARENT);
	return _GUICommon.m_brush[3];
}

INT_PTR VrDlgSpeedTest::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) 
	{
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
			
			if (wItem==IDC_START)	{ this->OnBtnStart();  }
			if (wItem==IDC_CLEAR)	{ this->OnBtnClear();  }
		}
		break;
	}
	return 0;
}

CStringA VrDlgSpeedTest::GetSpeedAsText(UINT64 bytes, double time)
{
	UINT32 speed = (UINT32)(8*bytes/time);
	CStringA str = CCommon::ConvertIDToString(speed, "0");

	int c = 11 - str.GetLength();

	for (int i=0; i<c; i++) {
		str = " " + str;
	}

	return str;
}

void VrDlgSpeedTest::DoSpeedTest(double test_time)
{
	const UINT32 block_size = 4*1024;

	CStringA line;

	if (true)
	{
		double time_max, time_min, time_total = 0; // in milliseconds
		int loops = 0;

		while (true)
		{
			RLTimer t1;

			m_transport->SendByte(aaStPing, TRUE);
			UINT8 msg = m_transport->ReadByte();

			if (msg!=aaStPingReply) throw INVALID_PROTOCOL;

			double time1 = t1.GetElapsedSeconds();

			if (loops==0) {
				time_max = time_min = time1;
			}
			else {
				if (time_max<time1) time_max=time1;
				if (time_min>time1) time_min=time1;
			}

			time_total += time1;
			loops++;

			if (time_total>=test_time)
				break;
		}

		// convert to ms
		time_max   *= 1000;
		time_min   *= 1000;
		time_total *= 1000;

		time_total /= loops;

		line.Format("Ping time (ms) min-avr-max  %2.2f %2.2f %2.2f   loops=%u", time_min, time_total, time_max, loops);  
		this->AddText(line);
	}

	// download test
	if (true)
	{
		RLTimer t1;
		RLStream buffer(block_size);
		buffer.AddUINT8(aaStDownload);
		buffer.AddRaw(&test_time, sizeof(test_time));
		buffer.AddUINT32(block_size);

		char* pBuffer = (char*)buffer.GetBuffer();

		m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);

		UINT loops = 0;

		DWORD dwTicks;

		while(true) {
			UINT8 b = m_transport->ReadByte();
			if (loops==0) {
				t1.Start();
				dwTicks = ::GetTickCount();
			}
			else {
				if (b==aaStDownloadFinished) break;
			}
			if (b!=aaStDownloadData) throw INVALID_PROTOCOL;
			m_transport->ReadExact(pBuffer, block_size-1);

			loops++;
		}

		double time = t1.GetElapsedSeconds();
		dwTicks = ::GetTickCount() - dwTicks;
		UINT64 bytes = block_size*loops;
		
		CStringA speedTxt = GetSpeedAsText(bytes, time);
		//line.Format("Dnload speed=%s   bytes transferred=%u, time=%4.3f %u", speedTxt, (int)bytes, time, dwTicks);
		line.Format("Dnload speed=%s bit/sec", speedTxt);
		this->AddText(line);
	}

	// upload test
	if (true)
	{
		RLStream buffer(block_size);
		buffer.AddUINT8(aaStUpload);
		buffer.AddUINT32(block_size);

		char* pBuffer = (char*)buffer.GetBuffer();

		m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);

		UINT loops = 0;

		pBuffer[0] = aaStUploadData;
		for (int i=1; i<block_size; i++) pBuffer[i] = (char)i;

		RLTimer t1;

		while(true) {
			m_transport->SendExact(pBuffer, block_size, TRUE);
			loops++;
			double time = t1.GetElapsedSeconds();
			if (time>=test_time)
				break;
		}

		m_transport->SendByte(aaStUploadFinished, TRUE);

		double time;
		m_transport->ReadExact(&time, sizeof(time));
		UINT64 bytes = block_size*loops;

		CStringA speedTxt = GetSpeedAsText(bytes, time);
		//line.Format("Upload speed=%s    bytes transferred=%u", speedTxt, (int)bytes);
		line.Format("Upload speed=%s bit/sec", speedTxt);
		this->AddText(line);
	}
}

