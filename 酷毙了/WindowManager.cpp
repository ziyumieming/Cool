
#include "Globals.h"
#include "WindowManager.h"

const wchar_t* TARGET_BUTTONS[];// ���尴ť��ʶ���������ʵ��ϵͳ�汾������

const wchar_t CLASS_NAME[] = L"MagnifierWindow";
const wchar_t MOUSE_OVERLAY_CLASS[] = L"MouseOverlayClass";

HINSTANCE hInstGlobal; // ��� hInstance ����
HWND hwndMouseOverlay;
HWND hwndMagnifier;
HWND hTaskbar = FindWindow(L"Shell_TrayWnd", NULL);

HHOOK hKeyboardHook;
HHOOK hMouseHook;
IUIAutomation* pAutomation = nullptr;
IUIAutomation* pUIA = nullptr;


int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

const wchar_t* TARGET_BUTTONS[]= {
	L"StartButton",       // ��ʼ��ť
	L"TaskViewButton",    // ������ͼ��ť��Win10/11��
	L"SearchBox",         // ������ť��Win10��
	L"SearchButton",      // ������ť��Win11��
	L"WidgetsButton",     // С�����ť��Win11��
	L"ShowDesktopButton", // ��ʾ���水ť�����½ǣ�
	L"SystemTrayIcon",	 // ϵͳ����ͼ�꣨�����������磩
};

bool exiting = false;
steady_clock::time_point lastEscPress;
static int cutPosition = 0; // ��̬��������¼�и�λ��;

