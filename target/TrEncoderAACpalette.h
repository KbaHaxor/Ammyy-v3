#ifndef __TR_ENCODER_AAC_PALLETETT_H__82615_INCLUDED_
#define __TR_ENCODER_AAC_PALLETETT_H__82615_INCLUDED_

class TrEncoderAACPalette
{
public:
	struct ColorList {
		ColorList *next;
		int idx;
		UINT32 rgb;
	};

	struct Entry {
		ColorList *listNode;
		int numPixels;
	};

public:
	TrEncoderAACPalette(int maxColors = 254);

	void Reset();

	void SetMaxColors(int maxColors);

	int Insert(UINT32 rgb, int numPixels);

	__forceinline int GetNumColors() const {
		return m_numColors;
	}

	__forceinline UINT32 GetEntry(int i) const {
		return (i < m_numColors) ? m_entry[i].listNode->rgb : (UINT32)-1;
	}

	__forceinline int GetCount(int i) const {
		return (i < m_numColors) ? m_entry[i].numPixels : 0;
	}

	__forceinline UINT8 GetIndex(UINT32 rgb) const 
	{
		ColorList *pnode = m_hash[HashFunc(rgb)];
		while (pnode != NULL) {
			if (pnode->rgb == rgb) {
				return (UINT8)pnode->idx;
			}
			pnode = pnode->next;
		}
		return 0xFF;  
	}

private:
	__forceinline static int HashFunc(UINT32 rgb) { return (rgb ^ (rgb >> 13)) & 0xFF; }

private:
	int m_maxColors;
	int m_numColors;

	Entry		m_entry[256];
	ColorList*	m_hash[256];
	ColorList	m_list[256];
};


#endif // __TR_ENCODER_AAC_PALLETETT_H__82615_INCLUDED_
