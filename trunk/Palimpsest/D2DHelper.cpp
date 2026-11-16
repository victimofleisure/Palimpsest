// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		11nov25	initial version
		01		23nov25	fix incorrect x-axis bounds in glyph iterator
		02		24nov25	fix glyph iterator handling of RTL languages
		03		27nov25	add tight vertical bounds flag to glyph iterator
		04		30nov25	add calculate maximum glyph bounds
		05		11dec25 add font collection class
		06		18dec25	add set position method to glyph iterator
		07		19dec25	add save transform class
		08		29dec25	add method to create compatible bitmap

*/

#pragma once

#include "stdafx.h"
#include "D2DHelper.h"
#define _USE_MATH_DEFINES
#include "math.h"

#define DTF(x) static_cast<FLOAT>(x)

CGlyphIter::CGlyphIter(CKD2DPointF ptBaselineOrigin, DWRITE_GLYPH_RUN const* pGlyphRun, bool bTightVertBounds)
	: m_ptOrigin(ptBaselineOrigin)
{
	ASSERT(pGlyphRun != NULL);
	m_pGlyphRun = pGlyphRun;
	DWRITE_FONT_METRICS	fontMetrics;	// font metrics for run
	pGlyphRun->fontFace->GetMetrics(&fontMetrics);
	m_fEmScale = pGlyphRun->fontEmSize / fontMetrics.designUnitsPerEm;
	m_fAscent = fontMetrics.ascent * m_fEmScale;
	m_fDescent = fontMetrics.descent * m_fEmScale;
	m_iGlyph = 0;
	m_fOriginX = m_ptOrigin.x;
	m_fAdvance = 0;
	m_bTightVertBounds = bTightVertBounds;
}

bool CGlyphIter::GetNext(UINT& iGlyph, CKD2DRectF& rGlyph)
{
	// assumes a horizontal run
	ASSERT(m_pGlyphRun != NULL);
	iGlyph = m_iGlyph;
	if (iGlyph >= m_pGlyphRun->glyphCount)	// if end of glyphs
		return false;
	UINT16	nGlyphIndex = m_pGlyphRun->glyphIndices[iGlyph];
	DWRITE_GLYPH_METRICS	gm;
	m_pGlyphRun->fontFace->GetDesignGlyphMetrics(&nGlyphIndex, 1, &gm);
	FLOAT	fAdvanceWidth = gm.advanceWidth * m_fEmScale;	// may differ from run's advance width
	FLOAT	fLeftBearing = gm.leftSideBearing * m_fEmScale;
	FLOAT	fRightBearing = gm.rightSideBearing * m_fEmScale;
	bool	bRTL = m_pGlyphRun->bidiLevel & 1;	// odd level indicates right-to-left
	// horizontal ink extents (black box) in run direction
	if (bRTL) {	// if right-to-left
		rGlyph.left = m_ptOrigin.x - fAdvanceWidth + fLeftBearing;
		rGlyph.right = m_ptOrigin.x - fRightBearing;
	} else {	// left-to-right
		rGlyph.left = m_ptOrigin.x + fLeftBearing;
		rGlyph.right = m_ptOrigin.x + fAdvanceWidth - fRightBearing;
	}
	if (m_bTightVertBounds) {	// if tight vertical bounds requested
		FLOAT	fTopBearing = gm.topSideBearing * m_fEmScale;
		FLOAT	fAdvanceHeight = gm.advanceHeight * m_fEmScale;
		FLOAT	fBottomBearing = gm.bottomSideBearing * m_fEmScale;
		FLOAT	fVertOriginY = gm.verticalOriginY * m_fEmScale;
		rGlyph.top = m_ptOrigin.y - fVertOriginY + fTopBearing;
		rGlyph.bottom = m_ptOrigin.y - fVertOriginY + fAdvanceHeight - fBottomBearing;
	} else {	// use font metrics for vertical bounds
		rGlyph.top = m_ptOrigin.y - m_fAscent;
		rGlyph.bottom = m_ptOrigin.y + m_fDescent;
	}
	if (m_pGlyphRun->glyphOffsets) {	// if offsets specified
		const DWRITE_GLYPH_OFFSET& goff = m_pGlyphRun->glyphOffsets[iGlyph];
		// advanceOffset is along the run direction, ascenderOffset is along Y
		// we're in screen coords, so if right-to-left, flip advanceOffset
		FLOAT	fAdvOffset = bRTL ? -goff.advanceOffset : goff.advanceOffset;
		rGlyph.OffsetRect(fAdvOffset, goff.ascenderOffset);
	}
	// if run's advance pointer is null, substitute font's advance width
	m_fAdvance = m_pGlyphRun->glyphAdvances != NULL ?
		m_pGlyphRun->glyphAdvances[iGlyph] : fAdvanceWidth;
	if (bRTL) {	// if right-to-left
		m_ptOrigin.x -= m_fAdvance;	// negative advance
	} else {	// left-to-right
		m_ptOrigin.x += m_fAdvance;	// positive advance
	}
	m_iGlyph++;	// next glyph
	return true;
}

