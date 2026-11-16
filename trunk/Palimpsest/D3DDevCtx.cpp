// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		rev		date	comments
		00		15jan25	initial version
		01		13nov26	rename for Direct3D
		02		16nov26	add export

*/

#include "stdafx.h"
#include "D3DDevCtx.h"
#include "D2DHelper.h"

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}
#define CHECK_VOID(x) { HRESULT hr = x; if (FAILED(hr)) OnError(hr, __FILE__, __LINE__, __DATE__); }

CD3DDevCtx::CD3DDevCtx()
{
	m_szExportFrameSize = CSize(0, 0);
}

void CD3DDevCtx::OnError(HRESULT hr, LPCSTR pszSrcFileName, int nLineNum, LPCSTR pszSrcFileDate)
{
	UNREFERENCED_PARAMETER(hr);
	UNREFERENCED_PARAMETER(pszSrcFileName);
	UNREFERENCED_PARAMETER(nLineNum);
	UNREFERENCED_PARAMETER(pszSrcFileDate);
}

bool CD3DDevCtx::OnCreateDevice()
{
	return true;
}

bool CD3DDevCtx::CreateUserResources()
{
	return true;
}

void CD3DDevCtx::DestroyUserResources()
{
}

void CD3DDevCtx::OnPreResize()
{
}

bool CD3DDevCtx::OnResize()
{
	return true;
}

bool CD3DDevCtx::Create(HWND hWnd, bool bDebugDevice)
{
	if (!CreateDevice(bDebugDevice))
		return false;
	if (!CreateSwapChain(hWnd))
		return false;
	if (!CreateD2D())
		return false;
	if (!OnCreateDevice())
		return false;
	if (!CreateUserResources())
		return false;
	return Resize();
}

bool CD3DDevCtx::CreateOffscreen()
{
	// create without swap chain and resizing
	if (!CreateDevice())
		return false;
	if (!CreateD2D())
		return false;
	if (!OnCreateDevice())
		return false;
	if (!CreateUserResources())
		return false;
	return true;
}

bool CD3DDevCtx::CreateDevice(bool bDebugDevice)
{
	D3D_FEATURE_LEVEL aFeatureLevels[] = {D3D_FEATURE_LEVEL_11_0};
	UINT nFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	if (bDebugDevice)
		nFlags |= D3D11_CREATE_DEVICE_DEBUG;
	CHECK(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, nFlags,	// hardware only
		aFeatureLevels, _countof(aFeatureLevels), D3D11_SDK_VERSION, 
		&m_pDevice, NULL, &m_pContext));
	return true;
}

bool CD3DDevCtx::CreateSwapChain(HWND hWnd)
{
	CComPtr<IDXGIDevice> pDxDevice;
	CHECK(m_pDevice->QueryInterface(IID_PPV_ARGS(&pDxDevice)));
	CComPtr<IDXGIAdapter> pAdapter;
	CHECK(pDxDevice->GetAdapter(&pAdapter));
	CComPtr<IDXGIFactory2> pDxFactory;
	CHECK(pAdapter->GetParent(IID_PPV_ARGS(&pDxFactory)));
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// flip presentation model avoids blits
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	CHECK(pDxFactory->CreateSwapChainForHwnd(m_pDevice, hWnd, 
		&swapChainDesc, NULL, NULL, &m_pSwapChain));
	CHECK(pDxFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));	// disable Alt+Enter shortcut
	CComPtr<IDXGIDevice1> pDxDevice1;
	CHECK(pDxDevice->QueryInterface(IID_PPV_ARGS(&pDxDevice1)));
	CHECK(pDxDevice1->SetMaximumFrameLatency(1));
	CHECK(m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&m_pBackBuffer)));
	return true;
}

bool CD3DDevCtx::CreateD2D()
{
	CComPtr<IDXGIDevice> pDxDevice;
	CHECK(m_pDevice->QueryInterface(IID_PPV_ARGS(&pDxDevice)));
	D2D1_FACTORY_OPTIONS options = {};
	CHECK(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1),
		&options, reinterpret_cast<void **>(&m_pD2Factory)));
	CHECK(m_pD2Factory->CreateDevice(pDxDevice, &m_pD2Device));
	CHECK(m_pD2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
		&m_pD2Context));
	return true;
}

void CD3DDevCtx::Destroy()
{
	DestroyUserResources();
	m_pExportStaging.Release();
	m_pWICFactory.Release();
	if (m_pD2Context != NULL)
		m_pD2Context->SetTarget(NULL);  // unbind target before release
	if (m_pContext != NULL) {
		m_pContext->ClearState();  // Unbind all views
		m_pContext->Flush();       // Force immediate destruction
	}
	m_pBackView.Release();
	m_pBackBuffer.Release();
	// don't release swap chain in full-screen mode as it could cause DXGI exception
	if (m_pSwapChain != NULL)
		CHECK_VOID(m_pSwapChain->SetFullscreenState(FALSE, NULL));	// set windowed mode
	m_pSwapChain.Release();
	m_pD2Context.Release();
	m_pD2Device.Release();
	m_pD2Factory.Release();
	m_pContext.Release();
	m_pDevice.Release();	// this implicitly releases the DXGI device
}

bool CD3DDevCtx::Resize()
{
	ASSERT(m_pSwapChain != NULL);
	OnPreResize();
	m_pContext->OMSetRenderTargets(0, NULL, NULL);
	m_pBackView.Release();
	m_pBackBuffer.Release();
	CHECK(m_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
	CHECK(m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&m_pBackBuffer)));
	CHECK(m_pDevice->CreateRenderTargetView(m_pBackBuffer, NULL, &m_pBackView));
	return OnResize();
}

