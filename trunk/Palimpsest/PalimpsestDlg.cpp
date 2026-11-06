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

// PalimpsestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Palimpsest.h"
#include "PalimpsestDlg.h"
#include "afxdialogex.h"
#include "mmsystem.h"
#include "Easing.h"
#include "AboutDlg.h"
#include "gdiplus.h"
#include "ProgressDlg.h"
#include "Midi.h"
#include "Note.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define RANDOM_SEED			123		// random number seed, or zero to use system time
#define MIDI_OUT_DEVICE		1		// output MIDI device index

#define EXPORT_IMAGE_SEQ	0		// non-zero to export an image sequence
#define EXPORT_FRAME_RATE	60		// export frame rate in frames per second
#define EXPORT_FRAME_COUNT	19691	// export duration in frames
#define EXPORT_FRAME_SIZE	CSize(1920, 1080)	// export frame size in pixels
#define EXPORT_SUPERSAMPLE	4		// 1 = original indexed PNG; 4 = render 4x, downsample
#define EXPORT_FOLDER_PATH	_T("E:\\temp\\video\\Palimpsest\\")	// export folder path
#define EXPORT_MIDI_ONLY	0	// non-zero to export MIDI only

#define PRINT_FRAME_RATE	0	// non-zero to print display frame rate in console
#define LOG_CYCLE_TIMES		0	// non-zero to write cycle timing to log file

// CPalimpsestDlg dialog

CPalimpsestDlg::CPalimpsestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPalimpsestDlg::IDD, pParent), m_stat(120)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pOldBitmap = NULL;
	m_szBackBuf = CSize(0, 0);
	m_gdiplusToken = 0;
	m_nRandSeed = RANDOM_SEED ? RANDOM_SEED : GetTickCount();
	srand(m_nRandSeed);
	m_nTimerPeriod = 16;
	m_nSpeed = 4;
	m_bContracting = false;
	m_bIsFullScreen = false;
	m_bIsPaused = false;
	m_bSingleStep = false;
	m_bFreshStart = false;
	m_nLaunchType = LT_FULLSCREEN;
	m_nFrameCount = 0;
	m_nPrevFrameCount = 0;
	InitGeom();

	// musical accompaniment
	ZeroMemory(m_arrColorNote, sizeof(m_arrColorNote));
	ZeroMemory(m_arrChanNote, sizeof(m_arrChanNote));
	ZeroMemory(m_arrChanVolume, sizeof(m_arrChanVolume));
	m_nCurMidiChan = 0;
	m_nChanCycles = 0;
	m_bIsMidiExport = false;
	m_nPrevNote = -1;
}

void CPalimpsestDlg::InitGeom()
{
	m_ptCenter = CPoint(0, 0);
	m_szEllipse = CSize(0, 0);
	m_iCurColor = 0;
	m_nChanMask = 0;
	m_nRadius = -1;  // -1 means initialize a new ellipse
	ZeroMemory(&m_clrStart, sizeof(m_clrStart));
	ZeroMemory(&m_clrTarget, sizeof(m_clrTarget));
	m_bFreshStart = true;
}

void CPalimpsestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

bool CPalimpsestDlg::CreateBackbuffer(CSize sz)
{
	if (sz.cx <= 0 || sz.cy <= 0)
		return false;

	// Release previous backbuffer, if any.
	if (m_dcBack.GetSafeHdc()) {
		m_dcBack.SelectObject(m_pOldBitmap);
		m_dcBack.DeleteDC();
		m_pOldBitmap = NULL;
	}
	m_bmpBack.DeleteObject();

	struct {
		BITMAPINFOHEADER hdr;
		RGBQUAD colors[COLORS];
	} bmi = {};

	bmi.hdr.biSize = sizeof(BITMAPINFOHEADER);
	bmi.hdr.biWidth = sz.cx;
	bmi.hdr.biHeight = -sz.cy;  // top-down
	bmi.hdr.biPlanes = 1;
	bmi.hdr.biBitCount = 4;
	bmi.hdr.biCompression = BI_RGB;
	bmi.hdr.biClrUsed = COLORS;
	bmi.hdr.biClrImportant = COLORS;

	// initial palette
	for (int iClr = 0; iClr < COLORS; iClr++) {
		bmi.colors[iClr].rgbRed = BYTE(rand());
		bmi.colors[iClr].rgbGreen = BYTE(rand());
		bmi.colors[iClr].rgbBlue = BYTE(rand());
	}

	if (!m_dcBack.CreateCompatibleDC(NULL))
		return false;

	void *pBits = NULL;
	HBITMAP hBitmap = CreateDIBSection(
		m_dcBack.GetSafeHdc(),
		reinterpret_cast<BITMAPINFO *>(&bmi),
		DIB_RGB_COLORS, &pBits, NULL, 0);

	if (!hBitmap) {
		m_dcBack.DeleteDC();
		return false;
	}

	m_szBackBuf = sz;
	m_bmpBack.Attach(hBitmap);
	m_pOldBitmap = m_dcBack.SelectObject(&m_bmpBack);

	// Initialize all pixel indices to zero.
	PatBlt(m_dcBack, 0, 0, sz.cx, sz.cy, BLACKNESS);

	InitGeom();

	return true;
}

