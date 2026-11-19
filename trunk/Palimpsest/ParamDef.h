// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      15nov26	initial version
		01		19nov26	add record MIDI only

*/

#ifdef PARAMDEF

PARAMDEF(help)			// display help
PARAMDEF(fullscr)		// start application in full screen mode
PARAMDEF(seed)			// starting point for random number generation
PARAMDEF(speed)			// animation speed in window heights per second
PARAMDEF(gradient)		// non-zero to use gradient brush
PARAMDEF(midiout)		// output MIDI device index
PARAMDEF(record)		// recording destination folder path
PARAMDEF(recsize)		// recording frame size, in pixels
PARAMDEF(recrate)		// recording frame rate, in frames per second
PARAMDEF(recdur)		// recording duration, in seconds
PARAMDEF(recmidi)		// record MIDI only; skip generating images
PARAMDEF(markdown)		// write help markdown to the specified file
PARAMDEF(license)		// display the license

#undef PARAMDEF
#endif

