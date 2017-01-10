#ifndef _VTC_LOGGING
#define _VTC_LOGGING

// Macros for sticking in the current file name
//#define VTCLOG(s)	(__FILE__ ":\t" s)
#define VTCLOG(s)	__FILE__ , s

class RLLogEx2 : public RLLogEx
{
public:
	RLLogEx2(LPCSTR prefix=NULL);
	~RLLogEx2();

	// use like _log.Print(2, VTCLOG("x = %d"), x);
	//
    inline void Print(DWORD level, LPCSTR file, LPSTR format, ...) {
        if (level > this->m_level) return;
        va_list ap;
        va_start(ap, format);
        ReallyPrint(file, format, ap);
        va_end(ap);
    }   

private:
	void ReallyPrint(LPCSTR file, LPSTR format, va_list ap);

	// Path prefix to remove from log records
	char *m_prefix;
	size_t m_prefix_len;
};



// LOGGING SUPPORT for target & viewer
#define _log2 _log

// Log errors
#define LL_ERR	0

// warning
#define LL_WRN 1

//info
#define LL_INF 2

// Log socket errors
#define LL_SOCKERR	2

#define LL_SOUND 2


#endif // _VTC_LOGGING
