// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      13nov26	initial version
        01      16nov26	move to helper class
		02		17nov26	map volume fading to pitch bend for better resolution
		03		17nov26	map expansion/contraction to change of instrument
		04		17nov26	map gradient core fraction to note velocity

*/

#include "stdafx.h"
#include "Resource.h"
#include "PalimpsestMusic.h"
#include "Midi.h"
#include "Note.h"

#define MUSIC_LOGGING	0	// 0 = none, 1 = log to console, 2 = log to file
#define LOG_NOTES		0	// non-zero to log notes, else log key changes only

CPalimpsestMusic::CPalimpsestMusic()
{
	ZeroMemory(m_aColorNote, sizeof(m_aColorNote));
	ZeroMemory(m_aColorTone, sizeof(m_aColorTone));
	ZeroMemory(m_aChanNote, sizeof(m_aChanNote));
	ZeroMemory(m_aChanVolume, sizeof(m_aChanVolume));
	m_nCurMidiChan = 0;
	m_nChanCycles = 0;
	m_nStartNote = 0;
	m_nPrevNote = 0xff;
	m_bIsExport = false;
	m_fFrameRate = 0;
	m_nFrames = 0;
	m_fLog = NULL;
}

CPalimpsestMusic::~CPalimpsestMusic()
{
	if (m_fLog)
		fclose(m_fLog);
}

int CPalimpsestMusic::MakeScale(int iCycle)
{
	// assign notes from Lydian mode of major scale
	static const int aScale[] = {0, 2, 4, 6, 7, 9, 11};
	const int nScaleLen = _countof(aScale);
//	const int	nBottomNote = OCTAVE * 3 + C;	// starting with C is easier for testing
	const int	nBottomNote = OCTAVE * 3 + E;	// the actual cycle starts with E
	int	nStartNote = nBottomNote + iCycle * 5 % OCTAVE;
	static const int aKeyOffset[12] = {
		0, 3, 6, 2, 5, 1, 4, 6, 2, 5, 1, 4
	};
	// rotate color assignments across keys to retain pitch continuity where possible
	int	nOffset = aKeyOffset[iCycle % OCTAVE];
	for (int iTone = 0; iTone < COLORS; iTone++) {
		int	nNote = nStartNote + iTone / nScaleLen * OCTAVE + aScale[iTone % nScaleLen];
		if (!iTone)
			nNote -= OCTAVE;
		ASSERT(nNote >= 0 && nNote < MIDI_NOTES);
		int	iOfsTone = (iTone + nOffset) % COLORS;
		m_aColorNote[iOfsTone] = static_cast<BYTE>(nNote);
		m_aColorTone[iOfsTone] = static_cast<BYTE>(iTone);
	}
	return nStartNote;	// return start note
}

void CPalimpsestMusic::PrintScales()
{
	CString	sNum;
	CString	sColHdr("   key ");
	for (int iClr = 0; iClr < COLORS; iClr++) {
		sNum.Format(_T("%3d"), iClr);
		sColHdr += sNum + ' ';
	}
	_putts(sColHdr);
	for (int iCycle = 0; iCycle < OCTAVE; iCycle++) {
		int	iKey = MakeScale(iCycle);
		CString	sKey(CNote(iKey).Name(F));
		_tprintf(_T("%2d %2s: %s\n"), iCycle, sKey, FormatScale(iKey));
	}
	printf("\n");
}

CString CPalimpsestMusic::FormatScale(int nKey)
{
	CString sScale;
	const int nFieldWidth = 3;
	for (int iTone = 0; iTone < COLORS; iTone++) {
		if (iTone)
			sScale += ' ';
		CNote	note(m_aColorNote[iTone]);
		CString	s(note.MidiName(nKey));
		int	nPad = nFieldWidth - s.GetLength();
		for (int iPad = 0; iPad < nPad; iPad++)
			sScale += ' ';
		sScale += s;
	}
	return sScale;
}

bool CPalimpsestMusic::Open(UINT nMidiOutDev)
{
	if (MIDI_FAILED(m_midiOut.Open(nMidiOutDev, NULL, NULL, 0))) {
		AfxMessageBox(_T("Can't open MIDI output device"));
		return false;
	}
	Init();
	return true;
}

