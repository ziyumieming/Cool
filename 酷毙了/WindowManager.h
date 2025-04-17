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
//���Ӧ�ó���û�������Լ�Ϊ DPI ��֪��DPI-aware����Windows ���ܻ��Զ��Դ������ݽ��� DPI ���⻯�����š���������£���Ȼ�߼�����ĳ��򲶻���ȫ����������ʾʱϵͳ�ᰴ��ǰ DPI �����Զ��Ŵ����С�������ݡ�
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
extern std::atomic<bool> running;   // �߳̿��Ʊ���
extern std::thread* monitorThread; // �߳�ָ��

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
			MessageBox(NULL, L"���ؿ�ʼ��ť���", L"��ʾ", MB_OK);
		}
		return S_OK;
	}
};
