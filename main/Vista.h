#if !defined(_VISTA_H__BBDE6CDC__INCLUDED_)
#define _VISTA_H__BBDE6CDC__INCLUDED_

class CVista  
{
public:
	static bool IsElevated();
	static bool Elevate();
	static BOOL SetForegroundWindow(HWND hWnd);
};

#endif // _VISTA_H__BBDE6CDC__INCLUDED_
