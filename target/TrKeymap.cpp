// This code originally just mapped between X keysyms and local Windows virtual keycodes.  
// Now it actually does the local-end simulation of key presses, to keep this messy code on one place!

#include "stdafx.h"
#include "TrKeymap.h"

#ifdef _DEBUG
//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define XK_MISCELLANY
#include "TrKeyDef.h"
#include "TrService.h"

#pragma warning(disable : 4786)
#include <map>



// Mapping of X keysyms to windows VK codes.  Ordering here is the same as keysymdef.h to make checking easier

struct keymap_t {
  UINT32 keysym;
  UINT8 vk;
  bool extended;
};

static keymap_t keymap[] = 
{
  // TTY functions

  { XK_BackSpace,        VK_BACK, 0 },
  { XK_Tab,              VK_TAB, 0 },
  { XK_Clear,            VK_CLEAR, 0 },
  { XK_Return,           VK_RETURN, 0 },
  { XK_Pause,            VK_PAUSE, 0 },
  { XK_Escape,           VK_ESCAPE, 0 },
  { XK_Delete,           VK_DELETE, 1 },

  // Japanese stuff - almost certainly wrong...
  { XK_Kanji,            VK_KANJI, 0 },
  { XK_Kana_Shift,       VK_KANA, 0 },

  // Cursor control & motion

  { XK_Home,             VK_HOME, 1 },
  { XK_Left,             VK_LEFT, 1 },
  { XK_Up,               VK_UP, 1 },
  { XK_Right,            VK_RIGHT, 1 },
  { XK_Down,             VK_DOWN, 1 },
  { XK_Page_Up,          VK_PRIOR, 1 },
  { XK_Page_Down,        VK_NEXT, 1 },
  { XK_End,              VK_END, 1 },

  // Misc functions

  { XK_Select,           VK_SELECT, 0 },
  { XK_Print,            VK_SNAPSHOT, 0 },
  { XK_Execute,          VK_EXECUTE, 0 },
  { XK_Insert,           VK_INSERT, 1 },
  { XK_Help,             VK_HELP, 0 },
  { XK_Break,            VK_CANCEL, 1 },

  // Keypad Functions, keypad numbers

  { XK_KP_Space,         VK_SPACE, 0 },
  { XK_KP_Tab,           VK_TAB, 0 },
  { XK_KP_Enter,         VK_RETURN, 1 },
  { XK_KP_Home,          VK_HOME, 0 },
  { XK_KP_Left,          VK_LEFT, 0 },
  { XK_KP_Up,            VK_UP, 0 },
  { XK_KP_Right,         VK_RIGHT, 0 },
  { XK_KP_Down,          VK_DOWN, 0 },
  { XK_KP_End,           VK_END, 0 },
  { XK_KP_Page_Up,       VK_PRIOR, 0 },
  { XK_KP_Page_Down,     VK_NEXT, 0 },
  { XK_KP_Begin,         VK_CLEAR, 0 },
  { XK_KP_Insert,        VK_INSERT, 0 },
  { XK_KP_Delete,        VK_DELETE, 0 },
  // XXX XK_KP_Equal should map in the same way as ascii '='
  { XK_KP_Multiply,      VK_MULTIPLY, 0 },
  { XK_KP_Add,           VK_ADD, 0 },
  { XK_KP_Separator,     VK_SEPARATOR, 0 },
  { XK_KP_Subtract,      VK_SUBTRACT, 0 },
  { XK_KP_Decimal,       VK_DECIMAL, 0 },
  { XK_KP_Divide,        VK_DIVIDE, 1 },

  { XK_KP_0,             VK_NUMPAD0, 0 },
  { XK_KP_1,             VK_NUMPAD1, 0 },
  { XK_KP_2,             VK_NUMPAD2, 0 },
  { XK_KP_3,             VK_NUMPAD3, 0 },
  { XK_KP_4,             VK_NUMPAD4, 0 },
  { XK_KP_5,             VK_NUMPAD5, 0 },
  { XK_KP_6,             VK_NUMPAD6, 0 },
  { XK_KP_7,             VK_NUMPAD7, 0 },
  { XK_KP_8,             VK_NUMPAD8, 0 },
  { XK_KP_9,             VK_NUMPAD9, 0 },

  // Auxilliary Functions

  { XK_F1,               VK_F1, 0 },
  { XK_F2,               VK_F2, 0 },
  { XK_F3,               VK_F3, 0 },
  { XK_F4,               VK_F4, 0 },
  { XK_F5,               VK_F5, 0 },
  { XK_F6,               VK_F6, 0 },
  { XK_F7,               VK_F7, 0 },
  { XK_F8,               VK_F8, 0 },
  { XK_F9,               VK_F9, 0 },
  { XK_F10,              VK_F10, 0 },
  { XK_F11,              VK_F11, 0 },
  { XK_F12,              VK_F12, 0 },
  { XK_F13,              VK_F13, 0 },
  { XK_F14,              VK_F14, 0 },
  { XK_F15,              VK_F15, 0 },
  { XK_F16,              VK_F16, 0 },
  { XK_F17,              VK_F17, 0 },
  { XK_F18,              VK_F18, 0 },
  { XK_F19,              VK_F19, 0 },
  { XK_F20,              VK_F20, 0 },
  { XK_F21,              VK_F21, 0 },
  { XK_F22,              VK_F22, 0 },
  { XK_F23,              VK_F23, 0 },
  { XK_F24,              VK_F24, 0 },

    // Modifiers
    
  { XK_Shift_L,          VK_SHIFT, 0 },
  { XK_Shift_R,          VK_RSHIFT, 0 },
  { XK_Control_L,        VK_CONTROL, 0 },
  { XK_Control_R,        VK_CONTROL, 1 },
  { XK_Alt_L,            VK_MENU, 0 },
  { XK_Alt_R,            VK_RMENU, 1 },
};


