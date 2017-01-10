// stdafx.cpp : source file that includes just the standard includes
//	AmmyCustomizer.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

Settings settings;
RLLogEx  _log;

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")


Settings::Settings() {}
void Settings::Save() {} // do nothing

// dublicate
bool Settings::Permissions::Comparator(const Permission& a, const Permission& b)
{
	int v = (a.m_id-b.m_id);
	if (v<0) return true;
	if (v>0) return false;
	return (memcmp(&a.m_password, &b.m_password, sizeof(a.m_password))<0);
}

// dublicate
void Settings::Permissions::Save(RLStream& stream)
{
	RLMutexLock l(m_lock);

	UINT count = m_items.size();

	stream.AddUINT32(count);

	for (UINT i=0; i<count; i++) {
		stream.AddUINT32(m_items[i].m_id);
		stream.AddUINT32(m_items[i].m_values);
		//stream.AddString1A(m_items[i].m_password); // "2.12"
		stream.AddRaw   (m_items[i].m_password.hash, 16);
	}
}


 



