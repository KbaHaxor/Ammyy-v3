#include "stdafx.h"
#include "vtcLog.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

RLLogEx2::RLLogEx2(LPCSTR prefix)
{
	// If the compiler returns full path names in __FILE__,
	// remember the path prefix, to remove it from the log messages.
	const char *path = (prefix!=NULL) ? prefix : __FILE__;

	m_prefix_len = strlen(path);

	int c=0;

	for (int i=m_prefix_len-1; i>=0; i--) {
		if (path[i]=='\\') {
			c++;
			if (c==2) {
				m_prefix_len = i+1;
				break;
			}
		}
	}

	if (m_prefix_len) {
		m_prefix = (char *)malloc(m_prefix_len + 1);
		memcpy(m_prefix, path, m_prefix_len);
		m_prefix[m_prefix_len] = 0;

	}	
}


RLLogEx2::~RLLogEx2()
{
	if (m_prefix != NULL) free(m_prefix);
}


void RLLogEx2::ReallyPrint(LPCSTR file, LPSTR format, va_list ap) 
{
	// Exclude path prefix from the format string if needed
	if (m_prefix_len>0 && file!=NULL && strlen(file)>m_prefix_len)
	{
		#ifdef _DEBUG
			if (_strnicmp(file, m_prefix, m_prefix_len) == 0)
		#else
			if (memcmp(file, m_prefix, m_prefix_len) == 0)
		#endif
				file += m_prefix_len;
	}

	char buffer[2048];

	int len1 = strlen(file);
	int len2 = strlen(format);

	if (len1+len2>sizeof(buffer)) {
		this->WriteError("Log::ReallyPrint() Buffer Overload");
	}
	else {
		memcpy(&buffer[0], file, len1);
		buffer[len1] = '\t';
		memcpy(&buffer[len1+1], format, len2+1); // plus null terminated

		this->WriteMsgV(buffer, ap);
	}

	//this->WriteMsgV(format_ptr, ap);
}
