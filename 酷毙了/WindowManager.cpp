
#include "Globals.h"
#include "WindowManager.h"

const wchar_t* TARGET_BUTTONS[];// 定义按钮标识符（需根据实际系统版本调整）

const wchar_t CLASS_NAME[] = L"MagnifierWindow";
const wchar_t MOUSE_OVERLAY_CLASS[] = L"MouseOverlayClass";

HINSTANCE hInstGlobal; // 解决 hInstance 问题
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
	L"StartButton",       // 开始按钮
	L"TaskViewButton",    // 任务视图按钮（Win10/11）
	L"SearchBox",         // 搜索按钮（Win10）
	L"SearchButton",      // 搜索按钮（Win11）
	L"WidgetsButton",     // 小组件按钮（Win11）
	L"ShowDesktopButton", // 显示桌面按钮（右下角）
	L"SystemTrayIcon",	 // 系统托盘图标（如音量、网络）
};

bool exiting = false;
steady_clock::time_point lastEscPress;
static int cutPosition = 0; // 静态变量，记录切割位置;

BOOL CALLBACK UpdateMagWindow(HWND hwnd, void* srcdata, MAGIMAGEHEADER srcheader, void* destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty)
{
	if (exiting) return FALSE;
	// 获取像素数据
	BYTE* srcPixels = (BYTE*)srcdata;
	BYTE* destPixels = (BYTE*)destdata;

	int width = destheader.width;
	int height = destheader.height;
	int stride = destheader.stride;  // 每行像素的字节数

	static auto startTime = steady_clock::now();
	auto elapsed = duration_cast<seconds>(steady_clock::now() - startTime).count() / 10;
	int speed = min(1 + elapsed, 50); // 每10秒加速 1 像素，最大 50
	cutPosition = (cutPosition + speed) % width;


	// 1. 更新切割位置：使其向右移动
	cutPosition = (cutPosition + speed) % width; // 每次移动 5 个像素，并进行wrap-around

	// 2. 创建临时缓冲区，用于存放拼接后的图像
	BYTE* tempBuffer = new BYTE[height * stride];
	if (!tempBuffer) {
		return FALSE; // 内存分配失败，返回 FALSE 阻止更新
	}

	// 3.  切割和拼接图像
	for (int y = 0; y < height; y++) {
		BYTE* srcRow = destPixels + y * stride;   // 注意，这里我们直接操作 destPixels，因为它包含了原始图像数据
		BYTE* destRow = tempBuffer + y * stride;
		for (int x = 0; x < width; x++) {
			int index = x * 4; // BGRA 格式

			if (x < cutPosition) {
				// 左侧部分： 从源图像的右侧部分复制
				int srcX = x + (width - cutPosition); // 计算源图像中对应右侧部分的 X 坐标
				if (srcX >= width) srcX -= width; // 理论上不需要，但为了保险起见
				if (srcX < 0) srcX += width;

				BYTE* srcPixel = srcRow + srcX * 4;
				destRow[index] = srcPixel[0]; // B
				destRow[index + 1] = srcPixel[1]; // G
				destRow[index + 2] = srcPixel[2]; // R
				destRow[index + 3] = srcPixel[3]; // A (Alpha)
			}
			else {
				// 右侧部分： 从源图像的左侧部分复制
				int srcX = x - cutPosition; // 计算源图像中对应左侧部分的 X 坐标
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
	//在缓冲区中作反相处理
	//for (int y = 0; y < height; y++) {
	//	BYTE* destRow = tempBuffer + y * stride;
	//	for (int x = 0; x < width; x++) {
	//		int index = x * 4; // BGRA 格式
	//		destRow[index] = 255 - destRow[index]; // B
	//		destRow[index + 1] = 255 - destRow[index + 1]; // G
	//		destRow[index + 2] = 255 - destRow[index + 2]; // R
	//		destRow[index + 3] = destRow[index + 3]; // A (Alpha)
	//	}
	//}
	// 5. 将临时缓冲区的内容复制回 destPixels，完成图像更新
	memcpy(destPixels, tempBuffer, height * stride);

	delete[] tempBuffer; // 释放临时缓冲区

	//POINT pt;
	//if (GetCursorPos(&pt)) {
	//	// 根据背景平移，调整鼠标 X 坐标：加上 cutPosition 后取模屏幕宽度
	//	int adjustedX = (pt.x + cutPosition) % width;
	//	int adjustedY = pt.y; // 垂直方向无平移

	//	// 假设我们的 Magnifier 源区域与屏幕一一对应，
	//	// 如果需要将屏幕坐标转换为源区域坐标，可用 ScreenToClient(hwndMagnifier, &pt)
	//	// 这里假设 (0,0) 对应屏幕左上

	//	// 绘制一个红色圆圈表示鼠标光标
	//	int radius = 10; // 光标半径
	//	for (int dy = -radius; dy <= radius; dy++) {
	//		int yPos = adjustedY + dy;
	//		if (yPos < 0 || yPos >= height) continue;
	//		for (int dx = -radius; dx <= radius; dx++) {
	//			if (dx * dx + dy * dy <= radius * radius) {
	//				int xPos = adjustedX + dx;
	//				if (xPos < 0 || xPos >= width) continue;
	//				int offset = yPos * stride + xPos * 4;
	//				// 设置该像素为纯红色，Alpha=255
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
			if (duration.count() < 1000) { // 1 秒内双击 End

				HWND hwndTarget = FindWindow(CLASS_NAME, NULL);
				SetForegroundWindow(hwndTarget);
				exiting = true;
				KillTimer(hwndTarget, 1);
				KillTimer(hwndTarget, 2);
				ShowWindow(hwndMagnifier, SW_HIDE);
				ShowWindow(hwndMouseOverlay, SW_HIDE);
				EnableWindow(hTaskbar, TRUE);  // 重新启用任务栏
				ShowWindow(hTaskbar, SW_SHOW); // 确保任务栏可见
				SystemParametersInfo(SPI_SETCURSORS, 0, NULL, SPIF_SENDCHANGE);
				int response = MessageBox(hwndTarget, L"确定要退出程序吗？", L"退出确认", MB_YESNO | MB_ICONQUESTION);
				// MessageBox函数显示一个消息框，参数分别是：父窗口句柄、消息内容、消息标题、消息框样式。MB_YESNO 表示消息框中有“是”和“否”两个按钮；MB_ICONQUESTION 表示消息框中有一个问号图标
				if (response == IDYES)// IDYES 是 MessageBox 函数的返回值，表示用户点击了“是”按钮；IDNO 表示用户点击了“否”按钮
					DestroyWindow(hwndTarget);
				else // 用户取消退出后，重启定时器
				{
					exiting = false;
					SetTimer(hwndTarget, 1, 15, NULL);
					SetTimer(hwndTarget, 2, 200, NULL);
					ShowWindow(hwndMagnifier, SW_SHOW);
					ShowWindow(hwndMouseOverlay, SW_SHOW);
					EnableWindow(hTaskbar, FALSE);  // 禁止任务栏交互
					HideSystemCursor();
				}
			}
			lastEscPress = now;
		}
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			if (pKey->vkCode == VK_LWIN || pKey->vkCode == VK_RWIN
				//||(pKey->vkCode == VK_ESCAPE && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
				) {
				return 1; // 拦截按键
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
			// 获取鼠标所在窗口 

			HWND hWndUnderMouse = WindowFromPoint(pt);

			wchar_t className[256];
			GetClassName(hWndUnderMouse, className, 256);
			//OutputDebugString(className);

			if (hWndUnderMouse)
			{
				wchar_t className[256];
				GetClassName(hWndUnderMouse, className, 256);
				//OutputDebugString(className);
				if (wcscmp(className, L"SysListView32") == 0 || wcscmp(className, L"DirectUIHWND") == 0 || wcscmp(className, L"Shell_TrayWnd") == 0 || wcscmp(className, L"TrayNotifyWnd") == 0 || wcscmp(className, L"MSTaskSwWClass") == 0)  // 桌面 资源管理器 任务栏 通知栏
					return 1;  // 阻止右键事件

			}
		}
		if (wParam == WM_MOUSEMOVE) {
			int screenWidth = GetSystemMetrics(SM_CXSCREEN);
			int screenHeight = GetSystemMetrics(SM_CYSCREEN);
			if (pt.x <= 1) {
				SetCursorPos(screenWidth - 1, pt.y);
				return 1; // 阻止消息传递
			}
			else if (pt.x >= screenWidth - 1) {
				SetCursorPos(0, pt.y);
				return 1; // 阻止消息传递
			}
		}
		if ((wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP))
		{
			// 获取鼠标所在窗口 
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
					return 1;  // 阻止右键事件

			}
		}
		if (wParam == WM_LBUTTONDOWN) {
			IUIAutomationElement* pElem = nullptr;
			pAutomation->ElementFromPoint(pt, &pElem);
			if (pElem) {
				// 获取按钮的 AutomationId 和 Name
				BSTR automationId, name;
				pElem->get_CurrentAutomationId(&automationId);
				pElem->get_CurrentName(&name);

				// 检查是否匹配目标按钮
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
					return 1; // 拦截点击
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
	static int dpiScaleX = 1, dpiScaleY = 1;  // 存储 DPI 缩放比例

	switch (msg) {
	case WM_CREATE:
	{
		// 设置窗口标题（任务管理器显示名称）
		SetWindowText(hwnd, L"此电脑");

		HICON hIcon = NULL;
		// 加载“此电脑”图标（在 shell32.dll 中，索引 4）
		ExtractIconEx(L"C:\\Windows\\System32\\shell32.dll", 4, &hIcon, NULL, 1);//返回值是图标的数量
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

		// 创建兼容 DC 和位图
		HDC hdcMem = CreateCompatibleDC(hdc);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdc, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		SelectObject(hdcMem, hbmMem);

		// 让整个窗口背景透明
		HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
		FillRect(hdcMem, &ps.rcPaint, hBrush);
		DeleteObject(hBrush);

		// 获取鼠标信息
		CURSORINFO ci = { sizeof(CURSORINFO) };
		if (GetCursorInfo(&ci) && ci.flags == CURSOR_SHOWING) {
			HICON hCursor = ci.hCursor ? ci.hCursor : LoadCursor(NULL, IDC_ARROW);
			// 获取鼠标的真实大小
			ICONINFO iconInfo;
			BITMAP bmpCursor;
			ICONMETRICS iconMetrics;
			iconMetrics.cbSize = sizeof(ICONMETRICS);
			int cursorWidth = 32;  // 默认大小
			int cursorHeight = 32;
			if (GetIconInfo(hCursor, &iconInfo)) {
				cursorWidth = GetSystemMetrics(SM_CXCURSOR);
				cursorHeight = GetSystemMetrics(SM_CYCURSOR);
			}
			// 获取鼠标位置
			POINT pt;
			GetCursorPos(&pt);

			CursorOffset offset = { 0, 0 };

			// 从映射表中查询当前光标的热点
			auto it = g_cursorOffsetMap.find(g_currentCursor);
			if (it != g_cursorOffsetMap.end()) {
				offset = it->second;
			}
			int adjustedX = pt.x;
			int adjustedY = pt.y;
			// 如果当前光标是缩放光标，则调整绘制位置
			if (g_currentCursor != g_cursorArrow)
			{
				adjustedX = pt.x - offset.offsetX;
				adjustedY = pt.y - offset.offsetY;
			}

			// 绘制鼠标
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
	// 上一次按下 Esc 键的时间点

	switch (msg) {
	case WM_CREATE: {
		if (hTaskbar) {
			//EnableWindow(hTaskbar, FALSE);  // 禁止任务栏交互
			//ShowWindow(hTaskbar, SW_HIDE); // 隐藏任务栏
		}
		// 设置窗口标题（任务管理器显示名称）
		SetWindowText(hwnd, L"主文件夹");

		HICON hIcon = ExtractIcon(GetModuleHandle(NULL), L"C:\\Windows\\explorer.exe", 0);
		if (hIcon) {
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}

		if (!MagInitialize()) {
			MessageBox(hwnd, L"无法初始化 Magnification API！", L"错误", MB_OK | MB_ICONERROR);
			return -1;
		}
		hwndMagnifier = CreateWindow(WC_MAGNIFIER, L"Magnifier", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hwnd, NULL, NULL, NULL);
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
		hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
		SetWindowPos(hwndMagnifier, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		//ShowWindow(hwndMagnifier, SW_HIDE);
		//ShowWindow(hwndMouseOverlay, SW_HIDE);
		HideSystemCursor();
		HWND hPreviewWnd = FindWindow(L"TaskListThumbnailWnd", NULL); // 任务栏预览窗口
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
			InvalidateRect(hwndMouseOverlay, NULL, FALSE); // 确保鼠标窗口刷新
			InvalidateRect(hwnd, NULL, FALSE); // InvalidateRect 主窗口 (hwnd) 而不是放大镜窗口 (hwndMagnifier)
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
		//此处其余绘画会被覆盖
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
	//		SetForegroundWindow(hwnd);  // 试图把焦点抢回来
	//		SetFocus(hwnd);              / 设置输入焦点
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
			monitorThread->detach(); // 让线程自己结束
			delete monitorThread;
			monitorThread = nullptr;
		}
		if (hTaskbar)
		{
			EnableWindow(hTaskbar, TRUE);  // 重新启用任务栏
			ShowWindow(hTaskbar, SW_SHOW); // 确保任务栏可见
		}
		MagUninitialize();
		SystemParametersInfo(SPI_SETCURSORS, 0, NULL, SPIF_SENDCHANGE);
		UnhookWindowsHookEx(hKeyboardHook);
		UnhookWindowsHookEx(hMouseHook);
		system("taskkill /f /im explorer.exe"); // 强制关闭 Explorer
		Sleep(2000); // 等待 2 秒
		system("start explorer.exe"); // 重新启动 Explorer
		PostQuitMessage(0);
		break;
	}

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}
