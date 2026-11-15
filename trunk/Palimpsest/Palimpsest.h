// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      03nov26	initial version

*/

// Palimpsest.h : main header file for the application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "WinAppCK.h"
#include "CritSec.h"
#include "PalimpsestDraw.h"

// CPalimpsestApp:
// See Palimpsest.cpp for the implementation of this class
//

class CPalimpsestApp : public CWinAppCK
{
public:
	CPalimpsestApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Operations
	void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
	bool	WriteLogEntry(CString sLogEntry);
	static bool	IsSpace(TCHAR c);

// Public data
	CPalimpsestDraw	m_rt;	// render thread

// Implementation
protected:
	WCritSec	m_csErrorLog;		// critical section for error log file

	DECLARE_MESSAGE_MAP()
};

inline bool CPalimpsestApp::IsSpace(TCHAR c)
{
	return c == ' ' || (c >= '\n' && c <= '\r');	// true if ASCII whitespace
}

extern CPalimpsestApp theApp;
