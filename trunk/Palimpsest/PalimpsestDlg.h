// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda
 
		revision history:
		rev		date	comments
		00      03nov26	initial version
		01		13nov26	port to render thread

*/

// PalimpsestDlg.h : header file
//

#pragma once

// CPalimpsestDlg dialog

class CPalimpsestDlg : public CDialogEx
{
// Construction
public:
	CPalimpsestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PALIMPSEST_DIALOG };

// Attributes

// Operations
	void	ShowHelp();
	void	FullScreen(bool bEnable);
	void	Pause(bool bEnable);
	void	SingleStep();

// Implementation
protected:
// Types

// Constants
	enum {
		IDM_ABOUTBOX	= 0x0010,
		IDM_SHOWHELP	= 0x0020,
		IDM_FULLSCREEN	= 0x0030,
	};

// Member data
	HICON m_hIcon;
	bool	m_bIsDrawReady;
	bool	m_bIsFullScreen;	// true if in full screen mode
	bool	m_bIsFullScreenPending;	// true while changing full screen mode
	bool	m_bIsPaused;		// true while paused

// Overrides
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Helpers
	bool	CustomizeSystemMenu();

// Message handlers
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg LRESULT OnDelayedCreate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFullScreenChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRenderThreadExit(WPARAM wParam, LPARAM lParam);
};
