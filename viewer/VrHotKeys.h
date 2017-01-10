#ifndef HOTKEYS_H__
#define HOTKEYS_H__

class VrHotKeys
{
public:
	VrHotKeys();
	void SetWindow(HWND hwnd) { m_hwnd = hwnd; }
	bool TranslateAccel(MSG *pmsg);
	virtual ~VrHotKeys();	

private:
	HWND	m_hwnd;
	HACCEL	m_hAccel;
};

#endif 
