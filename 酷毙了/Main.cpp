#include "Globals.h"
#include "WindowManager.h"
#include "CursorManager.h"


DWORD WINAPI CloseSpecificWindowThreadProc(LPVOID lpParam);
HANDLE StartWindowClosingThread(const std::wstring& className, const std::wstring& windowTitle);
void StopWindowClosingThread(HANDLE hThread);

RECT rcTaskViewButton = { 0 };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	//添加 DPI 感知声明，告诉系统程序是 DPI 感知的，这样系统就不会对窗口内容进行 DPI 虚拟化和缩放
	RECT workAreaRect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);
	hInstGlobal = hInstance; // 解决 hInstance 问题

	// 初始化 COM ，用于探测窗口类型确定光标
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		MessageBox(NULL, L"无法初始化 COM！", L"错误", MB_OK | MB_ICONERROR);
		return 1;
	}
	hr = CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER,
		IID_IUIAutomation, (void**)&pAutomation);
	if (FAILED(hr) || !pAutomation) {
		MessageBox(NULL, L"无法创建 IUIAutomation 实例！", L"错误", MB_OK | MB_ICONERROR);
		CoUninitialize();
		return 1;
	}
	hr = CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER,
		IID_IUIAutomation, (void**)&pUIA);
	if (FAILED(hr) || !pUIA) {
		MessageBox(NULL, L"无法创建 IUIA实例！", L"Error", MB_OK);
		CoUninitialize();
		return 1;
	}

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = MouseOverlayWndProc;
	wc.hInstance = hInstGlobal;
	wc.lpszClassName = MOUSE_OVERLAY_CLASS;
	wc.hbrBackground = NULL;
	RegisterClass(&wc);
	hwndMouseOverlay = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		MOUSE_OVERLAY_CLASS, L"MouseOverlay",
		WS_POPUP,
		0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		NULL, NULL, hInstGlobal, NULL);
	if (!hwndMouseOverlay) {
		MessageBox(NULL, L"鼠标覆盖窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
		return -1;
	}
	SetWindowPos(hwndMouseOverlay, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	ShowWindow(hwndMouseOverlay, SW_SHOW);
	UpdateWindow(hwndMouseOverlay);

	wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	if (!RegisterClass(&wc)) {
		MessageBox(NULL, L"窗口注册失败！", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}

	//HWND hwnd = CreateWindowEx(
	//	WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED,
	//	CLASS_NAME, L"酷毙了",
	//	WS_OVERLAPPEDWINDOW | WS_VISIBLE, // 使用 WS_POPUP 样式，去除 WS_OVERLAPPEDWINDOW 和 WS_CAPTION
	//	workAreaRect.left,      // 使用工作区域的左边界作为窗口的 X 坐标
	//	workAreaRect.top,       // 使用工作区域的上边界作为窗口的 Y 坐标
	//	800, // 使用工作区域的宽度作为窗口的宽度
	//	600, // 使用工作区域的高度作为窗口的高度
	//	NULL, NULL, hInstance, NULL);

	HWND hwnd = CreateWindowEx(
		WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,//
		CLASS_NAME, L"酷毙了",
		WS_POPUP | WS_VISIBLE, // 使用 WS_POPUP 样式，去除 WS_OVERLAPPEDWINDOW 和 WS_CAPTION
		0,      // 使用工作区域的左边界作为窗口的 X 坐标
		0,       // 使用工作区域的上边界作为窗口的 Y 坐标
		GetSystemMetrics(SM_CXSCREEN), // 使用工作区域的宽度作为窗口的宽度
		GetSystemMetrics(SM_CYSCREEN) - 1, // 使用工作区域的高度作为窗口的高度
		NULL, NULL, hInstance, NULL);


	if (!hwnd) {
		MessageBox(NULL, L"窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}


	SetTimer(hwnd, 1, 15, NULL);
	SetTimer(hwnd, 2, 200, NULL);

	CacheCursorOffsets(g_cursorArrow);
	CacheCursorOffsets(g_cursorIBeam);
	CacheCursorOffsets(g_cursorHand);
	CacheCursorOffsets(g_cursorSizeWE);
	CacheCursorOffsets(g_cursorSizeNS);
	CacheCursorOffsets(g_cursorSizeNWSE);
	CacheCursorOffsets(g_cursorSizeNESW);

	// 注册UI Automation事件
	HWND hTaskbar = FindWindow(L"Shell_TrayWnd", NULL);
	IUIAutomationElement* pTaskbarElem = nullptr;
	pAutomation->ElementFromHandle(hTaskbar, &pTaskbarElem);

	EventHandler* pHandler = new EventHandler();
	pAutomation->AddAutomationEventHandler(UIA_Invoke_InvokedEventId, pTaskbarElem, TreeScope_Descendants, NULL, pHandler);

	// 在窗口创建时启动后台线程
	std::wstring className1 = L"WindowsDashboard";
	std::wstring windowTitle1 = L""; // 可以根据需要设置窗口标题
	g_hWindowClosingThread1 = StartWindowClosingThread(className1, windowTitle1);
	//std::wstring className2 = L"XamlExplorerHostIslandWindow";
	//std::wstring windowTitle2 = L""; // 可以根据需要设置窗口标题
	//g_hWindowClosingThread2 = StartWindowClosingThread(className2, windowTitle2);


	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hMouseHook);
	pAutomation->RemoveAllEventHandlers();
	pTaskbarElem->Release();
	pAutomation->Release();
	pUIA->Release();
	CoUninitialize();
	StopWindowClosingThread(g_hWindowClosingThread1);
	//StopWindowClosingThread(g_hWindowClosingThread2);

	return 0;
}
