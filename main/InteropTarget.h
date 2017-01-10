#if !defined(_TARGET_INTEROP_H__INCLUDED_)
#define _TARGET_INTEROP_H__INCLUDED_

#include "InteropCommon.h"
#include "../RL/RLEvent.h"

class InteropTarget: public InteropCommon
{
public:
	InteropTarget();
	~InteropTarget();

	virtual bool  IsTarget() { return true; }
	
	virtual void SetState(DWORD state);

	virtual void ConnectToRDPserver();
	virtual void ReTranslate();

protected:

public:
	DWORD	m_state;

private:

	RLEvent m_eventState;

private:
	SOCKET m_socketRDP;
};

#endif // !defined(_TARGET_INTEROP_H__INCLUDED_)
