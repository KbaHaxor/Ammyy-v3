#include "stdafx.h"
#include "FastQueue.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

const DWORD NewBlockSize = 16*1024;
const bool  ReUseNodes = false;

CFastQueue::CNode*  CFastQueue::m_pNodesForReUsing = NULL;


CFastQueue::CFastQueue()
{
	m_pNodeFirst = NULL;
	m_pNodeLast  = NULL;
	m_DataLen	 = 0;
}

CFastQueue::~CFastQueue()
{
	while(m_pNodeFirst!=NULL) {
		CNode* pNextNode = m_pNodeFirst->Delete();
		m_pNodeFirst = pNextNode;
	}
}


bool CFastQueue::HasData()
{
	return (m_DataLen>0);
}


void CFastQueue::AddNewNode(DWORD size)
{
	CNode* pNode; 

	if (ReUseNodes && m_pNodesForReUsing!=NULL) {
		pNode = m_pNodesForReUsing;
		m_pNodesForReUsing = m_pNodesForReUsing->pNextNode;
		pNode->pNextNode = NULL;
	}
	else {
		pNode = new CNode(NewBlockSize);
	}

	if (m_pNodeLast != NULL) 
		m_pNodeLast->pNextNode = pNode;
	else
		m_pNodeFirst = pNode;	// this's the first node 

	m_pNodeLast = pNode;
}


void CFastQueue::PushData(const char* buffer, DWORD count)
{	
	m_DataLen += count;

	DWORD pushed = 0;

	//try write to the last node
	if (m_pNodeLast!=NULL) {
		pushed += m_pNodeLast->PushData(buffer, count);
	}

	while(true)
	{
		DWORD countRemain = count-pushed;
		if (countRemain<=0) break;

		AddNewNode(NewBlockSize);

		pushed += m_pNodeLast->PushData(buffer+pushed, countRemain);
	}
}


DWORD CFastQueue::PushDataEmpty(DWORD count, char** ppBuffer)
{	
	//try write to the last node
	if (m_pNodeLast!=NULL) {
		DWORD c = m_pNodeLast->PushDataEmpty(count, ppBuffer);
		if (c>0) {
			m_DataLen += c;
			return c;
		}
	}

	AddNewNode(NewBlockSize);

	DWORD c = m_pNodeLast->PushDataEmpty(count, ppBuffer);
	m_DataLen += c;
	return c;
}




// skip only for first node!!!, made for "commit transaction" for GetReadBuffer()
//
DWORD CFastQueue::ReadSkip(DWORD count)
{
	if (m_pNodeFirst == NULL)
		return 0;	

	DWORD curr = 0;
	while(curr<count)
	{
		CNode* pNode = m_pNodeFirst;

		curr += pNode->ReadSkip(count-curr);

		if (pNode->CanBeDeleted()) {
			m_pNodeFirst = pNode->Delete();

			if (m_pNodeFirst==NULL) {
				m_pNodeLast = NULL;				
				break;	// it's the last block
			}
		}
		else
			break;
	}
	
	m_DataLen -= curr;

	return curr;
}


DWORD CFastQueue::TakeData(char* buffer, DWORD count)
{
	if (m_pNodeFirst == NULL)
		return 0;	

	DWORD curr = 0;
	while(curr<count)
	{
		CNode* pNode = m_pNodeFirst;

		curr += pNode->ReadData(buffer+curr, count-curr, false);

		if (pNode->CanBeDeleted()) {
			m_pNodeFirst = pNode->Delete();

			if (m_pNodeFirst==NULL) {
				m_pNodeLast = NULL;				
				break;	// it's the last block
			}
		}
		else
			break;
	}

	//_log.WriteInfo("CFastQueue::TakeData() %d %d", count, taken);
	
	m_DataLen -= curr;

	return curr;
}


DWORD CFastQueue::PeekData(char* buffer, DWORD count)
{
	if (m_pNodeFirst == NULL)
		return 0;

	CNode* pNode = m_pNodeFirst;

	DWORD taken = 0;
	while(taken<count && pNode!=NULL)
	{
		taken += pNode->ReadData(buffer+taken, count-taken, true);

		pNode = pNode->pNextNode;
	}

	return taken;
}


// get count bytes that can be read in the first node and buffer for reading data
//
DWORD CFastQueue::GetReadBuffer(char** ppBuffer) 
{
	if (m_pNodeFirst==NULL) {
		*ppBuffer = NULL;
		return 0;
	}
	else {
		return m_pNodeFirst->GetBufferRd(ppBuffer);
	}
}



int CFastQueue::GetDataLen()
{
	return m_DataLen;
}


//_________________________________________________________________________________________________

CFastQueue::CNode::CNode(DWORD len)
{
	pNextNode	= NULL;
	pData		= new char[len];
	cLen		= len;
	cRead		= 0;
	cWritten	= 0;
}

CFastQueue::CNode::~CNode()
{
	if (pData!=NULL) 
		delete[] pData;
}


DWORD CFastQueue::CNode::PushData(const char* buffer, DWORD count)
{
	DWORD willWrite = min(cLen-cWritten, count);
	memcpy(pData+cWritten, buffer, willWrite);
	cWritten += willWrite;
	return willWrite;
}


DWORD CFastQueue::CNode::ReadData(char* buffer, DWORD count, bool peek)
{
	DWORD canTake = min(cWritten-cRead, count);
	memcpy(buffer, pData+cRead, canTake);
	if (!peek) cRead += canTake;
	return canTake;
}


DWORD CFastQueue::CNode::GetBufferRd(char** ppBuffer) const
{
	*ppBuffer = pData+cRead;
	return cWritten - cRead;
}

DWORD CFastQueue::CNode::PushDataEmpty(DWORD count, char** ppBuffer)
{
	DWORD c = min(cLen - cWritten, count);
	*ppBuffer = pData+cWritten;
	cWritten += c;
	return c;
}

DWORD CFastQueue::CNode::ReadSkip(DWORD count)
{
	count = min(count, cWritten-cRead);
	cRead += count;
	return count;
}


// detele this node and return next node
//
CFastQueue::CNode* CFastQueue::CNode::Delete()
{
	CNode* pNextNode_2 = pNextNode;

	if (ReUseNodes) {
		pNextNode = m_pNodesForReUsing;
		cWritten  = 0;
		cRead     = 0;
		m_pNodesForReUsing = this;
	}
	else { 
		delete this;
	}
	return pNextNode_2;
}