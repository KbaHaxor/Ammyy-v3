typedef struct { volatile LONG counter; } atomic_t;

// Modern 486+ processor
static inline LONG atomic_add_return(LONG i, atomic_t *v)
{
     int __i = i;
     asm volatile("lock xaddl %0, %1"
                   : "+r" (i), "+m" (v->counter)
                   : : "memory");
     return i + __i;
}

LONG InterlockedIncrement(LONG volatile *Addend) { return atomic_add_return(1,  (atomic_t*)Addend); } // { return ++(*lpValue); }
LONG InterlockedDecrement(LONG volatile *Addend) { return atomic_add_return(-1, (atomic_t*)Addend); } // { return --(*lpValue); }

// Win32 implementation
//__declspec(naked) __stdcall LONG InterlockedIncrement(LONG volatile *lpAddend)
//{
//	__asm
//	{
//		mov  ecx, dword ptr [esp+4]
//		mov  eax, 1 // 0xFFFFFFFF for Decrement
//		lock xadd dword ptr [ecx],eax  // eax_old= eax, eax = [ecx], [ecx]=[ecx]+eax_old
//		inc  eax    // 'dec eax' for Decrement
//		ret  4
//	}
//}

int lstrlenA(LPCSTR lpString)
{
	return (lpString) ? strlen(lpString) : 0;
}

void strupr(char *p)
{
  	while (*p) *(p++) = toupper(*p);
}

void strlwr(char *p)
{
	while (*p) *(p++) = tolower(*p);
}

char* strrev(char* szT)
{
	if ( !szT )                 // handle null passed strings.
        return (char*)"";

	int i = strlen(szT);
    int t = !(i % 2)? 1 : 0;      // check the length of the string .
    for (int j = i - 1 , k = 0 ; j > (i / 2 - t) ; j-- )
    {
    	char ch  = szT [j];
		szT [j]  = szT [k];
        szT [k++] = ch;
    }
    return szT;
}

/* can be more quickly without allocate memory
int _stricoll(const char *string1, const char *string2)
{
	char *string1copy = new char [strlen(string1)];
	char *string2copy = new char [strlen(string2)];
	strupr(string1copy);
	strupr(string2copy);
	int ret = strcoll(string1copy, string2copy);
	delete [] string1copy;
	delete [] string2copy;
	return ret;
}
*/