void CPalimpsestMusic::ExportInit(double fFrameRate)
{
	// when importing it's helpful if all lanes start at the same time
	ASSERT(fFrameRate > 0);
	m_bIsExport = true;
	m_fFrameRate = fFrameRate;
	// following forces creation of clips in Reason
	for (int iChan = 0; iChan < COLORS; iChan++) {
		MidiOut(WHEEL, iChan, 0, 0);
		MidiOut(CONTROL, iChan, CC_PAN, 0);
		MidiOut(CONTROL, iChan, CC_AMP_ENV_ATTACK, 0);
		MidiOut(CONTROL, iChan, CC_AMP_ENV_RELEASE, 0);
		MidiOut(CONTROL, iChan, CC_AMP_VEL_AMOUNT, 0);
		MidiOut(CONTROL, iChan, CC_BRASS_AMP_LEVEL, 0);
		MidiOut(CONTROL, iChan, CC_RECEIVE_NOTES, 0);
	}
	Init();	// then do regular init
}

void CPalimpsestMusic::Init()
{
	for (int iChan = 0; iChan < COLORS; iChan++) {
		// set amplitude envelope attack long enough to comfortably avoid a click
		// at start of note, due to the sampler's having a non-zero sample start
		MidiOut(CONTROL, iChan, CC_AMP_ENV_ATTACK, 50);
		// set amplitude velocity amount big enough to allow reasonable dynamics
		MidiOut(CONTROL, iChan, CC_AMP_VEL_AMOUNT, 94);
		// set brass master level for suitable balance with strings
		MidiOut(CONTROL, iChan, CC_BRASS_AMP_LEVEL, 80);
	}
#if MUSIC_LOGGING == 1	// if logging to console
	m_fLog = stdout;
#elif MUSIC_LOGGING == 2	// else if logging to file
	_tfopen_s(&m_fLog, _T("palimpsest_music.log"), _T("wt"));
#endif
//	PrintScales();
}

void CPalimpsestMusic::Close()
{
	// release active note
	if (m_aChanNote[m_nCurMidiChan]) {
		MidiOut(NOTE_ON, m_nCurMidiChan, m_aChanNote[m_nCurMidiChan], 0);
		m_aChanNote[m_nCurMidiChan] = 0;	// mark note released
	}
	m_midiOut.Close();
}

void CPalimpsestMusic::StartPrimitive(int iCurColor, bool bIsContracting, double fGradientCoreFrac)
{
	ASSERT(iCurColor >= 0 && iCurColor < COLORS);
	if (!m_nCurMidiChan) {	// if at start of channel cycle
		// make a new scale
		m_nStartNote = MakeScale(m_nChanCycles);
#if MUSIC_LOGGING
		_ftprintf(m_fLog, _T("%7d %-2s\n"), m_nFrames, CNote(m_nStartNote).Name(F));
#endif
		m_nChanCycles++;	// bump channel cycle count
	}
	// release current channel's note
	int	nActiveNote = m_aChanNote[m_nCurMidiChan];
	if (nActiveNote) {
		MidiOut(NOTE_ON, m_nCurMidiChan, nActiveNote, 0);
	}
	// advance current channel
	m_nCurMidiChan++;
	if (m_nCurMidiChan >= COLORS)	// if reached end of channels
		m_nCurMidiChan = 0;	// wrap around to first channel
	// set current channel's volume to zero
	// reset the next voice before its new note; previous voices continue releasing
	MidiOut(WHEEL, m_nCurMidiChan, 0, 0);
	// select instrument based on expansion versus contraction
	int	nInstSel = bIsContracting ? MIDI_NOTE_MAX : 0;
	MidiOut(CONTROL, m_nCurMidiChan, CC_RECEIVE_NOTES, nInstSel);
	// set current channel's panning
	// table contains each channel's position, evenly spaced
	// with lowest pitches centered and full range utilized
	static const BYTE aPanPos[COLORS] = {
		60, 68, 52, 76, 43, 85, 35, 93,
		26, 102, 18, 110, 9, 119, 0, 127
	};
	int	iTone = m_aColorTone[iCurColor];
	int	nPan = aPanPos[iTone];
	MidiOut(CONTROL, m_nCurMidiChan, CC_PAN, nPan);
	// set current channel's amplitude envelope release time
	const int nNormalRelease = 90;
	const int nBassRelease = 110;
	int	nRelease = iTone == 0 ? nBassRelease : nNormalRelease;
	MidiOut(CONTROL, m_nCurMidiChan, CC_AMP_ENV_RELEASE, nRelease);
	// start current channel's note
	int	nNote = m_aColorNote[iCurColor];
	// velocity tracks visual core opacity; exclude zero because it means note off
	int nVelo = Round(fGradientCoreFrac * (MIDI_NOTE_MAX - 1)) + 1;
	MidiOut(NOTE_ON, m_nCurMidiChan, nNote, nVelo);
#if MUSIC_LOGGING && LOG_NOTES
	_ftprintf(m_fLog, _T("%7d %2d %2d %-3s %3d %3d %3d\n"), m_nFrames, m_nCurMidiChan, 
		iCurColor, CNote(nNote).MidiName(m_nStartNote), nVelo, nPan - 64, nRelease);
#endif
	m_aChanNote[m_nCurMidiChan] = static_cast<BYTE>(nNote);	// update current channel's note state
	m_nPrevNote = static_cast<BYTE>(nNote);
}

