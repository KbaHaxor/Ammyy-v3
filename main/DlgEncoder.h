#if !defined(_DLG_ENCODER_H_62702545_E82F_4364_AE09_C310F99F089E__INCLUDED_)
#define _DLG_ENCODER_H_62702545_E82F_4364_AE09_C310F99F089E__INCLUDED_

#include "../RL/RLWnd.h"
#include "DlgEncoderList.h"

class DlgEncoder : public RLDlgBase  
{
public:
	DlgEncoder();
	virtual ~DlgEncoder();

	bool m_showDescr;

	DlgEncoderList::Item m_item;

protected:
	virtual BOOL OnEndDialog(BOOL ok);
	virtual BOOL OnInitDialog();
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void OnScroll(HWND hwnd);
	void OnChangedCompressionLevel();
	void OnChangedJpegLevel();
	void OnChangedEncoder();

	RLWndComboBox	m_wndEncoder;
	RLWndComboBox	m_wndColorQuality;
	RLWnd			m_wndCompressLevel, m_wndCompressText, m_wndCompress1, m_wndCompress2;
	RLWnd			m_wndJpegLevel, m_wndJpegText, m_wndJpeg1, m_wndJpeg2;
	RLWnd			m_wndDescription;
};

#endif // !defined(_DLG_ENCODER_H_62702545_E82F_4364_AE09_C310F99F089E__INCLUDED_)
