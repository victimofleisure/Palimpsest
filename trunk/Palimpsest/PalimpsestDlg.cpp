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

// PalimpsestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Palimpsest.h"
#include "PalimpsestDlg.h"
#include "AboutDlg.h"
#include "ProgressDlg.h"
#include "HelpDlg.h"
#include "PathStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPalimpsestDlg dialog

CPalimpsestDlg::CPalimpsestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPalimpsestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bIsDrawReady = false;
	m_bIsFullScreen = false;
	m_bIsFullScreenPending = false;
	m_bIsPaused = false;
}

bool CPalimpsestDlg::CustomizeSystemMenu()
{
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu == NULL)
		return false;
	CString sMenuItem;
	VERIFY(sMenuItem.LoadString(IDS_FULL_SCREEN));
	VERIFY(pSysMenu->InsertMenu(SC_MINIMIZE, 0, IDM_FULLSCREEN, sMenuItem));
	VERIFY(pSysMenu->AppendMenu(MF_SEPARATOR));
	VERIFY(sMenuItem.LoadString(IDS_HELP));
	VERIFY(pSysMenu->AppendMenu(MF_STRING, IDM_SHOWHELP, sMenuItem));
	VERIFY(sMenuItem.LoadString(IDS_ABOUTBOX));
	VERIFY(pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, sMenuItem));
	return true;
}

void CPalimpsestDlg::ShowHelp()
{
	CHelpDlg	dlgHelp;
	dlgHelp.DoModal();
}

void CPalimpsestDlg::FullScreen(bool bEnable)
{
	if (bEnable == m_bIsFullScreen)	// if already in requested state
		return;	// nothing to do
	if (m_bIsFullScreenPending)	// if mode switch in progress
		return;	// don't queue another
	m_bIsFullScreenPending = true;	// lock mode switch
	if (!theApp.m_rt.FullScreen(bEnable)) {	// queue mode switch; if queueing fails
		m_bIsFullScreenPending = false;	// reset mode switch
	}
}

void CPalimpsestDlg::Pause(bool bEnable)
{
	if (bEnable == m_bIsPaused)	// if already in requested state
		return;	// nothing to do
	if (theApp.m_rt.Pause(bEnable))
		m_bIsPaused = bEnable;
}

void CPalimpsestDlg::SingleStep()
{
	if (!m_bIsPaused)	// if not paused
		return;	// nothing to do
	theApp.m_rt.SingleStep();
}

bool CPalimpsestDlg::Export()
{
	if (!theApp.m_rt.CreateExport())
		return false;
	CProgressDlg	dlgProgress;
	if (!dlgProgress.Create())
		return false;
	UINT	nFrames = theApp.m_args.m_nRecDuration;
	dlgProgress.SetRange(0, nFrames);
	for (UINT iFrame = 0; iFrame < nFrames; iFrame++) {	// for each frame
		dlgProgress.SetPos(iFrame);
		if (dlgProgress.Canceled())
			return false;
		CString	sFileName;
		sFileName.Format(_T("img%06d"), iFrame);
		CPathStr sPath(theApp.m_args.m_sRecFolderPath);
		sPath.Append(sFileName + _T(".png"));
		if (!theApp.m_rt.ExportFrame(sPath))
			return false;
	}
	theApp.m_rt.WriteMidiFile(_T("palimpsest.mid"));
	return true;
}

// CPalimpsestDlg overrides

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

BOOL CPalimpsestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CustomizeSystemMenu();

	PostMessage(UWM_DELAYED_CREATE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPalimpsestDlg::OnOK()
{
}

void CPalimpsestDlg::OnCancel()
{
}

BOOL CPalimpsestDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		switch (pMsg->wParam) {
		case VK_F1:
			if (!m_bIsFullScreen)
				ShowHelp();
			return true;
		case VK_F11:
			FullScreen(!m_bIsFullScreen);
			return true;
		case VK_ESCAPE:
			if (m_bIsFullScreen) {
				FullScreen(false);
				return true;
			}
			break;
		case VK_SPACE:
			if (GetKeyState(VK_CONTROL) & GKS_DOWN) {
				SingleStep();
			} else {
				Pause(!m_bIsPaused);
			}
			return true;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

// CPalimpsestDlg message map

BEGIN_MESSAGE_MAP(CPalimpsestDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_SYSCOMMAND()
	ON_WM_SETCURSOR()
	ON_MESSAGE(UWM_DELAYED_CREATE, OnDelayedCreate)
	ON_MESSAGE(UWM_FULL_SCREEN_CHANGED, OnFullScreenChanged)
	ON_MESSAGE(UWM_RENDER_THREAD_EXIT, OnRenderThreadExit)
END_MESSAGE_MAP()

// CPalimpsestDlg message handlers

void CPalimpsestDlg::OnClose()
{
	// command render thread to exit; it notifies when it has exited
	m_bIsDrawReady = false;	// don't queue more commands after exit
	theApp.m_rt.SignalExit();	// signal render thread to exit
}

void CPalimpsestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		ValidateRect(NULL);	// suppress paint; Direct2D does drawing
	}
}

void CPalimpsestDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (m_bIsDrawReady)
		theApp.m_rt.Resize();
}

void CPalimpsestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch (nID & 0xFFF0) {
	case IDM_ABOUTBOX:
		{
			CAboutDlg dlgAbout;
			dlgAbout.DoModal();
		}
		break;
	case IDM_SHOWHELP:
		ShowHelp();
		break;
	case IDM_FULLSCREEN:
		FullScreen(!m_bIsFullScreen);
		break;
	default:
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

BOOL CPalimpsestDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bIsFullScreen) {
		SetCursor(NULL);
		return TRUE;
	}
	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

LRESULT CPalimpsestDlg::OnDelayedCreate(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	if (!theApp.m_args.m_sRecFolderPath.IsEmpty()) {	// if exporting
		Export();	// do the export; handles its own errors
		theApp.m_rt.Destroy();	// clean up resources unconditionally
		EndDialog(IDOK);	// close dialog
	} else {	// normal launch
		if (theApp.m_rt.Create(m_hWnd, this)) {	// create render thread
			m_bIsDrawReady = true;
		} else {	// create failed
			EndDialog(IDOK);	// close dialog
		}
	}
	return 0;
}

LRESULT CPalimpsestDlg::OnFullScreenChanged(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	m_bIsFullScreen = (wParam != 0);	// update our state
	m_bIsFullScreenPending = false;	// unlock mode switch
	return 0;
}

LRESULT CPalimpsestDlg::OnRenderThreadExit(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	EndDialog(IDOK);	// render thread exited, so it's safe to end
	return 0;
}
