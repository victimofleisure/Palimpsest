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

#pragma once

#include "RenderThread.h"
#include "Palimpsest.h"
#include "Statistics.h"
#include "RandList.h"
#include "CmdLineArgs.h"
#include "PalimpsestMusic.h"

class CPalimpsestDraw : public CRenderThread {
public:
// Construction
	CPalimpsestDraw();
	~CPalimpsestDraw();
	bool	Create(HWND hWnd, CWnd* pNotifyWnd = NULL);
	bool	CreateExport();
	void	Destroy();

// Attributes
	void	SetCmdLineArgs(const CCmdLineArgs& args);

// Commands
	bool	Pause(bool bEnable);
	bool	SingleStep();

// Operations
	void	Resize();
	bool	FullScreen(bool bEnable);
	bool	ExportFrame(LPCTSTR pszPath);
	void	WriteMidiFile(LPCTSTR pszPath);

protected:
// Constants
	enum {
		COLORS = 16,			// four-bit palette indices
		WEIGHT_TEXTURES = 4,	// four RGBA32_FLOAT textures contain sixteen weights
		STATE_BANKS = 2,		// committed state and current ellipse result
		SHADER_RESOURCES = WEIGHT_TEXTURES + 1,	// weights plus coverage
	};
	enum {	// drawing commands
		RC_PAUSE = BASE_RENDER_COMMANDS,	// param: bool, prop: none
		RC_SINGLE_STEP,			// param: none, prop: none
	};

// Types
	class CStateBank {
	// Construction
	public:
		CStateBank();
		~CStateBank();

	// Implementation
		CComPtr<ID3D11Texture2D>			m_aTexture[WEIGHT_TEXTURES];
		CComPtr<ID3D11ShaderResourceView>	m_aShaderView[WEIGHT_TEXTURES];
		CComPtr<ID3D11UnorderedAccessView>	m_aWriteView[WEIGHT_TEXTURES];
		CSize	m_szFrame;

	// Operations
		void Destroy();

	private:
	// Prevent binary copy
		CStateBank(const CStateBank&);
		CStateBank& operator=(const CStateBank&);
	};
	class CCoverageBuffer {
	// Construction
	public:
		CCoverageBuffer();
		~CCoverageBuffer();

	// Implementation
		CComPtr<ID3D11Texture2D>	m_pTexture;
		CComPtr<ID3D11ShaderResourceView>	m_pShaderView;
		CComPtr<ID3D11RenderTargetView>	m_pRenderView;
		CComPtr<ID2D1Bitmap1>	m_pBitmap;
		CSize	m_szFrame;

	// Operations
		void Destroy();

	private:
	// Prevent binary copy
		CCoverageBuffer(const CCoverageBuffer&);
		CCoverageBuffer& operator=(const CCoverageBuffer&);
	};
	class CPaletteColor {
	public:
	// Public data
		float fRed;
		float fGreen;
		float fBlue;
		float fAlpha;
	
	// Construction
		CPaletteColor();
		CPaletteColor(float fRedVal, float fGreenVal, float fBlueVal, float fAlphaVal = 1);

	// Operations
		void	Randomize();
		void	Tween(const CPaletteColor& clrStart, const CPaletteColor& clrTarget, double fPos);
	};
	// parameters shared between C++ and HLSL shaders; layout must match the shader Parameters
	// constant buffer, whose size must be a multiple of 16 bytes, and must be 16-byte aligned
	class DECLSPEC_ALIGN(16) CShaderParams {
	public:
	// Public data
		CPaletteColor aPalette[COLORS];
		UINT nWidth;
		UINT nHeight;
		UINT nChanMask;
		UINT iCurColor;

	// Construction
		CShaderParams();

	// Operations
		void	RandomizePalette();
	};
	class CEllipseState {
	public:
	// Public data
		float fCenterX;
		float fCenterY;
		float fRadiusX;
		float fRadiusY;
		float fOuterRadiusX;
		float fOuterRadiusY;
		bool bContracting;

