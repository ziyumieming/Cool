
#include "Globals.h"

static int cutPosition = 0; // ��̬��������¼�и�λ��
HCURSOR g_cursorIBeam = CopyIcon(LoadCursor(NULL, IDC_IBEAM));
HCURSOR g_cursorArrow = CopyIcon(LoadCursor(NULL, IDC_ARROW));
HCURSOR g_cursorHand = CopyIcon(LoadCursor(NULL, IDC_HAND));
HCURSOR g_cursorSizeAll = CopyIcon(LoadCursor(NULL, IDC_SIZEALL));
HCURSOR g_cursorSizeNESW = CopyIcon(LoadCursor(NULL, IDC_SIZENESW));
HCURSOR g_cursorSizeNS = CopyIcon(LoadCursor(NULL, IDC_SIZENS));
HCURSOR g_cursorSizeNWSE = CopyIcon(LoadCursor(NULL, IDC_SIZENWSE));
HCURSOR g_cursorSizeWE = CopyIcon(LoadCursor(NULL, IDC_SIZEWE));

HCURSOR g_currentCursor = g_cursorArrow;

std::unordered_map<HCURSOR, CursorOffset> g_cursorOffsetMap;// ȫ��ӳ����洢ÿ������ƫ����
std::unordered_map<int, HCURSOR> g_cursorMap = {
	{ UIA_ButtonControlTypeId,        g_cursorHand     },  // ��ťͨ���ɵ����ʹ�����͹��
	{ UIA_CalendarControlTypeId,      g_cursorArrow    },
	{ UIA_CheckBoxControlTypeId,      g_cursorHand     },  // ��ѡ��ɵ��
	{ UIA_ComboBoxControlTypeId,      g_cursorArrow    },  // ������
	{ UIA_CustomControlTypeId,        g_cursorArrow    },  // �Զ���ؼ���Ĭ�ϼ�ͷ��
	{ UIA_DataGridControlTypeId,      g_cursorArrow    },
	{ UIA_DataItemControlTypeId,      g_cursorArrow    },
	{ UIA_DocumentControlTypeId,      g_cursorIBeam    },  // �ĵ�һ������룬ʹ�� IBeam
	{ UIA_EditControlTypeId,          g_cursorIBeam    },  // �ı������
	{ UIA_GroupControlTypeId,         g_cursorArrow    },
	{ UIA_HeaderControlTypeId,        g_cursorArrow   },  // ���ͷͨ�����϶�
	{ UIA_HeaderItemControlTypeId,    g_cursorArrow   },
	{ UIA_HyperlinkControlTypeId,     g_cursorHand     },  // ������ͨ�������͹��
	{ UIA_ImageControlTypeId,         g_cursorArrow    },
	{ UIA_ListControlTypeId,          g_cursorArrow    },
	{ UIA_ListItemControlTypeId,      g_cursorArrow    },
	{ UIA_MenuBarControlTypeId,       g_cursorArrow    },
	{ UIA_MenuControlTypeId,          g_cursorArrow    },
	{ UIA_MenuItemControlTypeId,      g_cursorArrow    },
	{ UIA_PaneControlTypeId,          g_cursorArrow    },
	{ UIA_ProgressBarControlTypeId,   g_cursorArrow    },  // ����������Ҫ����
	{ UIA_RadioButtonControlTypeId,   g_cursorHand     },  // ��ѡ��ť�ɵ��
	{ UIA_ScrollBarControlTypeId,     g_cursorArrow   },  // ������ͨ��֧�ִ�ֱ�϶�
	{ UIA_SemanticZoomControlTypeId,  g_cursorSizeAll  },
	{ UIA_SeparatorControlTypeId,     g_cursorArrow    },
	{ UIA_SliderControlTypeId,        g_cursorArrow   },  // ����ͨ����ˮƽ����
};

void HideSystemCursor() {
	// ���� 1x1 �հ׹��
	BYTE andMask[1] = { 0xFF };  // ʹ��ȫ1 mask����ʾ͸����������ֵ�Ӳ��Զ�����
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


	// �滻ϵͳĬ�ϵļ�ͷ���
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
	g_currentCursor = g_cursorArrow; // Ĭ��Ϊ��ͷ
	HWND hwndUnderCursor = WindowFromPoint(pt);
	if (hwndUnderCursor) {
		RECT rect;
		if (GetWindowRect(hwndUnderCursor, &rect)) {
			int edgeThreshold = 10;  // �жϱ�Ե�����ط�Χ

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

			// �жϴ����ĸ��ǵĵ�����С���
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

			// �жϴ��ڱ�Ե�ĵ�����С���
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

			// ��ӳ����в��Ҷ�Ӧ�Ĺ��
			auto it = g_cursorMap.find(controlType);
			if (it != g_cursorMap.end()) {
				g_currentCursor = it->second;
			}
			else {
				g_currentCursor = g_cursorArrow; // Ĭ��Ϊ��ͷ
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

		// �ͷ�λͼ��Դ�������ͷţ�����ᵼ���ڴ�й©��
		if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
		if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
	}
}