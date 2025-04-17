#include <windows.h>
#include <magnification.h>
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "Magnification.lib")
#include <chrono> 
using namespace std::chrono;
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
const wchar_t CLASS_NAME[] = L"MagnifierWindow";
HWND hwndMagnifier;
HHOOK hKeyboardHook;
bool exiting = false;
static int cutPosition = 0; 
BOOL CALLBACK UpdateMagWindow(HWND hwnd, void* srcdata, MAGIMAGEHEADER srcheader, void* destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty)
{
	if (exiting) return FALSE;
	BYTE* srcPixels = (BYTE*)srcdata;
	BYTE* destPixels = (BYTE*)destdata;
	int width = destheader.width;
	int height = destheader.height;
	int stride = destheader.stride;  
	static auto startTime = steady_clock::now();
	auto elapsed = duration_cast<seconds>(steady_clock::now() - startTime).count();
	int speed = 5;
	cutPosition = (cutPosition + speed) % width;
	cutPosition = (cutPosition + speed) % width; 
	BYTE* tempBuffer = new BYTE[height * stride];
	if (!tempBuffer) {
		return FALSE; 
	}
	for (int y = 0; y < height; y++) {
		BYTE* srcRow = destPixels + y * stride;   
		BYTE* destRow = tempBuffer + y * stride;
		for (int x = 0; x < width; x++) {
			int index = x * 4; 
			if (x < cutPosition) {
				int srcX = x + (width - cutPosition); 
				if (srcX >= width) srcX -= width; 
				if (srcX < 0) srcX += width;
				BYTE* srcPixel = srcRow + srcX * 4;
				destRow[index] = srcPixel[0]; 
				destRow[index + 1] = srcPixel[1]; 
				destRow[index + 2] = srcPixel[2]; 
				destRow[index + 3] = srcPixel[3]; 
			}
			else {
				int srcX = x - cutPosition; 
				if (srcX >= width) srcX -= width;
				if (srcX < 0) srcX += width;
				BYTE* srcPixel = srcRow + srcX * 4;
				destRow[index] = srcPixel[0]; 
				destRow[index + 1] = srcPixel[1]; 
				destRow[index + 2] = srcPixel[2]; 
				destRow[index + 3] = srcPixel[3]; 
			}
		}
	}
	memcpy(destPixels, tempBuffer, height * stride);
	delete[] tempBuffer; 
	return TRUE;
}
steady_clock::time_point lastEscPress;
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
		if (wParam == WM_KEYDOWN && pKey->vkCode == VK_ESCAPE) {
			auto now = steady_clock::now();
			auto duration = duration_cast<milliseconds>(now - lastEscPress);
			if (duration.count() < 1000) { 
				HWND hwndTarget = FindWindow(CLASS_NAME, NULL);
				SetForegroundWindow(hwndTarget);
				exiting = true;
				KillTimer(hwndTarget, 1);  
				int response = MessageBox(hwndTarget, L"确定要退出程序吗？", L"退出确认", MB_YESNO | MB_ICONQUESTION);
				if (response == IDYES)
					DestroyWindow(hwndTarget);
				else 
				{
					exiting = false;
					SetTimer(hwndTarget, 1, 75, NULL);
				}
			}
			lastEscPress = now;
		}
	} 
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			if (pKeyboard->vkCode == VK_LWIN || pKeyboard->vkCode == VK_RWIN ||
				(pKeyboard->vkCode == VK_ESCAPE && (GetAsyncKeyState(VK_CONTROL) & 0x8000))) {
				return 1; 
			}
		}
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}
HWND hTaskbar = FindWindow(L"Shell_TrayWnd", NULL);
static HCURSOR g_hCursor = NULL; 
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static steady_clock::time_point lastEscPress = steady_clock::now();
	SetTimer(hwnd, 1, 75, NULL);
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int titleBarHeight = GetSystemMetrics(SM_CYCAPTION);  
	static HDC hMemDC = NULL;  
	static HBITMAP hBitmap = NULL; 
	static HBITMAP hOldBitmap = NULL; 
	switch (msg) {
	case WM_CREATE: {
		if (hTaskbar) {
		}
		if (!MagInitialize()) {
			MessageBox(hwnd, L"无法初始化 Magnification API！", L"错误", MB_OK | MB_ICONERROR);
			return -1;
		}
		hwndMagnifier = CreateWindow(WC_MAGNIFIER, L"Magnifier",
			WS_CHILD | WS_VISIBLE,
			0, 0, 100, 100,
			hwnd, NULL, NULL, NULL);
		if (!hwndMagnifier) {
			MessageBox(hwnd, L"无法创建放大窗口！", L"错误", MB_OK | MB_ICONERROR);
			return -1;
		}
		MAGTRANSFORM matrix = { 0 };
		matrix.v[0][0] = 1.0f;
		matrix.v[1][1] = 1.0f;
		matrix.v[2][2] = 1.0f;
		MagSetWindowTransform(hwndMagnifier, &matrix);
		MagSetImageScalingCallback(hwndMagnifier, UpdateMagWindow);
		hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
		break;
	}
	case WM_SIZE: {
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);
		SetWindowPos(hwndMagnifier, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
		RECT rcSource = { 0, 0, width, height };
		MagSetWindowSource(hwndMagnifier, rcSource);
		break;
	}
	case WM_TIMER:
		InvalidateRect(hwnd, NULL, FALSE); 
		break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		RECT rect;
		GetClientRect(hwnd, &rect); 
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		if (!hMemDC) {
			hMemDC = CreateCompatibleDC(hdc);
		}
		if (!hBitmap || width != GetDeviceCaps(hMemDC, HORZRES) || height != GetDeviceCaps(hMemDC, VERTRES)) {
			if (hBitmap) { 
				SelectObject(hMemDC, hOldBitmap); 
				DeleteObject(hBitmap);
			}
			hBitmap = CreateCompatibleBitmap(hdc, width, height);
			hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap); 
		}
		HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH); 
		FillRect(hMemDC, &rect, hBrush);
		DeleteObject(hBrush);
		POINT pt;
		GetCursorPos(&pt);
		TextOut(hMemDC, 10, 10, L"双缓冲绘制测试", 8);
		Ellipse(hdc, pt.x - 10, pt.y - 10, pt.x + 10, pt.y + 10);
		BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			ShowWindow(hwnd, SW_SHOW);
		}
		break;
	case WM_DESTROY:
	{
		KillTimer(hwnd, 1);  
		if (hTaskbar)
		{
			ShowWindow(hTaskbar, SW_SHOW); 
		}
		MagUninitialize();
		if (hBitmap) {
			SelectObject(hMemDC, hOldBitmap); 
			DeleteObject(hBitmap);
			hBitmap = NULL; 
		}
		if (hMemDC) {
			DeleteDC(hMemDC);
			hMemDC = NULL; 
		}
		UnhookWindowsHookEx(hKeyboardHook);
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	RECT workAreaRect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	if (!RegisterClass(&wc)) {
		MessageBox(NULL, L"窗口注册失败！", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}
	HWND hwnd = CreateWindowEx(
		WS_EX_TOPMOST,
		CLASS_NAME, L"酷毙了",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
		workAreaRect.left,      
		workAreaRect.top,       
		800, 
		600, 
		NULL, NULL, hInstance, NULL);
	if (!hwnd) {
		MessageBox(NULL, L"窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}