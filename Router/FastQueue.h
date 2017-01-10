#if !defined(AFX_FASTQUEUE_H__59658E7E_DB71_457D_B44D_EEBFE8234413__INCLUDED_)
#define AFX_FASTQUEUE_H__59658E7E_DB71_457D_B44D_EEBFE8234413__INCLUDED_

//#include "../RL/RLStream.h"

class CFastQueue  
{
public:
	CFastQueue();
	~CFastQueue();
	void  PushData(char* buffer, DWORD count);
	DWORD TakeData(char* buffer, DWORD count);
	DWORD GetReadBuffer(char** ppBuffer);
	void  SkipReading(DWORD count);

	int	 m_DataLen;		//count bytes avalaible for reading for all notes

private:
	class CNode {
	public:
		CNode(DWORD len);
		~CNode();
		inline DWORD PushData(char* buffer, DWORD count);
		inline bool  CanBeDeleted() { return (m_data.GetReadPos()==m_data.GetCapacity()); }

		CNode*  Delete();

	public:
		CNode*		m_pNextNode;
		RLStream	m_data;
	};	

	CNode*				m_pNodeFirst;
	CNode*				m_pNodeLast;	

	static CNode* m_pNodesForReUsing;

	class CNode friend;
};

#endif // !defined(AFX_FASTQUEUE_H__59658E7E_DB71_457D_B44D_EEBFE8234413__INCLUDED_)
