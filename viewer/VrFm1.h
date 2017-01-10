#ifndef _VR_FM_WINDOW_H_
#define _VR_FM_WINDOW_H_

#include <vector>
#include <map>
#include "../RL/other/RLToolTipButton.h"
#include "../RL/other/TextProgressCtrl.h"
#include "../target/TrFmFileSys.h"

#define WM_ENABLE_INTERFACE WM_USER+1
#define WM_SERVER_REPLY WM_USER+2

//class VrFmItem
//class VrFmIconizer
//class VrFmListView;
//class VrFmStatusBar;
//class VrFmCreateDirDlg;
//class VrFmEditDirBox;
class VrFmBlock;
class VrFmBlockR;
class VrFmMainWindow;

class VrFmItem
{
public:			
	VrFmItem(const WCHAR* const name, TrFmFileSys::ItemType type, UINT64 size, const FILETIME* pTime, DWORD attrs)
	{
		this->name  = name;
		this->type  = type;
		this->size  = size;
		this->attrs = attrs;

		if (pTime)
			this->time = *pTime;		
		else
			this->time.dwLowDateTime = this->time.dwHighDateTime = 0;
	}
		
	VrFmItem(const VrFmItem& other)
	{
		name  = other.name;
		type  = other.type;
		size  = other.size;
		attrs = other.attrs;
		time  = other.time;
	}

	~VrFmItem() {}
				
	const VrFmItem& operator=(const VrFmItem& other)
	{
		if (this != &other) {
			name  = other.name;
			type  = other.type;
			size  = other.size;
			attrs = other.attrs;
			time  = other.time;			
		}
		return *this;
	}
	
	CStringW GetReadbleSize() const;
	CStringW GetReadableAttrs() const;
	CStringW GetReadbleTime  () const;

public:
	CStringW			  name;
	TrFmFileSys::ItemType type;
	UINT64				  size;
	FILETIME			  time;
	DWORD				  attrs;	// for file and disk			
};

class VrFmIconizer
{
public:
	VrFmIconizer();
	~VrFmIconizer();

	void AddRef();
	void Release();

	int GetIconId(VrFmItem& item, bool remote);

	HIMAGELIST m_smallList;

private:
	int AddIcon(LPCWSTR pszPath, bool useFileAttr);
	int AddIcon2(HMODULE hLib, LPCSTR lpIconName);
	
	typedef std::map<CStringW, int> IconIdMap;
	int m_icons[9];

	volatile UINT m_counter;
	IconIdMap m_mSmallIds;
};



class VrFmListView : public RLWndList
{
public:
	VrFmListView();

	void Create(HWND hwndParent);
	void OnRenaming();	
	void SortItems2(int columnId);
	void UpdateContent();
	void SelectPrevPath(const CStringW& prevPath);	
	void SetSelected(int index);
	bool IsAllowedDirCreation() const;
	bool IsAllowedEditing    () const;
	bool IsAllowedDeletion   () const;
		
private:	
	void InsertColumn(LPCTSTR text, int index, int width, bool textLeft = false);

	bool m_sortOrder[3]; // for 3 columns

public:
	int  m_lastSelectedIndex;
	int  m_selectedCount;
	VrFmBlock* m_pBlock;
	HWND m_hWndHeader;
};


class VrFmStatusBar : public RLWndEx
{
public:
	void Create(HWND parentHWnd);
	void SetPartWidths(const int width1, const int width2);
	void SetTextForPart(const int iPart, LPCWSTR text);
};

class VrFmCreateDirDlg : public RLDlgBase
{
public:
	VrFmCreateDirDlg();
	~VrFmCreateDirDlg();

	CStringW m_dirName;
private:
	BOOL OnInitDialog();
	BOOL OnEndDialog(BOOL ok);
};


class VrFmEditDirBox : public RLWndEx
{
public:
	void Create(HWND parentHWnd, UINT nId, HFONT hFont);
	VrFmBlock* m_pBlock;

private:
	LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};


class VrFmBlock {
public:
	VrFmBlock();
	void SetButtonsState();
	void OnListViewItemChanged(LPNMLISTVIEW pnmv);
	void OnItemClicked(int itemIndex);
	void SetStatusBarText();
	void OnDirBoxEnterKeyDown();
	CStringW GetDeletionText();
	void OnKeyDown(WORD wVKey);
	void SetSelected(LPCWSTR itemname);
	
	virtual void TrySetNewDir(LPCWSTR pathName=NULL) = 0;
	virtual void CreateDirectory(LPCWSTR dirName) = 0;
	virtual void DeleteSelectedItems() = 0;	
	virtual bool RenameItem(LPCWSTR extName, LPCWSTR newName) = 0;
	virtual bool IsBlockL() { return false; }

	typedef bool (*FmCompFunction)(const VrFmItem& i1, const VrFmItem& i2);
	typedef std::vector<VrFmItem> ItemVector;
		
