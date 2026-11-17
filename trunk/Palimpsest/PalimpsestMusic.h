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
		02		17nov26	enhance mapping

*/

#pragma once

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
	bool	Open(UINT nMidiOutDev);
	void	Init();
	void	Close();
	void	StartPrimitive(int iCurColor, bool bIsContracting, double fGradientCoreFrac);
	void	ContinuePrimitive(double fPos);
	void	MidiOut(int nCmd, int nChan, int nP1, int nP2);
	void	ExportInit(double fFrameRate);
	void	WriteFile(LPCTSTR pszPath);

protected:
// Constants
	enum {	// continuous controller numbers
		// Pitch Bend is mapped to Mixer Channel Volume via Remote Override Mapping.
		// Pitch Bend gives 14-bit precision, verified by analysis of recorded audio.
		// Note that 0.00 dB is approximately 3830 signed (12022 unsigned or 0x2EF6).
		// Receive Notes is cross-mapped to choose between a pair of NN-19 samplers.
		CC_PAN				= 20,	// Mixer Channel Strip Pan via Remote Override Mapping
		CC_AMP_ENV_ATTACK	= 71,	// Combinator Control 1 -> NN-19 Amplitude Envelope Attack
		CC_AMP_ENV_RELEASE	= 72,	// Combinator Control 2 -> NN-19 Amplitude Envelope Release
		CC_AMP_VEL_AMOUNT	= 73,	// Combinator Control 3 -> NN-19 Amplitude Velocity Amount
		CC_BRASS_AMP_LEVEL	= 74,	// Combinator Control 4 -> Brass NN-19 Master Level
		CC_RECEIVE_NOTES	= 75,	// Combinator Button 1 -> Combinator Receive Notes
	};
	enum {
		MF_TIMEBASE = 120,		// MIDI file timebase in ticks per quarter note
		MF_TEMPO = 120,			// MIDI file tempo in beats per minute
	};

// Member data
	CMidiOut	m_midiOut;		// MIDI output device
	CMidiFile::CMidiEventArray	m_aMidiEvent[MIDI_CHANNELS];	// array of MIDI events for each channel
	BYTE	m_aColorNote[COLORS];	// each color's note
	BYTE	m_aColorTone[COLORS];	// each color's unrotated tone index
	BYTE	m_aChanNote[COLORS];	// each MIDI channel's note
	BYTE	m_aChanVolume[COLORS];	// each MIDI channel's volume
	int		m_nCurMidiChan;		// current MIDI channel
	int		m_nChanCycles;		// number of cycles through all channels
	int		m_nStartNote;		// current start note
	BYTE	m_nPrevNote;		// previous note
	bool	m_bIsExport;		// true if exporting MIDI file
	double	m_fFrameRate;		// frame rate when exporting
	ULONGLONG	m_nFrames;		// number of frames rendered
	FILE*	m_fLog;				// log file for debugging

// Helpers
	int		MakeScale(int iCycle);
	CString	FormatScale(int nKey);
	DWORD	GetFrameTick(ULONGLONG iFrame);
	void	PrintScales();
};

inline bool CPalimpsestMusic::IsOpen() const
{
	return m_midiOut.IsOpen();
}

