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

#pragma once

#include "Palimpsest.h"
#include "MidiWrap.h"
#include "MidiFile.h"
#include "Midi.h"

class CPalimpsestMusic {
public:
// Construction
	CPalimpsestMusic();
	~CPalimpsestMusic();

// Constants
	enum {
		COLORS = MIDI_CHANNELS	// required as drawing uses 16-color scheme
	};

// Attributes
	bool	IsOpen() const;

// Operations
	bool	Open();
	void	Init();
	void	Close();
	void	StartPrimitive(int iCurColor);
	void	ContinuePrimitive(double fPos);
	void	MidiOut(int nCmd, int nChan, int nP1, int nP2);
	void	ExportInit();
	void	WriteFile(LPCTSTR pszPath);

protected:
// Constants
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

// Member data
	CMidiOut	m_midiOut;		// MIDI output device
	CMidiFile::CMidiEventArray	m_arrMidiEvent[MIDI_CHANNELS];	// array of MIDI events for each channel
	BYTE	m_arrColorNote[COLORS];	// each color's note
	BYTE	m_arrChanNote[COLORS];	// each MIDI channel's note
	BYTE	m_arrChanVolume[COLORS];	// each MIDI channel's volume
	int		m_nCurMidiChan;		// current MIDI channel
	int		m_nChanCycles;		// number of cycles through all channels
	BYTE	m_nPrevNote;		// previous note
	bool	m_bIsExport;		// true if exporting MIDI file
	double	m_fFrameRate;		// frame rate when exporting
	ULONGLONG	m_nFrames;		// number of frames rendered

// Helpers
	void	MakeScale(int nStartNote);
	void	PrintScale();
	DWORD	GetFrameTick(ULONGLONG iFrame);
};

inline bool CPalimpsestMusic::IsOpen() const
{
	return m_midiOut.IsOpen();
}

