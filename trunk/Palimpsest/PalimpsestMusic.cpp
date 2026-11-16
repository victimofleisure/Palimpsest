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

*/

#include "stdafx.h"
#include "Palimpsest.h"
#include "PalimpsestMusic.h"
#include "Midi.h"
#include "Note.h"

#define LOG_CYCLE_TIMES		0		// non-zero to write cycle timing to log file

CPalimpsestMusic::CPalimpsestMusic()
{
	ZeroMemory(m_arrColorNote, sizeof(m_arrColorNote));
	ZeroMemory(m_arrChanNote, sizeof(m_arrChanNote));
	ZeroMemory(m_arrChanVolume, sizeof(m_arrChanVolume));
	m_nCurMidiChan = 0;
	m_nChanCycles = 0;
	m_nPrevNote = 0xff;
	m_bIsExport = false;
	m_fFrameRate = 0;
	m_nFrames = 0;
}

CPalimpsestMusic::~CPalimpsestMusic()
{
}

void CPalimpsestMusic::MakeScale(int nStartNote)
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

void CPalimpsestMusic::PrintScale()
{
	for (int iTone = 0; iTone < COLORS; iTone++) {
		CNote	note(m_arrColorNote[iTone]);
		_tprintf(_T("%s "), note.MidiName());
	}
}

bool CPalimpsestMusic::Open()
{
	if (MIDI_FAILED(m_midiOut.Open(theApp.m_args.m_nMidiOut, NULL, NULL, 0))) {
		AfxMessageBox(_T("Can't open MIDI output device"));
		return false;
	}
	Init();
	return true;
}

void CPalimpsestMusic::ExportInit()
{
	m_bIsExport = true;
	m_fFrameRate = theApp.m_args.m_fRecFrameRate;
	ASSERT(m_fFrameRate > 0);
	// following forces creation of clips in Reason
	for (int iChan = 0; iChan < COLORS; iChan++) {
		MidiOut(CONTROL, iChan, CC_VOLUME, 0);
		MidiOut(CONTROL, iChan, CC_PAN, 0);
		MidiOut(CONTROL, iChan, CC_AMP_ENV_ATTACK, 0);
		MidiOut(CONTROL, iChan, CC_AMP_ENV_RELEASE, 0);
		MidiOut(CONTROL, iChan, CC_AMP_VEL_AMOUNT, 0);
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
	}
}

void CPalimpsestMusic::Close()
{
	// release active note
	if (m_arrChanNote[m_nCurMidiChan]) {
		MidiOut(NOTE_ON, m_nCurMidiChan, m_arrChanNote[m_nCurMidiChan], 0);
		m_arrChanNote[m_nCurMidiChan] = 0;	// mark note released
	}
	m_midiOut.Close();
}

void CPalimpsestMusic::StartPrimitive(int iCurColor)
{
	ASSERT(iCurColor >= 0 && iCurColor < COLORS);
	if (!m_nCurMidiChan) {	// if at start of channel cycle
		// make a new scale
		int	nBottomNote = 36 + 4;
		int	nStartNote = nBottomNote + m_nChanCycles * 5 % 24;
		MakeScale(nStartNote);
		if (LOG_CYCLE_TIMES) {
			CString	sLine;
			sLine.Format(_T("%d: cycle %d\n"), m_nFrames, m_nChanCycles);
			theApp.WriteLogEntry(sLine);
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
	int	iPanPos = arrPanPos[iCurColor];
	int	nPan = min(iPanPos * 8, 127);	// convert logical position to pan value
	MidiOut(CONTROL, m_nCurMidiChan, CC_PAN, nPan);
	// set current channel's amplitude envelope release time
	int	nReleaseTime = iCurColor ? 90 : 110;	// give bass note a longer duration
	MidiOut(CONTROL, m_nCurMidiChan, CC_AMP_ENV_RELEASE, nReleaseTime);
	// start current channel's note
	int	nNote = m_arrColorNote[iCurColor];
	int	nVel = min(64 + (COLORS - 1 - iCurColor) * 4, 127);
	MidiOut(NOTE_ON, m_nCurMidiChan, nNote, nVel);
	m_arrChanNote[m_nCurMidiChan] = static_cast<BYTE>(nNote);	// update current channel's note state
	m_nPrevNote = static_cast<BYTE>(nNote);
}

void CPalimpsestMusic::ContinuePrimitive(double fPos)
{
	// update current channel's volume
	int	nVolume = Round(fPos * 127);
	if (nVolume != m_arrChanVolume[m_nCurMidiChan]) {	// if volume actually changed
		MidiOut(CONTROL, m_nCurMidiChan, CC_VOLUME, nVolume);
		m_arrChanVolume[m_nCurMidiChan] = static_cast<BYTE>(nVolume);	// update cached volume
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
		DWORD	dwTime = GetFrameTick(m_nFrames);
		CMidiFile::MIDI_EVENT	evt = {dwTime, dwMsg};
		m_arrMidiEvent[nChan].Add(evt);
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