	// Construction
		CEllipseState();
	};

// Data members
	D3D11_VIEWPORT	m_viewport;	// viewport
	CStateBank	m_aBank[STATE_BANKS];	// array of state banks
	CShaderParams	m_params;	// shader parameters
	CCoverageBuffer	m_coverage;	// antialiased coverage mask for current ellipse
	CComPtr<ID3D11ComputeShader>	m_pUpdateShader;	// update shader
	CComPtr<ID3D11VertexShader>		m_pVertexShader;	// vertex shader
	CComPtr<ID3D11PixelShader>		m_pPixelShader;	// pixel shader
	CComPtr<ID3D11Buffer>			m_pParameters;	// parameters
	CComPtr<ID3D11RasterizerState>	m_pRasterState;	// rasterizer state
	CComPtr<ID2D1SolidColorBrush>	m_pWhiteBrush;	// solid white brush
	CComPtr<ID2D1RadialGradientBrush>	m_pGradientBrush;	// radial gradient brush
	CCmdLineArgs	m_args;	// command line arguments
	CStatistics	m_stats;	// statistics for measuring throughput
	CRandList	m_rlColor;	// shuffle for colors
	CRandList	m_rlChanMask;	// shuffle for color channel masks
	CPoint	m_ptCenter;		// center point of current ellipse, in DIPs
	CSize	m_szEllipse;	// final ellipse radii (X and Y), in DIPs
	double	m_fRadiusStep;	// nominal ellipse radius increment per frame, in DIPs
	int		m_nEllipseFrames;	// total number of frames in current ellipse
	int		m_iEllipseCurFrame;	// current ellipse frame; -1 means start new ellipse
	CPaletteColor	m_clrStart;		// starting color for tweening
	CPaletteColor	m_clrTarget;	// target color for tweening
	int		m_iCommittedBank;	// index of committed bank
	int		m_iShownBank;	// index of shown bank
	bool	m_bContracting;	// true if ellipse is contracting instead of expanding
	bool	m_bFreshStart;	// true if geometry has been reset due to screen size change
	bool	m_bIsPaused;	// true if animation is paused
	bool	m_bSingleStep;	// true to step forward one frame while paused
	bool	m_bIsExport;	// true if exporting image sequence
	ULONGLONG	m_nFrames;	// number of frames rendered
	double	m_fPrevTime;	// previous time for measuring throughput
	double	m_fFrameRate;	// frame rate in frames per second
	CDoubleArray	m_aFrameBenchmark;	// array of per-frame benchmarks for logging
	CPalimpsestMusic	m_music;	// musical accompaniment

// CRenderTarget overrides
	virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
	virtual	bool	OnCreateDevice();
	virtual	bool	CreateUserResources();
	virtual	void	DestroyUserResources();
	virtual	bool	OnThreadCreate();
	virtual	void	OnPreResize();
	virtual	bool	OnResize();
	virtual	bool	OnDraw();
	virtual void	OnRenderCommand(const CRenderCmd& cmd);

// Command handlers
	void	OnPause(bool bEnable);
	void	OnSingleStep();

// Helpers
	bool	CreateCoverageBuffer(CCoverageBuffer& buffer, CSize szFrame);
	bool	CreateStateBank(CStateBank& bank, CSize szFrame);
	void	ClearBank(CStateBank& bank);
	void	UnbindD3DResources();
	static	HRESULT	LoadShader(LPCTSTR pszFileName, CByteArrayEx& aCode);
	bool	CreateShaders();
	bool	CreateRasterizer();
	bool	RasterizeCoverage(CCoverageBuffer& buffer, const CEllipseState& state);
	void	UpdateWeights(CStateBank& oldBank, CStateBank& newBank, CCoverageBuffer& mask);
	void	DrawFrame(CStateBank& bank, CCoverageBuffer& mask);
	void	ResetGeometry();
	void	LogBenchmarks();
	bool	CreateFrameResources(CSize szFrame, double fFrameRate);
	static double  Exponential(double fVal, double fPower);
};