// DoKeyboardEvent wraps the system keybd_event function and attempts to find
// the appropriate scancode corresponding to the supplied virtual keycode.

static inline void DoKeyboardEvent(BYTE vkCode,DWORD flags) 
{
	UINT vkey = ::MapVirtualKey(vkCode, 0);
	keybd_event(vkCode, vkey, flags, 0);
}

static inline bool IsKeyDown(int vKey) 
{ 
	return (::GetAsyncKeyState(vKey) & 0x8000)!=0; 
}


// KeyStateModifier is a class which helps simplify generating a "fake" press
// or release of shift, ctrl, alt, etc.  An instance of the class is created
// for every key which may need to be pressed or released.  Then either press()
// or release() may be called to make sure that the corresponding key is in the
// right state.  The destructor of the class automatically reverts to the
// previous state.



class KeyStateModifier 
{
public:
	KeyStateModifier(int vkCode_)
    : vkCode(vkCode_), pressed(false), released(false)
	{}
  
	void press()
	{
		if (!IsKeyDown(vkCode)) {
			DoKeyboardEvent(vkCode, 0);
			_log.Print(LL_INF, VTCLOG("fake %d down"), vkCode);
			pressed = true;
		}
	}

	void release() 
	{
		if (IsKeyDown(vkCode)) {
			DoKeyboardEvent(vkCode, KEYEVENTF_KEYUP);
			_log.Print(LL_INF, VTCLOG("fake %d up"), vkCode);
			released = true;
		}
	}
  
	~KeyStateModifier() 
	{
		if (pressed) {
			DoKeyboardEvent(vkCode, KEYEVENTF_KEYUP);
			_log.Print(LL_INF, VTCLOG("fake %d up"), vkCode);
		} else if (released) {
			DoKeyboardEvent(vkCode, 0);
			_log.Print(LL_INF, VTCLOG("fake %d down"), vkCode);
		}
	}

private:
	int vkCode;
	bool pressed;
	bool released;
};

// Keymapper - a single instance of this class is used to generate Windows key events

class KeyMapper 
{
public:
	KeyMapper()
	{
		m_initialized = false;
	}

