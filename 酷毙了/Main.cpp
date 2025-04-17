#include "Globals.h"
#include "WindowManager.h"
#include "CursorManager.h"


DWORD WINAPI CloseSpecificWindowThreadProc(LPVOID lpParam);
HANDLE StartWindowClosingThread(const std::wstring& className, const std::wstring& windowTitle);
void StopWindowClosingThread(HANDLE hThread);

RECT rcTaskViewButton = { 0 };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	//��� DPI ��֪����������ϵͳ������ DPI ��֪�ģ�����ϵͳ�Ͳ���Դ������ݽ��� DPI ���⻯������
	RECT workAreaRect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);
	hInstGlobal = hInstance; // ��� hInstance ����

	// ��ʼ�� COM ������̽�ⴰ������ȷ�����
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		MessageBox(NULL, L"�޷���ʼ�� COM��", L"����", MB_OK | MB_ICONERROR);
		return 1;
	}
	hr = CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER,
		IID_IUIAutomation, (void**)&pAutomation);
	if (FAILED(hr) || !pAutomation) {
		MessageBox(NULL, L"�޷����� IUIAutomation ʵ����", L"����", MB_OK | MB_ICONERROR);
		CoUninitialize();
		return 1;
	}
	hr = CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER,
		IID_IUIAutomation, (void**)&pUIA);
	if (FAILED(hr) || !pUIA) {
		MessageBox(NULL, L"�޷����� IUIAʵ����", L"Error", MB_OK);
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
		MessageBox(NULL, L"��긲�Ǵ��ڴ���ʧ�ܣ�", L"����", MB_OK | MB_ICONERROR);
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
		MessageBox(NULL, L"����ע��ʧ�ܣ�", L"����", MB_OK | MB_ICONERROR);
		return 0;
	}

	//HWND hwnd = CreateWindowEx(
	//	WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED,
	//	CLASS_NAME, L"�����",
	//	WS_OVERLAPPEDWINDOW | WS_VISIBLE, // ʹ�� WS_POPUP ��ʽ��ȥ�� WS_OVERLAPPEDWINDOW �� WS_CAPTION
	//	workAreaRect.left,      // ʹ�ù����������߽���Ϊ���ڵ� X ����
	//	workAreaRect.top,       // ʹ�ù���������ϱ߽���Ϊ���ڵ� Y ����
	//	800, // ʹ�ù�������Ŀ����Ϊ���ڵĿ��
	//	600, // ʹ�ù�������ĸ߶���Ϊ���ڵĸ߶�
	//	NULL, NULL, hInstance, NULL);

	HWND hwnd = CreateWindowEx(
		WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,//
		CLASS_NAME, L"�����",
		WS_POPUP | WS_VISIBLE, // ʹ�� WS_POPUP ��ʽ��ȥ�� WS_OVERLAPPEDWINDOW �� WS_CAPTION
		0,      // ʹ�ù����������߽���Ϊ���ڵ� X ����
		0,       // ʹ�ù���������ϱ߽���Ϊ���ڵ� Y ����
		GetSystemMetrics(SM_CXSCREEN), // ʹ�ù�������Ŀ����Ϊ���ڵĿ��
		GetSystemMetrics(SM_CYSCREEN) - 1, // ʹ�ù�������ĸ߶���Ϊ���ڵĸ߶�
		NULL, NULL, hInstance, NULL);


	if (!hwnd) {
		MessageBox(NULL, L"���ڴ���ʧ�ܣ�", L"����", MB_OK | MB_ICONERROR);
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

	// ע��UI Automation�¼�
	HWND hTaskbar = FindWindow(L"Shell_TrayWnd", NULL);
	IUIAutomationElement* pTaskbarElem = nullptr;
	pAutomation->ElementFromHandle(hTaskbar, &pTaskbarElem);

	EventHandler* pHandler = new EventHandler();
	pAutomation->AddAutomationEventHandler(UIA_Invoke_InvokedEventId, pTaskbarElem, TreeScope_Descendants, NULL, pHandler);

	// �ڴ��ڴ���ʱ������̨�߳�
	std::wstring className1 = L"WindowsDashboard";
	std::wstring windowTitle1 = L""; // ���Ը�����Ҫ���ô��ڱ���
	g_hWindowClosingThread1 = StartWindowClosingThread(className1, windowTitle1);
	//std::wstring className2 = L"XamlExplorerHostIslandWindow";
	//std::wstring windowTitle2 = L""; // ���Ը�����Ҫ���ô��ڱ���
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
