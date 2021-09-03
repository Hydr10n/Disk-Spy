#include "pch.h"

#include "FilterPropSheetPage.h"

#include "Device.h"

#include "WindowHelpers.h"

#include "MyAppData.h"

#include <Uxtheme.h>

using namespace Hydr10n::Device;
using namespace std;
using namespace WindowHelpers;

struct ColumnData {
	const LPCTSTR Text;
	const int Index;
};

union {
	struct { ColumnData Drive, ID, Excluded; };
	ColumnData Array[3]{
		{ TEXT("Drive\t\t"), 0 },
		{ TEXT("ID"), 1 },
		{ TEXT("Excluded"), 2 }
	};
} g_whitelistColumnData;

inline LPARAM WINAPI GetListViewItemData(HWND hWnd, int iItem) {
	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iItem = iItem;
	item.iSubItem = 0;
	return ListView_GetItem(hWnd, &item) ? item.lParam : -1;
}

inline int WINAPI FindListViewItem(HWND hWnd, int iItem, LPARAM lParam) {
	LVFINDINFO info;
	info.flags = LVFI_PARAM;
	info.lParam = lParam;
	return ListView_FindItem(hWnd, iItem, &info);
}

void LoadWhitelistMenu(HWND hWnd, LPPOINT lpPoint) {
	const auto index = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
	if (index == -1)
		return;

	const auto menu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_MENU_WHITELIST));
	if (menu != nullptr) {
		const auto submenu = GetSubMenu(menu, 0);
		if (submenu != nullptr) {
			if (ListView_GetSelectedCount(hWnd) == 1) {
				TCHAR szExcluded[2]{};
				ListView_GetItemText(hWnd, index, g_whitelistColumnData.Excluded.Index, szExcluded, ARRAYSIZE(szExcluded));
				if (*szExcluded)
					DeleteMenu(menu, *szExcluded == 'Y' ? ID_WHITELIST_EXCLUDE : ID_WHITELIST_INCLUDE, MF_BYCOMMAND);
			}

			if (lpPoint != nullptr)
				TrackPopupMenu(submenu, TPM_LEFTALIGN, static_cast<int>(lpPoint->x), static_cast<int>(lpPoint->y), 0, GetParent(hWnd), nullptr);
			else {
				POINT pt;
				if (ListView_GetItemPosition(hWnd, index, &pt) && MapWindowPoints(hWnd, HWND_DESKTOP, &pt, 1))
					TrackPopupMenu(submenu, TPM_LEFTALIGN, static_cast<int>(pt.x), static_cast<int>(pt.y), 0, GetParent(hWnd), nullptr);
			}
		}

		DestroyMenu(menu);
	}
}

BOOL InsertRowToWhitelist(HWND hWnd, int iItem, LPCTSTR lpDrive, const MyAppData::FilterData::WhitelistData::DriveData& driveData) {
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_PARAM;
	item.iItem = iItem;
	item.iSubItem = g_whitelistColumnData.Drive.Index;
	item.pszText = LPTSTR(lpDrive);
	item.lParam = static_cast<LPARAM>(driveData.ID);

	const auto index = ListView_InsertItem(hWnd, &item);
	if (index != -1) {
		TCHAR szDriveID[11];
		wsprintf(szDriveID, TEXT("0x%lX"), driveData.ID);
		ListView_SetItemText(hWnd, index, g_whitelistColumnData.ID.Index, szDriveID);
		ListView_SetItemText(hWnd, index, g_whitelistColumnData.Excluded.Index, LPTSTR(driveData.Excluded ? TEXT("Yes") : TEXT("No")));

		return TRUE;
	}

	return FALSE;
}

