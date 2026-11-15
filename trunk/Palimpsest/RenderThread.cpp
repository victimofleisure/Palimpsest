// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      06feb25	initial version
        01      13nov26	port to Direct3D
		02		14nov26	in OnSetFullScreen, return actual config result
		03		14nov26	in DestroyThread, compare thread IDs not handles

*/

#include "stdafx.h"
#include "RenderThread.h"

#define CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { OnError(hr, __FILE__, __LINE__, __DATE__); return false; }}

CRenderThread::CRenderThread()
{
	m_pThread = NULL;
	m_bThreadExit = false;
	m_hRenderWnd = 0;
	m_pNotifyWnd = NULL;
}

CRenderThread::~CRenderThread()
{
}

bool CRenderThread::OnThreadCreate()
{
	return true;
}

bool CRenderThread::OnDraw()
{
	return true;
}

void CRenderThread::OnRenderCommand(const CRenderCmd& cmd)
{
	UNREFERENCED_PARAMETER(cmd);
}

void CRenderThread::Log(CString sMsg)
{
	_fputts(sMsg + '\n', stdout);
}

bool CRenderThread::CreateCommandQueue(int nCmdQueueSize)
{
	if (m_pThread != NULL) {	// if thread was launched
		return false;	// too late to create command queue
	}
	m_qCmd.Create(nCmdQueueSize);
	return true;
}

bool CRenderThread::CreateThread(HWND hRenderWnd, CWnd* pNotifyWnd)
{
	ASSERT(hRenderWnd);	// validate window handle
	ASSERT(m_pThread == NULL);	// only one thread at a time
	if (!m_qCmd.GetSize()) {	// if command queue wasn't created yet
		m_qCmd.Create(COMMAND_QUEUE_SIZE);	// create command queue
	}
	m_hRenderWnd = hRenderWnd;	// store render window handle in member var
	m_pNotifyWnd = pNotifyWnd;	// store notification window pointer in member var
	// create thread in suspended state so we can safely disable auto-delete
	m_pThread = AfxBeginThread(ThreadFunc, this, THREAD_PRIORITY_ABOVE_NORMAL, 0, CREATE_SUSPENDED);
	if (m_pThread == NULL)	// if begin thread failed
		return false;
	m_pThread->m_bAutoDelete = false;	// disable auto-delete so destroy can wait on thread handle
	m_pThread->ResumeThread();	// launch render thread
	return true;
}

void CRenderThread::SignalExit()
{
	m_bThreadExit = true;	// signal thread to exit
}

void CRenderThread::DestroyThread()
{
	// This is a fallback; the preferred cleanup method is to call RequestExit,
	// handle UWM_RENDER_THREAD_EXIT, and destroy your window in that handler.
	// DestroyThread can potentially deadlock due to DXGI's use of SendMessage.
	if (m_pThread != NULL) {	// if thread was launched
		m_bThreadExit = true;	// signal thread to exit
		if (GetCurrentThreadId() != m_pThread->m_nThreadID)	{	// if caller isn't render thread
			WaitForSingleObject(m_pThread->m_hThread, INFINITE);	// wait for thread to exit
		}
		delete m_pThread;	// delete thread instance
		m_pThread = NULL;	// mark instance deleted
	}
	m_pNotifyWnd = NULL;
}

UINT CRenderThread::ThreadFunc()
{
	UINT	nResult = 1;
	if (Create(m_hRenderWnd)) {	// create device context
		if (OnThreadCreate()) {	// create derived class objects
			while (!m_bThreadExit) {	// while thread should run
				CRenderCmd	cmd;
				while (m_qCmd.Pop(cmd)) {	// while commands remain
					ProcessCommand(cmd);	// process command
				}
				RenderFrame();	// render frame
			}
			nResult = 0;	// success
		}
	}
	Destroy();	// destroy device context
	PostMsgToMainWnd(UWM_RENDER_THREAD_EXIT);	// notify of exit
	return nResult;
}

