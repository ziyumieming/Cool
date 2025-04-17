
#include "Globals.h"

static int cutPosition = 0; // 静态变量，记录切割位置
HCURSOR g_cursorIBeam = CopyIcon(LoadCursor(NULL, IDC_IBEAM));
HCURSOR g_cursorArrow = CopyIcon(LoadCursor(NULL, IDC_ARROW));
HCURSOR g_cursorHand = CopyIcon(LoadCursor(NULL, IDC_HAND));
HCURSOR g_cursorSizeAll = CopyIcon(LoadCursor(NULL, IDC_SIZEALL));
HCURSOR g_cursorSizeNESW = CopyIcon(LoadCursor(NULL, IDC_SIZENESW));
HCURSOR g_cursorSizeNS = CopyIcon(LoadCursor(NULL, IDC_SIZENS));
HCURSOR g_cursorSizeNWSE = CopyIcon(LoadCursor(NULL, IDC_SIZENWSE));
HCURSOR g_cursorSizeWE = CopyIcon(LoadCursor(NULL, IDC_SIZEWE));

HCURSOR g_currentCursor = g_cursorArrow;

std::unordered_map<HCURSOR, CursorOffset> g_cursorOffsetMap;// 全局映射表：存储每个光标的偏移量
std::unordered_map<int, HCURSOR> g_cursorMap = {
	{ UIA_ButtonControlTypeId,        g_cursorHand     },  // 按钮通常可点击，使用手型光标
	{ UIA_CalendarControlTypeId,      g_cursorArrow    },
	{ UIA_CheckBoxControlTypeId,      g_cursorHand     },  // 复选框可点击
	{ UIA_ComboBoxControlTypeId,      g_cursorArrow    },  // 下拉框
	{ UIA_CustomControlTypeId,        g_cursorArrow    },  // 自定义控件（默认箭头）
	{ UIA_DataGridControlTypeId,      g_cursorArrow    },
	{ UIA_DataItemControlTypeId,      g_cursorArrow    },
	{ UIA_DocumentControlTypeId,      g_cursorIBeam    },  // 文档一般可输入，使用 IBeam
	{ UIA_EditControlTypeId,          g_cursorIBeam    },  // 文本输入框
	{ UIA_GroupControlTypeId,         g_cursorArrow    },
	{ UIA_HeaderControlTypeId,        g_cursorArrow   },  // 表格头通常可拖动
	{ UIA_HeaderItemControlTypeId,    g_cursorArrow   },
	{ UIA_HyperlinkControlTypeId,     g_cursorHand     },  // 超链接通常用手型光标
	{ UIA_ImageControlTypeId,         g_cursorArrow    },
	{ UIA_ListControlTypeId,          g_cursorArrow    },
	{ UIA_ListItemControlTypeId,      g_cursorArrow    },
	{ UIA_MenuBarControlTypeId,       g_cursorArrow    },
	{ UIA_MenuControlTypeId,          g_cursorArrow    },
	{ UIA_MenuItemControlTypeId,      g_cursorArrow    },
	{ UIA_PaneControlTypeId,          g_cursorArrow    },
	{ UIA_ProgressBarControlTypeId,   g_cursorArrow    },  // 进度条不需要交互
	{ UIA_RadioButtonControlTypeId,   g_cursorHand     },  // 单选按钮可点击
	{ UIA_ScrollBarControlTypeId,     g_cursorArrow   },  // 滚动条通常支持垂直拖动
	{ UIA_SemanticZoomControlTypeId,  g_cursorSizeAll  },
	{ UIA_SeparatorControlTypeId,     g_cursorArrow    },
	{ UIA_SliderControlTypeId,        g_cursorArrow   },  // 滑块通常是水平调整
};

