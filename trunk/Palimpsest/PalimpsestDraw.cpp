// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      13nov26	initial version
        01      16nov26	add export

*/

#include "stdafx.h"
#include "Palimpsest.h"
#include "PalimpsestDraw.h"
#include <d3dcompiler.h>
#include "Easing.h"

#define DTF(x) static_cast<float>(x)

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

#define MATCH_REFERENCE		1		// non-zero to match reference video's randomization
#define DISPLAY_BENCHMARKS	0		// non-zero to display benchmarks in console window
#define LOG_BENCHMARKS		0		// non-zero to write benchmarks in app's log file
#define BENCHMARK_GROW_BY	7200	// number of samples by which benchmark array grows
#define STATISTICS_FRAMES	120		// number of frames to accumulate statistics over
#define EXPORT_MIDI_ONLY	0		// non-zero to export MIDI file only, skipping images

CPalimpsestDraw::CPalimpsestDraw()
	: m_stats(STATISTICS_FRAMES)
{
	static_assert(COLORS == CPalimpsestMusic::COLORS, "drawing and music share 16-color scheme");
	ZeroMemory(&m_viewport, sizeof(m_viewport));
	m_rlColor.Init(COLORS);
	m_rlChanMask.Init(COLORS - 1);
	m_ptCenter = CPoint(0, 0);
	m_szEllipse = CSize(0, 0);
	m_fRadiusStep = 0;
	m_nEllipseFrames = 0;
	m_iEllipseCurFrame = 0;
	m_iCommittedBank = 0;
	m_iShownBank = 0;
	m_bContracting = false;
	m_bFreshStart = false;
	m_bIsPaused = false;
	m_bSingleStep = false;
	m_bIsExport = false;
	m_nFrames = 0;
	m_fPrevTime = 0;
	m_aFrameBenchmark.SetGrowBy(BENCHMARK_GROW_BY);
}

CPalimpsestDraw::~CPalimpsestDraw()
{
}

void CPalimpsestDraw::SetCmdLineArgs(const CCmdLineArgs& args)
{
	ASSERT(m_pThread == NULL);	// only safe before thread is launched
	m_args = args;
}

bool CPalimpsestDraw::Pause(bool bEnable)
{
	return PushCommand(CRenderCmd(RC_PAUSE, bEnable));
}

bool CPalimpsestDraw::SingleStep()
{
	return PushCommand(CRenderCmd(RC_SINGLE_STEP));
}

bool CPalimpsestDraw::Create(HWND hWnd, CWnd* pNotifyWnd)
{
	return CreateThread(hWnd, pNotifyWnd);	// create render thread
}

void CPalimpsestDraw::ResetGeometry()
{
	m_iEllipseCurFrame = -1;  // -1 means initialize a new ellipse
	m_bFreshStart = true;
	m_params.iCurColor = 0;
	m_params.nChanMask = 0;
}

void CPalimpsestDraw::Destroy()
{
	m_bThreadExit = true;	// request render thread to exit
	if (m_bIsExport) {	// if exporting
		CD3DDevCtx::Destroy();
	} else {	// normal run
		DestroyThread();	// destroy render thread
	}
#if LOG_BENCHMARKS
	LogBenchmarks();
#endif
	if (m_music.IsOpen())
		m_music.Close();
}

void CPalimpsestDraw::Resize()
{
	// don't access render thread's data in this method
	PushCommand(RC_RESIZE);	// enqueue command
}

bool CPalimpsestDraw::FullScreen(bool bEnable)
{
	return PushCommand(CRenderCmd(RC_SET_FULLSCREEN, bEnable));
}

bool CPalimpsestDraw::OnCreateDevice()
{
	UINT nSupport = 0;
	CHECK(m_pDevice->CheckFormatSupport(DXGI_FORMAT_R32G32B32A32_FLOAT, &nSupport));
	if (!(nSupport & D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW)) {
		CHECK(DXGI_ERROR_UNSUPPORTED);
	}
	return true;
}

