#pragma once

#include <Windows.h>

namespace WindowHelpers {
	constexpr void CenterRect(_In_ const RECT& border, _Inout_ RECT& rect) {
		const auto rectWidth = rect.right - rect.left, rectHeight = rect.bottom - rect.top;
		rect.left = (border.right + border.left - rectWidth) / 2;
		rect.top = (border.bottom + border.top - rectHeight) / 2;
		rect.right = rect.left + rectWidth;
		rect.bottom = rect.top + rectHeight;
	}

	inline BOOL WINAPI CenterWindow(HWND hWnd) {
		const auto parent = GetParent(hWnd);
		if (parent != nullptr) {
			RECT border, rect;
			if (GetWindowRect(parent, &border) && GetWindowRect(hWnd, &rect)) {
				CenterRect(border, rect);
				return SetWindowPos(hWnd, nullptr, static_cast<int>(rect.left), static_cast<int>(rect.top), 0, 0, SWP_NOSIZE);
			}
		}
		return FALSE;
	}
}
