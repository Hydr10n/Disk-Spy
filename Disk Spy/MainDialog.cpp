#include "pch.h"

#include "MainDialog.h"

#include "File.h"
#include "FindFiles.h"

#include "Device.h"

#include "RegistryHelpers.h"

#include "MyAppData.h"

#include "FilterPropSheetPage.h"
#include "AboutDialog.h"

#include <ShlObj.h>

using namespace Hydr10n::Device;
using namespace Hydr10n::File;
using namespace Hydr10n::RegistryHelpers;
using namespace std;

#define TIME_FORMAT L"[%hu-%02hu-%02hu %02hu:%02hu:%02hu]"

enum class State : BOOL { Running, Stopped, Paused };

enum class MyWindowMessage { SetTextW = WM_USER + 1 };

union Windows {
	struct { HWND hButtonStartCopying, hButtonStopCopying, hButtonSkipCurrentFile, hProgressCopiedData, hEditDir, hEditFile; };
	HWND Array[6];
};

struct CopyDataParam {
	int DriveNumber;
	DWORD DriveID;

	State State;

	BOOL SkipCurrentFile;

	LPCWSTR DataPath;

	HANDLE hLogFile;

	struct { DWORD64 Start, PauseDuration; } Time;

	struct {
		DWORD DirCreatedCount, FileCopiedCount;

		DWORD64 SizeCopied;
	} Statistics;

	Windows Windows;

	DEV_BROADCAST_HANDLE DevBroadcastHandle;
};

struct CopyDataParamEx : CopyDataParam { HANDLE hThread; };

WCHAR g_exePath[UNICODE_STRING_MAX_CHARS], g_localDataPath[] = LR"(?:\Data\)";

HWND g_hDlg;
Windows* g_windows;

MyAppData* g_myAppData;

BOOL MySetWindowText(HWND hWnd, LPCWSTR lpText) { return PSTMSG(g_hDlg, static_cast<UINT>(MyWindowMessage::SetTextW), reinterpret_cast<WPARAM>(hWnd), reinterpret_cast<LPARAM>(StrDupW(lpText))); }

DWORD64 GetFilterMaxFileSize() {
	using FileSizeUnit = MyAppData::FilterData::FileSizeUnit;
	DWORD64 dw64MaxFileSize = g_myAppData->Filter.CopyLimits.MaxFileSize;
	switch (g_myAppData->Filter.CopyLimits.FileSizeUnit) {
	case FileSizeUnit::KB: dw64MaxFileSize <<= 10; break;
	case FileSizeUnit::MB: dw64MaxFileSize <<= 20; break;
	case FileSizeUnit::GB: dw64MaxFileSize <<= 30; break;
	}
	return dw64MaxFileSize;
}

DWORD64 GetFilterMaxDuration() {
	using TimeUnit = MyAppData::FilterData::TimeUnit;
	DWORD64 dw64MaxDuration = g_myAppData->Filter.CopyLimits.MaxDuration;
	switch (g_myAppData->Filter.CopyLimits.TimeUnit) {
	case TimeUnit::Sec: dw64MaxDuration *= 1000; break;
	case TimeUnit::Min: dw64MaxDuration *= 1000 * 60; break;
	}
	return dw64MaxDuration;
}

DWORD64 GetFilterReservedStorageSpace() { return static_cast<DWORD64>(g_myAppData->Filter.CopyLimits.ReservedStorageSpaceGB) << 30; }

bool FilterFile(const WIN32_FIND_DATAW& sourceFindData, const WIN32_FIND_DATAW* destinationFindData, CopyDataParam& param) {
	const auto sourceFileSize = ULARGE_INTEGER{ sourceFindData.nFileSizeLow , sourceFindData.nFileSizeHigh }.QuadPart;

	const auto reservedStorageSpace = GetFilterReservedStorageSpace();
	ULARGE_INTEGER totalNumberOfFreeBytes;
	if (reservedStorageSpace
		&& (!GetDiskFreeSpaceExW(initializer_list<WCHAR>({ static_cast<WCHAR>('A' + PathGetDriveNumberW(param.DataPath)), ':', 0 }).begin(), nullptr, nullptr, &totalNumberOfFreeBytes)
			|| (totalNumberOfFreeBytes.QuadPart < sourceFileSize
				|| totalNumberOfFreeBytes.QuadPart - sourceFileSize < GetFilterReservedStorageSpace()))) {
		param.State = State::Stopped;

		return false;
	}

	const auto maxFileSize = GetFilterMaxFileSize();
	return (destinationFindData == nullptr
		|| (sourceFileSize != ULARGE_INTEGER{ destinationFindData->nFileSizeLow,destinationFindData->nFileSizeHigh }.QuadPart
			|| CompareFileTime(&sourceFindData.ftCreationTime, &destinationFindData->ftCreationTime)
			|| CompareFileTime(&sourceFindData.ftLastWriteTime, &destinationFindData->ftLastWriteTime)))
		&& (!maxFileSize || sourceFileSize <= maxFileSize);
}