void CPalimpsestDlg::FullScreen(bool bEnable)
{
	if (bEnable == m_bIsFullScreen)	// if already in requested state
		return;	// nothing to do

	if (bEnable) {	// if entering full screen mode
		m_dwWindowStyle = GetStyle();
		m_dwWindowExStyle = GetExStyle();
		GetWindowPlacement(&m_wpWindow);

		ModifyStyle(WS_CAPTION | WS_THICKFRAME | WS_BORDER |
			WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0);

		ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE |
			WS_EX_STATICEDGE, 0);

		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(
			MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

		SetWindowPos(&wndTopMost,
			mi.rcMonitor.left, mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	} else {	// entering windowed mode
		SetWindowLongPtr(m_hWnd, GWL_STYLE, m_dwWindowStyle);
		SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, m_dwWindowExStyle);

		SetWindowPos(&wndNoTopMost, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE |
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		SetWindowPlacement(&m_wpWindow);
	}

	m_bIsFullScreen = bEnable;
}

void CPalimpsestDlg::Pause(bool bEnable)
{
	if (bEnable == m_bIsPaused)	// if already in requested state
		return;	// nothing to do
	m_bIsPaused = bEnable;
	m_bSingleStep = false;
}

void CPalimpsestDlg::SingleStep()
{
	if (m_bIsPaused) {
		m_bSingleStep = true;
	}
}

bool CPalimpsestDlg::SaveFrame(LPCTSTR pszPath)
{
	// GDI must finish writing the DIB before GDI+ reads its bits.
	::GdiFlush();

	DIBSECTION ds = {};
	if (m_bmpBack.GetObject(sizeof(ds), &ds) != sizeof(ds))
		return false;

	RGBQUAD colors[COLORS];
	if (::GetDIBColorTable(m_dcBack.GetSafeHdc(), 0, COLORS, colors) != COLORS)
		return false;

	// Wrap the existing 4-bpp top-down pixels; don't copy or convert them.
	Gdiplus::Bitmap bmp(ds.dsBm.bmWidth, abs(ds.dsBm.bmHeight),
		ds.dsBm.bmWidthBytes, PixelFormat4bppIndexed,
		static_cast<BYTE *>(ds.dsBm.bmBits));
	if (bmp.GetLastStatus() != Gdiplus::Ok)
		return false;

	struct Palette16 {
		UINT Flags;
		UINT Count;
		Gdiplus::ARGB Entries[COLORS];
	} palette = {};
	palette.Count = COLORS;
	for (int iClr = 0; iClr < COLORS; iClr++) {
		palette.Entries[iClr] = Gdiplus::Color(255,
			colors[iClr].rgbRed, colors[iClr].rgbGreen, colors[iClr].rgbBlue).GetValue();
	}
	if (bmp.SetPalette(reinterpret_cast<Gdiplus::ColorPalette *>(&palette))
		!= Gdiplus::Ok)
		return false;

	// Built-in GDI+ PNG encoder.
	static const CLSID pngEncoder = {
		0x557cf406, 0x1a04, 0x11d3,
		{0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
	};
	if (EXPORT_IMAGE_SEQ && EXPORT_SUPERSAMPLE > 1) {
		// Resolve the palette before filtering: antialiasing must mix RGB,
		// not the numerical palette indices.
		const int nWidth = ds.dsBm.bmWidth / EXPORT_SUPERSAMPLE;
		const int nHeight = abs(ds.dsBm.bmHeight) / EXPORT_SUPERSAMPLE;
		Gdiplus::Bitmap output(nWidth, nHeight, PixelFormat32bppRGB);
		if (output.GetLastStatus() != Gdiplus::Ok)
			return false;

		Gdiplus::Graphics graphics(&output);
		if (graphics.GetLastStatus() != Gdiplus::Ok)
			return false;
		graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
		if (graphics.DrawImage(&bmp, Gdiplus::Rect(0, 0, nWidth, nHeight),
			0, 0, ds.dsBm.bmWidth, abs(ds.dsBm.bmHeight),
			Gdiplus::UnitPixel) != Gdiplus::Ok)
			return false;
		return output.Save(pszPath, &pngEncoder, NULL) == Gdiplus::Ok;
	} else {
		return bmp.Save(pszPath, &pngEncoder, NULL) == Gdiplus::Ok;
	}
}

// Scale geometry only; the simulation continues in output-frame coordinates.
static CRect ScaleRect(const CRect& rc, int nScale)
{
	return CRect(rc.left * nScale, rc.top * nScale,
		rc.right * nScale, rc.bottom * nScale);
}

void CPalimpsestDlg::RenderFrame()
{
	const int nScale = EXPORT_IMAGE_SEQ ? EXPORT_SUPERSAMPLE : 1;
	CSize szWnd(m_szBackBuf.cx / nScale, m_szBackBuf.cy / nScale);
	int nSize = min(szWnd.cx, szWnd.cy);
	if (nSize <= 0)
		return;

	if (m_nRadius < 0) {
		// Initialize a new ellipse
		m_ptCenter = CPoint(CRandList::Rand(szWnd.cx), CRandList::Rand(szWnd.cy));

		const double fMinFrac = 0.125;
		const double fMaxFrac = 0.5;

		m_szEllipse = CSize(
			Round(nSize * fMinFrac) +
				CRandList::Rand(Round(nSize * (fMaxFrac - fMinFrac))),
			Round(nSize * fMinFrac) +
				CRandList::Rand(Round(nSize * (fMaxFrac - fMinFrac))));

		m_iCurColor = m_rlColor.GetNext(m_iCurColor);
		if (m_bFreshStart) {	// if first ellipse after a recreate
			m_nChanMask = COLORS - 1;	// ensure something gets drawn
			m_bFreshStart = false;	// reset flag
		} else {	// usual case
			m_nChanMask = m_rlChanMask.GetNext(m_nChanMask - 1) + 1;
		}
		m_nRadius = 0;

		// Start from the current palette color.
		VERIFY(GetDIBColorTable(m_dcBack.GetSafeHdc(),
			m_iCurColor, 1, &m_clrStart) == 1);
		// Choose a random destination color.
		m_clrTarget.rgbRed = BYTE(rand());
		m_clrTarget.rgbGreen = BYTE(rand());
		m_clrTarget.rgbBlue = BYTE(rand());
		m_clrTarget.rgbReserved = 0;

		m_bContracting = (rand() & 1) != 0;	// select direction

		MidiStartDraw();
	}

	// Expand at constant radial speed
	int nMaxRadius = max(m_szEllipse.cx, m_szEllipse.cy);
	m_nRadius = min(m_nRadius + m_nSpeed, nMaxRadius);

	double fPos = double(m_nRadius) / nMaxRadius;
	double fEasedPos = EaseOut(fPos, 0.3);
	double fBidirPos = m_bContracting ? 1.0 - fEasedPos : fEasedPos;

	CSize sz(
		Round(m_szEllipse.cx * fBidirPos),
		Round(m_szEllipse.cy * fBidirPos));

	CRect rEllipse(
		m_ptCenter.x - sz.cx,
		m_ptCenter.y - sz.cy,
		m_ptCenter.x + sz.cx,
		m_ptCenter.y + sz.cy);

	if (EXPORT_SUPERSAMPLE > 1) {
		// Convert only the final drawing geometry to physical pixels.
		rEllipse = ScaleRect(rEllipse, nScale);
	}

	m_dcBack.SelectObject(GetStockObject(NULL_PEN));
	m_dcBack.SelectObject(GetStockObject(DC_BRUSH));

	if (m_bContracting) {
		CRect rOuter(
			m_ptCenter.x - m_szEllipse.cx,
			m_ptCenter.y - m_szEllipse.cy,
			m_ptCenter.x + m_szEllipse.cx,
			m_ptCenter.y + m_szEllipse.cy);

		if (EXPORT_SUPERSAMPLE > 1) {
			rOuter = ScaleRect(rOuter, nScale);
		}

		CRgn rgnOuter, rgnInner, rgnClip;
		rgnOuter.CreateEllipticRgnIndirect(rOuter);
		rgnClip.CreateRectRgn(0, 0, 0, 0);

		if (sz.cx > 0 && sz.cy > 0) {
			rgnInner.CreateEllipticRgnIndirect(rEllipse);
			rgnClip.CombineRgn(&rgnOuter, &rgnInner, RGN_DIFF);
		} else {
			rgnClip.CopyRgn(&rgnOuter);
		}

		m_dcBack.SelectClipRgn(&rgnClip);
		rEllipse = rOuter;
	}

	m_dcBack.SetROP2(R2_MASKPEN);
	m_dcBack.SetDCBrushColor(DIBINDEX(m_nChanMask ^ 15));
	m_dcBack.Ellipse(rEllipse);

	m_dcBack.SetROP2(R2_MERGEPEN);
	m_dcBack.SetDCBrushColor(DIBINDEX(m_iCurColor & m_nChanMask));
	m_dcBack.Ellipse(rEllipse);

	if (m_bContracting)
		m_dcBack.SelectClipRgn(NULL);

	// Interpolate the active palette entry.
	RGBQUAD clr;
	clr.rgbRed = BYTE(Round(
		m_clrStart.rgbRed +
		(m_clrTarget.rgbRed - m_clrStart.rgbRed) * fEasedPos));

	clr.rgbGreen = BYTE(Round(
		m_clrStart.rgbGreen +
		(m_clrTarget.rgbGreen - m_clrStart.rgbGreen) * fEasedPos));

	clr.rgbBlue = BYTE(Round(
		m_clrStart.rgbBlue +
		(m_clrTarget.rgbBlue - m_clrStart.rgbBlue) * fEasedPos));

	clr.rgbReserved = 0;

	VERIFY(SetDIBColorTable(m_dcBack.GetSafeHdc(),
		m_iCurColor, 1, &clr) == 1);

	MidiContinueDraw(fEasedPos);

	if (m_nRadius >= nMaxRadius)
		m_nRadius = -1;

	m_nFrameCount++;
}

bool CPalimpsestDlg::ExportVideo(CSize szFrame, int nFrames, LPCTSTR pszFolder)
{
	CSize szRender(szFrame.cx * EXPORT_SUPERSAMPLE,
		szFrame.cy * EXPORT_SUPERSAMPLE);
	if (!CreateBackbuffer(szRender))
		return false;
	m_bIsMidiExport = true;
	MidiFileInit();
	CProgressDlg	dlgProg;
	if (!dlgProg.Create())
		return false;
	dlgProg.SetRange(0, nFrames);
	int	nTrailer = 300;	// pad with extra frames at end
	for (int iFrame = 0; iFrame < nFrames + nTrailer; iFrame++) {
		if (iFrame < nFrames)
			RenderFrame();
		else {
			if (iFrame == nFrames) {
				MidiClose();
			}
			m_nFrameCount++;
		}
		if (!EXPORT_MIDI_ONLY) {
			CString sFrameIdx;
			sFrameIdx.Format(_T("%06d"), iFrame);
			CString	sImgFilePath(CString(pszFolder) 
				+ _T("img") + sFrameIdx + _T(".png"));
			if (!SaveFrame(sImgFilePath))
				return false;
		}
		dlgProg.SetPos(iFrame);
		if (dlgProg.Canceled())
			return false;
	}
	MidiClose();	// release active note
	MidiFileInit();	// helps avoid ragged clips in Reason
	CString	sMidiFilePath(CString(pszFolder) + _T("Palimpsest.mid"));
	WriteMidiFile(sMidiFilePath);
	return true;
}

// musical accompaniment

void CPalimpsestDlg::MakeScale(int nStartNote)
{
	// assign notes from Lydian mode of major scale
	static const int arrScale[] = {0, 2, 4, 6, 7, 9, 11};
	const int nScaleLen = _countof(arrScale);
	m_arrColorNote[0] = static_cast<BYTE>(nStartNote - 12);	// drop lowest note an octave
	for (int iTone = 1; iTone < COLORS; iTone++) {
		int	nNote = nStartNote + iTone / nScaleLen * 12 + arrScale[iTone % nScaleLen];
		ASSERT(nNote >= 0 && nNote < MIDI_NOTES);
		m_arrColorNote[iTone] = static_cast<BYTE>(nNote);
	}
}

void CPalimpsestDlg::PrintScale()
{
	for (int iTone = 0; iTone < COLORS; iTone++) {
		CNote	note(m_arrColorNote[iTone]);
		_tprintf(_T("%s "), note.MidiName());
	}
}

bool CPalimpsestDlg::MidiOpen()
{
	if (FAILED(m_midiOut.Open(MIDI_OUT_DEVICE, NULL, NULL, 0))) {
		AfxMessageBox(_T("Can't open MIDI output device"));
		return false;
	}
	MidiInit();
	return true;
}

void CPalimpsestDlg::MidiFileInit()
{
	// forces creation of event blocks in Reason
	ASSERT(m_bIsMidiExport);
	for (int iChan = 0; iChan < COLORS; iChan++) {
		MidiOut(CONTROL, iChan, CC_VOLUME, 0);
		MidiOut(CONTROL, iChan, CC_PAN, 0);
		MidiOut(CONTROL, iChan, CC_AMP_ENV_ATTACK, 0);
		MidiOut(CONTROL, iChan, CC_AMP_ENV_RELEASE, 0);
		MidiOut(CONTROL, iChan, CC_AMP_VEL_AMOUNT, 0);
	}
	MidiInit();	// then do regular init
}

void CPalimpsestDlg::MidiInit()
{
	for (int iChan = 0; iChan < COLORS; iChan++) {
		// set amplitude envelope attack long enough to comfortably avoid a click
		// at start of note, due to the sampler's having a non-zero sample start
		MidiOut(CONTROL, iChan, CC_AMP_ENV_ATTACK, 25);
		// set amplitude velocity amount big enough to allow reasonable dynamics
		MidiOut(CONTROL, iChan, CC_AMP_VEL_AMOUNT, 94);
	}
}

void CPalimpsestDlg::MidiClose()
{
	// release active note
	if (m_arrChanNote[m_nCurMidiChan]) {
		MidiOut(NOTE_ON, m_nCurMidiChan, m_arrChanNote[m_nCurMidiChan], 0);
		m_arrChanNote[m_nCurMidiChan] = 0;	// mark note released
	}
	m_midiOut.Close();
}

void CPalimpsestDlg::MidiStartDraw()
{
	// following assumes COLORS == MIDI_CHANNELS
	if (!m_nCurMidiChan) {	// if at start of channel cycle
		// make a new scale
		int	nBottomNote = 36 + 4;
		int	nStartNote = nBottomNote + m_nChanCycles * 5 % 24;
		MakeScale(nStartNote);
		if (LOG_CYCLE_TIMES) {
			CString	sLine;
			sLine.Format(_T("%d: cycle %d\n"), m_nFrameCount, m_nChanCycles);
			m_fLog.WriteString(sLine);
		}
		m_nChanCycles++;	// bump channel cycle count
	}
	// release current channel's note
	int	nActiveNote = m_arrChanNote[m_nCurMidiChan];
	if (nActiveNote) {
		MidiOut(NOTE_ON, m_nCurMidiChan, nActiveNote, 0);
	}
	// advance current channel
	m_nCurMidiChan++;
	if (m_nCurMidiChan >= COLORS)	// if reached end of channels
		m_nCurMidiChan = 0;	// wrap around to first channel
	// set current channel's volume to zero
	MidiOut(CONTROL, m_nCurMidiChan, CC_VOLUME, 0);
	// set current channel's panning
	// define logical position of each channel, with lowest pitches centered
	static const BYTE arrPanPos[COLORS] = {8,7,9,6,10,5,11,4,12,3,13,2,14,1,15,0};
	int	iPanPos = arrPanPos[m_iCurColor];
	int	nPan = min(iPanPos * 8, 127);	// convert logical position to pan value
	MidiOut(CONTROL, m_nCurMidiChan, CC_PAN, nPan);
	// set current channel's amplitude envelope release time
	int	nReleaseTime = m_iCurColor ? 90 : 110;	// give bass note a longer duration
	MidiOut(CONTROL, m_nCurMidiChan, CC_AMP_ENV_RELEASE, nReleaseTime);
	// start current channel's note
	int	nNote = m_arrColorNote[m_iCurColor];
	int	nVel = min(64 + (COLORS - 1 - m_iCurColor) * 4, 127);
	MidiOut(NOTE_ON, m_nCurMidiChan, nNote, nVel);
	m_arrChanNote[m_nCurMidiChan] = nNote;	// update current channel's note state
	m_nPrevNote = nNote;
}

void CPalimpsestDlg::MidiContinueDraw(double fPos)
{
	// update current channel's volume
	int	nVolume = Round(fPos * 127);
	if (nVolume != m_arrChanVolume[m_nCurMidiChan]) {	// if volume actually changed
		MidiOut(CONTROL, m_nCurMidiChan, CC_VOLUME, nVolume);
		m_arrChanVolume[m_nCurMidiChan] = nVolume;	// update cached volume
	}
}

DWORD CPalimpsestDlg::GetFrameTick(int iFrame)
{
	const double	fTicksPerSec = (double(MF_TEMPO) / 60) * MF_TIMEBASE;
	double fFrameTimeSecs = (1.0 / EXPORT_FRAME_RATE) * iFrame;
	return Round(fTicksPerSec * fFrameTimeSecs);
}

void CPalimpsestDlg::MidiOut(int nCmd, int nChan, int nP1, int nP2)
{
	DWORD	dwMsg = MakeMidiMsg(nCmd, nChan, nP1, nP2);
	if (m_bIsMidiExport) {
		DWORD	dwTime = GetFrameTick(m_nFrameCount);
		CMidiFile::MIDI_EVENT	evt = {dwTime, dwMsg};
		m_arrMidiEvent[nChan].Add(evt);
	} else {
		m_midiOut.OutShortMsg(dwMsg);
	}
}

void CPalimpsestDlg::WriteMidiFile(LPCTSTR pszPath)
{
	ASSERT(m_bIsMidiExport);
	CMidiFile	fMidi(pszPath, CFile::modeCreate | CFile::modeWrite);
	DWORD	dwDuration = GetFrameTick(m_nFrameCount);
	fMidi.WriteHeader(COLORS, MF_TIMEBASE, MF_TEMPO, dwDuration, NULL, NULL, NULL);
	CStdioFile	fText(_T("midi.txt"), CFile::modeCreate | CFile::modeWrite);
	for (int iChan = 0; iChan < COLORS; iChan++) {
		CMidiFile::CMidiEventArray& arrEvt = m_arrMidiEvent[iChan];
		int	nEvts = arrEvt.GetSize();
		DWORD	nPrevTime = 0;
		for (int iEvt = 0; iEvt < nEvts; iEvt++) {
			CMidiFile::MIDI_EVENT& evt = arrEvt[iEvt];
			CString	s;
			s.Format(_T("%d\t%x\n"), evt.DeltaT, evt.Msg);
			fText.WriteString(s);
			DWORD	nCurTime = evt.DeltaT;	// absolute time 
			evt.DeltaT = nCurTime - nPrevTime;	// convert to delta time
			nPrevTime = nCurTime;	// update previous time
		}
		fMidi.WriteTrack(arrEvt);
		fText.WriteString(_T("\n"));
	}
}

BEGIN_MESSAGE_MAP(CPalimpsestDlg, CDialogEx)
	ON_MESSAGE(UWM_MM_TIMER, OnMMTimer)
	ON_WM_DESTROY()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
	ON_MESSAGE(UWM_DELAYED_CREATE, OnDelayedCreate)
END_MESSAGE_MAP()

// CPalimpsestDlg message handlers

BOOL CPalimpsestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add items to system menu.
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_FULL_SCREEN, _T("Full Screen\tF11"));
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_rlColor.Init(COLORS);
	m_rlChanMask.Init(COLORS - 1);

	m_fLog.Open(_T("log.txt"), CFile::modeCreate | CFile::modeWrite);
	CString	sRandSeed;
	sRandSeed.Format(_T("%d"), m_nRandSeed);
	m_fLog.WriteString(_T("Seed: ") + sRandSeed + '\n');

	if (EXPORT_IMAGE_SEQ) {
		PostMessage(UWM_DELAYED_CREATE);
	} else {
		switch (m_nLaunchType) {
		case LT_MAXIMIZE:
			ShowWindow(SW_MAXIMIZE);
			break;
		case LT_FULLSCREEN:
			FullScreen(true);
			break;
		}
		CRect rc;
		GetClientRect(rc);
		CreateBackbuffer(rc.Size());	// create initial backbuffer
		m_timerResolution.Create(1);
		m_timer.Create(m_nTimerPeriod, 1, TimerProc,
			reinterpret_cast<DWORD_PTR>(GetSafeHwnd()),
			TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
		MidiOpen();
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
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
	case IDM_FULL_SCREEN:
		FullScreen(!m_bIsFullScreen);
		break;
	default:
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

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
		CPaintDC dc(this);
		if (!EXPORT_IMAGE_SEQ) {
			if (m_dcBack.GetSafeHdc()) {
				CRect rc;
				GetClientRect(rc);
				dc.BitBlt(0, 0, rc.Width(), rc.Height(),
					&m_dcBack, 0, 0, SRCCOPY);
			}
		}
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPalimpsestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPalimpsestDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	Invalidate();
	if (!EXPORT_IMAGE_SEQ) {
		CreateBackbuffer(CSize(cx, cy));
	}
}

void CALLBACK CPalimpsestDlg::TimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	HWND hWnd = reinterpret_cast<HWND>(dwUser);
	::PostMessage(hWnd, UWM_MM_TIMER, 0, 0);
}

LRESULT CPalimpsestDlg::OnMMTimer(WPARAM wParam, LPARAM lParam)
{
	if (!m_bIsPaused || m_bSingleStep) {
		m_bSingleStep = false;
//		CBenchmark	b;
		RenderFrame();
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
//		m_stat.Print(b.Elapsed());
	}
	if (PRINT_FRAME_RATE) {
		double	fElapsed = m_bmFrameTimer.Elapsed();
		if (fElapsed > 1) {
			m_bmFrameTimer.Reset();
			LONGLONG	nFrames = m_nFrameCount - m_nPrevFrameCount;
			printf("%lld\n", nFrames);
			m_nPrevFrameCount = m_nFrameCount;
		}
	}
	return 0;
}

BOOL CPalimpsestDlg::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

BOOL CPalimpsestDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		switch (pMsg->wParam) {
		case VK_F11:
			FullScreen(!m_bIsFullScreen);
			return TRUE;
		case ' ':
			if (GetKeyState(VK_SHIFT) & 0x8000) {
				SingleStep();
			} else {
				Pause(!m_bIsPaused);
			}
			return TRUE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

BOOL CPalimpsestDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bIsFullScreen) {
		::SetCursor(NULL);
		return TRUE;
	}
	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}

void CPalimpsestDlg::OnDestroy()
{
	// Export never starts the multimedia timer.
	// The timer wrappers retain their existing RAII cleanup behavior.
	if (m_gdiplusToken) {
		Gdiplus::GdiplusShutdown(m_gdiplusToken);
		m_gdiplusToken = 0;
	}
	MidiClose();
	CDialogEx::OnDestroy();
}

LRESULT CPalimpsestDlg::OnDelayedCreate(WPARAM wParam, LPARAM lParam)
{
	// to match the reference output, we must account for two extra CreateBackbuffer
	// calls during window sizing, each of which calls rand three times for each color
	for (int i = 0; i < COLORS * 6; i++) {
		rand();
	}
	Gdiplus::GdiplusStartupInput input;
	if (Gdiplus::GdiplusStartup(&m_gdiplusToken, &input, NULL) != Gdiplus::Ok)
		return 0;
	bool bSuccess = ExportVideo(EXPORT_FRAME_SIZE, EXPORT_FRAME_COUNT, EXPORT_FOLDER_PATH);
	if (!bSuccess)
		AfxMessageBox(_T("Video export failed."));
	EndDialog(bSuccess ? IDOK : IDCANCEL);
	return 0;
}
