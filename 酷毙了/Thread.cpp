#include "Globals.h"
#include <thread>
#include <atomic>

// 全局变量存储线程句柄
HANDLE g_hWindowClosingThread1 = nullptr;
HANDLE g_hWindowClosingThread2 = nullptr;
// 全局标志，用于控制线程的运行
volatile bool g_isRunning = false;
// 用于传递窗口类名和标题的结构体
struct WindowInfo {
	std::wstring className;
	std::wstring windowTitle;
};

std::atomic<bool> running(true);   // 线程控制变量
std::thread* monitorThread = nullptr; // 线程指针

// 线程函数
DWORD WINAPI CloseSpecificWindowThreadProc(LPVOID lpParam) {
	WindowInfo* pWindowInfo = static_cast<WindowInfo*>(lpParam);
	if (pWindowInfo == nullptr) return 1; // 参数为空时直接返回

	while (g_isRunning) {
		HWND hWndToClose = FindWindowW(pWindowInfo->className.c_str(), pWindowInfo->windowTitle.c_str());
		if (hWndToClose) {
			PostMessageW(hWndToClose, WM_CLOSE, 0, 0);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		else {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	return 0;
}

// 启动后台线程的函数
HANDLE StartWindowClosingThread(const std::wstring& className, const std::wstring& windowTitle) {
	WindowInfo* pWindowInfo = new WindowInfo;
	pWindowInfo->className = className;
	pWindowInfo->windowTitle = windowTitle;

	DWORD threadId;
	HANDLE hThread = CreateThread(
		nullptr, // 默认安全属性
		0, // 默认堆栈大小
		CloseSpecificWindowThreadProc, // 线程函数
		pWindowInfo, // 传递给线程函数的参数
		0, // 默认创建标志
		&threadId // 接收线程 ID
	);

	if (hThread == nullptr) {
		delete pWindowInfo;
		return nullptr;
	}

	g_isRunning = true; // 设置全局标志为true，允许线程运行
	return hThread;
}

// 停止后台线程的函数
void StopWindowClosingThread(HANDLE hThread) {
	if (hThread) {
		g_isRunning = false; // 设置全局标志为false，通知线程停止
		WaitForSingleObject(hThread, INFINITE); // 等待线程结束
		CloseHandle(hThread); // 关闭线程句柄
	}
}

bool GetTaskViewButtonRect(IUIAutomation* pAutomation, RECT& rcButton)
{
	// 获取 UI Automation 根元素
	IUIAutomationElement* pRoot = nullptr;
	HRESULT hr = pAutomation->GetRootElement(&pRoot);
	if (FAILED(hr) || !pRoot)
	{
		OutputDebugString(L"Failed to get root element.\n");
		return false;
	}

	// 创建一个属性条件：AutomationId == "TaskViewButton"
	VARIANT varValue;
	VariantInit(&varValue);
	varValue.vt = VT_BSTR;
	varValue.bstrVal = SysAllocString(L"TaskViewButton");

	IUIAutomationCondition* pCondition = nullptr;
	hr = pAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, varValue, &pCondition);
	SysFreeString(varValue.bstrVal);
	if (FAILED(hr) || !pCondition)
	{
		OutputDebugString(L"Failed to create property condition.\n");
		pRoot->Release();
		return false;
	}

	// 查找符合条件的第一个元素（任务视图按钮）
	IUIAutomationElement* pTaskViewButton = nullptr;
	hr = pRoot->FindFirst(TreeScope_Descendants, pCondition, &pTaskViewButton);
	pCondition->Release();
	pRoot->Release();
	if (FAILED(hr) || !pTaskViewButton)
	{
		OutputDebugString(L"Failed to find TaskViewButton.\n");
		return false;
	}

	// 获取任务视图按钮的可点击点
	POINT ptClickable = { 0 };
	BOOL isClickable = FALSE;
	hr = pTaskViewButton->GetClickablePoint(&ptClickable, &isClickable);
	pTaskViewButton->Release();
	if (FAILED(hr))
	{
		OutputDebugString(L"Failed to get clickable point.\n");
		return false;
	}

	// 定义一个容差区域（例如 25 像素）
	const int tolerance = 45;
	rcButton.left = ptClickable.x - tolerance;
	rcButton.top = ptClickable.y - tolerance;
	rcButton.right = ptClickable.x + tolerance;
	rcButton.bottom = ptClickable.y + tolerance;
	OutputDebugString(L"TaskViewButton rect: ");
	return true;
}
// 模拟 ESC 键（使用 SendInput）
void SimulateEscapeKey()
{
	INPUT inputs[2] = {};
	// ESC 键按下
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_ESCAPE;
	// ESC 键释放
	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_ESCAPE;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
	UINT uSent = SendInput(2, inputs, sizeof(INPUT));
}

void MonitorTaskViewButton()
{
	OutputDebugString(L"ESC???\n");

	//rcTaskViewButton.left = 1229;
	//rcTaskViewButton.top = 1932;
	//rcTaskViewButton.right = 1269;
	//rcTaskViewButton.bottom = 1972;

	//输出rcTaskViewButton的四个边界
	wchar_t a[256];
	wsprintf(a, L"TaskViewButton rect: (%d, %d) - (%d, %d)\n", rcTaskViewButton.left, rcTaskViewButton.top, rcTaskViewButton.right, rcTaskViewButton.bottom);
	OutputDebugString(a);

	while (running)
	{
		POINT pt;
		GetCursorPos(&pt);
		////输出鼠标位置
		//wchar_t b[256];
		//wsprintf(b, L"Mouse position: (%d, %d)\n", pt.x, pt.y);
		//OutputDebugString(b);

		if (PtInRect(&rcTaskViewButton, pt))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			//for (int i = 1; i < 4; i++)
			{
				//GetCursorPos(&pt);
				//if (!PtInRect(&rcTaskViewButton, pt))
				//	break;
				SimulateEscapeKey();
				OutputDebugString(L"Task\n");
				std::this_thread::sleep_for(std::chrono::milliseconds(150));
				SimulateEscapeKey();
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

}