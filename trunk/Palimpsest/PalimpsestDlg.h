// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      03nov26	initial version
		01		05nov26	make export offline
		02		05nov26	add supersampling

*/

// PalimpsestDlg.h : header file
//

#pragma once

#include "MidiWrap.h"
#include "RandList.h"
#include "Benchmark.h"
#include "Statistics.h"

// CPalimpsestDlg dialog
class CPalimpsestDlg : public CDialogEx
{
// Construction
public:
	CPalimpsestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PALIMPSEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Operations
	void	FullScreen(bool bEnable);
	void	Pause(bool bEnable);
	void	SingleStep();

// Implementation
protected:
	HICON m_hIcon;

	enum {
		COLORS = 16,	// number of colors in a 4-bit DIB
	};
	enum {	// following IDs must be in system command range
		IDM_ABOUTBOX		= 0x0010,
		IDM_FULL_SCREEN		= 0x0020,
	};
	enum {	// launch types
		LT_NORMAL,
		LT_MAXIMIZED,
		LT_FULLSCREEN,
	};
	CDC		m_dcBack;	// backbuffer device context
	CBitmap	m_bmpBack;	// backbuffer bitmap
	CBitmap	*m_pOldBitmap;	// previous bitmap
	CSize	m_szBackBuf;	// size of backbuffer
	ULONG_PTR	m_gdiplusToken;	// for GDI+ startup

	CRandList	m_rlColor;	// shuffle for colors
	CRandList	m_rlChanMask;	// shuffle for color channel masks
	CPoint	m_ptCenter;		// center point of current ellipse
	CSize	m_szEllipse;	// size of current ellipse
	int		m_iCurColor;	// color of current ellipse, as a palette index
	int		m_nChanMask;	// color channel mask of current ellipse
	int		m_nRadius;		// radius of current ellipse
	int		m_nTimerPeriod;	// timer period in milliseconds
	RGBQUAD	m_clrStart;		// starting color for tweening
	RGBQUAD	m_clrTarget;	// target color for tweening
	bool	m_bContracting;	// true if ellipse is contracting
	bool	m_bIsFullScreen;	// true if full screen
	bool	m_bIsPaused;	// true if paused
	bool	m_bSingleStep;	// true if advancing by one frame while paused
	bool	m_bFreshStart;	// true if backbuffer was recreated
	int		m_nLaunchType;	// launch app in full screen mode or maximized; see enum
	int		m_nSpeed;		// expansion/contraction speed, in pixels per tick
	LONGLONG	m_nFrameCount;	// frame counter for capture
	LONGLONG	m_nPrevFrameCount;	// for monitoring frame rate
	CBenchmark	m_bmFrameTimer;	// for monitoring frame rate
	CStatistics	m_stat;		// for performance monitoring

	DWORD	m_dwWindowStyle;
	DWORD	m_dwWindowExStyle;
	WINDOWPLACEMENT	m_wpWindow;

	CMMTimerPeriod	m_timerResolution;	// multimedia timer resolution
	CMMTimer	m_timer;	// multimedia timer 

	static void CALLBACK TimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	void	InitGeom();
	bool	CreateBackbuffer(CSize sz);
	bool	SaveFrame(LPCTSTR pszPath);
	void	RenderFrame();
	bool	ExportVideo(CSize szFrame, int nFrames, LPCTSTR pszFolder);

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	// Generated message map functions
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnMMTimer(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnDelayedCreate(WPARAM wParam, LPARAM lParam);
};
