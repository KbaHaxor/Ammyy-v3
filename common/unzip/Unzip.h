#if !defined(_UNZIP_H__E90EEA8A__INCLUDED_)
#define _UNZIP_H__E90EEA8A__INCLUDED_

class CUnzip  
{
public:	
	//static bool UnzipFromFile(CString fileName, CString pathTo, CStringArray& unzippedFiles);

	static void UnzipFirstFile(LPCWSTR fileNameZip, const CStringW& fileNameUnZip);
	static void UnzipFromBuffer(LPCVOID pBuffer, DWORD bufferLen, LPCWSTR fileNameOut);
};

#endif // !defined(_UNZIP_H__E90EEA8A__INCLUDED_)