bool CPalimpsestDraw::CreateUserResources()
{
	if (!CreateShaders())
		return false;
	if (!CreateRasterizer())
		return false;
	D2D1::ColorF	clr(D2D1::ColorF::White);
	CHECK(m_pD2Context->CreateSolidColorBrush(clr, &m_pWhiteBrush));
	return true;
}

void CPalimpsestDraw::DestroyUserResources()
{
	m_pUpdateShader.Release();
	m_pVertexShader.Release();
	m_pPixelShader.Release();
	m_pParameters.Release();
	m_pRasterState.Release();
	m_pWhiteBrush.Release();
	m_pGradientBrush.Release();
	OnPreResize();
	if (m_bThreadExit)	// if thread exiting
		CoUninitialize();	// needed for WIC
}

bool CPalimpsestDraw::OnThreadCreate()
{
	srand(m_args.m_nRandSeed);	// set random seed in render thread
	if (MATCH_REFERENCE) {
		// to match the reference output, we must account for two extra CreateBackbuffer
		// calls during window sizing, each of which calls rand three times for each color
		for (int i = 0; i < COLORS * 6; i++) {
			rand();
		}
	}
	CHECK(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));	// needed for WIC
	m_params.RandomizePalette();
	if (m_args.m_bStartFullScreen) {	// if full screen launch requested
		if (!OnSetFullScreen(true))
			return false;
	}
	m_fPrevTime = CBenchmark::Time();
	if (m_args.m_nMidiOut >= 0)
		m_music.Open();
	return true;
}

void CPalimpsestDraw::OnPreResize()
{
	if (m_pD2Context != NULL)
		m_pD2Context->SetTarget(NULL);	// unbind Direct2D target
	if (m_pContext != NULL)
		UnbindD3DResources();	// unbind Direct3D resources
	// destroy state banks
	for (int iBank = 0; iBank < STATE_BANKS; iBank++) {	// for each state bank
		m_aBank[iBank].Destroy();
	}
	m_coverage.Destroy();	// destroy coverage buffer
}

bool CPalimpsestDraw::OnResize()
{
	D3D11_TEXTURE2D_DESC desc;
	m_pBackBuffer->GetDesc(&desc);
	DWORD	nDisplayFreq;
	// display frequency can potentially be 0 or 1, indicating default refresh rate
	if (!GetDisplayFrequency(nDisplayFreq) || nDisplayFreq <= 1)
		nDisplayFreq = 60;	// fall back to a reasonable default
	CSize szFrame(desc.Width, desc.Height);
	return CreateFrameResources(szFrame, nDisplayFreq);
}

bool CPalimpsestDraw::CreateFrameResources(CSize szFrame, double fFrameRate)
{
	ASSERT(szFrame.cx && szFrame.cy && fFrameRate > 0);
	m_params.nWidth = szFrame.cx;
	m_params.nHeight = szFrame.cy;
	m_fFrameRate = fFrameRate;
	// convert diameter growth in window heights/sec to radius growth in DIPs/frame
	m_fRadiusStep = m_args.m_fSpeed * szFrame.cy / (2 * fFrameRate);
	ResetGeometry();
	D3D11_VIEWPORT viewport = {0, 0, float(szFrame.cx), float(szFrame.cy), 0, 1};
	m_viewport = viewport;
	for (int iBank = 0; iBank < STATE_BANKS; iBank++) {	// for each state bank
		if (!CreateStateBank(m_aBank[iBank], szFrame))	// create bank
			return false;
		ClearBank(m_aBank[iBank]);	// clear bank
	}
	return CreateCoverageBuffer(m_coverage, szFrame);
}

inline double CPalimpsestDraw::Exponential(double fVal, double fPower)
{
	return (pow(fPower, fVal) - 1) / (fPower - 1);
}

