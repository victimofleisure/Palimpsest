// Copyleft 2005 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      02jun05	initial version
        01      01jul12	fix Rand to avoid possible overrun
        02      06jan13	add cast to GetSize for x64
		03		07jun21	rename rounding functions
		04		03mar25	modernize style
		05		26oct25	overload get next to avoid duplicate values
		06		16nov26	fix no-repeat shuffle to preserve all values

		random sequence without duplicates
 
*/

#include "stdafx.h"
#include "RandList.h"

CRandList::CRandList()
{
	m_nAvail = 0;
	m_nCurVal = -1;
}

CRandList::CRandList(int nSize)
{
	Init(nSize);
}

void CRandList::Init(int nSize)
{
	m_List.SetSize(nSize);
	for (int i = 0; i < nSize; i++)
		m_List[i] = i;
	m_nAvail = 0;
	m_nCurVal = -1;
}

int CRandList::Rand(int nVals)
{
	if (nVals <= 0)
		return -1;
	int	i = Trunc(rand() / double(RAND_MAX) * nVals);
	return min(i, nVals - 1);
}

int CRandList::GetNext()
{
	if (!m_nAvail)
		m_nAvail = GetSize();
	ASSERT(m_nAvail > 0);
	int	idx = Rand(m_nAvail);
	m_nAvail--;
	int	tmp = m_List[idx];
	m_List[idx] = m_List[m_nAvail];
	m_List[m_nAvail] = tmp;
	return tmp;
}

int	CRandList::GetNextNoRepeat()
{
	int nVal = GetNext();
	if (nVal == m_nCurVal && m_nAvail) {
		int idx = Rand(m_nAvail);
		m_List[m_nAvail] = m_List[idx];
		m_List[idx] = nVal;
		nVal = m_List[m_nAvail];
	}
	m_nCurVal = nVal;
	return nVal;
}
