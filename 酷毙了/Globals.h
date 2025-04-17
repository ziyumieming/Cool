#pragma once

#define OEMRESOURCE
#include <windows.h>
#include <string>
#include <unordered_map>

#include <chrono> // 用于计时
using namespace std::chrono;


#include <uiautomation.h>
#include <comdef.h>
#pragma comment(lib, "uiautomationcore.lib")

// 全局变量声明
extern HINSTANCE hInstGlobal;
extern HWND hwndMagnifier;
extern HWND hwndMouseOverlay;
extern HWND hTaskbar;

extern const wchar_t CLASS_NAME[];
extern const wchar_t MOUSE_OVERLAY_CLASS[];

extern HHOOK hKeyboardHook;
extern HHOOK hMouseHook;

extern bool exiting;
extern int cutPosition;

extern HCURSOR g_cursorArrow;
extern HCURSOR g_cursorIBeam;
extern HCURSOR g_cursorHand;
extern HCURSOR g_cursorSizeAll;
extern HCURSOR g_cursorSizeNESW;
extern HCURSOR g_cursorSizeNS;
extern HCURSOR g_cursorSizeNWSE;
extern HCURSOR g_cursorSizeWE;
extern HCURSOR g_currentCursor;

extern HANDLE g_hWindowClosingThread1;
extern HANDLE g_hWindowClosingThread2;

struct CursorOffset {
	int offsetX;
	int offsetY;
};

// 全局映射表声明
extern std::unordered_map<HCURSOR, CursorOffset> g_cursorOffsetMap;
extern std::unordered_map<int, HCURSOR> g_cursorMap;

// UI Automation 全局指针
extern IUIAutomation* pAutomation;
extern IUIAutomation* pUIA;

extern int screenHeight;
extern int screenWidth;

extern RECT rcTaskViewButton;

void HideSystemCursor();
void CacheCursorOffsets(HCURSOR hCursor);
void UpdateVirtualCursorType();
bool GetTaskViewButtonRect(IUIAutomation* pAutomation, RECT& rcButton);