#pragma once

#include "pch.h"

#include "WindowHelpers.h"

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG: WindowHelpers::CenterWindow(hDlg); break;

	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL: EndDialog(hDlg, LOWORD(wParam)); return TRUE;
		}
	}	break;

	case WM_NOTIFY: {
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case NM_CLICK:
		case NM_RETURN: {
			if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == GetDlgItem(hDlg, IDC_SYSLINK_GITHUB_REPO))
				ShellExecuteW(nullptr, L"open", reinterpret_cast<PNMLINK>(lParam)->item.szUrl, nullptr, nullptr, SW_SHOW);
		}	break;
		}
	}	break;
	}

	return FALSE;
}