bool CPalimpsestDraw::OnDraw()
{
#if (DISPLAY_BENCHMARKS && defined(_DEBUG)) || LOG_BENCHMARKS
	double	tNow = CBenchmark::Time();
	double	tElapsed = tNow - m_fPrevTime;
	m_fPrevTime = tNow;
#endif
	m_iShownBank = !m_iCommittedBank;
	int nSize = min(m_params.nWidth, m_params.nHeight);
	if (nSize <= 0)
		return true;
	if (m_iEllipseCurFrame < 0) {	// if new ellipse requested
		// initialize a new ellipse; randomize ellipse center point
		m_ptCenter = CPoint(CRandList::Rand(m_params.nWidth), CRandList::Rand(m_params.nHeight));
		const double fMinFrac = 0.125;	// minimum size as a fraction of screen
		const double fMaxFrac = 0.5;	// maximum size as a fraction of screen
		m_szEllipse = CSize(	// randomize ellipse size within specified limits
			Round(nSize * fMinFrac) +
				CRandList::Rand(Round(nSize * (fMaxFrac - fMinFrac))),
			Round(nSize * fMinFrac) +
				CRandList::Rand(Round(nSize * (fMaxFrac - fMinFrac))));
		// get color channel from randomized list
		m_params.iCurColor = m_rlColor.GetNext(m_params.iCurColor);
		if (m_bFreshStart) {	// if first ellipse after a recreate
			m_params.nChanMask = COLORS - 1;	// ensure something gets drawn
			m_bFreshStart = false;	// reset flag
		} else {	// usual case; get channel mask from randomized list
			m_params.nChanMask = m_rlChanMask.GetNext(m_params.nChanMask - 1) + 1;
		}
		int nMaxRadius = max(m_szEllipse.cx, m_szEllipse.cy);
		if (m_fRadiusStep <= 0)	// if invalid radius step
			return true;	// avoid divide by zero
		m_iEllipseCurFrame = 0;
		m_nEllipseFrames = max(1, Round(nMaxRadius / m_fRadiusStep));
		m_clrStart = m_params.arrPalette[m_params.iCurColor];	// start color is current color
		m_clrTarget.Randomize();	// choose a random destination color
		m_bContracting = (rand() & 1) != 0;	// randomly select expansion or contraction
		if (m_music.IsOpen() || m_bIsExport)	// if MIDI output device open or exporting
			m_music.StartPrimitive(m_params.iCurColor);
		if (m_args.m_bUseGradient) {	// if using gradient brush
			const double fPower = 0.1;
			double	fNormRand = rand() / double(RAND_MAX);
			double	fGradientCoreFrac = Exponential(fNormRand, fPower);
			// stops array is constant except for middle stop's position
			static	D2D1_GRADIENT_STOP	aStops[] = {
				{0, D2D1::ColorF(1, 1, 1, 1)},
				{0, D2D1::ColorF(1, 1, 1, 1)},
				{1, D2D1::ColorF(1, 1, 1, 0)}
			};
			aStops[1].position = DTF(fGradientCoreFrac);
			CComPtr<ID2D1GradientStopCollection> pStops;
			CHECK(m_pD2Context->CreateGradientStopCollection(
				aStops, _countof(aStops), &pStops));
			static const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES propsGrad = {};
			m_pGradientBrush.Release();
			CHECK(m_pD2Context->CreateRadialGradientBrush(
				propsGrad, pStops, &m_pGradientBrush));
		}
	}
	bool	bAdvancing = true;
	if (m_bIsPaused) {	// if paused
		if (m_bSingleStep) {	// if single step requested
			m_bSingleStep = false;	// clear single step flag
		} else {	// really paused
			bAdvancing = false;
		}
	}
	if (bAdvancing) {
		m_iEllipseCurFrame++;
	}
	double fPos = double(m_iEllipseCurFrame) / m_nEllipseFrames;
	double fEasedPos = EaseOut(fPos, 0.3);
	double fBidirPos = m_bContracting ? 1.0 - fEasedPos : fEasedPos;
	CEllipseState	state;
	state.fCenterX = float(m_ptCenter.x);
	state.fCenterY = float(m_ptCenter.y);
	state.fOuterRadiusX = float(m_szEllipse.cx);
	state.fOuterRadiusY = float(m_szEllipse.cy);
	state.fRadiusX = float(m_szEllipse.cx * fBidirPos);
	state.fRadiusY = float(m_szEllipse.cy * fBidirPos);
	state.bContracting = m_bContracting;
	m_params.arrPalette[m_params.iCurColor].Tween(
		m_clrStart, m_clrTarget, fEasedPos);
	if (!EXPORT_MIDI_ONLY || !m_bIsExport) {	// if not skipping images
		// nearly all the overhead is from this block
		m_pContext->UpdateSubresource(m_pParameters, 0, NULL, &m_params, 0, 0);
		RasterizeCoverage(m_coverage, state);
		UpdateWeights(m_aBank[m_iCommittedBank],
			m_aBank[m_iShownBank], m_coverage);
		DrawFrame(m_aBank[m_iShownBank], m_coverage);
	}
	if (m_iEllipseCurFrame >= m_nEllipseFrames) {	// if ellipse complete
		m_iCommittedBank = m_iShownBank;	// commit current bank
		m_iEllipseCurFrame = -1;	// trigger new ellipse on next frame
	}
	if (m_music.IsOpen() || m_bIsExport)	// if MIDI output device open or exporting
		m_music.ContinuePrimitive(fEasedPos);
	m_nFrames++;
#if DISPLAY_BENCHMARKS && defined(_DEBUG)
	m_stats.Print(tElapsed);
#endif
#if LOG_BENCHMARKS
	m_aFrameBenchmark.Add(tElapsed);
#endif
	return true;
}