void HideSystemCursor() {
	// 创建 1x1 空白光标
	BYTE andMask[1] = { 0xFF };  // 使用全1 mask，表示透明（具体数值视测试而定）
	BYTE xorMask[1] = { 0x00 };
	HCURSOR hBlankCursor1 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor2 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor3 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor4 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor5 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor6 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor7 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor8 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor9 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor10 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor11 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor12 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);
	HCURSOR hBlankCursor13 = CreateCursor(hInstGlobal, 0, 0, 1, 1, andMask, xorMask);


	// 替换系统默认的箭头光标
	SetSystemCursor(hBlankCursor1, OCR_NORMAL);
	SetSystemCursor(hBlankCursor2, OCR_APPSTARTING);
	SetSystemCursor(hBlankCursor3, OCR_NORMAL);
	SetSystemCursor(hBlankCursor4, OCR_CROSS);
	SetSystemCursor(hBlankCursor5, OCR_HAND);
	SetSystemCursor(hBlankCursor6, OCR_WAIT);
	SetSystemCursor(hBlankCursor7, OCR_IBEAM);
	SetSystemCursor(hBlankCursor8, OCR_NO);
	SetSystemCursor(hBlankCursor9, OCR_SIZEALL);
	SetSystemCursor(hBlankCursor10, OCR_SIZENESW);
	SetSystemCursor(hBlankCursor11, OCR_SIZENS);
	SetSystemCursor(hBlankCursor12, OCR_SIZENWSE);
	SetSystemCursor(hBlankCursor13, OCR_SIZEWE);
}


void UpdateVirtualCursorType() {
	POINT pt;
	GetCursorPos(&pt);
	g_currentCursor = g_cursorArrow; // 默认为箭头
	HWND hwndUnderCursor = WindowFromPoint(pt);
	if (hwndUnderCursor) {
		RECT rect;
		if (GetWindowRect(hwndUnderCursor, &rect)) {
			int edgeThreshold = 10;  // 判断边缘的像素范围

			bool leftEdge = (pt.x >= rect.left - edgeThreshold && pt.x <= rect.left + edgeThreshold);
			bool rightEdge = (pt.x <= rect.right + edgeThreshold && pt.x >= rect.right - edgeThreshold);
			bool topEdge = (pt.y >= rect.top - edgeThreshold && pt.y <= rect.top + edgeThreshold);
			bool bottomEdge = (pt.y <= rect.bottom + edgeThreshold && pt.y >= rect.bottom - edgeThreshold);
			if (PtInRect(&rect, pt)) {
				int leftDist = pt.x - rect.left;
				int rightDist = rect.right - pt.x;
				int topDist = pt.y - rect.top;
				int bottomDist = rect.bottom - pt.y;
				if (leftDist < edgeThreshold) leftEdge = true;
				if (rightDist < edgeThreshold) rightEdge = true;
				if (topDist < edgeThreshold)topEdge = true;
				if (bottomDist < edgeThreshold) bottomEdge = true;
			}

			// 判断窗口四个角的调整大小光标
			if (leftEdge && topEdge) {
				g_currentCursor = g_cursorSizeNWSE;  
				return;
			}
			else if (rightEdge && bottomEdge) {
				g_currentCursor = g_cursorSizeNWSE;  
				return;
			}
			else if (leftEdge && bottomEdge) {
				g_currentCursor = g_cursorSizeNESW;  
				return;
			}
			else if (rightEdge && topEdge) {
				g_currentCursor = g_cursorSizeNESW;  
				return;
			}

			// 判断窗口边缘的调整大小光标
			if (leftEdge || rightEdge) {
				g_currentCursor = g_cursorSizeWE;  
				return;
			}
			if (topEdge || bottomEdge) {
				g_currentCursor = g_cursorSizeNS;  
				return;
			}
		}
	}

	IUIAutomationElement* pElement = nullptr;
	HRESULT hr = pAutomation->ElementFromPoint(pt, &pElement);
	if (SUCCEEDED(hr) && pElement) {
		VARIANT var;
		VariantInit(&var);
		hr = pElement->GetCurrentPropertyValue(UIA_ControlTypePropertyId, &var);
		if (SUCCEEDED(hr) && var.vt == VT_I4) {
			int controlType = var.lVal;

			// 在映射表中查找对应的光标
			auto it = g_cursorMap.find(controlType);
			if (it != g_cursorMap.end()) {
				g_currentCursor = it->second;
			}
			else {
				g_currentCursor = g_cursorArrow; // 默认为箭头
			}
		}
		VariantClear(&var);
		pElement->Release();
	}
}


void CacheCursorOffsets(HCURSOR hCursor) {
	ICONINFO iconInfo;
	if (GetIconInfo(hCursor, &iconInfo)) {
		CursorOffset offset = { iconInfo.xHotspot, iconInfo.yHotspot };
		g_cursorOffsetMap[hCursor] = offset;

		// 释放位图资源（必须释放，否则会导致内存泄漏）
		if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
		if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
	}
}