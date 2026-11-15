// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		revision history:
		rev		date	comments
		00		11nov26	standalone pixel-weight proof of concept
		01		11nov26	match original random sequence
		02		11nov26	rewrite in Palimpsest coding style

*/

// Names, offsets and registers are kept identical to the proven shader ABI.
cbuffer Parameters : register(b0)
{
	float4 Palette[16];
	uint Width;
	uint Height;
	uint PlaneMask;
	uint NewColor;
};

// Each RGBA32_FLOAT texture holds four of the sixteen palette-index weights.
Texture2D<float4> State0 : register(t0);
Texture2D<float4> State1 : register(t1);
Texture2D<float4> State2 : register(t2);
Texture2D<float4> State3 : register(t3);
Texture2D<float4> Coverage : register(t4);
