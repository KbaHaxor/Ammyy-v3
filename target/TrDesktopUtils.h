#ifndef _TR__DESKTOP_UTILS_H_4076AE22__INCLUDED
#define _TR__DESKTOP_UTILS_H_4076AE22__INCLUDED

// The object to hide/show wallpaper and ActiveDesktop
//
class TrDesktopUtils
{
public:
	TrDesktopUtils();
	~TrDesktopUtils();

	void ResetAll();
	void ApplyAll();
	void RestoreAll();

private:
	void Apply1();
	void Apply2();
	void Apply3();
	void Restore1();
	void Restore2();
	void Restore3();

	bool IsNeedRestore();

	static bool SystemParametersInfoMy(UINT uiAction, UINT uiParam, PVOID pvParam);

	//void KillActiveDesktop();
	//void RestoreActiveDesktop();
	//bool m_activeDesktop;

	// true - if need restore
	bool m_wallpaper;
	bool m_composition;
	bool m_effects[9];   // need only 9 now

	HMODULE m_hDwmapi;
};


class TrDesktopOptimizer
{
public:
	void OnDesktopOpen (LPCSTR name);
	void OnDesktopClose(LPCSTR name);
	void OnDesktopReset(LPCSTR name);

private:
	int Find(LPCSTR name, bool add);

	struct Item
	{
		UINT			count;
		CStringA		name;
		TrDesktopUtils  item;
	};

	RLMutex			  m_mutex;
	std::vector<Item> m_items;
};

#endif // _TR__DESKTOP_UTILS_H_4076AE22__INCLUDED