DWORD CALLBACK CopyProgressRoutine(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber, DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData) {
	const auto param = reinterpret_cast<CopyDataParam*>(lpData);

	const auto timePaused = GetTickCount64();
	while (param->State == State::Paused)
		;
	const auto timeNow = GetTickCount64();
	param->Time.PauseDuration += timeNow - timePaused;
	const auto maxDuration = GetFilterMaxDuration();
	if ((maxDuration && timeNow - param->Time.Start - param->Time.PauseDuration > maxDuration)
		|| param->State == State::Stopped)
		return PROGRESS_CANCEL;

	PSTMSG(param->Windows.hProgressCopiedData, PBM_SETPOS, static_cast<WPARAM>(TotalFileSize.QuadPart ? 100 * TotalBytesTransferred.QuadPart / TotalFileSize.QuadPart : 0), 0);

	if (TotalBytesTransferred.QuadPart == TotalFileSize.QuadPart) {
		param->Statistics.SizeCopied += static_cast<DWORD64>(TotalFileSize.QuadPart);

		FILETIME creationTime, lastAccessTime, lastWriteTime;
		if (GetFileTime(hSourceFile, &creationTime, &lastAccessTime, &lastWriteTime))
			SetFileTime(hDestinationFile, &creationTime, &lastAccessTime, &lastWriteTime);
	}

	return PROGRESS_CONTINUE;
}

BOOL CALLBACK OnEnteringDirectory(LPCWSTR lpPath, const WIN32_FIND_DATAW& findData, LPVOID lpParam) {
	const auto param = reinterpret_cast<CopyDataParam*>(lpParam);

	if (!IsCurrentOrParentDirectory(findData.cFileName)) {
		constexpr auto StrLen = UNICODE_STRING_MAX_CHARS + 30;
		const unique_ptr<WCHAR> buf(new WCHAR[StrLen]);
		const auto str = buf.get();
		if (str == nullptr)
			return FALSE;

		wnsprintfW(str, StrLen, L"%ls%ls\\\r\n->\r\n%ls", lpPath + 4, findData.cFileName, param->DataPath + 4);
		MySetWindowText(param->Windows.hEditDir, str);

		DWORD dwNumberOfBytesWritten;
		WriteFile(param->hLogFile, str, sizeof(*str) * wnsprintfW(str, StrLen, L"\r\n%ls%ls\\", PathSkipRootW(lpPath), findData.cFileName), &dwNumberOfBytesWritten, nullptr);

		wnsprintfW(str, StrLen, L"%ls%ls%ls", param->DataPath, PathSkipRootW(lpPath), findData.cFileName);

		if (SHCreateDirectory(nullptr, str) == ERROR_SUCCESS) {
			constexpr WCHAR szLogData[] = L" ✓\r\n";
			WriteFile(param->hLogFile, szLogData, sizeof(szLogData) - sizeof(*szLogData), &dwNumberOfBytesWritten, nullptr);

			param->Statistics.DirCreatedCount++;
		}
		else {
			constexpr WCHAR szLogData[] = L"\r\n";
			WriteFile(param->hLogFile, szLogData, sizeof(szLogData) - sizeof(*szLogData), &dwNumberOfBytesWritten, nullptr);
		}
	}

	return TRUE;
}

BOOL CALLBACK OnLeavingDirectory(LPCWSTR lpPath, const WIN32_FIND_DATAW& findData, LPVOID lpParam) {
	const auto param = reinterpret_cast<CopyDataParam*>(lpParam);

	if (!IsCurrentOrParentDirectory(findData.cFileName)) {
		constexpr auto StrLen = UNICODE_STRING_MAX_CHARS;
		const unique_ptr<WCHAR> buf(new WCHAR[StrLen]);
		const auto str = buf.get();
		if (str == nullptr)
			return FALSE;

		wnsprintfW(str, StrLen, L"%ls%ls%ls", param->DataPath, PathSkipRootW(lpPath), findData.cFileName);

		SetFileTime(str, &findData.ftCreationTime, &findData.ftLastAccessTime, &findData.ftLastWriteTime);

		SetFileAttributesW(str, findData.dwFileAttributes);
	}

	return TRUE;
}

