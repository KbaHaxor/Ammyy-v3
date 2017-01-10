#if !defined(_DLG_CONTACT_BOOK_H__INCLUDED_)
#define _DLG_CONTACT_BOOK_H__INCLUDED_

#include "../RL/RLWnd.h"
#include "../RL/other/RLToolTipButton.h"
#include <vector>

class DlgMain;

class DlgContactBook : public RLDlgBase
{
	/*
public:
	class FolderWrapper
	{
	public:
		 FolderWrapper();
		~FolderWrapper();
	};
	*/

public:
	DlgContactBook();
	virtual ~DlgContactBook();

	void OnChangeLanguage();

	static CStringW FindNameByID(UINT32 id);
	static void LoadContactBook();
	       void SaveContactBook(CStringW path);

	static void AddButtons(RLToolTipButton& m_wndBtnAdd, RLToolTipButton&m_wndBtnEdit, 
								RLToolTipButton& m_wndBtnRemove, RLToolTipButton& m_wndBtnUp, 
								RLToolTipButton&m_wndBtnDn, HWND m_hWnd, int x, int y);


	struct ContactBookItem
	{
	public:
		ContactBookItem() {}
		
		ContactBookItem(const ContactBookItem& v)
		{
			id   = v.id;
			name = v.name;
			descr = v.descr;
			id_int = v.id_int;
			children = v.children;
		}

		bool IsFolder() const { return (id.GetLength()==0); }
		int  GetTypeForSorting() const
		{
			if (IsFolder()) return 1;
			return (id_int>0) ? 2 : 3;
		}

		CStringW id;
		CStringW name;
		CStringW descr; // description
		std::vector<ContactBookItem> children;
		int		 id_int; // ONLY for sorting
	};

	CStringW m_connectToID;
	static ContactBookItem  m_root;
	ContactBookItem* m_curr;
	std::vector<void*> m_parents; // better ContactBookItem*, but can make more exe

	DlgMain* m_pDlgMain;

protected:
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL OnInitDialog();

private:
	CStringW GetInitPathExportImport();
	inline bool IsRoot() { return (m_curr == &m_root); }
	virtual BOOL OnEndDialog(BOOL ok);
	void OnSize(int cx, int cy);
	void OnAdd();
	void OnEdit();
	void OnDelete();
	void OnUp();
	void OnDown();
	void OnConnect();
	BOOL OnItemChanged(NMLISTVIEW* pNMListView);
	void SetButtonsState();
	void OnBtnExportImport(bool export);
	void OnDoubleClick();

	bool CheckIfEntryExist(const DlgContactBook::ContactBookItem& item, int exclude);
	void FillList();
	void SortItems(int column);
	static void LoadContactBook_v2(LPCWSTR fileName);
	static void LoadContactBook_v3(LPCWSTR fileName);
	static void SaveOneItem(RLStream& stream, const DlgContactBook::ContactBookItem& item);
	static void LoadOneItem(RLStream& stream,       DlgContactBook::ContactBookItem& item);
	void AddItem(LPCWSTR id, LPCWSTR alias, int image);
	static CStringW FindNameByIDInternal(UINT32 id, ContactBookItem& item);

	static bool FnIdAsc   (const ContactBookItem& v1, const ContactBookItem& v2);
	static bool FnIdDesc  (const ContactBookItem& v1, const ContactBookItem& v2);
	static bool FnNameAsc (const ContactBookItem& v1, const ContactBookItem& v2);
	static bool FnNameDesc(const ContactBookItem& v1, const ContactBookItem& v2);

	int m_selectedIndex;
	int m_count;

	int  m_sortColumn;
	bool m_sortOrder; // true - ASC, false - DESC
	static bool m_loaded;

	RLWndList  m_pcList;
	RLWnd	   m_wndBtnConnect;
	RLToolTipButton m_wndBtnAdd, m_wndBtnEdit, m_wndBtnRemove;
	RLToolTipButton m_wndBtnUp, m_wndBtnDn;
	RLToolTipButton m_wndBtnImport, m_wndBtnExport;
};

#endif // !defined(_DLG_CONTACT_BOOK_H__INCLUDED_)