	void SortItems(FmCompFunction compFunc);

protected:
	void ShowMsgBoxError(LPCWSTR lpszFormat, ...);

public:
	CStringW		 m_currentPath;
	VrFmListView	 m_listview;
	RLToolTipButton  m_btns[5];
	VrFmEditDirBox	 m_dirBox;
	VrFmMainWindow*	 m_pMainWnd;
	ItemVector		 m_items;

protected:
	FmCompFunction	m_currentComp;
	CStringW		m_linkPath[2];
};


class VrFmBlockL: public VrFmBlock 
{
public:
	VrFmBlockL();
	void TrySetNewDir(LPCWSTR pathName=NULL);
	void CreateDirectory(LPCWSTR dirName);
	void DeleteSelectedItems();
	bool RenameItem(LPCWSTR extName, LPCWSTR newName);
	virtual bool IsBlockL() { return true; }

private:
	bool LoadItemsLocaly(LPCWSTR path);
};


class VrFmCopy 
{
public:
	VrFmCopy():m_timer1(false) {};

	void OnInit(bool upload);
private:
	void OnTimerInternal();
public:
	void OnTimer();
	void OnCancel();
	void ReSetCurFileSize(INT64 size);
	bool TryNext();

	inline void PushBack(const CStringW name, UINT64 size)
	{
		TrFmFileSys::FileSize item;
		item.name = name;
		item.size = size;
		m_fileList.push_back(item);
	}

	inline void StartFile(UINT64 offset)
	{
		m_fileOne.pos = offset;
		m_skipped    += offset;
	}

private:
	void Finish();


public:
	CTextProgressCtrl m_progressOne;
	CTextProgressCtrl m_progressAll;
	RLWnd m_wndStatic1;	
	RLWnd m_btnCancel;

	struct Progress64 {
		UINT64 pos;
		UINT64 end;

		static double GetComplete(__int64 pos, __int64 end)
		{ 
			if (end==0) return 0;
			return (double)pos/(double)end;
		}
	};

	Progress64 m_fileOne;		// current file size and position for upload & download
	Progress64 m_fileAll;

	UINT32 m_cntFilesTotal;
	UINT32 m_cntFilesCopied;

	CStringW fileName; // current file name for upload & download
	bool	m_canceling;
	bool	m_upload;
	bool	m_visible;

	VrFmBlockR*     m_pBlockR;
	VrFmMainWindow* m_pMainWnd;

	TrFmFileSys::FileList m_fileList; // list for upload or download

private:
	UINT64   m_skipped;		// skipped by continue download/upload in bytes
	bool	 m_activated;
	RLTimer m_timer1;
};

#include "VrFmBlockR.h"


class VrFmMainWindow : public RLWndEx
{
public:
	VrFmBlockR m_R; // Right - Remote block
	VrFmBlockL m_L; // Left  - Local  block

 	friend class VrFmListView;
	friend class VrFmBlock;

	VrFmMainWindow();
	~VrFmMainWindow();

	void Create(const CStringW& caption);

	void EnableInterface(BOOL enable);
	bool IsCopyAllow();
	void ShowCopyControls(bool visible);

	VrFmIconizer m_fmIconizer; // NOTE: may be static better

private:
	enum ButtonIDs {
		BTNID_REFRESH  = 0x1000,
		BTNID_SEND     = 0x1002,
		BTNID_RENAME   = 0x1004,
		BTNID_CREATEFOLDER = 0x1006,
		BTNID_DELETE   = 0x1008,
		EDITID_LOCAL   = 0x1010,
		EDITID_REMOTE  = 0x1011,
	};

	LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void RecalcLayout(const int splitterX, const bool changeKoef = false);

	void OnSize(int width, int height);
	void OnMouseDown(int x, int y, UINT flags);
	void OnMouseUp(int x, int y, UINT flags);
	void OnMouseMove(int x, int y, UINT flags);

	void OnBtnDelete   (VrFmBlock* pBlock);
	void OnBtnCreateDir(VrFmBlock* pBlock);
	void OnBtnRefresh  (VrFmBlock* pBlock);
	void OnKeyDown(VrFmBlock* pBlock, WORD wVKey);
 		
private:
	VrFmStatusBar	 m_wndStatusBar;
 	int	 m_cx, m_cy;	// main window client area width and height
	int  m_cy2;			// height of main area without status bar & copy controls
	int  m_splitterWidth;
 	bool m_splitterCatched;
	bool m_sizedCursor;
 
 	int		m_lastSplitterX;
 	double  m_layoutKoef;
	HCURSOR m_curNormal;
	HCURSOR m_curSized;
	HBITMAP m_hBitmaps[5*2]; // bitmaps for buttons

public:
	 VrFmCopy m_copy;

public:
	HWND    m_wndMain; // only set if "File Manager only" selected
};



#endif // _VR_FM_WINDOW_H_
