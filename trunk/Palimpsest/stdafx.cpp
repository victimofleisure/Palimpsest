
// stdafx.cpp : source file that includes just the standard includes
// Palimpsest.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "VersionInfo.h"

CString GetVersionString()
{
	VS_FIXEDFILEINFO	AppInfo;
	CString	sVersion;
	CVersionInfo::GetFileInfo(AppInfo, NULL);
	sVersion.Format(_T("%d.%d.%d.%d"), 
		HIWORD(AppInfo.dwFileVersionMS), LOWORD(AppInfo.dwFileVersionMS),
		HIWORD(AppInfo.dwFileVersionLS), LOWORD(AppInfo.dwFileVersionLS));
#ifdef _WIN64
	sVersion += _T(" x64");
#else
	sVersion += _T(" x86");
#endif
#ifdef _DEBUG
	sVersion += _T(" Debug");
#else
	sVersion += _T(" Release");
#endif
	return sVersion;
}
