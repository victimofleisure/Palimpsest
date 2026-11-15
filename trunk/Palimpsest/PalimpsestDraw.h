// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      13nov26	initial version

*/

#pragma once

#include "RenderThread.h"
#include "Statistics.h"
#include "RandList.h"
#include "CmdLineParams.h"
#include "MidiWrap.h"
#include "MidiFile.h"

class CPalimpsestDraw : public CRenderThread, protected CCmdLineParams {
public:
// Construction
	CPalimpsestDraw();
	~CPalimpsestDraw();
	bool	Create(HWND hWnd, CWnd* pNotifyWnd = NULL);
	void	Destroy();

// Attributes
	void	SetParams(const CCmdLineParams& params);

// Commands

// Operations
	void	Resize();
	bool	FullScreen(bool bEnable);

protected:
// Constants
	enum {
		COLORS = 16,			// four-bit palette indices
		WEIGHT_TEXTURES = 4,	// four RGBA32_FLOAT textures contain sixteen weights
		STATE_BANKS = 2,		// committed state and current ellipse result
		SHADER_RESOURCES = WEIGHT_TEXTURES + 1,	// weights plus coverage
	};

// Types
	class CStateBank {
	// Construction
	public:
		CStateBank();
		~CStateBank();

	// Implementation
		CComPtr<ID3D11Texture2D>			m_arrTexture[WEIGHT_TEXTURES];
		CComPtr<ID3D11ShaderResourceView>	m_arrShaderView[WEIGHT_TEXTURES];
		CComPtr<ID3D11UnorderedAccessView>	m_arrWriteView[WEIGHT_TEXTURES];
		CSize	m_szFrame;

		void Destroy();

	private:
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

		void Destroy();

	private:
		CCoverageBuffer(const CCoverageBuffer&);
		CCoverageBuffer& operator=(const CCoverageBuffer&);
	};
	class CPaletteColor {
	public:
		float fRed;
		float fGreen;
		float fBlue;
		float fAlpha;

		CPaletteColor();
		CPaletteColor(float fRedVal, float fGreenVal, float fBlueVal, float fAlphaVal = 1);
		void	Randomize();
		void	Tween(const CPaletteColor& clrStart, const CPaletteColor& clrTarget, double fPos);
	};
	class DECLSPEC_ALIGN(16) CShaderParams {
	public:
		CPaletteColor arrPalette[COLORS];
		UINT nWidth;
		UINT nHeight;
		UINT nChanMask;
		UINT iCurColor;

		CShaderParams();
		void	RandomizePalette();
	};
	class CEllipseState {
	public:
		float fCenterX;
		float fCenterY;
		float fRadiusX;
		float fRadiusY;
		float fOuterRadiusX;
		float fOuterRadiusY;
		bool bContracting;

		CEllipseState();
	};

// Data members
	CStatistics	m_stats;	// statistics for measuring throughput
	CStateBank	m_aBank[STATE_BANKS];	// array of state banks
	CCoverageBuffer	m_coverage;	// antialiased coverage mask for current ellipse
	CComPtr<ID3D11ComputeShader>	m_pUpdateShader;	// update shader
	CComPtr<ID3D11VertexShader>		m_pVertexShader;	// vertex shader
	CComPtr<ID3D11PixelShader>		m_pPixelShader;	// pixel shader
	CComPtr<ID3D11Buffer>			m_pParameters;	// parameters
	CComPtr<ID3D11RasterizerState>	m_pRasterState;	// rasterizer state
	CComPtr<ID2D1SolidColorBrush>	m_pWhiteBrush;	// solid white brush
	D3D11_VIEWPORT	m_viewport;	// viewport
	CShaderParams	m_params;	// shader parameters
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
	ULONGLONG	m_nFrames;	// number of frames rendered
	double	m_fPrevTime;	// previous time for measuring throughput
	CDoubleArray	m_aFrameBenchmark;	// array of per-frame benchmarks for logging

// Musical accompaniment
	enum {	// continuous controller numbers
		CC_VOLUME			= 71,	// Combinator Control 1 -> Line Mixer Channel 1 Volume
		CC_PAN				= 72,	// Combinator Control 2 -> Line Mixer Channel 1 pan
		CC_AMP_ENV_ATTACK	= 73,	// Combinator Control 3 -> NN-19 Amplitude Envelope Attack
		CC_AMP_ENV_RELEASE	= 74,	// Combinator Control 4 -> NN-19 Amplitude Envelope Release
		CC_AMP_VEL_AMOUNT	= 2,	// Combinator Breath -> NN-19 Amplitude Velocity Amount
	};
	enum {
		MF_TIMEBASE = 120,		// MIDI file timebase in ticks per quarter note
		MF_TEMPO = 120,			// MIDI file tempo in beats per minute
	};
	CMidiOut	m_midiOut;		// MIDI output device
	BYTE	m_arrColorNote[COLORS];	// each color's note
	BYTE	m_arrChanNote[COLORS];	// each MIDI channel's note
	BYTE	m_arrChanVolume[COLORS];	// each MIDI channel's volume
	int		m_nCurMidiChan;		// current MIDI channel
	int		m_nChanCycles;		// number of cycles through all channels
	bool	m_bIsMidiExport;	// true if exporting a MIDI file; otherwise live MIDI
	BYTE	m_nPrevNote;		// previous note
	CMidiFile::CMidiEventArray	m_arrMidiEvent[COLORS];	// array of MIDI events for each channel

	void	MakeScale(int nStartNote);
	void	PrintScale();
	bool	MidiOpen();
	void	MidiInit();
	void	MidiClose();
	void	MidiStartDraw();
	void	MidiContinueDraw(double fPos);
	void	MidiOut(int nCmd, int nChan, int nP1, int nP2);
	static DWORD	GetFrameTick(ULONGLONG iFrame);
	void	MidiFileInit();
	void	WriteMidiFile(LPCTSTR pszPath);

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
};
