// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      26oct25	initial version
        01      15nov25	add attach stdout to parent console

*/

// stdafx.cpp : source file that includes just the standard includes
// Palimpsest.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "resource.h"

CString FormatSystemError(DWORD ErrorCode)
{
	LPTSTR	lpszTemp;
	DWORD	bRet = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, ErrorCode, 0, (LPTSTR)&lpszTemp, 0, NULL);	// default language
	CString	sError;	
	if (bRet) {	// if format succeeded
		sError = lpszTemp;	// create string from message buffer
		LocalFree(lpszTemp);	// free message buffer
	} else	// format failed
		sError.Format(IDS_APP_UNKNOWN_SYSTEM_ERROR, ErrorCode, ErrorCode);
	return(sError);
}

CString	GetLastErrorString()
{
	return(FormatSystemError(GetLastError()));
}

bool FilesEqual(LPCTSTR pszFile1, LPCTSTR pszFile2)
{
	CFile f1(pszFile1, CFile::modeRead);
	CFile f2(pszFile2, CFile::modeRead);
	UINT nSize = (UINT)f1.GetLength();
	if (nSize != (UINT)f2.GetLength())
		return false;
	CByteArray b1, b2;
	b1.SetSize(nSize);
	b2.SetSize(nSize);
	f1.Read(b1.GetData(), nSize);
	f2.Read(b2.GetData(), nSize);
	return !memcmp(b1.GetData(), b2.GetData(), nSize);
}

int WildcardDeleteFile(CString sPath)
{
	// Note that the destination path is double-null terminated. CString's
	// get buffer method allocates the specified number of characters plus
	// one for the null terminator, but we need space for two terminators,
	// hence we must increment nPathLen.
	int	nPathLen = sPath.GetLength();
	LPTSTR	pszPath = sPath.GetBufferSetLength(nPathLen + 1);
	pszPath[nPathLen + 1] = '\0';	// double-null terminated string
	SHFILEOPSTRUCT	SHFileOp;
	ZeroMemory(&SHFileOp, sizeof(SHFileOp));
	SHFileOp.wFunc = FO_DELETE;
	SHFileOp.pFrom = pszPath;
	SHFileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_FILESONLY | FOF_NORECURSION;
	return SHFileOperation(&SHFileOp);
}

bool AttachStdoutToParentConsole()
{
	// if stdout is already a console, file or pipe, leave it be
	HANDLE	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut != NULL && hStdOut != INVALID_HANDLE_VALUE) {
		DWORD	nFileType = GetFileType(hStdOut);
		switch (nFileType) {
		case FILE_TYPE_CHAR:
		case FILE_TYPE_DISK:
		case FILE_TYPE_PIPE:
			return true;	// stdout is already usable
		}
	}
	// try to attach stdout to console of parent process
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        return false;
	FILE	*pStream = NULL;
	if (freopen_s(&pStream, "CONOUT$", "w", stdout))
		return false;
	hStdOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hStdOut == INVALID_HANDLE_VALUE)
		return false;
	if (!SetStdHandle(STD_OUTPUT_HANDLE, hStdOut))
		return false;
	return true;	// success
}