CKD2DSizeF CGlyphIter::CalcMaxGlyphBounds()
{
	CKD2DSizeF	szMax(0, 0);
	CKD2DRectF	rGlyph;
	UINT	iGlyph;
	while (GetNext(iGlyph, rGlyph)) {	// for each glyph
		CKD2DSizeF	szGlyph(rGlyph.Size());
		if (szGlyph.width > szMax.width)	// if wider
			szMax.width = szGlyph.width;	// update width
		if (szGlyph.height > szMax.height)	// if taller
			szMax.height = szGlyph.height;	// update height
	}
	return szMax;
}

void CGlyphIter::SetPos(UINT iGlyph)
{
	ASSERT(m_pGlyphRun != NULL);
	ASSERT(iGlyph >= 0 && iGlyph < m_pGlyphRun->glyphCount);	// check index range
	if (iGlyph < m_iGlyph)	// if target index is before current index
		Reset();	// only advance is supported, so start at beginning
	bool	bRTL = m_pGlyphRun->bidiLevel & 1;	// odd level indicates right-to-left
	while (m_iGlyph < iGlyph) {	// while below target index, advance
		FLOAT	fGlyphAdvance = m_pGlyphRun->glyphAdvances[m_iGlyph];
		if (bRTL) {	// if right-to-left
			m_ptOrigin.x -= fGlyphAdvance;	// negative advance
		} else {	// left-to-right
			m_ptOrigin.x += fGlyphAdvance;	// positive advance
		}
		m_iGlyph++;	// next glyph
	}
}

void AddEllipse(ID2D1GeometrySink *pSink, D2D1_POINT_2F ptOrigin, D2D1_SIZE_F szRadius)
{
	ASSERT(pSink != NULL);
	CKD2DPointF	ptOrg1(ptOrigin.x, ptOrigin.y - szRadius.height);
	CKD2DPointF	ptOrg2(ptOrigin.x, ptOrigin.y + szRadius.height);
	pSink->BeginFigure(ptOrg1, D2D1_FIGURE_BEGIN_FILLED);
	D2D1_ARC_SEGMENT	arc1 = {ptOrg2, szRadius,
		0, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL};
	D2D1_ARC_SEGMENT	arc2 = {ptOrg1, szRadius,
		0, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL};
	pSink->AddArc(&arc1);
	pSink->AddArc(&arc2);
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
}

