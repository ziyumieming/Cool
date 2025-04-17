#pragma once

#include <windows.h>
#include <uiautomation.h>
#include <comdef.h>
#pragma comment(lib, "uiautomationcore.lib")
#include <thread>
#include <atomic>
#include <shellapi.h>
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
//如果应用程序没有声明自己为 DPI 感知（DPI-aware），Windows 可能会自动对窗口内容进行 DPI 虚拟化和缩放。这种情况下，虽然逻辑上你的程序捕获了全屏，但在显示时系统会按当前 DPI 设置自动放大或缩小窗口内容。
#include <magnification.h>
#pragma comment(lib, "Magnification.lib")

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseOverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK UpdateMagWindow(HWND hwnd, void* srcdata, MAGIMAGEHEADER srcheader, void* destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty);

bool GetTaskViewButtonRect(IUIAutomation* pAutomation, RECT& rcButton);
void SimulateEscapeKey();
void MonitorTaskViewButton();
extern std::atomic<bool> running;   // 线程控制变量
extern std::thread* monitorThread; // 线程指针

class EventHandler : public IUIAutomationEventHandler {
public:
	ULONG m_ref = 0;
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
		if (riid == __uuidof(IUnknown) || riid == __uuidof(IUIAutomationEventHandler)) {
			*ppv = static_cast<IUIAutomationEventHandler*>(this);
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() override { return ++m_ref; }
	STDMETHODIMP_(ULONG) Release() override { if (--m_ref == 0) delete this; return m_ref; }

	STDMETHODIMP HandleAutomationEvent(IUIAutomationElement* pSender, EVENTID eventId) override {
		if (eventId == UIA_Invoke_InvokedEventId) {
			MessageBox(NULL, L"拦截开始按钮点击", L"提示", MB_OK);
		}
		return S_OK;
	}
};