UINT CRenderThread::ThreadFunc(LPVOID pParam)
{
	CRenderThread	*pThis = static_cast<CRenderThread *>(pParam);
	return pThis->ThreadFunc();
}

bool CRenderThread::RenderFrame()
{
	if (!m_pSwapChain)	// sanity check
		return false;
	// begin drawing
	OnDraw();	// do derived drawing
	// drawing is done; present the frame
	static const DXGI_PRESENT_PARAMETERS	parmsPresent = {0};
	HRESULT	hr = m_pSwapChain->Present1(1, 0, &parmsPresent);
	if (hr != S_OK) {	// if unsuccessful
		bool	bIsRecovered = false;
		switch (hr) {
		case DXGI_STATUS_OCCLUDED:
		case DXGI_ERROR_DEVICE_REMOVED:
		case DXGI_ERROR_DEVICE_RESET:
			Log(_T("Device lost! Attempting recovery..."));
			bIsRecovered = HandleDeviceLost();
			break;
		default:
			CString	sMsg;
			sMsg.Format(_T("Unexpected error from Present1: 0x%x"), hr);
			Log(sMsg);
		}
		return bIsRecovered;
	}
	return true;
}

bool CRenderThread::HandleDeviceLost()
{
	OnSetFullScreen(false);
	PostMsgToMainWnd(UWM_FULL_SCREEN_CHANGED, IsFullScreen(), 0);
	HWND	hWnd;
	CHECK(m_pSwapChain->GetHwnd(&hWnd));
	Destroy();
	if (!Create(hWnd)) {
		Log(_T("Device recovery failed!"));
		return false;
	}
	Log(_T("Device successfully recovered."));
	return true;
}

void CRenderThread::ProcessCommand(const CRenderCmd& cmd)
{
	switch (cmd.m_nCmd) {
	case RC_RESIZE:
		CD3DDevCtx::Resize();
		break;
	case RC_SET_FULLSCREEN:
		OnSetFullScreen(cmd.m_nParam != 0);
		break;
	case RC_EXIT:
		m_bThreadExit = true;	// quit render loop
		break;
	default:
		OnRenderCommand(cmd);	// handle derived commands
	}
}

bool CRenderThread::LogDeviceInfo()
{
	DXGI_ADAPTER_DESC	descAdapter;
	if (!GetAdapterDesc(descAdapter))
		return false;
	DXGI_OUTPUT_DESC	desc;
	if (!GetOutputDesc(desc))
		return false;
	DISPLAY_DEVICE	dd;
	if (!GetDisplayDeviceInfo(desc.DeviceName, dd))
		return false;
	DEVMODE	mode;
	if (!GetDeviceMode(desc, mode))
		return false;
	CString	sMsg;
	sMsg.Format(_T("Adapter: %s\nMonitor: %s\nMode: %d x %d, %d Hz, %d bits"), 
		descAdapter.Description, dd.DeviceString, mode.dmPelsWidth, 
		mode.dmPelsHeight, mode.dmDisplayFrequency, mode.dmBitsPerPel);
	Log(sMsg);
	return true;
}

bool CRenderThread::OnSetFullScreen(bool bEnable)
{
	bool	bResult = true;
	bool	bPrevEnable = IsFullScreen();
	if (bEnable != bPrevEnable) {	// if not in requested state
		Log(bEnable ? _T("fullscreen") : _T("windowed"));
		bResult = CD3DDevCtx::SetFullScreen(bEnable);	// configure swap chain
		if (bResult) {	// if mode change succeeded
			if (bEnable) {	// if full screen mode
				LogDeviceInfo();
			}
		} else {	// mode change failed
			bEnable = bPrevEnable;
		}
	}	
	PostMsgToMainWnd(UWM_FULL_SCREEN_CHANGED, bEnable, bResult);
	return bResult;
}

BOOL CRenderThread::PostMsgToMainWnd(UINT nMsg, WPARAM wParam, LPARAM lParam) const
{
	if (m_pNotifyWnd != NULL) {	// if notification window exists
		return m_pNotifyWnd->PostMessage(nMsg, wParam, lParam);
	}
	return false;
}
