// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		21feb25	initial version
		01		27oct25	add size and inflate methods
		02		01nov25	add more constructors
		03		11nov25	add glyph iterator
		04		18nov25	add RGBA color init
		05		23nov25	add font metrics member to glyph iterator
		06		24nov25	add get origin method to glyph iterator
		07		27nov25	add tight vertical bounds flag to glyph iterator
		08		29nov25	add dips to pixels conversion methods
		09		30nov25	add ascent and descent attributes to glyph iterator
		10		01dec25	add reset transform
		11		03dec25 add auto unmap resource
		12		11dec25 add BGR color init
		13		11dec25 add font collection class
		14		18dec25	add set position method to glyph iterator
		15		19dec25	add save transform class
		16		27dec25	add inline point and size classes
		17		29dec25	add method to create compatible bitmap
		18		16nov26	fix W4 compiler errors

*/

#pragma once

#include "d2d1_1.h"
#include "d3d11.h"

inline D2D1_SIZE_F GetDpi(ID2D1RenderTarget* pRT)
{
	D2D1_SIZE_F	szDpi = {96.0f, 96.0f};
	if (pRT)
		pRT->GetDpi(&szDpi.width, &szDpi.height);
	return szDpi;
}

inline D2D1_SIZE_U DipsToPixels(D2D1_SIZE_F szDIPs, D2D1_SIZE_F szDpi)
{
	UINT	nX = static_cast<UINT>(ceilf(szDIPs.width * szDpi.width / 96.0f));
	UINT	nY = static_cast<UINT>(ceilf(szDIPs.height * szDpi.height / 96.0f));
	return D2D1::SizeU(nX, nY);
}

inline D2D1_SIZE_F PixelsToDips(D2D1_SIZE_U szPixels, D2D1_SIZE_F szDpi)
{
	FLOAT	fX = szPixels.width * 96.0f / szDpi.width;
	FLOAT	fY = szPixels.height * 96.0f / szDpi.height;
	return D2D1::SizeF(fX, fY);
}

inline D2D1_SIZE_U DipsToPixels(ID2D1RenderTarget* pRT, D2D1_SIZE_F szDIPs)
{
	return DipsToPixels(szDIPs, GetDpi(pRT));
}

inline D2D1_SIZE_F PixelsToDips(ID2D1RenderTarget* pRT, D2D1_SIZE_U szPixels)
{
	return PixelsToDips(szPixels, GetDpi(pRT));
}

class CKD2DPointF : public D2D1_POINT_2F {	// like base class but inline
public:
	CKD2DPointF(const CPoint& pt);
	CKD2DPointF(const D2D1_POINT_2F& pt);
	CKD2DPointF(const D2D1_POINT_2F* pt);
	CKD2DPointF(FLOAT fX = 0., FLOAT fY = 0.);
	operator CPoint() { return CPoint((LONG)x, (LONG)y); }
};

inline CKD2DPointF::CKD2DPointF(const CPoint& pt)
{
	x = (FLOAT)pt.x;
	y = (FLOAT)pt.y;
}

inline CKD2DPointF::CKD2DPointF(const D2D1_POINT_2F& pt)
{
	x = pt.x;
	y = pt.y;
}

inline CKD2DPointF::CKD2DPointF(const D2D1_POINT_2F* pt)
{
	x = pt == NULL ? 0 : pt->x;
	y = pt == NULL ? 0 : pt->y;
}

inline CKD2DPointF::CKD2DPointF(FLOAT fX, FLOAT fY)
{
	x = fX;
	y = fY;
}

class CKD2DSizeF : public D2D1_SIZE_F {	// like base class but inline
public:
	CKD2DSizeF(const CSize& size);
	CKD2DSizeF(const D2D1_SIZE_F& size);
	CKD2DSizeF(const D2D1_SIZE_F* size);
	CKD2DSizeF(FLOAT cx = 0., FLOAT cy = 0.);
	BOOL IsNull() const { return width == 0. && height == 0.; }
	operator CSize() { return CSize((LONG)width, (LONG)height); }
};

