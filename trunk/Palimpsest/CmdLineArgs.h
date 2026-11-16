// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      15nov26	initial version

*/

#pragma once

class CCmdLineArgs {
public:
// Construction
	CCmdLineArgs();

// Types

// Constants

// Public data
	bool	m_bStartFullScreen;	// true if starting in full screen mode
	bool	m_bUseGradient;		// true if using gradient brush
	UINT	m_nRandSeed;		// starting point for random number generation
	double	m_fSpeed;			// animation speed in window heights per second
	int		m_nMidiOut;			// output MIDI device index
	CString	m_sRecFolderPath;	// recording destination folder path
	CSize	m_szRecFrameSize;	// recording frame size in pixels
	float	m_fRecFrameRate;	// recording frame rate
	int		m_nRecDuration;		// recording duration in frames

// Attributes

// Operations

// Constants
};
