#if !defined(AFX_FASTQUEUE_H__59658E7E_DB71_457D_B44D_EEBFE8234413__INCLUDED_)
#define AFX_FASTQUEUE_H__59658E7E_DB71_457D_B44D_EEBFE8234413__INCLUDED_

class CFastQueue  
{
public:
	CFastQueue();
	virtual ~CFastQueue();
	void  PushData(const char* buffer, DWORD count);
	DWORD PushDataEmpty(DWORD count, char** ppBuffer);
	DWORD TakeData(char* buffer, DWORD count);
	DWORD PeekData(char* buffer, DWORD count);
	DWORD GetReadBuffer(char** ppBuffer);
	DWORD GetBufferWr(char** ppBuffer);
	DWORD ReadSkip(DWORD count);
	bool  HasData();

	int GetDataLen();

private:
	class CNode {
	public:
		CNode(DWORD len);
		~CNode();
		DWORD PushData(const char* buffer, DWORD count);		
		DWORD ReadData(char* buffer, DWORD count, bool peek);
		DWORD GetBufferRd(char** ppBuffer) const;
		DWORD PushDataEmpty(DWORD count, char** ppBuffer);
		DWORD ReadSkip(DWORD count);

		CNode*  Delete();

		inline DWORD GetLength()   { return cLen; }
		inline bool CanBeDeleted() { return (cRead==cLen); }
		//inline bool CanBeRead()    { return (cRead<cWritten); }

		CNode* pNextNode;

	private:
		char*  pData;
		DWORD  cLen;		// length of this block
		DWORD  cWritten;	// count written data
		DWORD  cRead;		// count read data
	};	

private:
	void AddNewNode(DWORD size);

	CNode*				m_pNodeFirst;
	CNode*				m_pNodeLast;

	int	 m_DataLen;		//count bytes avalaible for reading (cWritten-cRead) - for all notes

	static CNode* m_pNodesForReUsing;

	class CNode friend;
};

#endif // !defined(AFX_FASTQUEUE_H__59658E7E_DB71_457D_B44D_EEBFE8234413__INCLUDED_)