inline CKD2DSizeF::CKD2DSizeF(const CSize& size)
{
	width = (FLOAT)size.cx;
	height = (FLOAT)size.cy;
}

inline CKD2DSizeF::CKD2DSizeF(const D2D1_SIZE_F& size)
{
	width = size.width;
	height = size.height;
}

inline CKD2DSizeF::CKD2DSizeF(const D2D1_SIZE_F* size)
{
	width = size == NULL ? 0 : size->width;
	height = size == NULL ? 0 : size->height;
}

inline CKD2DSizeF::CKD2DSizeF(FLOAT cx, FLOAT cy)
{
	width = cx;
	height = cy;
}

class CKD2DRectF : public D2D1_RECT_F {
public:
// Construction
	CKD2DRectF(const CRect& rect);
	CKD2DRectF(const D2D1_RECT_F& rect);
	CKD2DRectF(const D2D1_RECT_F* rect);
	CKD2DRectF(FLOAT fLeft = 0, FLOAT fTop = 0, FLOAT fRight = 0, FLOAT fBottom = 0);
	CKD2DRectF(const D2D1_POINT_2F& point, const D2D1_SIZE_F& size);
	CKD2DRectF(const D2D1_POINT_2F& ptTopLeft, const D2D1_POINT_2F& ptBottomRight);

// Attributes
	FLOAT	Width() const;
	FLOAT	Height() const;
	CKD2DSizeF	Size() const;
	CKD2DPointF	TopLeft() const;
	CKD2DPointF	BottomRight() const;
	CKD2DPointF	CenterPoint() const;
	bool	IsNormal() const;

// Operations
	void	OffsetRect(FLOAT dx, FLOAT dy);
	void	InflateRect(FLOAT dx, FLOAT dy);
	void	SetRectEmpty();
	bool	PtInRect(const D2D_POINT_2F& pt) const;
	void	Normalize();
	void	Union(const CKD2DRectF& r);
};

inline CKD2DRectF::CKD2DRectF(const CRect& rect)
{
	left = (FLOAT)rect.left;
	right = (FLOAT)rect.right;
	top = (FLOAT)rect.top;
	bottom = (FLOAT)rect.bottom;
}

inline CKD2DRectF::CKD2DRectF(const D2D1_RECT_F& rect)
{
	left = rect.left;
	right = rect.right;
	top = rect.top;
	bottom = rect.bottom;
}

inline CD2DRectF::CD2DRectF(const D2D1_RECT_F* rect)
{
	left = rect == NULL ? 0 : rect->left;
	right = rect == NULL ? 0 : rect->right;
	top = rect == NULL ? 0 : rect->top;
	bottom = rect == NULL ? 0 : rect->bottom;
}

inline CKD2DRectF::CKD2DRectF(FLOAT fLeft, FLOAT fTop, FLOAT fRight, FLOAT fBottom) 
{
	left = fLeft;
	top = fTop;
	right = fRight;
	bottom = fBottom;
}

inline CKD2DRectF::CKD2DRectF(const D2D1_POINT_2F& point, const D2D1_SIZE_F& size)
{
	left = point.x;
	top = point.y;
	right = point.x + size.width;
	bottom = point.y + size.height;
}

inline CKD2DRectF::CKD2DRectF(const D2D1_POINT_2F& ptTopLeft, const D2D1_POINT_2F& ptBottomRight)
{
	left = ptTopLeft.x;
	top = ptTopLeft.y;
	right = ptBottomRight.x;
	bottom = ptBottomRight.y;
}

inline FLOAT CKD2DRectF::Width() const
{ 
	return right - left;
}

inline FLOAT CKD2DRectF::Height() const
{ 
	return bottom - top;
}

inline CKD2DSizeF CKD2DRectF::Size() const
{
	return CKD2DSizeF(Width(), Height());
}

inline CKD2DPointF CKD2DRectF::TopLeft() const
{
	return CKD2DPointF(left, top);
}

inline CKD2DPointF CKD2DRectF::BottomRight() const
{
	return CKD2DPointF(bottom, right);
}