BOOL CALLBACK OnFileFound(LPCWSTR lpPath, const WIN32_FIND_DATAW& findData, LPVOID lpParam) {
	const auto param = reinterpret_cast<CopyDataParam*>(lpParam);

	const auto timePaused = GetTickCount64();
	while (param->State == State::Paused)
		;
	const auto timeNow = GetTickCount64();
	param->Time.PauseDuration += timeNow - timePaused;
	const auto maxDuration = GetFilterMaxDuration();
	if ((maxDuration && timeNow - param->Time.Start - param->Time.PauseDuration > maxDuration)
		|| param->State == State::Stopped)
		return FALSE;

	constexpr auto StrLen = UNICODE_STRING_MAX_CHARS;
	const unique_ptr<WCHAR> buf(new WCHAR[StrLen]);
	const auto str = buf.get();
	if (str == nullptr)
		return FALSE;

	wnsprintfW(str, StrLen, L"%ls%ls%ls", param->DataPath, PathSkipRootW(lpPath), findData.cFileName);

	WIN32_FIND_DATAW destinationFindData;
	const auto fileExists = FindClose(FindFirstFileW(str, &destinationFindData));
	if (FilterFile(findData, fileExists ? &destinationFindData : nullptr, *param)) {
		constexpr auto StrLen2 = UNICODE_STRING_MAX_CHARS;
		const unique_ptr<WCHAR> buf2(new WCHAR[StrLen2]);
		const auto str2 = buf2.get();
		if (str2 == nullptr)
			return FALSE;

		WCHAR szByteSize[20] = L"0 KB";
		StrFormatByteSizeEx(ULARGE_INTEGER{ findData.nFileSizeLow, findData.nFileSizeHigh }.QuadPart, SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS, szByteSize, ARRAYSIZE(szByteSize));

		wnsprintfW(str2, StrLen2, L"%ls\r\n(%ls)", findData.cFileName, szByteSize);
		MySetWindowText(param->Windows.hEditFile, str2);

		DWORD dwNumberOfBytesWritten;

		SYSTEMTIME localTime;
		GetLocalTime(&localTime);
		WriteFile(param->hLogFile, str2, sizeof(*str2) * wnsprintfW(str2, StrLen2, L"  %ls (%ls) " TIME_FORMAT, findData.cFileName, szByteSize, localTime.wYear, localTime.wMonth, localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond), &dwNumberOfBytesWritten, nullptr);

		if (fileExists) {
			WriteFile(param->hLogFile, L" ↑", 2, &dwNumberOfBytesWritten, nullptr);

			SetFileAttributesW(str, FILE_ATTRIBUTE_NORMAL);
		}

		wnsprintfW(str2, StrLen2, L"%ls%ls", lpPath, findData.cFileName);
		if (CopyFileExW(str2, str, CopyProgressRoutine, lpParam, &param->SkipCurrentFile, COPY_FILE_ALLOW_DECRYPTED_DESTINATION)) {
			constexpr WCHAR szLogData[] = L" ✓\r\n";
			WriteFile(param->hLogFile, szLogData, sizeof(szLogData) - sizeof(*szLogData), &dwNumberOfBytesWritten, nullptr);

			param->Statistics.FileCopiedCount++;
		}
		else {
			constexpr WCHAR szLogData[] = L"\r\n";
			WriteFile(param->hLogFile, szLogData, sizeof(szLogData) - sizeof(*szLogData), &dwNumberOfBytesWritten, nullptr);

			param->SkipCurrentFile = FALSE;
		}
	}

	return TRUE;
}

