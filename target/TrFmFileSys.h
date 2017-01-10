#ifndef _TR_FM_FILESYSTEM_H_
#define _TR_FM_FILESYSTEM_H_

#include <list>
#include <vector>

#include "../main/TransportT.h"


class TrFmLookUpBase
{
public:
	void Do();
protected:
	virtual void OnBegin() = 0;
	virtual void OnAdd(WIN32_FIND_DATAW& fd) = 0;
	virtual void OnBeginDrives(DWORD drives) {};
	virtual void OnAddDrive(LPCWSTR path) = 0;
	virtual void OnEndDrives() = 0;

public:
	DWORD	m_dwError;

protected:
	static inline UINT64 GetFileSize64(const WIN32_FIND_DATAW& fd) 
	{ 
		return ((UINT64)fd.nFileSizeHigh << 32) + fd.nFileSizeLow;
	}

	LPCWSTR m_path;
};

class TrFmFileSys
{
public:
	struct FileSize {
		CStringW name;
		UINT64   size;
	};

	enum ItemType
	{
		TYPE_DOTS = 0,
		TYPE_DISK = 1,
		TYPE_DIR  = 2,
		TYPE_FILE = 4,
		TYPE_LINK_0 = 0x10, // My desktop
		TYPE_LINK_1 = 0x11, // My Documents
	};

	typedef std::list<FileSize>		FileList;

	enum CONSTANTS {
		INIT_BLOCKS = 5,
		BLOCK_SIZE  = 16*1024,
	};

	static void inline SetLenUINT16(RLStream& buf,int off) { *(UINT16*)((char*)buf.GetBuffer()+off) = UINT16(buf.GetLen()-off-2); }
	static void inline SetLenUINT32(RLStream& buf,int off) { *(UINT32*)((char*)buf.GetBuffer()+off) = UINT32(buf.GetLen()-off-4); }

	static CStringW GetFileExt(LPCWSTR fileName);

	inline static int GetNextBlockSize(const INT64 total, const INT64 done)
	{
		ASSERT(done>=0);
		ASSERT(done<=total);
		INT64 size = total - done;
		return (size >= TrFmFileSys::BLOCK_SIZE) ? TrFmFileSys::BLOCK_SIZE : (int)size;
	}

	static void FillLinks(CStringW* links);

	static DWORD ReadFile (HANDLE hFile, LPVOID  lpBuffer, DWORD nNumberOfBytes);
	static DWORD WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytes);
	static INT64 GetFileSize(HANDLE hFile);
	static INT64 GetFileSizeByName(LPCWSTR fileName);
	
	static bool IsFolder(const CStringW& path);
	static void ThisFolder(CStringW& path)  { if (!IsFolder(path)) path += '\\'; }
	static bool IsSpecialName(LPCWSTR name) { return (!wcscmp(name, L".") || !wcscmp(name, L"..")); }
	static bool IsDirAndLink(DWORD dwFileAttr) 
	{
		return ((dwFileAttr & FILE_ATTRIBUTE_REPARSE_POINT) && (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY));
	}

	static DWORD CreateRecurseDir(LPCWSTR dirName);

	static DWORD DeleteDir(LPCWSTR dirName, CStringW &path);

	//static DWORD GetMD5ForFile(LPCWSTR fileName, INT64& size, void* bufferOut);
	static DWORD GetMD5ForFile(HANDLE hFile,     INT64  size, void* bufferOut);

	// »щет рекурсивно файлы дл€ указанной директории и возвращает количество найденных файлов.
	//
	static UINT32 SearchRecursively(LPCWSTR pathName, FileList& fList, bool relative, const CStringW& relPath, DWORD& dwError, CStringW& errorPath);

	static int    AddFileSize(RLStream& buffer, UINT64 size);
	static UINT64 GetFileSize(RLStream& buffer);
	static UINT64 GetFileSize(Transport* pTranport);
};

#endif // _TR_FM_FILESYSTEM_H_