void AddPieWedge(ID2D1GeometrySink *pSink, D2D1_POINT_2F ptOrigin, D2D1_SIZE_F szRadius, FLOAT fStartAngle, FLOAT fWedgeFrac)
{
	ASSERT(pSink != NULL);
	double	fTheta1 = fStartAngle / 180 * M_PI;
	CKD2DPointF	pt1(
		DTF(sin(fTheta1) * szRadius.width + ptOrigin.x), 
		DTF(cos(fTheta1) * szRadius.height + ptOrigin.y));
	double	fTheta2 = fTheta1 + fWedgeFrac * M_PI * 2;
	CKD2DPointF	pt2(
		DTF(sin(fTheta2) * szRadius.width + ptOrigin.x), 
		DTF(cos(fTheta2) * szRadius.height + ptOrigin.y));
	//
	// A single AddArc can't draw a circle, so check if the wedge is the
	// entire pie, and if so, handle that by calling AddEllipse instead.
	//
	const double fEpsilon = 0.01;	// tolerance in DIPs
	// if the arc's end points are uncomfortably close together
	if (fabs(pt2.x - pt1.x) < fEpsilon && fabs(pt2.y - pt1.y) < fEpsilon) {
		if (fWedgeFrac > 0.5) // if the wedge is entire pie, draw ellipse
			AddEllipse(pSink, ptOrigin, szRadius);
		return;	// if wedge is ultra-thin, draw nothing
	}
	pSink->BeginFigure(ptOrigin, D2D1_FIGURE_BEGIN_FILLED);
	pSink->AddLine(pt1);
	D2D1_ARC_SIZE	arcSize = fTheta2 - fTheta1 > M_PI ?
		D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL;
	D2D1_ARC_SEGMENT	arc = {pt2, szRadius, 0,
		D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, arcSize};
	pSink->AddArc(&arc);
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
}

bool CD2DFontCollection::Create()
{
	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, 
		__uuidof(IDWriteFactory), (IUnknown**)&m_pDWriteFactory)))
		return false;
	if (FAILED(m_pDWriteFactory->GetSystemFontCollection(&m_pFontCollection, FALSE)))
		return false;
	UINT32	nFamilies = m_pFontCollection->GetFontFamilyCount();
	WCHAR	szLocaleName[LOCALE_NAME_MAX_LENGTH] = {};
	int	nLocaleLen = GetUserDefaultLocaleName(szLocaleName, _countof(szLocaleName));
	m_aFontFamilyName.SetSize(nFamilies);
	m_aFontFamilyName.FastRemoveAll();
	for (UINT32 iFamily = 0; iFamily < nFamilies; iFamily++) {
		CComPtr<IDWriteFontFamily> pFamily;
		if (FAILED(m_pFontCollection->GetFontFamily(iFamily, &pFamily)))
			continue;
		CComPtr<IDWriteLocalizedStrings> pNames;
		if (FAILED(pFamily->GetFamilyNames(&pNames)))
			continue;
		UINT32	iIndex = 0;
		BOOL	bExists = FALSE;
		if (nLocaleLen) {
			if (FAILED(pNames->FindLocaleName(szLocaleName, &iIndex, &bExists)))
				continue;
		}
		if (!bExists) {
			if (FAILED(pNames->FindLocaleName(L"en-us", &iIndex, &bExists)))
				continue;
		}
		UINT32	nNameLen = 0;
		if (FAILED(pNames->GetStringLength(iIndex, &nNameLen)))
			continue;
		CString	sFamilyName;
		LPTSTR	pszFamilyName = sFamilyName.GetBuffer(nNameLen);
		if (FAILED(pNames->GetString(iIndex, pszFamilyName, nNameLen + 1)))
			continue;
		sFamilyName.ReleaseBuffer();
		m_aFontFamilyName.Add(sFamilyName);
	}
	return true;
}

HRESULT CreateCompatibleBitmap(ID2D1DeviceContext *pDC, ID2D1Bitmap1 **pOff, D2D1_BITMAP_OPTIONS nOptions)
{
	// create bitmap with same size, pixel format, and DPI as given device context
	CComPtr<ID2D1Image>	pTarget;
	pDC->GetTarget(&pTarget);
	CComPtr<ID2D1Bitmap1> pTargetBitmap;
	HRESULT	hr;
	hr = pTarget->QueryInterface(IID_PPV_ARGS(&pTargetBitmap));
	if (FAILED(hr))
		return hr;
	CKD2DSizeF szDpi;
	pTargetBitmap->GetDpi(&szDpi.width, &szDpi.height);
	D2D1_BITMAP_PROPERTIES1	props = {pTargetBitmap->GetPixelFormat(), 
		szDpi.width, szDpi.height, nOptions};
	hr = pDC->CreateBitmap(pTargetBitmap->GetPixelSize(), NULL, 0, &props, pOff);
	return hr;
}