void StartCopying(CopyDataParamEx& param) {
	if (param.hThread == nullptr && (param.hThread = CreateThread(nullptr, 0, [](LPVOID lpParam) -> DWORD {
		const auto param = reinterpret_cast<CopyDataParam*>(lpParam);

		const auto driveLetter = static_cast<WCHAR>('A' + param->DriveNumber);

		if (driveLetter == *g_localDataPath)
			return ERROR_BAD_ARGUMENTS;

		if (param->DriveID) {
			auto& devBroadcastHandle = param->DevBroadcastHandle;
			devBroadcastHandle = { sizeof(devBroadcastHandle), DBT_DEVTYP_HANDLE };
			if ((devBroadcastHandle.dbch_handle = CreateFileW(initializer_list<WCHAR>({ '\\', '\\', '.', '\\', driveLetter, ':', 0 }).begin(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr)) != INVALID_HANDLE_VALUE) {
				if ((devBroadcastHandle.dbch_hdevnotify = RegisterDeviceNotification(g_hDlg, &devBroadcastHandle, DEVICE_NOTIFY_WINDOW_HANDLE)) != nullptr) {
					WCHAR szDriveID[11];
					wsprintfW(szDriveID, L"0x%lX", param->DriveID);

					WCHAR szDataPath[40], szLogPath[60];
					wsprintfW(szDataPath, LR"(\\?\%ls%ls\)", g_localDataPath, szDriveID);
					lstrcpyW(szLogPath, szDataPath);
					lstrcatW(szDataPath, LR"(Data\)");
					lstrcatW(szLogPath, LR"(Logs\)");

					SHCreateDirectory(nullptr, szDataPath);
					SHCreateDirectory(nullptr, szLogPath);

					SetFileAttributesW(g_localDataPath, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);

					SYSTEMTIME localTime;
					GetLocalTime(&localTime);
					wsprintfW(szLogPath, L"%ls%hu-%02hu-%02hu %02hu-%02hu-%02hu.log", szLogPath, localTime.wYear, localTime.wMonth, localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond);
					if ((param->hLogFile = CreateFileW(szLogPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)) != INVALID_HANDLE_VALUE) {
						auto& statistics = param->Statistics;
						statistics = {};

						WCHAR szLogData[120];

						DWORD dwNumberOfBytesWritten;

						WriteFile(param->hLogFile, "\xff\xfe", 2, &dwNumberOfBytesWritten, nullptr);
						WriteFile(param->hLogFile, szLogData, sizeof(*szLogData) * wsprintfW(szLogData, L"Drive %ls\r\n", szDriveID), &dwNumberOfBytesWritten, nullptr);

						param->DataPath = szDataPath;
						param->SkipCurrentFile = FALSE;
						param->Time = {};
						param->State = State::Running;

						FindFiles(initializer_list<WCHAR>({ '\\', '\\', '?', '\\', driveLetter, ':', '\\', 0 }).begin(), reinterpret_cast<LPBOOL>(&param->State), OnFileFound, OnEnteringDirectory, OnLeavingDirectory, lpParam);

						WCHAR szByteSize[20] = L"0 KB";
						StrFormatByteSizeEx(statistics.SizeCopied, SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS, szByteSize, ARRAYSIZE(szByteSize));

						WriteFile(param->hLogFile, szLogData, sizeof(*szLogData) * wsprintfW(szLogData, L"\r\nSuccessfully created %lu %ls, copied %lu %ls (%ls)", statistics.DirCreatedCount, statistics.DirCreatedCount == 1 ? L"directory" : L"directories", statistics.FileCopiedCount, statistics.FileCopiedCount == 1 ? L"file" : L"files", szByteSize), &dwNumberOfBytesWritten, nullptr);

						param->State = State::Stopped;

						CloseHandle(param->hLogFile);

						if (!statistics.DirCreatedCount && !statistics.FileCopiedCount)
							DeleteFileW(szLogPath);
					}

					UnregisterDeviceNotification(devBroadcastHandle.dbch_hdevnotify);
				}

				CloseHandle(devBroadcastHandle.dbch_handle);
			}
		}

		return ERROR_SUCCESS;
		}, &param, CREATE_SUSPENDED, nullptr)) != nullptr) {
		const HANDLE hThread = CreateThread(nullptr, 0, [](LPVOID lpParam) -> DWORD {
			const auto param = reinterpret_cast<CopyDataParamEx*>(lpParam);

			const auto& windows = param->Windows;

			SetWindowLongPtr(windows.hButtonStartCopying, GWLP_ID, IDC_BUTTON_PAUSE_COPYING);

			EnableWindow(windows.hButtonStopCopying, TRUE);

			EnableWindow(windows.hButtonSkipCurrentFile, TRUE);

			WaitForSingleObject(param->hThread, INFINITE);
			CloseHandle(param->hThread);
			param->hThread = nullptr;

			SetWindowLongPtr(windows.hButtonStartCopying, GWLP_ID, IDC_BUTTON_START_COPYING);

			EnableWindow(windows.hButtonStopCopying, FALSE);

			EnableWindow(windows.hButtonSkipCurrentFile, FALSE);

			PSTMSG(windows.hProgressCopiedData, PBM_SETSTATE, PBST_NORMAL, 0);
			PSTMSG(windows.hProgressCopiedData, PBM_SETPOS, 100, 0);

			SetWindowTextW(windows.hEditDir, L"[Directory: Completed]");

			SetWindowTextW(windows.hEditFile, L"[File: Completed]");

			return ERROR_SUCCESS;
			}, &param, 0, nullptr);
		if (hThread == nullptr)
#pragma warning(suppress: 6258)
			TerminateThread(param.hThread, GetLastError());
		else {
			CloseHandle(hThread);

			ResumeThread(param.hThread);
		}
	}
}

void ResumeCopying(CopyDataParamEx& param) {
	param.State = State::Running;

	const auto& windows = param.Windows;

	SetWindowLongPtr(windows.hButtonStartCopying, GWLP_ID, IDC_BUTTON_PAUSE_COPYING);

	EnableWindow(windows.hButtonSkipCurrentFile, TRUE);

	PSTMSG(windows.hProgressCopiedData, PBM_SETSTATE, PBST_NORMAL, 0);
}

void PauseCopying(CopyDataParamEx& param) {
	param.State = State::Paused;

	const auto& windows = param.Windows;

	SetWindowLongPtr(windows.hButtonStartCopying, GWLP_ID, IDC_BUTTON_RESUME_COPYING);

	EnableWindow(windows.hButtonSkipCurrentFile, FALSE);

	PSTMSG(windows.hProgressCopiedData, PBM_SETSTATE, PBST_PAUSED, 0);
}

void StopCopying(CopyDataParamEx& param) { param.State = State::Stopped; }

void AddTooltips(const Windows& windows) {
	const struct {
		HWND Window;
		LPCWSTR Text;
	} tooltipData[]{
		{ windows.hButtonStartCopying, L"Start/pause copying" },
		{ windows.hButtonStopCopying, L"Stop copying" },
		{ windows.hButtonSkipCurrentFile, L"Skip current file" }
	};
	for (const auto data : tooltipData) {
		const auto tooltip = CreateWindowW(TOOLTIPS_CLASSW, nullptr, WS_POPUP, 0, 0,
			0, 0, g_hDlg, nullptr, GetModuleHandle(nullptr), nullptr);
		if (tooltip != nullptr) {
			TOOLINFOW toolInfo;
			toolInfo.cbSize = sizeof(toolInfo);
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
			toolInfo.uId = reinterpret_cast<UINT_PTR>(data.Window);
			toolInfo.lpszText = LPWSTR(data.Text);
			SNDMSG(tooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
		}
	}
}

void CopyWindows(Windows& windows) {
	for (int i = 0; i < ARRAYSIZE(g_windows->Array); i++) {
		const auto g_hWnd = g_windows->Array[i];

		TCHAR szClassName[20], szWindowText[20];
		GetClassName(g_hWnd, szClassName, ARRAYSIZE(szClassName));
		GetWindowText(g_hWnd, szWindowText, ARRAYSIZE(szWindowText));

		const auto parent = GetParent(g_hWnd);

		RECT windowRect;
		GetWindowRect(g_hWnd, &windowRect);
		MapWindowRect(HWND_DESKTOP, parent, &windowRect);

		windows.Array[i] = CreateWindowEx(GetWindowExStyle(g_hWnd), szClassName, szWindowText, WS_VISIBLE | GetWindowStyle(g_hWnd), static_cast<int>(windowRect.left), static_cast<int>(windowRect.top), static_cast<int>(windowRect.right - windowRect.left), static_cast<int>(windowRect.bottom - windowRect.top), parent, GetMenu(g_hWnd), GetWindowInstance(g_hWnd), nullptr);

		SetWindowFont(windows.Array[i], GetWindowFont(g_hWnd), FALSE);
	}
}

INT_PTR CALLBACK MainWindowProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HFONT s_hFont;

	static HANDLE s_hHistoryLogFile = INVALID_HANDLE_VALUE;

	static int s_selectedDriveNumber;

	static CopyDataParamEx s_copyDataParamEx[26];

	switch (uMsg) {
	case WM_INITDIALOG: {
		if (!GetModuleFileNameW(nullptr, g_exePath, ARRAYSIZE(g_exePath)) || (g_myAppData = new MyAppData()) == nullptr) {
			DestroyWindow(hDlg);
			break;
		}

		if (!g_myAppData->Filter.Load()) {
			const auto lastError = GetLastError();
			if (lastError == ERROR_FILE_NOT_FOUND || lastError == ERROR_INVALID_DATA)
				g_myAppData->Filter.Save();
		}

		s_selectedDriveNumber = PathGetDriveNumberW(g_exePath);

		*g_localDataPath = 'A' + s_selectedDriveNumber;

		g_hDlg = hDlg;

		const auto comboDrives = GetDlgItem(hDlg, IDC_COMBO_DRIVES);
		int selectedIndex;
		for (int i = 0; i < ARRAYSIZE(s_copyDataParamEx); i++) {
			s_copyDataParamEx[i].DriveNumber = i;

			s_copyDataParamEx[i].DriveID = GenerateDriveID(i);

			const auto driveLetter = static_cast<TCHAR>('A' + i);

			int index = CB_ERR;
			if (i == s_selectedDriveNumber)
				selectedIndex = index = ComboBox_AddString(comboDrives, initializer_list<TCHAR>({ driveLetter, ':', ' ', '*', 0 }).begin());
			else {
				const TCHAR szDrive[]{ driveLetter, ':', 0 };
				if (GetDiskFreeSpaceEx(szDrive, nullptr, nullptr, nullptr))
					index = ComboBox_AddString(comboDrives, szDrive);
			}
			if (index != CB_ERR)
				ComboBox_SetItemData(comboDrives, index, i);
		}
		ComboBox_SetCurSel(comboDrives, selectedIndex);

		g_windows = &s_copyDataParamEx[s_selectedDriveNumber].Windows;

		g_windows->hButtonStartCopying = GetDlgItem(hDlg, IDC_BUTTON_START_COPYING);
		g_windows->hButtonStopCopying = GetDlgItem(hDlg, IDC_BUTTON_STOP_COPYING);
		g_windows->hButtonSkipCurrentFile = GetDlgItem(hDlg, IDC_BUTTON_SKIP_CURRENT_FILE);
		g_windows->hProgressCopiedData = GetDlgItem(hDlg, IDC_PROGRESS_COPIED_DATA);

		g_windows->hEditDir = GetDlgItem(hDlg, IDC_EDIT_DIR);
		g_windows->hEditFile = GetDlgItem(hDlg, IDC_EDIT_FILE);
		LOGFONT logFont;
		if (GetObject(GetWindowFont(hDlg), sizeof(logFont), &logFont)) {
			logFont.lfHeight = logFont.lfHeight * 13 / 16;
			if ((s_hFont = CreateFontIndirect(&logFont)) != nullptr) {
				SetWindowFont(g_windows->hEditDir, s_hFont, FALSE);
				SetWindowFont(g_windows->hEditFile, s_hFont, FALSE);
			}
		}
		SetWindowTextW(g_windows->hEditDir, L"[Directory: N/A]");
		SetWindowTextW(g_windows->hEditFile, L"[File: N/A]");

		CheckMenuItem(GetMenu(hDlg), ID_OPTIONS_RUN_AT_STARTUP, RunAtStartup(RegistryOperation::QueryValue, AppName, g_exePath, nullptr, FALSE) == ERROR_SUCCESS ? MF_CHECKED : MF_UNCHECKED);
	}	break;

	case static_cast<UINT>(MyWindowMessage::SetTextW): {
		const auto text = reinterpret_cast<LPWSTR>(lParam);

		SetWindowTextW(reinterpret_cast<HWND>(wParam), text);

		LocalFree(text);
	}	break;

	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case ID_TOOLS_FILTER: {
			const auto instance = GetModuleHandle(nullptr);

			PROPSHEETPAGE propSheetPage;
			propSheetPage.dwSize = sizeof(propSheetPage);
			propSheetPage.dwFlags = PSP_DEFAULT;
			propSheetPage.hInstance = instance;
			propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_FILTER);
			propSheetPage.pfnDlgProc = FilterPropSheetPageProc;
			propSheetPage.lParam = reinterpret_cast<LPARAM>(g_myAppData);

			PROPSHEETHEADER propSheetHeader;
			propSheetHeader.dwSize = sizeof(propSheetHeader);
			propSheetHeader.dwFlags = PSH_PROPSHEETPAGE | PSH_NOCONTEXTHELP;
			propSheetHeader.hInstance = instance;
			propSheetHeader.hwndParent = hDlg;
			propSheetHeader.pszCaption = TEXT("Filter");
			propSheetHeader.ppsp = &propSheetPage;
			propSheetHeader.nPages = 1;
			PropertySheet(&propSheetHeader);
		}	break;

		case ID_OPTIONS_OPENDATADIR: {
			SHCreateDirectory(nullptr, g_localDataPath);

			SetFileAttributesW(g_localDataPath, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);

			ShellExecuteW(nullptr, L"explore", g_localDataPath, nullptr, nullptr, SW_SHOW);
		}	break;

		case ID_OPTIONS_RUN_IN_BACKGROUND: ShowWindow(hDlg, SW_HIDE); break;

		case ID_OPTIONS_RUN_AT_STARTUP: {
			const auto menu = GetMenu(hDlg);
			const auto menuState = GetMenuState(menu, ID_OPTIONS_RUN_AT_STARTUP, MF_BYCOMMAND);
			if (RunAtStartup(menuState == MF_CHECKED ? RegistryOperation::DeleteValue : RegistryOperation::SetValue, AppName, g_exePath, Args.Background, FALSE) == ERROR_SUCCESS)
				CheckMenuItem(menu, ID_OPTIONS_RUN_AT_STARTUP, menuState == MF_CHECKED ? MF_UNCHECKED : MF_CHECKED);
		}	break;

		case ID_HELP_ABOUT: DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hDlg, AboutDialogProc); break;

		case IDC_COMBO_DRIVES: {
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				const auto comboDrives = reinterpret_cast<HWND>(lParam);

				const auto driveNumber = static_cast<int>(ComboBox_GetItemData(comboDrives, ComboBox_GetCurSel(comboDrives)));

				if (driveNumber == CB_ERR || s_selectedDriveNumber == driveNumber)
					break;

				auto& param = s_copyDataParamEx[driveNumber];

				auto& windows = param.Windows;

				if (windows.hButtonStartCopying == nullptr) {
					CopyWindows(windows);

					if (windows.hButtonStartCopying == nullptr)
						for (auto& window : windows.Array) {
							DestroyWindow(window);
							window = nullptr;
						}
					else {
						EnableWindow(windows.hButtonStartCopying, TRUE);

						AddTooltips(windows);
					}
				}
				else
					for (const auto window : windows.Array)
						ShowWindow(window, SW_SHOW);

				for (auto& window : s_copyDataParamEx[s_selectedDriveNumber].Windows.Array)
					if (window != nullptr)
						ShowWindow(window, SW_HIDE);

				s_selectedDriveNumber = driveNumber;
			}
		}	break;

		case IDC_BUTTON_START_COPYING: StartCopying(s_copyDataParamEx[s_selectedDriveNumber]); break;

		case IDC_BUTTON_RESUME_COPYING: ResumeCopying(s_copyDataParamEx[s_selectedDriveNumber]); break;

		case IDC_BUTTON_PAUSE_COPYING: PauseCopying(s_copyDataParamEx[s_selectedDriveNumber]); break;

		case IDC_BUTTON_STOP_COPYING: StopCopying(s_copyDataParamEx[s_selectedDriveNumber]); break;

		case IDC_BUTTON_SKIP_CURRENT_FILE: s_copyDataParamEx[s_selectedDriveNumber].SkipCurrentFile = TRUE; break;
		}
	}	break;

	case WM_DEVICECHANGE: {
		switch (wParam) {
		case DBT_DEVICEARRIVAL:
		case DBT_DEVICEREMOVECOMPLETE: {
			if (reinterpret_cast<PDEV_BROADCAST_HDR>(lParam)->dbch_devicetype == DBT_DEVTYP_VOLUME) {
				const auto driveNumber = GetFirstDriveNumber(reinterpret_cast<PDEV_BROADCAST_VOLUME>(lParam)->dbcv_unitmask);

				const TCHAR szDrive[]{ static_cast<TCHAR>('A' + driveNumber), ':', 0 };

				const auto comboDrives = GetDlgItem(hDlg, IDC_COMBO_DRIVES);

				auto& param = s_copyDataParamEx[driveNumber];

				WCHAR szLogPath[30];

				if (s_hHistoryLogFile == INVALID_HANDLE_VALUE) {
					SHCreateDirectory(nullptr, g_localDataPath);

					SetFileAttributesW(g_localDataPath, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);

					wsprintfW(szLogPath, L"%lsHistory.log", g_localDataPath);

					SetFileAttributesW(szLogPath, FILE_ATTRIBUTE_NORMAL);

					s_hHistoryLogFile = CreateFileW(szLogPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

					SetFilePointer(s_hHistoryLogFile, 0, nullptr, FILE_END);
				}

				SYSTEMTIME localTime;
				GetLocalTime(&localTime);

				DWORD dwNumberOfBytesWritten;

				WCHAR szLogData[60];

				if (wParam == DBT_DEVICEARRIVAL) {
					if (GetDiskFreeSpaceEx(szDrive, nullptr, nullptr, nullptr) && (param.DriveID = GenerateDriveID(driveNumber))) {
						WriteFile(s_hHistoryLogFile, szLogData, sizeof(*szLogData) * wsprintfW(szLogData, TIME_FORMAT L" Drive 0x%lX connected\r\n", localTime.wYear, localTime.wMonth, localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond, param.DriveID), &dwNumberOfBytesWritten, nullptr);

						BOOL ret = TRUE;
						if (driveNumber == s_selectedDriveNumber)
							EnableWindow(param.Windows.hButtonStartCopying, TRUE);
						else {
							const auto index = ComboBox_AddString(comboDrives, szDrive);
							ret = index != CB_ERR && ComboBox_SetItemData(comboDrives, index, driveNumber) != CB_ERR;
						}

						if (ret) {
							if (IsWindowVisible(hDlg))
								FlashWindow(hDlg, TRUE);
							else if (DriveType(driveNumber) != DRIVE_CDROM) {
								const auto& whitelist = g_myAppData->Filter.Whitelist;

								BOOL excluded{};
								for (UINT i = 0; i < whitelist.DriveCount; i++)
									if (whitelist.Drives[i].ID == param.DriveID && whitelist.Drives[i].Excluded) {
										excluded = TRUE;
										break;
									}

								if (!excluded) {
									WCHAR path[3 + ARRAYSIZE(whitelist.SpecialFileName)];
									wsprintfW(path, LR"(%lc:\%ls)", static_cast<WCHAR>('A' + driveNumber), whitelist.SpecialFileName);
									if (!PathFileExistsW(path))
										StartCopying(param);
								}
							}
						}
					}
				}
				else {
					if (driveNumber == s_selectedDriveNumber)
						EnableWindow(param.Windows.hButtonStartCopying, FALSE);
					else if (param.DriveID) {
						ComboBox_DeleteString(comboDrives, ComboBox_FindString(comboDrives, -1, szDrive));

						for (auto& window : s_copyDataParamEx[driveNumber].Windows.Array)
							if (window != nullptr) {
								DestroyWindow(window);
								window = nullptr;
							}

						WriteFile(s_hHistoryLogFile, szLogData, sizeof(*szLogData) * wsprintfW(szLogData, TIME_FORMAT L" Drive 0x%lX disconnected\r\n", localTime.wYear, localTime.wMonth, localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond, param.DriveID), &dwNumberOfBytesWritten, nullptr);

						param.DriveID = 0;
					}
				}
			}
		}	break;

		case DBT_DEVICEQUERYREMOVE: {
			if (reinterpret_cast<PDEV_BROADCAST_HDR>(lParam)->dbch_devicetype == DBT_DEVTYP_HANDLE)
				for (auto& param : s_copyDataParamEx)
					if (param.DevBroadcastHandle.dbch_handle == reinterpret_cast<PDEV_BROADCAST_HANDLE>(lParam)->dbch_handle) {
						StopCopying(param);

						MsgWaitForMultipleObjects(1, &param.hThread, TRUE, INFINITE, QS_ALLINPUT);
					}
		}	return TRUE;
		}
	}	break;

	case WM_CTLCOLORSTATIC: SetTextColor(reinterpret_cast<HDC>(wParam), RGB(0, 150, 136)); return reinterpret_cast<INT_PTR>(GetStockBrush(WHITE_BRUSH));

	case WM_CLOSE: {
		bool copying{};
		for (const auto& param : s_copyDataParamEx)
			if (param.State == State::Running) {
				copying = true;

				break;
			}

		if (!copying || MessageBoxW(hDlg, L"Are you sure you want to exit?", L"Confirm", MB_ICONQUESTION | MB_YESNO) == IDYES)
			DestroyWindow(hDlg);
	}	break;

	case WM_DESTROY: {
		for (auto& param : s_copyDataParamEx) {
			StopCopying(param);

			MsgWaitForMultipleObjects(1, &param.hThread, TRUE, INFINITE, QS_ALLINPUT);
		}

		delete g_myAppData;

		CloseHandle(s_hHistoryLogFile);

		DeleteFont(s_hFont);

		PostQuitMessage(ERROR_SUCCESS);
	}	break;
	}

	return FALSE;
}