#ifndef __AMMYY_WIN32_DYNAMICFN_H__
#define __AMMYY_WIN32_DYNAMICFN_H__

class DynamicFnBase 
{
public:
	DynamicFnBase(LPCSTR dllName, LPCSTR fnName);
	~DynamicFnBase();
	bool isValid() const {return m_fnPtr != 0;}

	static FARPROC GetProcAddr(HMODULE dllHandle, LPCSTR fnName);

private:
	DynamicFnBase(const DynamicFnBase&);
	DynamicFnBase operator=(const DynamicFnBase&);

	//HMODULE m_dllHandle;

protected:
	void*   m_fnPtr;
};

template<class T> class DynamicFn : public DynamicFnBase {
public:
	DynamicFn(const TCHAR* dllName, const char* fnName) : DynamicFnBase(dllName, fnName) {}
	T operator *() const {return (T)m_fnPtr;};
};

#endif // __AMMYY_WIN32_DYNAMICFN_H__
