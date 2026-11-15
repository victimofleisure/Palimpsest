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

// Palimpsest.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Palimpsest.h"
#include "PalimpsestDlg.h"
#include "Win32Console.h"
#include "StdioFileEx.h"
#include "PathStr.h"
#include "ParamParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPalimpsestApp

BEGIN_MESSAGE_MAP(CPalimpsestApp, CWinApp)
END_MESSAGE_MAP()

// CPalimpsestApp construction

CPalimpsestApp::CPalimpsestApp()
{
}

// The one and only CPalimpsestApp object

CPalimpsestApp theApp;

// CPalimpsestApp initialization

BOOL CPalimpsestApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

#ifdef _DEBUG
	Win32Console::Create();
#endif

	AfxEnableControlContainer();

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	SetRegistryKey(_T("Anal Software"));

	{
		CParamParser	parser;
		TRY {
			if (!parser.ParseCommandLine())
				return false;
		}
		CATCH (CFileException, e) {
			e->ReportError();	// report the exception
			return false;	// failure
		}
		END_CATCH
		m_rt.SetParams(parser);
	}

	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
		AfxMessageBox(IDS_ERR_CANT_INIT_COM);
		return false;
	}

	CPalimpsestDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CPalimpsestApp::ExitInstance()
{
	// render thread should already be destroyed by this point
	m_rt.Destroy();	// but in case not, destroy render thread
	CoUninitialize();
	return CWinApp::ExitInstance();
}

void CPalimpsestApp::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
#ifdef _DEBUG
	printf("%x %s %d %s\n", hr, pszSrcFileName, nLineNum, pszSrcFileDate);
#endif
	CString	sSrcFileName(pszSrcFileName);	// convert to Unicode
	CString	sSrcFileDate(pszSrcFileDate);
	CString	sErrorMsg;
	sErrorMsg.Format(_T("Error %d (0x%x) in %s line %d (%s)"), hr, hr,
		sSrcFileName.GetString(), nLineNum, sSrcFileDate.GetString());
	sErrorMsg += '\n' + FormatSystemError(hr);
	WriteLogEntry(sErrorMsg);
	UINT	nMBFlags = MB_RETRYCANCEL | MB_ICONSTOP;
	if (AfxMessageBox(sErrorMsg, nMBFlags) == IDCANCEL) {	// if user canceled
		if (m_pMainWnd != NULL)	// if valid main window pointer
			PostMessage(m_pMainWnd->m_hWnd, WM_CLOSE, 0, 0);	// exit app
	}
}

bool CPalimpsestApp::WriteLogEntry(CString sLogEntry)
{
	// This method is callable by worker threads, and access to the log file
	// is therefore serialized via a critical section. The caller may not be
	// expecting MFC exceptions, but both CString and CStdioFile throw them,
	// so the entire method is wrapped in an MFC-style TRY block.
	TRY {
		// the intended path is %USERPROFILE%\AppData\Local\AppName\AppName.log
		CString	sLogFolder;
		GetLocalAppDataFolder(sLogFolder);
		if (!CreateFolder(sLogFolder)) {	// if unable to create folder
			AfxMessageBox(IDS_APP_ERR_CANT_CREATE_APP_DATA_FOLDER);
			return false;	// failure
		}
		CPathStr	sLogPath(sLogFolder);	// start with the log folder
		sLogPath.Append(m_pszAppName);	// append the application name
		sLogPath += _T(".log");	// append the log file extension
		SYSTEMTIME	sysTime;
		GetSystemTime(&sysTime);
		CString	sTimestamp;
		sTimestamp.Format(_T("%04d-%02d-%02d %02d:%02d:%02d.%03d"),	// ISO 8601
			sysTime.wYear, sysTime.wMonth, sysTime.wDay,
			sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
		sLogEntry.Replace('\n', ' ');	// replace any internal newlines with spaces
		sLogEntry.Remove('\r');	// remove all internal carriage returns
		sLogEntry += '\n';	// append a single trailing newline
		// Enter the critical section as late as possible; we'll exit
		// it automatically when the lock instance goes out of scope.
		WCritSec::Lock	lock(m_csErrorLog);	// serialize access to the log file
		CStdioLogFile	fLog(sLogPath);	// open UTF-8, append, text, commit
		fLog.WriteString(sTimestamp + ' ' + sLogEntry);	// write the log entry
		fLog.Flush();	// force any buffered data to be written to the file
	}
	CATCH (CFileException, e) {
		e->ReportError();	// report the exception
		return false;	// failure
	}
	END_CATCH
	return true;	// success
}
