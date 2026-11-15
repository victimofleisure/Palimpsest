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

class CCmdLineParams {
public:
// Construction
	CCmdLineParams();

// Types

// Constants

// Public data
	bool	m_bStartFullScreen;	// true if starting in full screen mode
	UINT	m_nRandSeed;		// starting point for random number generation
	double	m_fSpeed;			// animation speed in screen heights per second
	int		m_nMidiOut;			// output MIDI device index

// Attributes

// Operations

// Constants
};
