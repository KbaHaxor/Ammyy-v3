#if !defined(AFX_AMMYYINSTAPP_H__C34C5B69_CB27_441A_8812_4BC4E2A509A1__INCLUDED_)
#define AFX_AMMYYINSTAPP_H__C34C5B69_CB27_441A_8812_4BC4E2A509A1__INCLUDED_

class CAmmyyInstApp  
{
public:
	CAmmyyInstApp();
	~CAmmyyInstApp();

	static CStringA GetSetProxyString();

	bool TryToRun(LPCSTR args);

	void WinMain();

};

#endif // !defined(AFX_AMMYYINSTAPP_H__C34C5B69_CB27_441A_8812_4BC4E2A509A1__INCLUDED_)