bool CD3DDevCtx::IsFullScreen()
{
	ASSERT(m_pSwapChain != NULL);
	BOOL bIsFullscreen = FALSE;
	CHECK(m_pSwapChain->GetFullscreenState(&bIsFullscreen, NULL));
	return bIsFullscreen != 0;
}

bool CD3DDevCtx::SetFullScreen(bool bEnable)
{
	if (bEnable == IsFullScreen())
		return false;
	CHECK(m_pSwapChain->SetFullscreenState(bEnable, NULL));
	return Resize();
}

bool CD3DDevCtx::GetOutputDesc(DXGI_OUTPUT_DESC& desc)
{
	ASSERT(m_pSwapChain != NULL);
	CComPtr<IDXGIOutput> pOutput;
	CHECK(m_pSwapChain->GetContainingOutput(&pOutput));
	CHECK(pOutput->GetDesc(&desc));
	return true;
}

bool CD3DDevCtx::GetAdapterDesc(DXGI_ADAPTER_DESC& desc)
{
	ASSERT(m_pDevice != NULL);
	CComPtr<IDXGIDevice> pDXGIDevice;
	CHECK(m_pDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice)));
	CComPtr<IDXGIAdapter> pDXGIAdapter;
    CHECK(pDXGIDevice->GetAdapter(&pDXGIAdapter));
    CHECK(pDXGIAdapter->GetDesc(&desc));
	return true;
}

bool CD3DDevCtx::GetDisplayDeviceInfo(LPCTSTR pszDevName, DISPLAY_DEVICE& dd)
{
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	return EnumDisplayDevices(pszDevName, 0, &dd, 0) != 0;
}

bool CD3DDevCtx::GetDeviceMode(const DXGI_OUTPUT_DESC& desc, DEVMODE& mode)
{
	ZeroMemory(&mode, sizeof(mode));
	mode.dmSize = sizeof(mode);
	if (!EnumDisplaySettings(desc.DeviceName, ENUM_CURRENT_SETTINGS, &mode))
		return false;
	return true;
}

bool CD3DDevCtx::GetDisplayFrequency(DWORD& dwFrequency)
{
	DXGI_OUTPUT_DESC	desc;
	if (!GetOutputDesc(desc))
		return false;
	DEVMODE	mode;
	if (!GetDeviceMode(desc, mode))
		return false;
	dwFrequency = mode.dmDisplayFrequency;
	return true;
}

bool CD3DDevCtx::CreateBackBuffer(CSize szFrame)
{
	ASSERT(m_pDevice != NULL);
	ASSERT(szFrame.cx > 0 && szFrame.cy > 0);
	m_pBackView.Release();
	m_pBackBuffer.Release();
	D3D11_TEXTURE2D_DESC	desc = {};
	desc.Width = szFrame.cx;
	desc.Height = szFrame.cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	CHECK(m_pDevice->CreateTexture2D(&desc, NULL, &m_pBackBuffer));
	CHECK(m_pDevice->CreateRenderTargetView(m_pBackBuffer, NULL, &m_pBackView));
	return true;
}

bool CD3DDevCtx::CreateExportResources(CSize szFrame, REFGUID guidContainerFormat)
{
	if (!CreateBackBuffer(szFrame))
		return false;
	D3D11_TEXTURE2D_DESC desc;
	m_pBackBuffer->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;
	CHECK(m_pDevice->CreateTexture2D(&desc, NULL, &m_pExportStaging));
	CHECK(m_pWICFactory.CoCreateInstance(CLSID_WICImagingFactory));
	m_szExportFrameSize = szFrame;
	m_guidContainerFormat = guidContainerFormat;
	return true;
}

bool CD3DDevCtx::WriteFrame(LPCTSTR pszPath)
{
	m_pContext->CopyResource(m_pExportStaging, m_pBackBuffer);
	D3D11_MAPPED_SUBRESOURCE map = {};
	CHECK(m_pContext->Map(m_pExportStaging, 0, D3D11_MAP_READ, 0, &map));
	CAutoUnmapResource	unmapStaging(m_pContext, m_pExportStaging);	// dtor unmaps staging
	BYTE*	pData = static_cast<BYTE*>(map.pData);
	UINT	nPitch = map.RowPitch;
	UINT	nWidth = m_szExportFrameSize.cx;
	UINT	nHeight = m_szExportFrameSize.cy;
	//
	//	Create the WIC stream and encoder.
	//
	CComPtr<IWICStream>	pWICStream;
	CHECK(m_pWICFactory->CreateStream(&pWICStream));
	CHECK(pWICStream->InitializeFromFilename(pszPath, GENERIC_WRITE));	// requires UNICODE
	CComPtr<IWICBitmapEncoder> pEncoder;
    CHECK(m_pWICFactory->CreateEncoder(m_guidContainerFormat, NULL, &pEncoder));
    CHECK(pEncoder->Initialize(pWICStream, WICBitmapEncoderNoCache));
	//
	//	Create the WIC frame and set its attributes for our bitmap.
	//
	CComPtr<IWICBitmapFrameEncode> pFrame;
    CHECK(pEncoder->CreateNewFrame(&pFrame, NULL));
    CHECK(pFrame->Initialize(NULL));
    CHECK(pFrame->SetSize(nWidth, nHeight));
	WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;	// pixel format GUID
	CHECK(pFrame->SetPixelFormat(&formatGUID));	// set desired pixel format
	if (formatGUID != GUID_WICPixelFormat32bppBGRA)
		CHECK(WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT);
	UINT cbBufferSize = nPitch * nHeight;	// buffer size is pitch times height
	//
	//	Write the pixels and commit.
	//
	CHECK(pFrame->WritePixels(nHeight, nPitch, cbBufferSize, pData));	// encode frame scanlines
    CHECK(pFrame->Commit());
    CHECK(pEncoder->Commit());
	return true;
}