inline CKD2DPointF CKD2DRectF::CenterPoint() const
{
	return CKD2DPointF(left + Width() / 2, top + Height() / 2);
}

inline bool CKD2DRectF::IsNormal() const
{
	return left <= right && top <= bottom;
}

inline void CKD2DRectF::OffsetRect(FLOAT dx, FLOAT dy)
{ 
	left += dx;
	top += dy;
	right += dx;
	bottom += dy;
}

inline void CKD2DRectF::InflateRect(FLOAT dx, FLOAT dy)
{
	left -= dx;
	top -= dy;
	right += dx;
	bottom += dy;
}

inline void CKD2DRectF::SetRectEmpty() 
{ 
	left = 0;
	top = 0;
	right = 0;
	bottom = 0;
}

inline bool CKD2DRectF::PtInRect(const D2D_POINT_2F& pt) const
{ 
	return pt.x >= left && pt.y >= top && pt.x < right && pt.y < bottom;
}

inline void CKD2DRectF::Normalize()
{
	if (left > right)
		Swap(left, right);
	if (top > bottom)
		Swap(top, bottom);
}

inline void CKD2DRectF::Union(const CKD2DRectF& r)
{
	left = min(left, r.left);
	top = min(top, r.top);
	right = max(right, r.right);
	bottom = max(bottom, r.bottom);
}

class CGlyphIter {
public:
	CGlyphIter(CKD2DPointF ptBaselineOrigin, DWRITE_GLYPH_RUN const* pGlyphRun, bool bTightVertBounds = false);
	//
	// Note that rGlyph is the painted area of the glyph, also known as its ink
	// box or black box, already in world coordinates. Do NOT use rGlyph as the
	// glyph origin for drawing; use GetOrigin() instead, and be sure to obtain 
	// the origin *before* calling GetNext(), which updates it.
	//
	bool	GetNext(UINT& iGlyph, CKD2DRectF& rGlyph);
	CKD2DPointF	GetOrigin() const;
	FLOAT	GetAscent() const;
	FLOAT	GetDescent() const;
	void	Reset();
	void	SetPos(UINT iGlyph);
	CKD2DSizeF	CalcMaxGlyphBounds();
	FLOAT	GetAdvance() const;
	const FLOAT*	GetAdvancePtr() const;

protected:
	CKD2DPointF	m_ptOrigin;		// current origin
	DWRITE_GLYPH_RUN const* m_pGlyphRun;	// pointer to run
	FLOAT	m_fEmScale;		// scaling factor from font's design units to DIPs
	FLOAT	m_fAscent;		// scaled ascent
	FLOAT	m_fDescent;		// scaled descent
	UINT	m_iGlyph;		// index of current glyph
	FLOAT	m_fOriginX;		// original origin X for reset
	FLOAT	m_fAdvance;		// horizontal advance, even if run glyph advances are null
	bool	m_bTightVertBounds;	// if true, compute tight vertical bounds, else use ascent
};

inline CKD2DPointF CGlyphIter::GetOrigin() const
{
	return m_ptOrigin;
}

inline void CGlyphIter::Reset()
{
	m_iGlyph = 0;
	m_ptOrigin.x = m_fOriginX;
}

inline FLOAT CGlyphIter::GetAscent() const
{
	return m_fAscent;
}

inline FLOAT CGlyphIter::GetDescent() const
{
	return m_fDescent;
}

inline FLOAT CGlyphIter::GetAdvance() const
{
	return m_fAdvance;
}

inline const FLOAT* CGlyphIter::GetAdvancePtr() const
{
	return &m_fAdvance;
}

void	AddEllipse(ID2D1GeometrySink *pSink, D2D1_POINT_2F ptOrigin, D2D1_SIZE_F szRadius);
void	AddPieWedge(ID2D1GeometrySink *pSink, D2D1_POINT_2F ptOrigin, D2D1_SIZE_F szRadius, FLOAT fStartAngle, FLOAT fWedgeFrac);

