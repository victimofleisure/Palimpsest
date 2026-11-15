// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      14nov25	initial version

*/

// HelpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "HelpDlg.h"
#include "afxdialogex.h"
#include "ParamParser.h"

// CHelpDlg dialog

IMPLEMENT_DYNAMIC(CHelpDlg, CDialog)

CHelpDlg::CHelpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHelpDlg::IDD, pParent)
{
}

CHelpDlg::~CHelpDlg()
{
}

void CHelpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HELP_EDIT, m_edit);
}

BEGIN_MESSAGE_MAP(CHelpDlg, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()

// CHelpDlg message handlers

BOOL CHelpDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_edit.SetTabStops(80);	// hand-tune as needed
	CString	sHelp(CParamParser::GetHelpString());
	sHelp.Replace(_T("\n"), _T("\r\n"));	// replace newline with CRLF
	m_edit.SetWindowText(sHelp);	// pass help text to edit control
	m_edit.PostMessage(EM_SETSEL, WPARAM(-1), 0);	// remove initial selection
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CHelpDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (m_edit.m_hWnd) {	// if control exists
		m_edit.MoveWindow(0, 0, cx, cy);	// fit control to window
	}
}


BOOL CHelpDlg::PreTranslateMessage(MSG* pMsg)
{
	// dialog lacks an OK button so handle Enter key explicitly
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		OnOK();	// close dialog
	}
	return CDialog::PreTranslateMessage(pMsg);
}