int CALLBACK WhitelistCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
	const auto window = reinterpret_cast<HWND>(lParamSort);

	TCHAR szDrive[2][2]{};
	ListView_GetItemText(window, lParam1, g_whitelistColumnData.Drive.Index, szDrive[0], ARRAYSIZE(szDrive[0]));
	ListView_GetItemText(window, lParam2, g_whitelistColumnData.Drive.Index, szDrive[1], ARRAYSIZE(szDrive[1]));

	if (*szDrive[0] == *szDrive[1])
		return static_cast<DWORD>(GetListViewItemData(window, static_cast<int>(lParam1))) > static_cast<DWORD>(GetListViewItemData(window, static_cast<int>(lParam2)));

	if (*szDrive[0] == '?' && *szDrive[1] != '?')
		return 1;

	if (*szDrive[0] != '?' && *szDrive[1] == '?')
		return -1;

	return lstrcmp(szDrive[0], szDrive[1]);
}

INT_PTR CALLBACK FilterPropSheetPageProc(HWND hPage, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	using FilterData = MyAppData::FilterData;

	static bool s_isDataSaved;

	static MyAppData* s_myAppData;

	static DWORD s_driveIDs[26];

	const auto NotifyDataChanged = [&] {
		s_isDataSaved = false;

		PropSheet_Changed(GetParent(hPage), hPage);
	};

	const auto ReplaceWhitelistItem = [&](HWND hWnd, int iDriveNumber) {
		auto& driveID = s_driveIDs[iDriveNumber];
		if (driveID = GenerateDriveID(iDriveNumber)) {
			const TCHAR szDrive[]{ static_cast<TCHAR>('A' + iDriveNumber), ':', 0 };
			const auto index = FindListViewItem(hWnd, -1, driveID);
			if (index == -1) {
				if (InsertRowToWhitelist(hWnd, 0, szDrive, MyAppData::FilterData::WhitelistData::DriveData{ driveID, FALSE }))
					ListView_SortItemsEx(hWnd, WhitelistCompare, hWnd);
			}
			else
				ListView_SetItemText(hWnd, FindListViewItem(hWnd, -1, driveID), g_whitelistColumnData.Drive.Index, LPTSTR(szDrive));
		}
	};

	switch (uMsg) {
	case WM_INITDIALOG: {
		CenterWindow(GetParent(hPage));

		s_myAppData = reinterpret_cast<MyAppData*>(reinterpret_cast<LPPROPSHEETPAGE>(lParam)->lParam);

		auto& filter = s_myAppData->Filter;

		SetDlgItemInt(hPage, IDC_EDIT_MAX_FILE_SIZE, filter.CopyLimits.MaxFileSize, FALSE);

		SNDMSG(GetDlgItem(hPage, IDC_SPIN_MAX_FILE_SIZE), UDM_SETRANGE, 0, MAKELPARAM(FilterData::CopyLimitsData::MaxValue, 0));

		constexpr LPCTSTR FileSizeUnits[]{ TEXT("KB"), TEXT("MB"), TEXT("GB") };
		const auto comboFileSizeUnits = GetDlgItem(hPage, IDC_COMBO_FILE_SIZE_UNITS);
		for (const auto fileSizeUnit : FileSizeUnits)
			ComboBox_AddString(comboFileSizeUnits, fileSizeUnit);
		ComboBox_SetCurSel(comboFileSizeUnits, static_cast<int>(filter.CopyLimits.FileSizeUnit));

		SetDlgItemInt(hPage, IDC_EDIT_MAX_DURATION, filter.CopyLimits.MaxDuration, FALSE);

		SNDMSG(GetDlgItem(hPage, IDC_SPIN_MAX_DURATION), UDM_SETRANGE, 0, MAKELPARAM(FilterData::CopyLimitsData::MaxValue, 0));

		constexpr LPCTSTR TimeUnits[]{ TEXT("Sec"), TEXT("Min") };
		const auto comboTimeUnits = GetDlgItem(hPage, IDC_COMBO_TIME_UNITS);
		for (const auto timeUnit : TimeUnits)
			ComboBox_AddString(comboTimeUnits, timeUnit);
		ComboBox_SetCurSel(comboTimeUnits, static_cast<int>(filter.CopyLimits.TimeUnit));

		SetDlgItemInt(hPage, IDC_EDIT_RESERVED_STORAGE_SPACE, filter.CopyLimits.ReservedStorageSpaceGB, FALSE);

		SNDMSG(GetDlgItem(hPage, IDC_SPIN_RESERVED_STORAGE_SPACE), UDM_SETRANGE, 0, MAKELPARAM(FilterData::CopyLimitsData::MaxValue, 0));

		const auto whitelist = GetDlgItem(hPage, IDC_LIST_WHITELIST);
		ListView_SetExtendedListViewStyle(whitelist, LVS_EX_AUTOCHECKSELECT | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
		SetWindowTheme(whitelist, L"Explorer", nullptr);

		const auto header = ListView_GetHeader(whitelist);
		SetWindowLongPtr(header, GWL_STYLE, GetWindowStyle(header) | HDS_NOSIZING);
		RECT headerRect{};
		GetClientRect(whitelist, &headerRect);
		for (const auto& data : g_whitelistColumnData.Array) {
			LVCOLUMN column;
			column.mask = LVCF_TEXT | LVCF_WIDTH;
			column.pszText = LPTSTR(data.Text);
			column.cx = (headerRect.right - headerRect.left) / 3;
			ListView_InsertColumn(whitelist, data.Index, &column);
		}

		for (UINT i = 0; i < min(ARRAYSIZE(filter.Whitelist.Drives), filter.Whitelist.DriveCount); i++)
			InsertRowToWhitelist(whitelist, i, L"?", filter.Whitelist.Drives[i]);

		for (int i = 0; i < ARRAYSIZE(s_driveIDs); i++)
			ReplaceWhitelistItem(whitelist, i);

		ListView_SortItemsEx(whitelist, WhitelistCompare, whitelist);
	}	break;

	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case IDC_EDIT_MAX_FILE_SIZE:
		case IDC_EDIT_MAX_DURATION:
		case IDC_EDIT_RESERVED_STORAGE_SPACE: {
			if (HIWORD(wParam) == EN_CHANGE)
				NotifyDataChanged();
		}	break;

		case IDC_COMBO_FILE_SIZE_UNITS:
		case IDC_COMBO_TIME_UNITS: {
			if (HIWORD(wParam) == CBN_SELCHANGE)
				NotifyDataChanged();
		}	break;

		case ID_WHITELIST_DELETE: {
			const auto whitelist = GetDlgItem(hPage, IDC_LIST_WHITELIST);

			BOOL dataChanged{};
			int index;
			while ((index = ListView_GetNextItem(whitelist, -1, LVNI_SELECTED)) != -1)
				dataChanged = ListView_DeleteItem(whitelist, index);

			if (dataChanged)
				NotifyDataChanged();
		}	break;

		case ID_WHITELIST_INCLUDE:
		case ID_WHITELIST_EXCLUDE: {
			const auto whitelist = GetDlgItem(hPage, IDC_LIST_WHITELIST);

			BOOL dataChanged{};
			int i = -1;
			while ((i = ListView_GetNextItem(whitelist, i, LVNI_SELECTED)) != -1) {
				TCHAR szExcluded[2]{};
				ListView_GetItemText(whitelist, i, g_whitelistColumnData.Excluded.Index, szExcluded, ARRAYSIZE(szExcluded));
				if (*szExcluded) {
					const auto exclude = wParam == ID_WHITELIST_EXCLUDE ? TRUE : FALSE;
					if (exclude == (*szExcluded != 'Y')) {
						ListView_SetItemText(whitelist, i, g_whitelistColumnData.Excluded.Index, LPTSTR(exclude ? L"Yes" : L"No"));

						dataChanged = TRUE;
					}
				}
			}

			if (dataChanged)
				NotifyDataChanged();
		}	break;
		}
	}	break;

	case WM_NOTIFY: {
		const auto hdr = reinterpret_cast<LPNMHDR>(lParam);
		switch (hdr->code) {
		case LVN_KEYDOWN: {
			if (hdr->idFrom == IDC_LIST_WHITELIST) {
				switch (reinterpret_cast<LPNMLVKEYDOWN>(lParam)->wVKey) {
				case 'A': {
					if (GetAsyncKeyState(VK_CONTROL)) {
						const auto count = ListView_GetItemCount(hdr->hwndFrom);
						for (int i = 0; i < count; i++)
							ListView_SetCheckState(hdr->hwndFrom, i, TRUE);
					}
				}	break;

				case VK_APPS: LoadWhitelistMenu(hdr->hwndFrom, nullptr); break;

				case VK_DELETE: SNDMSG(hPage, WM_COMMAND, ID_WHITELIST_DELETE, 0); break;
				}
			}
		}	break;

		case NM_RCLICK: {
			if (hdr->idFrom == IDC_LIST_WHITELIST && reinterpret_cast<LPNMITEMACTIVATE>(lParam)->iItem != -1) {
				POINT pt;
				if (GetCursorPos(&pt))
					LoadWhitelistMenu(hdr->hwndFrom, &pt);
			}
		} break;

		case PSN_APPLY: {
			if (s_isDataSaved)
				break;

			auto& filter = s_myAppData->Filter;

			filter.CopyLimits.MaxFileSize = GetDlgItemInt(hPage, IDC_EDIT_MAX_FILE_SIZE, nullptr, FALSE);

			const auto comboFileSizeUnits = GetDlgItem(hPage, IDC_COMBO_FILE_SIZE_UNITS);
			filter.CopyLimits.FileSizeUnit = static_cast<FilterData::FileSizeUnit>(ComboBox_GetCurSel(comboFileSizeUnits));

			filter.CopyLimits.MaxDuration = GetDlgItemInt(hPage, IDC_EDIT_MAX_DURATION, nullptr, FALSE);

			const auto comboTimeUnits = GetDlgItem(hPage, IDC_COMBO_TIME_UNITS);
			filter.CopyLimits.TimeUnit = static_cast<FilterData::TimeUnit>(ComboBox_GetCurSel(comboTimeUnits));

			filter.CopyLimits.ReservedStorageSpaceGB = GetDlgItemInt(hPage, IDC_EDIT_RESERVED_STORAGE_SPACE, nullptr, FALSE);

			filter.Whitelist.DriveCount = 0;
			const auto whitelist = GetDlgItem(hPage, IDC_LIST_WHITELIST);
			const auto count = min(ARRAYSIZE(filter.Whitelist.Drives), ListView_GetItemCount(whitelist));
			for (UINT i = 0; i < count; i++) {
				auto& drive = filter.Whitelist.Drives[filter.Whitelist.DriveCount++];
				if ((drive.ID = static_cast<DWORD>(GetListViewItemData(whitelist, i))) != -1) {
					TCHAR szExcluded[2]{};
					ListView_GetItemText(whitelist, i, g_whitelistColumnData.Excluded.Index, szExcluded, ARRAYSIZE(szExcluded));
					if (*szExcluded)
						drive.Excluded = *szExcluded == 'Y' ? TRUE : FALSE;
				}
			}

			if (!(s_isDataSaved = filter.Save()))
				MessageBoxW(hPage, L"Failed to save changes to file.", nullptr, MB_OK | MB_ICONERROR);
		}	break;
		}
	}	break;

	case WM_DEVICECHANGE: {
		switch (wParam) {
		case DBT_DEVICEARRIVAL:
		case DBT_DEVICEREMOVECOMPLETE: {
			if (reinterpret_cast<PDEV_BROADCAST_HDR>(lParam)->dbch_devicetype == DBT_DEVTYP_VOLUME) {
				const auto driveNumber = GetFirstDriveNumber(reinterpret_cast<PDEV_BROADCAST_VOLUME>(lParam)->dbcv_unitmask);

				const auto whitelist = GetDlgItem(hPage, IDC_LIST_WHITELIST);

				if (wParam == DBT_DEVICEARRIVAL)
					ReplaceWhitelistItem(whitelist, driveNumber);
				else {
					ListView_SetItemText(whitelist, FindListViewItem(whitelist, -1, s_driveIDs[driveNumber]), g_whitelistColumnData.Drive.Index, LPTSTR(TEXT("?")));

					s_driveIDs[driveNumber] = 0;
				}
			}
		}	break;
		}
	}	break;
	}

	return FALSE;
}
