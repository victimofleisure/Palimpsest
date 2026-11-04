// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      05nov25	initial version
		01		28nov25	add symmetrical easing

*/

#pragma once

// s is the normalized progress of the motion, from 0 to 1
// F is the fraction of the motion to ease, from 0 to 1

double EaseIn(double s, double F);
double EaseOut(double s, double F);

// Ease in or out depending on a boolean, simplifying calling code
inline double EaseInOrOut(bool bIsOut, double s, double F)
{
	return bIsOut ? EaseOut(s, F) : EaseIn(s, F);
}

// Construct a symmetric ease-in-out from EaseIn and EaseOut
inline double EaseInAndOut(double s, double F)
{
	return s < 0.5 ? EaseIn(s * 2, F) / 2 : (EaseOut(s * 2 - 1, F) + 1) / 2;
}