void CPalimpsestDraw::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	theApp.OnError(hr, pszSrcFileName, nLineNum, pszSrcFileDate);
}

void CPalimpsestDraw::OnPause(bool bEnable)
{
	m_bIsPaused = bEnable;
}

void CPalimpsestDraw::OnSingleStep()
{
	if (m_bIsPaused) {
		m_bSingleStep = true;
	}
}

void CPalimpsestDraw::OnRenderCommand(const CRenderCmd& cmd)
{
//	printf("OnRenderCommand %d %d %llx\n", cmd.m_nCmd, cmd.m_nParam, cmd.m_prop);
	switch (cmd.m_nCmd) {
	case RC_PAUSE:
		OnPause(cmd.m_nParam != 0);
		break;
	case RC_SINGLE_STEP:
		OnSingleStep();
		break;
	default:
		NODEFAULTCASE;
	}
}

CPalimpsestDraw::CStateBank::CStateBank()
{
	m_szFrame.cx = 0;
	m_szFrame.cy = 0;
}

CPalimpsestDraw::CStateBank::~CStateBank()
{
	Destroy();
}

void CPalimpsestDraw::CStateBank::Destroy()
{
	for (int iTexture = 0; iTexture < WEIGHT_TEXTURES; iTexture++) {	// for each weight texture
		m_arrWriteView[iTexture].Release();
		m_arrShaderView[iTexture].Release();
		m_arrTexture[iTexture].Release();
	}
}

CPalimpsestDraw::CCoverageBuffer::CCoverageBuffer()
{
	m_pTexture = NULL;
	m_pShaderView = NULL;
	m_pRenderView = NULL;
	m_pBitmap = NULL;
	m_szFrame.cx = 0;
	m_szFrame.cy = 0;
}

CPalimpsestDraw::CCoverageBuffer::~CCoverageBuffer()
{
	Destroy();
}