inline D2D1::ColorF RGBAColorF(UINT32 rgba)
{
	// adapted from D2D1::ColorF constructor
	static const UINT32 shiftR = 24;
	static const UINT32 shiftG = 16;
	static const UINT32 shiftB = 8;
	static const UINT32 shiftA = 0;
	static const UINT32 maskR = 0xffu << shiftR;
	static const UINT32 maskG = 0xffu << shiftG;
	static const UINT32 maskB = 0xffu << shiftB;
	static const UINT32 maskA = 0xffu << shiftA;
	return D2D1::ColorF(
		static_cast<FLOAT>((rgba & maskR) >> shiftR) / 255.f,
		static_cast<FLOAT>((rgba & maskG) >> shiftG) / 255.f,
		static_cast<FLOAT>((rgba & maskB) >> shiftB) / 255.f,
		static_cast<FLOAT>((rgba & maskA) >> shiftA) / 255.f);
}

inline D2D1::ColorF BGRColorF(COLORREF clr, FLOAT fAlpha = 1.0)
{
	return D2D1::ColorF(
		static_cast<FLOAT>(GetRValue(clr)) / 255.f,
		static_cast<FLOAT>(GetGValue(clr)) / 255.f,
		static_cast<FLOAT>(GetBValue(clr)) / 255.f,
		fAlpha);
}

inline COLORREF ColorFBGR(const D2D1::ColorF& clr)
{
	return RGB(Round(clr.r * 255.), Round(clr.g * 255.), Round(clr.b * 255.));
}

inline void ResetTransform(ID2D1DeviceContext* pDC)
{
	pDC->SetTransform(D2D1::Matrix3x2F::Identity());
}

// helper class to automatically unmap a resource when the helper's instance
// is destroyed; the resource must be mapped, otherwise behavior is undefined
class CAutoUnmapResource {
public:
	CAutoUnmapResource(ID3D11DeviceContext* pD3DDC, ID3D11Resource* pD3DRes);
	~CAutoUnmapResource();

protected:
	ID3D11DeviceContext*	m_pD3DDC;	// pointer to Direct3D device context
	ID3D11Resource*	m_pD3DRes;	// pointer to Direct3D resource to be unmapped
};

inline CAutoUnmapResource::CAutoUnmapResource(ID3D11DeviceContext* pD3DDC, ID3D11Resource* pD3DRes)
{
	ASSERT(pD3DDC != NULL);
	ASSERT(pD3DRes != NULL);
	m_pD3DDC = pD3DDC;
	m_pD3DRes = pD3DRes;	// assume the resource is already mapped
}

inline CAutoUnmapResource::~CAutoUnmapResource()
{
	m_pD3DDC->Unmap(m_pD3DRes, 0);	// unmap resource
}

class CD2DFontCollection {
public:
	bool	Create();
	int		GetFamilyCount() const;
	CString	GetFamilyName(int iFamily) const;

protected:
	CComPtr<IDWriteFactory>		m_pDWriteFactory;
	CComPtr<IDWriteFontCollection>	m_pFontCollection;
	CStringArrayEx	m_aFontFamilyName;
};

inline int CD2DFontCollection::GetFamilyCount() const
{
	return m_aFontFamilyName.GetSize();
}

inline CString CD2DFontCollection::GetFamilyName(int iFamily) const
{
	return m_aFontFamilyName[iFamily];
}

class CD2DSaveTransform {
public:
	CD2DSaveTransform(ID2D1DeviceContext *pDC);
	~CD2DSaveTransform();

protected:
	ID2D1DeviceContext	*m_pDC;
	D2D1::Matrix3x2F	m_matPrev;
};

inline CD2DSaveTransform::CD2DSaveTransform(ID2D1DeviceContext *pDC)
{
	ASSERT(pDC != NULL);
	m_pDC = pDC;
	pDC->GetTransform(&m_matPrev);
}

inline CD2DSaveTransform::~CD2DSaveTransform()
{
	m_pDC->SetTransform(&m_matPrev);
}

HRESULT CreateCompatibleBitmap(ID2D1DeviceContext *pDC, ID2D1Bitmap1 **pOff, D2D1_BITMAP_OPTIONS nOptions = D2D1_BITMAP_OPTIONS_TARGET);

