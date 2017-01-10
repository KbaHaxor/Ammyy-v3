#ifndef _TR_KEYMAP_H__INCLUDED_
#define _TR_KEYMAP_H__INCLUDED_

class TrKeymap
{
public:
	static void KeyEvent(UINT32 keysym, bool down);
	static void ClearShiftKeys();
	static void Init();
};

#endif // _TR_KEYMAP_H__INCLUDED_
