// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		15jan25	initial version
		01		12mar25	remove frame size accessor
		02		13nov26	rename for Direct3D

*/

#pragma once

#include "d2d1_1.h"
#include "d3d11.h"
#include "dxgi1_2.h"

class CD3DDevCtx {
public:
// Construction
	CD3DDevCtx();
	virtual ~CD3DDevCtx();

// Attributes
	bool	IsCreated() const;
	bool	IsFullScreen();

// Operations
	bool	Create(HWND hWnd, bool bDebugDevice = false);
	void	Destroy();
	bool	Resize();
	bool	SetFullScreen(bool bEnable);
	bool	GetOutputDesc(DXGI_OUTPUT_DESC& desc);
	bool	GetAdapterDesc(DXGI_ADAPTER_DESC& desc);
	bool	GetDisplayFrequency(DWORD& dwFrequency);
	static bool	GetDisplayDeviceInfo(LPCTSTR pszDevName, DISPLAY_DEVICE& dd);
	static bool	GetDeviceMode(const DXGI_OUTPUT_DESC& desc, DEVMODE& mode);

// Overrideables
	virtual void	OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate);
	virtual bool	OnCreateDevice();
	virtual	bool	CreateUserResources();
	virtual	void	DestroyUserResources();
	virtual void	OnPreResize();
	virtual bool	OnResize();

protected:
// Data members
	CComPtr<ID3D11Device> m_pDevice;	// Direct3D device
	CComPtr<ID3D11DeviceContext>	m_pContext;	// Direct3D device context
	CComPtr<ID2D1Factory1>	m_pD2Factory;	// Direct2D factory
	CComPtr<ID2D1Device>	m_pD2Device;	// Direct2D device
	CComPtr<ID2D1DeviceContext>	m_pD2Context;	// Direct2D device context
	CComPtr<IDXGISwapChain1>	m_pSwapChain;	// DXGI swap chain
	CComPtr<ID3D11Texture2D>	m_pBackBuffer;	// swap chain backbuffer
	CComPtr<ID3D11RenderTargetView>		m_pBackView;	// backbuffer target view

// Helpers
	bool	CreateDevice(bool bDebugDevice = false);
	bool	CreateSwapChain(HWND hWnd);
	bool	CreateD2D();
};

inline CD3DDevCtx::~CD3DDevCtx()
{
}

inline bool CD3DDevCtx::IsCreated() const
{
	return m_pSwapChain != NULL;
}
