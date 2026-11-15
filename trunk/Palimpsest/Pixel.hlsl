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

float4 DisplayPS(float4 vPosition : SV_Position) : SV_Target
{
	int3 ptPixel = int3(int2(vPosition.xy), 0);
	float4 vState0 = State0.Load(ptPixel), vState1 = State1.Load(ptPixel),
		vState2 = State2.Load(ptPixel), vState3 = State3.Load(ptPixel);
	float arrWeight[16] = {
		vState0.x, vState0.y, vState0.z, vState0.w,
		vState1.x, vState1.y, vState1.z, vState1.w,
		vState2.x, vState2.y, vState2.z, vState2.w,
		vState3.x, vState3.y, vState3.z, vState3.w};
	float3 vColor = 0;
	[unroll] for (uint iClr = 0; iClr < 16; ++iClr) {
		vColor += arrWeight[iClr] * Palette[iClr].rgb;
	}
	return float4(vColor, 1);
}
