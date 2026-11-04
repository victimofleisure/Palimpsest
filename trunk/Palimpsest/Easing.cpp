// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      05nov25	initial version

*/

#include "stdafx.h"
#define _USE_MATH_DEFINES
#include "math.h"

// s is the normalized progress of the motion, from 0 to 1
// F is the fraction of the motion to ease, from 0 to 1
//
// these methods were written by GPT5

double EaseIn(double s, double F)
{
	if (F <= 0)
		return s;	// no easing
	if (F >= 1) {
		// pure sine over full range
		return 1 - cos(M_PI_2 * s);
	}
	double V = M_PI / (2 * F);	// slope at join
	double N = 1 + V * (1 - F);	// normalization factor
	if (s <= F) {
		// cosine ease, normalized
		double u = s / F;
		return (1 - cos(M_PI_2 * u)) / N;
	} else {
		// linear continuation, normalized
		return (1 + V * (s - F)) / N;
	}
}

double EaseOut(double s, double F)
{
	if (F <= 0)
		return s;	// no easing
	if (F >= 1) {
		// pure sine over full range
		return sin(M_PI_2 * s);
	}
	double V = M_PI / (2 * F);	// slope at join
	double N = 1 + V * (1 - F);	// normalization factor
	if (s < 1 - F) {
		// linear segment first, normalized
		return V * s / N;
	} else {
		// final sine ease-out, normalized
		double u = (s - (1 - F)) / F;	// 0..1 within the easing window
		double E = sin(M_PI_2 * u);
		double base = V * (1 - F) / N;	// where linear left off
		double scale = (1 - base) / 1;	// amount remaining to reach 1
		return base + scale * E;
	}
}