void CPalimpsestDraw::CCoverageBuffer::Destroy()
{
	m_pBitmap.Release();
	m_pRenderView.Release();
	m_pShaderView.Release();
	m_pTexture.Release();
}

bool CPalimpsestDraw::CreateStateBank(CStateBank& bank, CSize szFrame)
{
	bank.m_szFrame = szFrame;
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = szFrame.cx;
	desc.Height = szFrame.cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	for (int iTexture = 0; iTexture < WEIGHT_TEXTURES; iTexture++) {	// for each weight texture
		CHECK(m_pDevice->CreateTexture2D(&desc, NULL, &bank.m_arrTexture[iTexture]));
		CHECK(m_pDevice->CreateShaderResourceView(bank.m_arrTexture[iTexture], NULL,
			&bank.m_arrShaderView[iTexture]));
		CHECK(m_pDevice->CreateUnorderedAccessView(bank.m_arrTexture[iTexture], NULL,
			&bank.m_arrWriteView[iTexture]));
	}
	return true;
}

bool CPalimpsestDraw::CreateCoverageBuffer(CCoverageBuffer& buffer, CSize szFrame)
{
	buffer.m_szFrame = szFrame;
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = szFrame.cx;
	desc.Height = szFrame.cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	CHECK(m_pDevice->CreateTexture2D(&desc, NULL, &buffer.m_pTexture));
	CHECK(m_pDevice->CreateShaderResourceView(buffer.m_pTexture, NULL, &buffer.m_pShaderView));
	CHECK(m_pDevice->CreateRenderTargetView(buffer.m_pTexture, NULL, &buffer.m_pRenderView));
	CComPtr<IDXGISurface> pSurface;
	CHECK(buffer.m_pTexture->QueryInterface(IID_PPV_ARGS(&pSurface)));
	D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
	CHECK(m_pD2Context->CreateBitmapFromDxgiSurface(pSurface, &props, &buffer.m_pBitmap));
	return true;
}

void CPalimpsestDraw::ClearBank(CStateBank& bank)
{
	for (int iTexture = 0; iTexture < WEIGHT_TEXTURES; iTexture++) {	// for each weight texture
		float arrValue[4] = {iTexture == 0 ? 1.f : 0.f, 0, 0, 0};	// four RGBA channels per texture
		m_pContext->ClearUnorderedAccessViewFloat(bank.m_arrWriteView[iTexture], arrValue);
	}
}

void CPalimpsestDraw::UnbindD3DResources()
{
	ID3D11ShaderResourceView *arrShaderView[SHADER_RESOURCES] = {};
	ID3D11UnorderedAccessView *arrWriteView[WEIGHT_TEXTURES] = {};
	m_pContext->CSSetShaderResources(0, SHADER_RESOURCES, arrShaderView);
	m_pContext->PSSetShaderResources(0, SHADER_RESOURCES, arrShaderView);
	m_pContext->CSSetUnorderedAccessViews(0, WEIGHT_TEXTURES, arrWriteView, NULL);
	m_pContext->OMSetRenderTargets(0, NULL, NULL);
}

HRESULT CPalimpsestDraw::LoadShader(LPCTSTR pszFileName, CByteArrayEx& aCode)
{
	// assume shaders are in same folder as executable
	CString sPath(CWinAppCK::GetAppFolder() + _T("\\") + pszFileName);
	TRY {	// CFile throws errors
		CFile fShader(sPath, CFile::modeRead);
		ULONGLONG nSize = fShader.GetLength();
		aCode.SetSize(INT_PTR(nSize));
		if (fShader.Read(aCode.GetData(), UINT(nSize)) != nSize)	// if too few bytes read
			return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);	// end of file error
	}
	CATCH (CFileException, e) {
		return HRESULT_FROM_WIN32(e->m_lOsError);	// specific OS error number
	}
	CATCH(CException, e) {
		return E_FAIL;	// unspecified error
	}
	END_CATCH;
	return S_OK;	// success
}

