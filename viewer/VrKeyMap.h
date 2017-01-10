// mapping of windows virtual key codes to X keysyms.

#ifndef KEYMAP_H__
#define KEYMAP_H__


#include "../main/aaProtocol.h"

// A single key press on the client may result in more than one going to the server

const unsigned int MaxKeysPerKey = 4;

// keycodes contains the keysyms terminated by an VoidKeyCode.
// The releaseModifiers is a set of ORed flags indicating whether 
// particular modifier-up messages should be sent before the keys and modifier-down after.

const UINT32 KEYMAP_LCONTROL = 0x0001;
const UINT32 KEYMAP_RCONTROL = 0x0002;
const UINT32 KEYMAP_LALT     = 0x0004;
const UINT32 KEYMAP_RALT     = 0x0008;

typedef struct {
    UINT32 keycodes[MaxKeysPerKey];
    UINT32 releaseModifiers;
} KeyActionSpec;



class VrKeyMap {
public:
	VrKeyMap();
	KeyActionSpec PCtoX(UINT virtkey, DWORD keyData);
private:

	HKL m_hrl;	//added by maxim

	// UINT32 keymap[256];
	unsigned char buf[4]; // lots of space for now
	BYTE keystate[256];
};

#endif // KEYMAP_H__