	void Init()
	{
		if (!m_initialized) {
			m_initialized = true;

			m_hrl = ::LoadKeyboardLayout("0419", 0);	// english
			ASSERT(m_hrl!=NULL);

			for (int i = 0; i < sizeof(keymap) / sizeof(keymap_t); i++) {
				m_vkMap[keymap[i].keysym] = keymap[i].vk;
				m_extendedMap[keymap[i].keysym] = keymap[i].extended;
			}
		}
	}

	void KeyEvent(UINT32 keysym, bool down)
	{
		if ((keysym >= 32  && keysym <= 126) || 
			(keysym >= 160 && keysym <= 255))
		{
			// ordinary Latin-1 character

			SHORT s = ::VkKeyScanEx((CHAR)keysym, m_hrl);
			if (s == -1) {
				_log2.Print(LL_WRN, VTCLOG("ignoring unrecognised Latin-1 keysym %d"), keysym);
				return;
			}

			BYTE vkCode = LOBYTE(s);

			KeyStateModifier ctrl(VK_CONTROL);
			KeyStateModifier alt(VK_MENU);
			KeyStateModifier shift(VK_SHIFT);
			KeyStateModifier lshift(VK_LSHIFT);
			KeyStateModifier rshift(VK_RSHIFT);

			if (down) {
				BYTE modifierState = HIBYTE(s);
				if (modifierState & 2) ctrl.press();
				if (modifierState & 4) alt.press();
				if (modifierState & 1) {
					shift.press();
				} else {
					lshift.release();
					rshift.release();
				}
			}
			_log2.Print(LL_INF, VTCLOG("latin-1 key: keysym %d(0x%x) vkCode 0x%x down %d"), keysym, keysym, vkCode, down);

			DoKeyboardEvent(vkCode, down ? 0 : KEYEVENTF_KEYUP);

		} else {

			// see if it's a recognised keyboard key, otherwise ignore it

			if (m_vkMap.find(keysym) == m_vkMap.end()) {
				_log2.Print(LL_WRN, VTCLOG("ignoring unknown keysym %d"),keysym);
				return;
			}
			BYTE vkCode = m_vkMap[keysym];

			_log2.Print(LL_INF, VTCLOG("keyboard key: keysym %d(0x%x) vkCode 0x%x ext %d down %d"),
                   keysym, keysym, vkCode, m_extendedMap[keysym], down);
			
			if (down && (vkCode == VK_DELETE) && IsKeyDown(VK_CONTROL) && IsKeyDown(VK_MENU))
			{
				TrService::SimulateCtrlAltDel();
				return;
			}

			DWORD flags = 0;
			if (m_extendedMap[keysym]) flags |= KEYEVENTF_EXTENDEDKEY;
			if (!down)                 flags |= KEYEVENTF_KEYUP;
			
			DoKeyboardEvent(vkCode, flags);
		}
	}

	// This routine sets the specified key to the up value
	static void ReleaseKey(BYTE key)
	{
		bool keystate = IsKeyDown(key);

		if (!keystate) return; // it's already up

		_log2.Print(LL_INF, VTCLOG("SetKeyState %d"), key);

		// Now send a key event to set the key to the new value
		DoKeyboardEvent(key, KEYEVENTF_KEYUP);
		
		//keystate = IsKeyDown(key);
		//_log2.Print(LL_INF, VTCLOG("new state %d (%s)"), key, keystate ? "down" : "up");
	}

private:
	bool	m_initialized;
	HKL		m_hrl;		//added by maxim
	std::map<UINT32,UINT8>	m_vkMap;
	std::map<UINT32,bool>	m_extendedMap;
} _KeyMapper;


void TrKeymap::Init()
{
	_KeyMapper.Init();
}

void TrKeymap::KeyEvent(UINT32 keysym, bool down)
{
	_KeyMapper.KeyEvent(keysym, down);
}

void TrKeymap::ClearShiftKeys()
{
	// LEFT
	KeyMapper::ReleaseKey(VK_LSHIFT);
	KeyMapper::ReleaseKey(VK_LCONTROL);
	KeyMapper::ReleaseKey(VK_LMENU);

	// RIGHT
	KeyMapper::ReleaseKey(VK_RSHIFT);
	KeyMapper::ReleaseKey(VK_RCONTROL);
	KeyMapper::ReleaseKey(VK_RMENU);
}
