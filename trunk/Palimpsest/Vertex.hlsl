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

#include "Common.hlsli"

float4 FullscreenVS(uint iVertex : SV_VertexID) : SV_Position
{
	// One oversized triangle; no vertex buffer or input layout is required.
	float2 vTexCoord = float2((iVertex << 1) & 2, iVertex & 2);
	return float4(vTexCoord * float2(2, -2) + float2(-1, 1), 0, 1);
}
