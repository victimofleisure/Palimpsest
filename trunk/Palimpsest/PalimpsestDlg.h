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
		03		06nov26	add musical accompaniment

*/

// PalimpsestDlg.h : header file
//

#pragma once

#include "MidiWrap.h"
#include "RandList.h"
#include "Benchmark.h"
#include "Statistics.h"
#include "MidiFile.h"

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
		LT_MAXIMIZE,
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
	RGBQUAD	m_clrStart;		// starting color for tweening
	RGBQUAD	m_clrTarget;	// target color for tweening
	int		m_nRandSeed;	// random number seed
	int		m_nTimerPeriod;	// timer period in milliseconds
	int		m_nSpeed;		// expansion/contraction speed, in pixels per tick
	bool	m_bContracting;	// true if ellipse is contracting
	bool	m_bIsFullScreen;	// true if full screen
	bool	m_bIsPaused;	// true if paused
	bool	m_bSingleStep;	// true if advancing by one frame while paused
	bool	m_bFreshStart;	// true if backbuffer was recreated
	int		m_nLaunchType;	// launch app in full screen mode or maximized; see enum
	DWORD	m_nFrameCount;	// frame counter for capture
	DWORD	m_nPrevFrameCount;	// for monitoring frame rate
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

	// musical accompaniment

	enum {	// continuous controller numbers
		CC_VOLUME			= 71,	// Combinator Control 1 -> Line Mixer Channel 1 Volume
		CC_PAN				= 72,	// Combinator Control 2 -> Line Mixer Channel 1 pan
		CC_AMP_ENV_ATTACK	= 73,	// Combinator Control 3 -> NN-19 Amplitude Envelope Attack
		CC_AMP_ENV_RELEASE	= 74,	// Combinator Control 4 -> NN-19 Amplitude Envelope Release
		CC_AMP_VEL_AMOUNT	= 2,	// Combinator Breath -> NN-19 Amplitude Velocity Amount
	};
	enum {
		MF_TIMEBASE = 120,		// MIDI file timebase in ticks per quarter note
		MF_TEMPO = 120,			// MIDI file tempo in beats per minute
	};
	CMidiOut	m_midiOut;		// MIDI output device
	BYTE	m_arrColorNote[COLORS];	// each color's note
	BYTE	m_arrChanNote[COLORS];	// each MIDI channel's note
	BYTE	m_arrChanVolume[COLORS];	// each MIDI channel's volume
	int		m_nCurMidiChan;		// current MIDI channel
	int		m_nChanCycles;		// number of cycles through all channels
	bool	m_bIsMidiExport;	// true if exporting a MIDI file; otherwise live MIDI
	BYTE	m_nPrevNote;		// previous note
	CMidiFile::CMidiEventArray	m_arrMidiEvent[COLORS];	// array of MIDI events for each channel
	CStdioFile	m_fLog;			// log file

	void	MakeScale(int nStartNote);
	void	PrintScale();
	bool	MidiOpen();
	void	MidiInit();
	void	MidiClose();
	void	MidiStartDraw();
	void	MidiContinueDraw(double fPos);
	void	MidiOut(int nCmd, int nChan, int nP1, int nP2);
	static DWORD	GetFrameTick(int iFrame);
	void	MidiFileInit();
	void	WriteMidiFile(LPCTSTR pszPath);

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
