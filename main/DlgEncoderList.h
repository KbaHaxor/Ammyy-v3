#if !defined(_DLG_ENCODER_LIST_H_62805345__INCLUDED_)
#define _DLG_ENCODER_LIST_H_62805345__INCLUDED_

#include "../RL/RLWnd.h"
#include "../RL/other/RLToolTipButton.h"
#include "../main/aaDesktop.h"

class DlgEncoderList : public RLDlgBase
{
public:
	DlgEncoderList();
	virtual ~DlgEncoderList();

	virtual BOOL OnEndDialog(BOOL ok);

protected:
	virtual BOOL OnInitDialog();
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	struct Item
	{
	public:
		Item() 
		{
			colorQuality = aaPixelFormat24;
			encoder = aaEncoderAAC;
			compressLevel = 6;
			jpegQualityLevel = aaJpegOFF;
		}

		Item(CStringW name, int colorQuality, int encoder, int compressLevel, int jpegQualityLevel)
		{
			this->name = name;
			this->colorQuality = colorQuality;
			this->encoder = encoder;
			this->compressLevel = compressLevel;
			this->jpegQualityLevel = jpegQualityLevel;
		}
		
		Item(const Item& v)
		{
			name          = v.name;
			colorQuality  = v.colorQuality;
			encoder       = v.encoder;
			compressLevel = v.compressLevel;
			jpegQualityLevel = v.jpegQualityLevel;
		}

		bool IsSameSettings(const Item& v)
		{
			return (colorQuality==v.colorQuality && encoder==v.encoder && compressLevel==v.compressLevel && jpegQualityLevel==v.jpegQualityLevel);
		}

		void FixJpegLevel()
		{
			if (jpegQualityLevel<0 || jpegQualityLevel>aaJpegOFF) {
				throw RLException("Invalid JPEG Level");
				//jpegQualityLevel = aaJpegOFF;
			}
		}

		CStringW name;
		INT8     colorQuality;
		INT8	 encoder;
		INT8	 compressLevel;
		INT8	 jpegQualityLevel;
	};
public:

	static void SetDefault(std::vector<Item>& items);

private:
	bool CheckNewItem(const Item& item, int index_exclude);
	void OnAdd();
	void OnEdit();
	void OnDelete();
	void OnUp();
	void OnDown();
	void FillList();
	void SetButtonsState();

private:
	std::vector<Item> m_items;

	RLWndListBox  m_list;
	RLToolTipButton m_wndBtnAdd, m_wndBtnEdit, m_wndBtnRemove;
	RLToolTipButton m_wndBtnUp, m_wndBtnDn;

	int m_count;
	int m_selectedIndex;
};

#endif // !defined(_DLG_ENCODER_LIST_H_62805345__INCLUDED_)
