#include <stdafx.h>
#include "TrEncoderAACpalette.h"

TrEncoderAACPalette::TrEncoderAACPalette(int maxColors)
{
	SetMaxColors(maxColors);
	Reset();
}

void TrEncoderAACPalette::Reset()
{
	m_numColors = 0;
	memset(m_hash, 0, 256 * sizeof(TrEncoderAACPalette::ColorList *));
}

void TrEncoderAACPalette::SetMaxColors(int maxColors)
{
	m_maxColors = maxColors;
	if (m_maxColors < 0) {
		m_maxColors = 0;
	} else if (m_maxColors > 254) {
		m_maxColors = 254;
	}
}

int TrEncoderAACPalette::Insert(UINT32 rgb, int numPixels)
{
	ColorList *pnode;
	ColorList *prev_pnode = NULL;
	int hash_key, idx, new_idx, count;

	hash_key = HashFunc(rgb);

	pnode = m_hash[hash_key];

	while (pnode != NULL) {
		if (pnode->rgb == rgb) {
		  new_idx = idx = pnode->idx;
		  count = m_entry[idx].numPixels + numPixels;
		  if (new_idx && m_entry[new_idx-1].numPixels < count) {
			do {
			  m_entry[new_idx] = m_entry[new_idx-1];
			  m_entry[new_idx].listNode->idx = new_idx;
			  new_idx--;
			}
			while (new_idx && m_entry[new_idx-1].numPixels < count);

			m_entry[new_idx].listNode = pnode;
			pnode->idx = new_idx;
		  }
		  m_entry[new_idx].numPixels = count;
		  return m_numColors;
		}
		prev_pnode = pnode;
		pnode = pnode->next;
	}

	if (m_numColors == 256 || m_numColors == m_maxColors) {
		m_numColors = 0;
		return 0;
	}

	for ( idx = m_numColors; idx > 0 && m_entry[idx-1].numPixels < numPixels; idx-- ) {
		m_entry[idx] = m_entry[idx-1];
		m_entry[idx].listNode->idx = idx;
	}

	pnode = &m_list[m_numColors];
	if (prev_pnode != NULL) {
		prev_pnode->next = pnode;
	} else {
		m_hash[hash_key] = pnode;
	}
	pnode->next = NULL;
	pnode->idx = idx;
	pnode->rgb = rgb;
	m_entry[idx].listNode = pnode;
	m_entry[idx].numPixels = numPixels;

	return ++m_numColors;
}
