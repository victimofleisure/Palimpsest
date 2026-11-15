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

RWTexture2D<float4> Next0 : register(u0);
RWTexture2D<float4> Next1 : register(u1);
RWTexture2D<float4> Next2 : register(u2);
RWTexture2D<float4> Next3 : register(u3);

[numthreads(8, 8, 1)]
void Update(uint3 vThread : SV_DispatchThreadID)
{
	uint2 ptPixel = vThread.xy;
	if (ptPixel.x >= Width || ptPixel.y >= Height)
		return;
	float4 vState0 = State0.Load(int3(ptPixel, 0));
	float4 vState1 = State1.Load(int3(ptPixel, 0));
	float4 vState2 = State2.Load(int3(ptPixel, 0));
	float4 vState3 = State3.Load(int3(ptPixel, 0));
	float fCoverage = saturate(Coverage.Load(int3(ptPixel, 0)).a);
	if (fCoverage == 0) {
		Next0[ptPixel] = vState0;
		Next1[ptPixel] = vState1;
		Next2[ptPixel] = vState2;
		Next3[ptPixel] = vState3;
		return;
	}
	float arrOldWeight[16] = {
		vState0.x, vState0.y, vState0.z, vState0.w,
		vState1.x, vState1.y, vState1.z, vState1.w,
		vState2.x, vState2.y, vState2.z, vState2.w,
		vState3.x, vState3.y, vState3.z, vState3.w};
	float arrNewWeight[16];
	[unroll] for (uint iClr = 0; iClr < 16; ++iClr)
		arrNewWeight[iClr] = (1 - fCoverage) * arrOldWeight[iClr];
	// Retain the uncovered part and transfer the covered part through the bit
	// overwrite. Several old indices can contribute to one new palette index.
	[unroll] for (uint iOldClr = 0; iOldClr < 16; ++iOldClr) {
		uint iNewClr = (iOldClr & (~PlaneMask & 15u)) | (NewColor & PlaneMask);
		arrNewWeight[iNewClr] += fCoverage * arrOldWeight[iOldClr];
	}
	// Same accumulation order and normalization as the proof. This prevents a
	// common scale error building up during very long installation runs.
	float fMass = 0;
	[unroll] for (uint iWeight = 0; iWeight < 16; ++iWeight)
		fMass += arrNewWeight[iWeight];
	float fInvMass = rcp(max(fMass, 1e-20));
	[unroll] for (uint iNorm = 0; iNorm < 16; ++iNorm)
		arrNewWeight[iNorm] *= fInvMass;
	Next0[ptPixel] = float4(arrNewWeight[0], arrNewWeight[1], arrNewWeight[2], arrNewWeight[3]);
	Next1[ptPixel] = float4(arrNewWeight[4], arrNewWeight[5], arrNewWeight[6], arrNewWeight[7]);
	Next2[ptPixel] = float4(arrNewWeight[8], arrNewWeight[9], arrNewWeight[10], arrNewWeight[11]);
	Next3[ptPixel] = float4(arrNewWeight[12], arrNewWeight[13], arrNewWeight[14], arrNewWeight[15]);
}