bool CPalimpsestDraw::CreateShaders()
{
	CByteArrayEx	aCode;
	CHECK(LoadShader(_T("Update.cso"), aCode));
	CHECK(m_pDevice->CreateComputeShader(aCode.GetData(), aCode.GetSize64(), NULL, &m_pUpdateShader));
	CHECK(LoadShader(_T("Vertex.cso"), aCode));
	CHECK(m_pDevice->CreateVertexShader(aCode.GetData(), aCode.GetSize64(), NULL, &m_pVertexShader));
	CHECK(LoadShader(_T("Pixel.cso"), aCode));
	CHECK(m_pDevice->CreatePixelShader(aCode.GetData(), aCode.GetSize64(), NULL, &m_pPixelShader));
	return true;
}

inline CPalimpsestDraw::CPaletteColor::CPaletteColor()
{
	fRed = 0;
	fGreen = 0;
	fBlue = 0;
	fAlpha = 1;
}

inline CPalimpsestDraw::CPaletteColor::CPaletteColor(float fRedVal, float fGreenVal, float fBlueVal, float fAlphaVal)
{
	fRed = fRedVal;
	fGreen = fGreenVal;
	fBlue = fBlueVal;
	fAlpha = fAlphaVal;
}

inline void CPalimpsestDraw::CPaletteColor::Randomize()
{
	fRed = float(BYTE(rand()) / 255.0);
	fGreen = float(BYTE(rand()) / 255.0);
	fBlue = float(BYTE(rand()) / 255.0);
}

inline void CPalimpsestDraw::CPaletteColor::Tween(const CPaletteColor& clrStart, const CPaletteColor& clrTarget, double fPos)
{
	fRed = DTF(clrStart.fRed + (clrTarget.fRed - clrStart.fRed) * fPos);
	fGreen = DTF(clrStart.fGreen + (clrTarget.fGreen - clrStart.fGreen) * fPos);
	fBlue = DTF(clrStart.fBlue + (clrTarget.fBlue - clrStart.fBlue) * fPos);
}

inline CPalimpsestDraw::CShaderParams::CShaderParams()
{
	nWidth = 0;
	nHeight = 0;
	nChanMask = 0;
	iCurColor = 0;
}

void CPalimpsestDraw::CShaderParams::RandomizePalette()
{
	for (int iColor = 0; iColor < COLORS; iColor++) {	// for each color
		arrPalette[iColor].Randomize();
	}
}

inline CPalimpsestDraw::CEllipseState::CEllipseState()
{
	fCenterX = 0;
	fCenterY = 0;
	fRadiusX = 0;
	fRadiusY = 0;
	fOuterRadiusX = 0;
	fOuterRadiusY = 0;
	bContracting = false;
}

bool CPalimpsestDraw::CreateRasterizer()
{	
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizeof(CShaderParams);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	CHECK(m_pDevice->CreateBuffer(&bufferDesc, NULL, &m_pParameters));
	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthClipEnable = TRUE;
	CHECK(m_pDevice->CreateRasterizerState(&rasterDesc, &m_pRasterState));
	return true;
}

