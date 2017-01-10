#include "stdafx.h"
#include "DynamicFn.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

DynamicFnBase::DynamicFnBase(LPCSTR dllName, LPCSTR fnName)
{
	m_fnPtr = 0;
	HMODULE m_dllHandle = ::GetModuleHandle(dllName);
	//m_dllHandle = ::LoadLibrary(dllName);
	if (!m_dllHandle) {
		_log.WriteInfo("DLL %s not found (error %d)", dllName, ::GetLastError());
		return;
	}
	m_fnPtr = ::GetProcAddress(m_dllHandle, fnName);
	if (!m_fnPtr)
		_log.WriteInfo("proc %s not found in %s (error %d)", fnName, dllName, ::GetLastError());
}

FARPROC DynamicFnBase::GetProcAddr(HMODULE dllHandle, LPCSTR fnName)
{
	FARPROC procPtr = ::GetProcAddress(dllHandle, fnName);
	if (!procPtr)
		throw RLException("proc %s not found (error %d)", fnName, ::GetLastError());

	return procPtr;
}


DynamicFnBase::~DynamicFnBase() 
{
	//if (m_dllHandle) ::FreeLibrary(m_dllHandle);
}