BOOL CALLBACK UpdateMagWindow(HWND hwnd, void* srcdata, MAGIMAGEHEADER srcheader, void* destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty)
{
	if (exiting) return FALSE;
	// ��ȡ��������
	BYTE* srcPixels = (BYTE*)srcdata;
	BYTE* destPixels = (BYTE*)destdata;

	int width = destheader.width;
	int height = destheader.height;
	int stride = destheader.stride;  // ÿ�����ص��ֽ���

	static auto startTime = steady_clock::now();
	auto elapsed = duration_cast<seconds>(steady_clock::now() - startTime).count() / 10;
	int speed = min(1 + elapsed, 50); // ÿ10����� 1 ���أ���� 50
	cutPosition = (cutPosition + speed) % width;


	// 1. �����и�λ�ã�ʹ�������ƶ�
	cutPosition = (cutPosition + speed) % width; // ÿ���ƶ� 5 �����أ�������wrap-around

	// 2. ������ʱ�����������ڴ��ƴ�Ӻ��ͼ��
	BYTE* tempBuffer = new BYTE[height * stride];
	if (!tempBuffer) {
		return FALSE; // �ڴ����ʧ�ܣ����� FALSE ��ֹ����
	}

	// 3.  �и��ƴ��ͼ��
	for (int y = 0; y < height; y++) {
		BYTE* srcRow = destPixels + y * stride;   // ע�⣬��������ֱ�Ӳ��� destPixels����Ϊ��������ԭʼͼ������
		BYTE* destRow = tempBuffer + y * stride;
		for (int x = 0; x < width; x++) {
			int index = x * 4; // BGRA ��ʽ

			if (x < cutPosition) {
				// ��ಿ�֣� ��Դͼ����Ҳಿ�ָ���
				int srcX = x + (width - cutPosition); // ����Դͼ���ж�Ӧ�Ҳಿ�ֵ� X ����
				if (srcX >= width) srcX -= width; // �����ϲ���Ҫ����Ϊ�˱������
				if (srcX < 0) srcX += width;

				BYTE* srcPixel = srcRow + srcX * 4;
				destRow[index] = srcPixel[0]; // B
				destRow[index + 1] = srcPixel[1]; // G
				destRow[index + 2] = srcPixel[2]; // R
				destRow[index + 3] = srcPixel[3]; // A (Alpha)
			}
			else {
				// �Ҳಿ�֣� ��Դͼ�����ಿ�ָ���
				int srcX = x - cutPosition; // ����Դͼ���ж�Ӧ��ಿ�ֵ� X ����
				if (srcX >= width) srcX -= width;
				if (srcX < 0) srcX += width;

				BYTE* srcPixel = srcRow + srcX * 4;
				destRow[index] = srcPixel[0]; // B
				destRow[index + 1] = srcPixel[1]; // G
				destRow[index + 2] = srcPixel[2]; // R
				destRow[index + 3] = srcPixel[3]; // A (Alpha)
			}
		}
	}
	//�ڻ������������ദ��
	//for (int y = 0; y < height; y++) {
	//	BYTE* destRow = tempBuffer + y * stride;
	//	for (int x = 0; x < width; x++) {
	//		int index = x * 4; // BGRA ��ʽ
	//		destRow[index] = 255 - destRow[index]; // B
	//		destRow[index + 1] = 255 - destRow[index + 1]; // G
	//		destRow[index + 2] = 255 - destRow[index + 2]; // R
	//		destRow[index + 3] = destRow[index + 3]; // A (Alpha)
	//	}
	//}
	// 5. ����ʱ�����������ݸ��ƻ� destPixels�����ͼ�����
	memcpy(destPixels, tempBuffer, height * stride);

	delete[] tempBuffer; // �ͷ���ʱ������

	//POINT pt;
	//if (GetCursorPos(&pt)) {
	//	// ���ݱ���ƽ�ƣ�������� X ���꣺���� cutPosition ��ȡģ��Ļ���
	//	int adjustedX = (pt.x + cutPosition) % width;
	//	int adjustedY = pt.y; // ��ֱ������ƽ��

	//	// �������ǵ� Magnifier Դ��������Ļһһ��Ӧ��
	//	// �����Ҫ����Ļ����ת��ΪԴ�������꣬���� ScreenToClient(hwndMagnifier, &pt)
	//	// ������� (0,0) ��Ӧ��Ļ����

	//	// ����һ����ɫԲȦ��ʾ�����
	//	int radius = 10; // ���뾶
	//	for (int dy = -radius; dy <= radius; dy++) {
	//		int yPos = adjustedY + dy;
	//		if (yPos < 0 || yPos >= height) continue;
	//		for (int dx = -radius; dx <= radius; dx++) {
	//			if (dx * dx + dy * dy <= radius * radius) {
	//				int xPos = adjustedX + dx;
	//				if (xPos < 0 || xPos >= width) continue;
	//				int offset = yPos * stride + xPos * 4;
	//				// ���ø�����Ϊ����ɫ��Alpha=255
	//				destPixels[offset] = 0;         // B
	//				destPixels[offset + 1] = 0;       // G
	//				destPixels[offset + 2] = 255;     // R
	//				destPixels[offset + 3] = 255;     // A
	//			}
	//		}
	//	}
	//}

	return TRUE;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
		if (wParam == WM_KEYDOWN && pKey->vkCode == VK_END && exiting == false) {
			auto now = steady_clock::now();
			auto duration = duration_cast<milliseconds>(now - lastEscPress);
			if (duration.count() < 1000) { // 1 ����˫�� End

				HWND hwndTarget = FindWindow(CLASS_NAME, NULL);
				SetForegroundWindow(hwndTarget);
				exiting = true;
				KillTimer(hwndTarget, 1);
				KillTimer(hwndTarget, 2);
				ShowWindow(hwndMagnifier, SW_HIDE);
				ShowWindow(hwndMouseOverlay, SW_HIDE);
				EnableWindow(hTaskbar, TRUE);  // ��������������
				ShowWindow(hTaskbar, SW_SHOW); // ȷ���������ɼ�
				SystemParametersInfo(SPI_SETCURSORS, 0, NULL, SPIF_SENDCHANGE);
				int response = MessageBox(hwndTarget, L"ȷ��Ҫ�˳�������", L"�˳�ȷ��", MB_YESNO | MB_ICONQUESTION);
				// MessageBox������ʾһ����Ϣ�򣬲����ֱ��ǣ������ھ������Ϣ���ݡ���Ϣ���⡢��Ϣ����ʽ��MB_YESNO ��ʾ��Ϣ�����С��ǡ��͡���������ť��MB_ICONQUESTION ��ʾ��Ϣ������һ���ʺ�ͼ��
				if (response == IDYES)// IDYES �� MessageBox �����ķ���ֵ����ʾ�û�����ˡ��ǡ���ť��IDNO ��ʾ�û�����ˡ��񡱰�ť
					DestroyWindow(hwndTarget);
				else // �û�ȡ���˳���������ʱ��
				{
					exiting = false;
					SetTimer(hwndTarget, 1, 15, NULL);
					SetTimer(hwndTarget, 2, 200, NULL);
					ShowWindow(hwndMagnifier, SW_SHOW);
					ShowWindow(hwndMouseOverlay, SW_SHOW);
					EnableWindow(hTaskbar, FALSE);  // ��ֹ����������
					HideSystemCursor();
				}
			}
			lastEscPress = now;
		}
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			if (pKey->vkCode == VK_LWIN || pKey->vkCode == VK_RWIN
				//||(pKey->vkCode == VK_ESCAPE && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
				) {
				return 1; // ���ذ���
			}
		}
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
		POINT pt = pMouseStruct->pt;
		if ((wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP))
		{
			// ��ȡ������ڴ��� 

			HWND hWndUnderMouse = WindowFromPoint(pt);

			wchar_t className[256];
			GetClassName(hWndUnderMouse, className, 256);
			//OutputDebugString(className);

			if (hWndUnderMouse)
			{
				wchar_t className[256];
				GetClassName(hWndUnderMouse, className, 256);
				//OutputDebugString(className);
				if (wcscmp(className, L"SysListView32") == 0 || wcscmp(className, L"DirectUIHWND") == 0 || wcscmp(className, L"Shell_TrayWnd") == 0 || wcscmp(className, L"TrayNotifyWnd") == 0 || wcscmp(className, L"MSTaskSwWClass") == 0)  // ���� ��Դ������ ������ ֪ͨ��
					return 1;  // ��ֹ�Ҽ��¼�

			}
		}
		if (wParam == WM_MOUSEMOVE) {
			int screenWidth = GetSystemMetrics(SM_CXSCREEN);
			int screenHeight = GetSystemMetrics(SM_CYSCREEN);
			if (pt.x <= 1) {
				SetCursorPos(screenWidth - 1, pt.y);
				return 1; // ��ֹ��Ϣ����
			}
			else if (pt.x >= screenWidth - 1) {
				SetCursorPos(0, pt.y);
				return 1; // ��ֹ��Ϣ����
			}
		}
		if ((wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP))
		{
			// ��ȡ������ڴ��� 
			HWND hWndUnderMouse = WindowFromPoint(pt);

			wchar_t className[256];
			GetClassName(hWndUnderMouse, className, 256);
			//OutputDebugString(className);

			if (hWndUnderMouse)
			{
				wchar_t className[256];
				GetClassName(hWndUnderMouse, className, 256);
				//OutputDebugString(className);
				if (wcscmp(className, L"Windows.UI.Core.CoreWindow") == 0)  // 
					return 1;  // ��ֹ�Ҽ��¼�

			}
		}
		if (wParam == WM_LBUTTONDOWN) {
			IUIAutomationElement* pElem = nullptr;
			pAutomation->ElementFromPoint(pt, &pElem);
			if (pElem) {
				// ��ȡ��ť�� AutomationId �� Name
				BSTR automationId, name;
				pElem->get_CurrentAutomationId(&automationId);
				pElem->get_CurrentName(&name);

				// ����Ƿ�ƥ��Ŀ�갴ť
				bool isTarget = false;
				for (const wchar_t* target : TARGET_BUTTONS) {
					if (wcscmp(automationId, target) == 0 || wcscmp(name, target) == 0) {
						isTarget = true;
						break;
					}
				}

				if (isTarget) {
					SysFreeString(automationId);
					SysFreeString(name);
					pElem->Release();
					return 1; // ���ص��
				}

				SysFreeString(automationId);
				SysFreeString(name);
				pElem->Release();
			}
		}
	}

	return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseOverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static int dpiScaleX = 1, dpiScaleY = 1;  // �洢 DPI ���ű���

	switch (msg) {
	case WM_CREATE:
	{
		// ���ô��ڱ��⣨�����������ʾ���ƣ�
		SetWindowText(hwnd, L"�˵���");

		HICON hIcon = NULL;
		// ���ء��˵��ԡ�ͼ�꣨�� shell32.dll �У����� 4��
		ExtractIconEx(L"C:\\Windows\\System32\\shell32.dll", 4, &hIcon, NULL, 1);//����ֵ��ͼ�������
		if (hIcon) {
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}

		GetTaskViewButtonRect(pUIA, rcTaskViewButton);
		running = true;
		monitorThread = new std::thread(MonitorTaskViewButton);

		break;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// �������� DC ��λͼ
		HDC hdcMem = CreateCompatibleDC(hdc);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdc, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		SelectObject(hdcMem, hbmMem);

		// ���������ڱ���͸��
		HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
		FillRect(hdcMem, &ps.rcPaint, hBrush);
		DeleteObject(hBrush);

		// ��ȡ�����Ϣ
		CURSORINFO ci = { sizeof(CURSORINFO) };
		if (GetCursorInfo(&ci) && ci.flags == CURSOR_SHOWING) {
			HICON hCursor = ci.hCursor ? ci.hCursor : LoadCursor(NULL, IDC_ARROW);
			// ��ȡ������ʵ��С
			ICONINFO iconInfo;
			BITMAP bmpCursor;
			ICONMETRICS iconMetrics;
			iconMetrics.cbSize = sizeof(ICONMETRICS);
			int cursorWidth = 32;  // Ĭ�ϴ�С
			int cursorHeight = 32;
			if (GetIconInfo(hCursor, &iconInfo)) {
				cursorWidth = GetSystemMetrics(SM_CXCURSOR);
				cursorHeight = GetSystemMetrics(SM_CYCURSOR);
			}
			// ��ȡ���λ��
			POINT pt;
			GetCursorPos(&pt);

			CursorOffset offset = { 0, 0 };

			// ��ӳ����в�ѯ��ǰ�����ȵ�
			auto it = g_cursorOffsetMap.find(g_currentCursor);
			if (it != g_cursorOffsetMap.end()) {
				offset = it->second;
			}
			int adjustedX = pt.x;
			int adjustedY = pt.y;
			// �����ǰ��������Ź�꣬���������λ��
			if (g_currentCursor != g_cursorArrow)
			{
				adjustedX = pt.x - offset.offsetX;
				adjustedY = pt.y - offset.offsetY;
			}

			// �������
			DrawIconEx(hdcMem, adjustedX, adjustedY, g_currentCursor, cursorWidth, cursorHeight, 0, NULL, DI_NORMAL);
			//wchar_t debugMsg[256];
			//wsprintf(debugMsg, L"MouseOverlayWndProc triggered on hwnd = %p\n", hwnd);
			//OutputDebugString(debugMsg);
		}

		BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
		POINT ptPos = { 0, 0 };
		SIZE sizeWnd = { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
		POINT ptSrc = { 0, 0 };
		UpdateLayeredWindow(hwndMouseOverlay, hdc, &ptPos, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

		DeleteObject(hbmMem);
		DeleteDC(hdcMem);

		EndPaint(hwnd, &ps);
		break;
	}

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static steady_clock::time_point lastEscPress = steady_clock::now();
	// ��һ�ΰ��� Esc ����ʱ���

	switch (msg) {
	case WM_CREATE: {
		if (hTaskbar) {
			//EnableWindow(hTaskbar, FALSE);  // ��ֹ����������
			//ShowWindow(hTaskbar, SW_HIDE); // ����������
		}
		// ���ô��ڱ��⣨�����������ʾ���ƣ�
		SetWindowText(hwnd, L"���ļ���");

		HICON hIcon = ExtractIcon(GetModuleHandle(NULL), L"C:\\Windows\\explorer.exe", 0);
		if (hIcon) {
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}

		if (!MagInitialize()) {
			MessageBox(hwnd, L"�޷���ʼ�� Magnification API��", L"����", MB_OK | MB_ICONERROR);
			return -1;
		}
		hwndMagnifier = CreateWindow(WC_MAGNIFIER, L"Magnifier", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hwnd, NULL, NULL, NULL);
		if (!hwndMagnifier) {
			MessageBox(hwnd, L"�޷������Ŵ󴰿ڣ�", L"����", MB_OK | MB_ICONERROR);
			return -1;
		}
		MAGTRANSFORM matrix = { 0 };
		matrix.v[0][0] = 1.0f;
		matrix.v[1][1] = 1.0f;
		matrix.v[2][2] = 1.0f;
		MagSetWindowTransform(hwndMagnifier, &matrix);
		MagSetImageScalingCallback(hwndMagnifier, UpdateMagWindow);
		hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
		hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
		SetWindowPos(hwndMagnifier, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		//ShowWindow(hwndMagnifier, SW_HIDE);
		//ShowWindow(hwndMouseOverlay, SW_HIDE);
		HideSystemCursor();
		HWND hPreviewWnd = FindWindow(L"TaskListThumbnailWnd", NULL); // ������Ԥ������
		if (hPreviewWnd)
			PostMessage(hPreviewWnd, WM_CLOSE, 0, 0);

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
	{
		if (wParam == 1)
		{
			InvalidateRect(hwndMouseOverlay, NULL, FALSE); // ȷ����괰��ˢ��
			InvalidateRect(hwnd, NULL, FALSE); // InvalidateRect ������ (hwnd) �����ǷŴ󾵴��� (hwndMagnifier)
		}

		if (wParam == 2)
		{
			UpdateVirtualCursorType();
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		//wchar_t debugMsg[256];
		//wsprintf(debugMsg, L"wparam: %d\n", wParam);
		//OutputDebugString(debugMsg);
		break;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		//�˴�����滭�ᱻ����
		EndPaint(hwnd, &ps);
		break;
	}

	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE) {
			ShowWindow(hwnd, SW_SHOW);
		}
		break;
	}

	//case WM_KILLFOCUS:
	//{
	//	if (exiting == false)
	//	{
	//		SetForegroundWindow(hwnd);  // ��ͼ�ѽ���������
	//		SetFocus(hwnd);              / �������뽹��
	//	}
	//	break;
	//}

	case WM_DESTROY:
	{
		KillTimer(hwnd, 1);
		KillTimer(hwnd, 2);
		KillTimer(hwnd, 3);
		running = false;
		if (monitorThread) {
			monitorThread->detach(); // ���߳��Լ�����
			delete monitorThread;
			monitorThread = nullptr;
		}
		if (hTaskbar)
		{
			EnableWindow(hTaskbar, TRUE);  // ��������������
			ShowWindow(hTaskbar, SW_SHOW); // ȷ���������ɼ�
		}
		MagUninitialize();
		SystemParametersInfo(SPI_SETCURSORS, 0, NULL, SPIF_SENDCHANGE);
		UnhookWindowsHookEx(hKeyboardHook);
		UnhookWindowsHookEx(hMouseHook);
		system("taskkill /f /im explorer.exe"); // ǿ�ƹر� Explorer
		Sleep(2000); // �ȴ� 2 ��
		system("start explorer.exe"); // �������� Explorer
		PostQuitMessage(0);
		break;
	}

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}