bool CPalimpsestDraw::RasterizeCoverage(CCoverageBuffer& buffer, const CEllipseState& state)
{
	UnbindD3DResources();
	m_pD2Context->SetTarget(buffer.m_pBitmap);
	m_pD2Context->SetTransform(D2D1::Matrix3x2F::Identity());
	m_pD2Context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	CComPtr<ID2D1EllipseGeometry> pOuter;
	CComPtr<ID2D1EllipseGeometry> pInner;
	CComPtr<ID2D1GeometryGroup> pRing;
	// This fixed pair of ellipses is only a transient coverage mask. Historical
	// state lives entirely in pixels; no geometry fragments accumulate.
	if (state.bContracting && state.fRadiusX > 0 && state.fRadiusY > 0
	&& (state.fRadiusX < state.fOuterRadiusX || state.fRadiusY < state.fOuterRadiusY)) {
		HRESULT hResult = m_pD2Factory->CreateEllipseGeometry(
			D2D1::Ellipse(D2D1::Point2F(state.fCenterX, state.fCenterY), state.fOuterRadiusX, state.fOuterRadiusY), &pOuter);
		if (SUCCEEDED(hResult)) {
			hResult = m_pD2Factory->CreateEllipseGeometry(
				D2D1::Ellipse(D2D1::Point2F(state.fCenterX, state.fCenterY), state.fRadiusX, state.fRadiusY), &pInner);
		}
		if (SUCCEEDED(hResult)) {
			ID2D1Geometry *arrShape[] = {pOuter, pInner};
			hResult = m_pD2Factory->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE, arrShape, 2, &pRing);
		}
		if (FAILED(hResult))
			CHECK(hResult);
	}
	ID2D1Brush*	pBrush;	// pointer to solid or gradient brush
	if (m_args.m_bUseGradient) {	// if using gradient brush
		float rx = state.bContracting ? state.fOuterRadiusX : state.fRadiusX;
		float ry = state.bContracting ? state.fOuterRadiusY : state.fRadiusY;
		m_pGradientBrush->SetCenter(D2D1::Point2F(state.fCenterX, state.fCenterY));
		m_pGradientBrush->SetRadiusX(max(1.0f, rx));
		m_pGradientBrush->SetRadiusY(max(1.0f, ry));
		pBrush = m_pGradientBrush;
	} else {	// use solid brush
		pBrush = m_pWhiteBrush;
	}
	m_pD2Context->BeginDraw();
	m_pD2Context->Clear(D2D1::ColorF(0, 0, 0, 0));
	if (state.bContracting) {
		if (state.fRadiusX <= 0 || state.fRadiusY <= 0) {
			m_pD2Context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(state.fCenterX, state.fCenterY),
				state.fOuterRadiusX, state.fOuterRadiusY), pBrush);
		} else if (pRing) {
			m_pD2Context->FillGeometry(pRing, pBrush);
		}
	} else if (state.fRadiusX > 0 && state.fRadiusY > 0) {
		m_pD2Context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(state.fCenterX, state.fCenterY),
			state.fRadiusX, state.fRadiusY), pBrush);
	}
	HRESULT hResult = m_pD2Context->EndDraw();
	m_pD2Context->SetTarget(NULL);
	if (FAILED(hResult))
		CHECK(hResult);
	return true;
}

void CPalimpsestDraw::UpdateWeights(CStateBank& oldBank, CStateBank& newBank, CCoverageBuffer& mask)
{
	ASSERT(oldBank.m_arrTexture[0] != newBank.m_arrTexture[0]); // Source and destination banks must differ
	ASSERT(oldBank.m_szFrame.cx == newBank.m_szFrame.cx && oldBank.m_szFrame.cy == newBank.m_szFrame.cy
		&& oldBank.m_szFrame.cx == mask.m_szFrame.cx && oldBank.m_szFrame.cy == mask.m_szFrame.cy); // Mismatched state dimensions
	UnbindD3DResources();
	ID3D11ShaderResourceView *arrShaderView[] = {
		oldBank.m_arrShaderView[0], oldBank.m_arrShaderView[1],
		oldBank.m_arrShaderView[2], oldBank.m_arrShaderView[3], mask.m_pShaderView};
	ID3D11UnorderedAccessView *arrWriteView[] = {
		newBank.m_arrWriteView[0], newBank.m_arrWriteView[1],
		newBank.m_arrWriteView[2], newBank.m_arrWriteView[3]};
	m_pContext->CSSetShader(m_pUpdateShader, NULL, 0);
	ID3D11Buffer *pBuffer = m_pParameters;
	m_pContext->CSSetConstantBuffers(0, 1, &pBuffer);
	m_pContext->CSSetShaderResources(0, 5, arrShaderView);
	m_pContext->CSSetUnorderedAccessViews(0, WEIGHT_TEXTURES, arrWriteView, NULL);
	m_pContext->Dispatch((oldBank.m_szFrame.cx + 7) / 8, (oldBank.m_szFrame.cy + 7) / 8, 1);
	UnbindD3DResources();
}