void CPalimpsestMusic::ContinuePrimitive(double fPos)
{
	static const int nNominalVolume = 0x2EF6;	// 0.00 dB in Reason
	int	nVolume = Round(fPos * nNominalVolume);
	// update current channel's volume
	if (nVolume != m_aChanVolume[m_nCurMidiChan]) {	// if volume actually changed
		MidiOut(WHEEL, m_nCurMidiChan, nVolume & 0x7f, nVolume >> 7);	// LSB, MSB
		m_aChanVolume[m_nCurMidiChan] = static_cast<WORD>(nVolume);	// update cached volume
	}
	m_nFrames++;
}

DWORD CPalimpsestMusic::GetFrameTick(ULONGLONG iFrame)
{
	const double	fTicksPerSec = (double(MF_TEMPO) / 60) * MF_TIMEBASE;
	double fFrameTimeSecs = (1.0 / m_fFrameRate) * iFrame;
	return Round(fTicksPerSec * fFrameTimeSecs);
}

void CPalimpsestMusic::MidiOut(int nCmd, int nChan, int nP1, int nP2)
{
	DWORD	dwMsg = MakeMidiMsg(nCmd, nChan, nP1, nP2);
	if (m_bIsExport) {	// if exporting, store event
		// derive event time from frame count so exported MIDI matches the image sequence
		DWORD	dwTime = GetFrameTick(m_nFrames);
		CMidiFile::MIDI_EVENT	evt = {dwTime, dwMsg};
		m_aMidiEvent[nChan].Add(evt);
	} else {	// output to MIDI device
		m_midiOut.OutShortMsg(dwMsg);
	}
}

void CPalimpsestMusic::WriteFile(LPCTSTR pszPath)
{
	ASSERT(m_bIsExport);	// only valid in export mode
	CMidiFile	fMidi(pszPath, CFile::modeCreate | CFile::modeWrite);
	DWORD	dwDuration = GetFrameTick(m_nFrames);
	fMidi.WriteHeader(COLORS, MF_TIMEBASE, MF_TEMPO, dwDuration, NULL, NULL, NULL);
	CStdioFile	fText(_T("midi.txt"), CFile::modeCreate | CFile::modeWrite);
	for (int iChan = 0; iChan < COLORS; iChan++) {
		CMidiFile::CMidiEventArray& aEvt = m_aMidiEvent[iChan];
		int	nEvts = aEvt.GetSize();
		DWORD	nPrevTime = 0;
		for (int iEvt = 0; iEvt < nEvts; iEvt++) {
			CMidiFile::MIDI_EVENT& evt = aEvt[iEvt];
			CString	s;
			s.Format(_T("%d\t%x\n"), evt.DeltaT, evt.Msg);
			fText.WriteString(s);
			DWORD	nCurTime = evt.DeltaT;	// absolute time 
			evt.DeltaT = nCurTime - nPrevTime;	// convert to delta time
			nPrevTime = nCurTime;	// update previous time
		}
		fMidi.WriteTrack(aEvt);
		fText.WriteString(_T("\n"));
	}
}
