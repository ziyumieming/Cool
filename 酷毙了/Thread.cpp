#include "Globals.h"
#include <thread>
#include <atomic>

// ȫ�ֱ����洢�߳̾��
HANDLE g_hWindowClosingThread1 = nullptr;
HANDLE g_hWindowClosingThread2 = nullptr;
// ȫ�ֱ�־�����ڿ����̵߳�����
volatile bool g_isRunning = false;
// ���ڴ��ݴ��������ͱ���Ľṹ��
struct WindowInfo {
	std::wstring className;
	std::wstring windowTitle;
};

std::atomic<bool> running(true);   // �߳̿��Ʊ���
std::thread* monitorThread = nullptr; // �߳�ָ��

// �̺߳���
DWORD WINAPI CloseSpecificWindowThreadProc(LPVOID lpParam) {
	WindowInfo* pWindowInfo = static_cast<WindowInfo*>(lpParam);
	if (pWindowInfo == nullptr) return 1; // ����Ϊ��ʱֱ�ӷ���

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

// ������̨�̵߳ĺ���
HANDLE StartWindowClosingThread(const std::wstring& className, const std::wstring& windowTitle) {
	WindowInfo* pWindowInfo = new WindowInfo;
	pWindowInfo->className = className;
	pWindowInfo->windowTitle = windowTitle;

	DWORD threadId;
	HANDLE hThread = CreateThread(
		nullptr, // Ĭ�ϰ�ȫ����
		0, // Ĭ�϶�ջ��С
		CloseSpecificWindowThreadProc, // �̺߳���
		pWindowInfo, // ���ݸ��̺߳����Ĳ���
		0, // Ĭ�ϴ�����־
		&threadId // �����߳� ID
	);

	if (hThread == nullptr) {
		delete pWindowInfo;
		return nullptr;
	}

	g_isRunning = true; // ����ȫ�ֱ�־Ϊtrue�������߳�����
	return hThread;
}

// ֹͣ��̨�̵߳ĺ���
void StopWindowClosingThread(HANDLE hThread) {
	if (hThread) {
		g_isRunning = false; // ����ȫ�ֱ�־Ϊfalse��֪ͨ�߳�ֹͣ
		WaitForSingleObject(hThread, INFINITE); // �ȴ��߳̽���
		CloseHandle(hThread); // �ر��߳̾��
	}
}

bool GetTaskViewButtonRect(IUIAutomation* pAutomation, RECT& rcButton)
{
	// ��ȡ UI Automation ��Ԫ��
	IUIAutomationElement* pRoot = nullptr;
	HRESULT hr = pAutomation->GetRootElement(&pRoot);
	if (FAILED(hr) || !pRoot)
	{
		OutputDebugString(L"Failed to get root element.\n");
		return false;
	}

	// ����һ������������AutomationId == "TaskViewButton"
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

	// ���ҷ��������ĵ�һ��Ԫ�أ�������ͼ��ť��
	IUIAutomationElement* pTaskViewButton = nullptr;
	hr = pRoot->FindFirst(TreeScope_Descendants, pCondition, &pTaskViewButton);
	pCondition->Release();
	pRoot->Release();
	if (FAILED(hr) || !pTaskViewButton)
	{
		OutputDebugString(L"Failed to find TaskViewButton.\n");
		return false;
	}

	// ��ȡ������ͼ��ť�Ŀɵ����
	POINT ptClickable = { 0 };
	BOOL isClickable = FALSE;
	hr = pTaskViewButton->GetClickablePoint(&ptClickable, &isClickable);
	pTaskViewButton->Release();
	if (FAILED(hr))
	{
		OutputDebugString(L"Failed to get clickable point.\n");
		return false;
	}

	// ����һ���ݲ��������� 25 ���أ�
	const int tolerance = 45;
	rcButton.left = ptClickable.x - tolerance;
	rcButton.top = ptClickable.y - tolerance;
	rcButton.right = ptClickable.x + tolerance;
	rcButton.bottom = ptClickable.y + tolerance;
	OutputDebugString(L"TaskViewButton rect: ");
	return true;
}
// ģ�� ESC ����ʹ�� SendInput��
void SimulateEscapeKey()
{
	INPUT inputs[2] = {};
	// ESC ������
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_ESCAPE;
	// ESC ���ͷ�
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

	//���rcTaskViewButton���ĸ��߽�
	wchar_t a[256];
	wsprintf(a, L"TaskViewButton rect: (%d, %d) - (%d, %d)\n", rcTaskViewButton.left, rcTaskViewButton.top, rcTaskViewButton.right, rcTaskViewButton.bottom);
	OutputDebugString(a);

	while (running)
	{
		POINT pt;
		GetCursorPos(&pt);
		////������λ��
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