void CPalimpsestDraw::DrawFrame(CStateBank& bank, CCoverageBuffer& mask)
{
	UnbindD3DResources();
	ID3D11RenderTargetView *pBackView = m_pBackView;
	m_pContext->OMSetRenderTargets(1, &pBackView, NULL);
	m_pContext->OMSetBlendState(NULL, NULL, 0xffffffff);
	m_pContext->OMSetDepthStencilState(NULL, 0);
	m_pContext->RSSetViewports(1, &m_viewport);
	m_pContext->RSSetState(m_pRasterState);
	m_pContext->IASetInputLayout(NULL);
	m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pContext->VSSetShader(m_pVertexShader, NULL, 0);
	m_pContext->GSSetShader(NULL, NULL, 0);
	m_pContext->HSSetShader(NULL, NULL, 0);
	m_pContext->DSSetShader(NULL, NULL, 0);
	m_pContext->PSSetShader(m_pPixelShader, NULL, 0);
	ID3D11Buffer *pBuffer = m_pParameters;
	m_pContext->PSSetConstantBuffers(0, 1, &pBuffer);
	ID3D11ShaderResourceView *arrShaderView[] = {
		bank.m_arrShaderView[0], bank.m_arrShaderView[1],
		bank.m_arrShaderView[2], bank.m_arrShaderView[3], mask.m_pShaderView};
	m_pContext->PSSetShaderResources(0, 5, arrShaderView);
	m_pContext->Draw(3, 0);
	UnbindD3DResources();
}

void CPalimpsestDraw::LogBenchmarks()
{
	int	nFrames = m_aFrameBenchmark.GetSize();	// array of per-frame benchmarks
	for (int iFrame = 0; iFrame < nFrames; iFrame += STATISTICS_FRAMES) {	// for each chunk
		m_stats.Reset();
		int	nSamps = min(iFrame + STATISTICS_FRAMES, nFrames);
		for (int iSamp = iFrame; iSamp < nSamps; iSamp++) {	// for each sample in chunk
			m_stats.Add(m_aFrameBenchmark[iSamp]);
		}
		CString	sLine;
		sLine.Format(_T("%d\t%f\t%f\t%f\t%f"), m_stats.GetCount(), m_stats.GetMin(), 
			m_stats.GetMax(), m_stats.GetAverage(), m_stats.GetStandardDeviation());
		theApp.WriteLogEntry(sLine);
	}
}

bool CPalimpsestDraw::CreateExport()
{
	ASSERT(m_pSwapChain == NULL);	// don't call Create when exporting
	// override problematic command line parameters
	m_args.m_bStartFullScreen = false;	// no full screen
	m_args.m_nMidiOut = -1;	// no MIDI output device
	m_music.ExportInit();
	m_bIsExport = true;
	if (!CreateOffscreen())
		return false;
	if (!OnThreadCreate())
		return false;
	CSize	szFrame(m_args.m_szRecFrameSize);
	double	fFrameRate = m_args.m_fRecFrameRate;
	if (!CreateFrameResources(szFrame, fFrameRate))
		return false;
	if (!CreateExportResources(szFrame))
		return false;
	return true;
}

bool CPalimpsestDraw::ExportFrame(LPCTSTR pszPath)
{
	OnDraw();
	if (EXPORT_MIDI_ONLY)
		return true;
	return WriteFrame(pszPath);
}

void CPalimpsestDraw::WriteMidiFile(LPCTSTR pszPath)
{
	m_music.WriteFile(pszPath);
}

