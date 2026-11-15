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

#include "stdafx.h"
#include "CmdLineParams.h"

CCmdLineParams::CCmdLineParams()
{
	m_bStartFullScreen = false;
	m_nRandSeed = 0;
	m_fSpeed = 2 * 4 * 60 / 1080.0;	// matches original GDI version
	m_nMidiOut = -1;
}

