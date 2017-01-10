#include "stdafx.h"
#include "FastQueue.h"

const DWORD NewBlockSize = 32*1024;
const bool  ReUseNodes = false; // TODO: with true will work faster, but still can't find problem with crash

CFastQueue::CNode*  CFastQueue::m_pNodesForReUsing = NULL;


CFastQueue::CFastQueue()
{
	m_pNodeFirst = NULL;
	m_pNodeLast  = NULL;
	m_DataLen	 = 0;
}

CFastQueue::~CFastQueue()
{
	while(m_pNodeFirst) {
		m_pNodeFirst = m_pNodeFirst->Delete();
	}
}


void CFastQueue::PushData(char* buffer, DWORD count)
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

		CNode* pNode;

		if (ReUseNodes && m_pNodesForReUsing!=NULL) {
			pNode = m_pNodesForReUsing;
			m_pNodesForReUsing = m_pNodesForReUsing->m_pNextNode;
			pNode->m_pNextNode = NULL;
		}
		else {
			pNode = new CNode(NewBlockSize);
		}

		pushed += pNode->PushData(buffer+pushed, countRemain);

		if (m_pNodeLast != NULL) 
			m_pNodeLast->m_pNextNode = pNode;
		else
			m_pNodeFirst = pNode;	// this's the first node 

		m_pNodeLast = pNode;
	}
}


DWORD CFastQueue::TakeData(char* buffer, DWORD count)
{
	if (m_pNodeFirst == NULL)
		return 0;	

	DWORD taken = 0;
	while(taken<count)
	{
		CNode* pNode = m_pNodeFirst;

		{
			DWORD canTake = min(pNode->m_data.GetAvailForReading(), count-taken);
			pNode->m_data.GetRaw(buffer+taken, canTake);
			taken += canTake;
		}		

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

	//_log::WriteInfo("CFastQueue::TakeData() %d %d", count, taken);
	
	m_DataLen -= taken;

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
		*ppBuffer = m_pNodeFirst->m_data.GetBufferRd();
		return m_pNodeFirst->m_data.GetAvailForReading();
	}
}


// skip only for first node!!!, made for "commit transaction" for GetReadBuffer()
//
void CFastQueue::SkipReading(DWORD count)
{
	if (count==0) return;

	ASSERT(m_pNodeFirst!=NULL);

	RLStream* pData = &m_pNodeFirst->m_data;

	pData->GetRaw(NULL, count);

	// if only 1 node, we can reset it
	if (m_pNodeFirst->m_pNextNode==NULL && pData->GetAvailForReading()==0) {
		pData->Reset();
	}
	else if (m_pNodeFirst->CanBeDeleted()) {		
		m_pNodeFirst = m_pNodeFirst->Delete();

		if (m_pNodeFirst==NULL) {
			m_pNodeLast = NULL;
		}
	}

	m_DataLen -= count;
}

//_________________________________________________________________________________________________

CFastQueue::CNode::CNode(DWORD len):
m_data(len)
{
	m_pNextNode	= NULL;
}

CFastQueue::CNode::~CNode()
{
}


DWORD CFastQueue::CNode::PushData(char* buffer, DWORD count)
{
	DWORD willWrite = min(m_data.GetCapacity() - m_data.GetLen(), count);
	m_data.AddRaw(buffer, willWrite);
	return willWrite;
}


// detele this node and return next node
//
CFastQueue::CNode* CFastQueue::CNode::Delete()
{
	CNode* pNextNode_2 = m_pNextNode;

	if (ReUseNodes) {
		m_data.Reset();
		m_pNextNode = m_pNodesForReUsing;
		m_pNodesForReUsing = this;
	}
	else { 
		delete this;
	}
	return pNextNode_2;
}
