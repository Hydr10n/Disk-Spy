/*
Source File: Disk Spy.c
Last Update: 2018/09/12
Minimum Supported Client: Microsoft Windows Vista [Desktop Only]

This project is hosted on https://github.com/Programmer-YangXun/Disk-Spy/
Copyright (C) 2016 - 2018 Programmer-Yang_Xun@outlook.com. All Rights Reserved.
*/

#pragma region Development Environment Check and Settings
#ifndef _MSC_VER
#error Microsoft Visual Studio IDE is required
#elif _MSC_VER < 1700
#error Microsoft Visual Studio 2012 or higher version IDE is required (2017 is recommended)
#endif
#ifdef __cplusplus
#error This source code must be compiled as a C Programming Language code. Try to rename the source file as "Disk Spy.c"
#endif
#pragma warning(disable:4091)
#pragma warning(disable:4311)
#pragma warning(disable:6011)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma endregion

#pragma region Header Files, Library Files, Manifest Files
#include <Windows.h>
#include <Dbt.h>
#include <Setupapi.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <Shlwapi.h>
#include <WinInet.h>
#include <CommCtrl.h>
#include <Richedit.h>
#include <Uxtheme.h>
#include <windowsx.h>
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "ComCtl32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Wininet.lib")
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifdef _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#pragma endregion

#pragma region Definitions of Macros and Variables, Declarations of Functions
#define MAX_DRIVE_NUMBER 26
#define MAX_UNICODE_PATH UNICODE_STRING_MAX_CHARS
#define STRING_TIME_FORMATW L"%hu/%02hu/%02hu %02hu:%02hu:%02hu"
#define MAKEDWORD(HIGH_WORD, LOW_WORD) ((DWORD)((((DWORD)(HIGH_WORD)) << 16) | (DWORD)(LOW_WORD)))
#define MAKEDWORD64(HIGH_DWORD, LOW_DWORD) ((DWORD64)((((DWORD64)(HIGH_DWORD)) << 32) | (DWORD64)(LOW_DWORD)))
#define ScaleX(iX, iCurrentDPI_X) MulDiv(iX, iCurrentDPI_X, USER_DEFAULT_SCREEN_DPI)
#define ScaleY(iY, iCurrentDPI_Y) MulDiv(iY, iCurrentDPI_Y, USER_DEFAULT_SCREEN_DPI)
#define DPIAware_CreateWindowExW(iCurrentDPI_X, iCurrentDPI_Y, dwExStyle, lpClassName, lpWindowName, dwStyle, iX, iY, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam) CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, ScaleX(iX, iCurrentDPI_X), ScaleY(iY, iCurrentDPI_Y), ScaleX(nWidth, iCurrentDPI_X), ScaleY(nHeight, iCurrentDPI_Y), hWndParent, hMenu, hInstance, lpParam)
enum AppMessages {
	AppMessage_ContinueCopy = WM_APP,
	AppMessage_PauseCopy
};
enum Identifiers {
	Identifier_Button_ApplyAndSaveCurrentSettings,
	Identifier_Button_StopCopy,
	Identifier_Button_DisableRunAtStartup,
	Identifier_Button_EnableRunAtStartup,
	Identifier_Button_BrowseStorageDirectory,
	Identifier_Button_Help,
	Identifier_Button_HideUserInterface,
	Identifier_Button_PauseCopy,
	Identifier_Button_ContinueCopy,
	Identifier_Button_RunAsAdministrator,
	Identifier_Button_SkipCurrentFile,
	Identifier_Button_StartCopy,
	Identifier_CheckBox_LimitCopyTime,
	Identifier_CheckBox_LimitFileSize,
	Identifier_ComboBox_SelectDrive,
	Identifier_ComboBox_SelectFileSizeUnit,
	Identifier_ComboBox_SelectOption,
	Identifier_ComboBox_SelectTimeUnit,
	Identifier_Edit_LimitCopyTime,
	Identifier_Edit_LimitFileSize,
	Identifier_ListView_DrivesList,
	Identifier_Menu_Delete,
	Identifier_Menu_Exclude,
	Identifier_Menu_Resume,
	Identifier_RichEdit_Copyright,
	Identifier_RichEdit_OutputDirectoryName,
	Identifier_RichEdit_OutputFileName
};
enum ListViewColumns {
	ListView_Column1_Drives,
	ListView_Column2_Letter,
	ListView_Column3_TotalSpace,
	ListView_Column4_Status,
	ListView_Column5_InitialTime,
	ListView_NumberOfColumns
};
enum RegistryOperationFlags {
	RegistryOperationFlag_DeleteValue,
	RegistryOperationFlag_QueryValue,
	RegistryOperationFlag_SetValue
};
enum TabControlPages {
	TabControl_Page_Home,
	TabControl_Page_Progress,
	TabControl_Page_Filters,
	TabControl_Page_About,
	TabControl_NumberOfPages
};
enum WindowEvents
{
	WindowEvent_OutputDirectoryName,
	WindowEvent_OutputFileName,
	WindowEvent_SetProgressBarPos
};
BYTE byteIsDirectoryCreated, byteIsFileCopied, byteLocalDriveIndex, byteSelectedDriveIndexes[2];
INT iCurrentDPI_X = USER_DEFAULT_SCREEN_DPI, iCurrentDPI_Y = USER_DEFAULT_SCREEN_DPI;
DWORD dwNumberOfBytesRead, dwNumberOfBytesWritten;
LPCWSTR lpcwProjectName = L"Disk Spy";
WCHAR szProgramFileName[MAX_PATH], szStorageDirectoryName[] = L"\0:\\Data\\";
HANDLE hFile_DrivesDat, hFile_DataCopyDetailsLog,
hThread_CopyData, hThread_Timer;
HFONT hFonts[2];
HWND hWnd_Button_ApplyAndSaveCurrentSettings,
hWnd_CheckBox_LimitCopyTime, hWnd_CheckBox_LimitFileSize,
hWnd_ComboBox_SelectFileSizeUnit, hWnd_ComboBox_SelectTimeUnit,
hWnd_Edit_LimitCopyTime, hWnd_Edit_LimitFileSize,
hWnd_ListView_DrivesList,
hWnd_TabControl_CurrentPage;
PROCESS_INFORMATION ProcessesInformation[MAX_DRIVE_NUMBER];
MSG msg;
STARTUPINFOW StartupInfo;
WNDCLASSEXW WndClass_MainWindow = { sizeof(WndClass_MainWindow) };
struct {
	HWND hWnd_Button_ContinueCopy, hWnd_Button_PauseCopy, hWnd_Button_SkipCurrentFile, hWnd_Button_StopCopy,
		hWnd_ProgressBar,
		hWnd_RichEdit_OutputDirectoryName, hWnd_RichEdit_OutputFileName;
	PROCESS_INFORMATION ProcessInformation;
} ProgressData[MAX_DRIVE_NUMBER];
typedef struct _DRIVEINFO {
	BYTE byteIsExcluded;
	DWORD dwDriveIdentifier;
	DWORD64 dw64TotalSpace;
	SYSTEMTIME InitialTime;
} DRIVEINFO, *PDRIVEINFO;
struct _SHAREDDATA {
	BYTE byteCopyData[MAX_DRIVE_NUMBER];
	BOOL bCancelCopy[MAX_DRIVE_NUMBER];
	DWORD dwDriveIdentifier[MAX_DRIVE_NUMBER], dwMaxCopyTime[MAX_DRIVE_NUMBER];
	DWORD64 dw64MaxFileSize[MAX_DRIVE_NUMBER];
	HWND hWnd_MainWindows[MAX_DRIVE_NUMBER];
} *pSharedData;
BOOL GetFileProductVersion(LPCWSTR lpcwFileName, LPWSTR lpwFileProductVersionBuffer, DWORD cchFileProductVersionBuffer);
DWORD GetEnhancedVolumeSerialNumberCRC32(WCHAR wchDriveLetter, DWORD dwVolumeSerialNumber);
DWORD CALLBACK CopyProgressRoutine(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber, DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData);
DWORD WINAPI Thread_CheckForUpdate(LPVOID lpvParam);
DWORD WINAPI Thread_CopyData(LPVOID lpvParam);
DWORD WINAPI Thread_Timer(LPVOID lpvParam);
DWORD WINAPI Thread_WaitForSingleProcess(LPVOID lpvParam);
INT CALLBACK ListViewSortCompareItemsEx(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
LRESULT CALLBACK WndProc_MessageOnly(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc_Page_Home(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc_Page_Progress(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc_Page_Filters(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc_Page_About(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LSTATUS RunAtStartup(BYTE byteParam);
VOID DrawComboBox(LPDRAWITEMSTRUCT lpDrawItemStruct);
VOID EnableSpecifiedChildWindows(BOOL bEnable);
VOID ProcessDataCopyDetailsLogFile(VOID);
VOID SendMessage_WM_COPYDATA(ULONG_PTR dwData, DWORD cbData, PVOID lpData);
VOID ShowSpecifiedChildWindows(BYTE byteIndex, INT iShowWindow);
#pragma endregion

#pragma region Functions
#ifdef _DEBUG
INT WINAPI wWinMain(HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
#else
#pragma comment(linker, "/ENTRY:wWinMainStartup")
VOID wWinMainStartup(VOID)
#endif
{
	GetModuleFileNameW(NULL, szProgramFileName, _countof(szProgramFileName));
	byteLocalDriveIndex = (*szStorageDirectoryName = (*szProgramFileName) &= 0xdf) - L'A';
	GetStartupInfoW(&StartupInfo);
	StartupInfo.wShowWindow = (StartupInfo.dwFlags & STARTF_USESHOWWINDOW) ? (StartupInfo.wShowWindow) : (SW_SHOWDEFAULT);
	if (!StrCmpIW(PathGetArgsW(GetCommandLineW()), L"-HUI"))
	{
		StartupInfo.wShowWindow = SW_HIDE;
	}
	HANDLE hFileMap_SharedData = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct _SHAREDDATA), L"SharedDataOfDiskSpy");
	if (hFileMap_SharedData == NULL || (pSharedData = (struct _SHAREDDATA*)MapViewOfFile(hFileMap_SharedData, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0)) == NULL)
	{
		ExitProcess(0);
	}
	BYTE byteIsMessageOnlyWindow = FALSE;
	for (; byteSelectedDriveIndexes[0] < MAX_DRIVE_NUMBER; byteSelectedDriveIndexes[0]++)
	{
		if (pSharedData->byteCopyData[byteSelectedDriveIndexes[0]] && !IsWindow(pSharedData->hWnd_MainWindows[byteSelectedDriveIndexes[0]]))
		{
			pSharedData->byteCopyData[byteSelectedDriveIndexes[0]] = 0;
			byteIsMessageOnlyWindow = TRUE;
			break;
		}
	}
	WndClass_MainWindow.lpszClassName = (byteIsMessageOnlyWindow) ? (L"MsgOnly") : (L"Win32");
	INT iMainWindowWidth = 448, iMainWindowHeight = 183;
	if (!byteIsMessageOnlyWindow)
	{
		HANDLE hMutex = CreateMutexW(NULL, FALSE, L"MutexForDiskSpy");
		if (GetLastError() == ERROR_ALREADY_EXISTS && (IsWindow(pSharedData->hWnd_MainWindows[byteLocalDriveIndex]) || (pSharedData->hWnd_MainWindows[byteLocalDriveIndex] = FindWindowW(WndClass_MainWindow.lpszClassName, lpcwProjectName)) != NULL))
		{
			ShowWindow(pSharedData->hWnd_MainWindows[byteLocalDriveIndex], SW_SHOW);
			ExitProcess(0);
		}
		byteSelectedDriveIndexes[0] = byteLocalDriveIndex;
		byteSelectedDriveIndexes[1] = MAX_DRIVE_NUMBER;
		HDC hDC = GetDC(NULL);
		if (hDC)
		{
			SetProcessDPIAware();
			iCurrentDPI_X = GetDeviceCaps(hDC, LOGPIXELSX);
			iCurrentDPI_Y = GetDeviceCaps(hDC, LOGPIXELSY);
			iMainWindowWidth = (iCurrentDPI_X == USER_DEFAULT_SCREEN_DPI) ? (iMainWindowWidth) : ScaleX(iMainWindowWidth - 2, iCurrentDPI_X);
			iMainWindowHeight = (iCurrentDPI_Y == USER_DEFAULT_SCREEN_DPI) ? (iMainWindowHeight) : ScaleY(iMainWindowHeight - 2, iCurrentDPI_Y);
			ReleaseDC(NULL, hDC);
		}
	}
	WndClass_MainWindow.lpfnWndProc = (byteIsMessageOnlyWindow) ? (WndProc_MessageOnly) : (WndProc_Page_Home);
	WndClass_MainWindow.hInstance = GetModuleHandleW(NULL);
	WndClass_MainWindow.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass_MainWindow.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	RegisterClassExW(&WndClass_MainWindow);
	pSharedData->hWnd_MainWindows[byteSelectedDriveIndexes[0]] = CreateWindowExW(0,
		WndClass_MainWindow.lpszClassName, lpcwProjectName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		(GetSystemMetrics(SM_CXSCREEN) - iMainWindowWidth) / 2, (GetSystemMetrics(SM_CYSCREEN) - iMainWindowHeight) / 2,
		iMainWindowWidth, iMainWindowHeight,
		(byteIsMessageOnlyWindow) ? (HWND_MESSAGE) : (NULL), NULL, NULL, NULL);
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		if (!IsDialogMessageW(hWnd_TabControl_CurrentPage, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	ExitProcess(0);
}

LRESULT CALLBACK WndProc_Page_Home(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static LPCWSTR lpcwAppName = L"Disable Automatic Data Copy if Following Settings Applied", lpcwKeyName = L"SpecifiedFileExistingInRootPath", lpcwDefaultSpecifiedFileName = L"0.0";
	static HANDLE hJob;
	static HWND hWnd_Button_StartCopy,
		hWnd_ComboBox_SelectOption,
		hWnd_TabControl,
		hWnd_ToolTip,
		hWnd_ComboBox_SelectDrive[2];
	static CHARFORMATW CharFormat = { sizeof(CharFormat) };
	static TTTOOLINFOW ToolTipInfo = { sizeof(ToolTipInfo) };
	static struct _SETTINGSDATA {
		BYTE byteIsNormalExit, byteFileSizeUnit[MAX_DRIVE_NUMBER], byteLimitCopyTime[MAX_DRIVE_NUMBER], byteLimitFileSize[MAX_DRIVE_NUMBER], byteTimeUnit[MAX_DRIVE_NUMBER];
		WORD wMaxCopyTime[MAX_DRIVE_NUMBER], wMaxFileSize[MAX_DRIVE_NUMBER];
	} *pSettingsData;
	static struct {
		LPWSTR lpwTitle;
		HWND hWnd;
		WNDPROC WndProc;
	} TabControlPagesData[TabControl_NumberOfPages] =
	{
		{ L"Home", NULL, WndProc_Page_Home },
		{ L"Progress", NULL, WndProc_Page_Progress },
		{ L"Filters", NULL, WndProc_Page_Filters },
		{ L"About", NULL, WndProc_Page_About }
	};
	switch (uMsg)
	{
	case WM_CREATE:
	{
		LPWCH lpwchFound = StrRChrW(szProgramFileName, NULL, L'\\');
		WCHAR szBytesSize[11], szCurrentDirectory[MAX_PATH], szDriveInfo[10], szTime[30];
		SYSTEMTIME LocalTime;
		DWORD dwLastErrorCodes[2];
		*lpwchFound = 0;
		wnsprintfW(szCurrentDirectory, _countof(szCurrentDirectory), L"%ls\\Config.ini", szProgramFileName);
		if (!PathFileExistsW(szCurrentDirectory))
		{
			WritePrivateProfileStringW(lpcwAppName, lpcwKeyName, lpcwDefaultSpecifiedFileName, szCurrentDirectory);
		}
		GetCurrentDirectoryW(_countof(szCurrentDirectory), szCurrentDirectory);
		SetCurrentDirectoryW(szProgramFileName);
		*lpwchFound = L'\\';
		hFile_DrivesDat = CreateFileW(L"Drives.dat", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		dwLastErrorCodes[0] = GetLastError();
		HANDLE hFile_SettingsDat = CreateFileW(L"Settings.dat", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		dwLastErrorCodes[1] = GetLastError();
		if (hFile_DrivesDat == INVALID_HANDLE_VALUE || hFile_SettingsDat == INVALID_HANDLE_VALUE)
		{
			ExitProcess(0);
		}
		HANDLE hFileMap_SettingsDat = CreateFileMappingW(hFile_SettingsDat, NULL, PAGE_READWRITE, 0, sizeof(struct _SETTINGSDATA), NULL);
		if (hFileMap_SettingsDat == NULL || (pSettingsData = (struct _SETTINGSDATA*)MapViewOfFile(hFileMap_SettingsDat, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0)) == NULL)
		{
			ExitProcess(0);
		}
		DEV_BROADCAST_DEVICEINTERFACE_W DevBroadcastDeviceInterface = { sizeof(DevBroadcastDeviceInterface) };
		DevBroadcastDeviceInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		DevBroadcastDeviceInterface.dbcc_classguid = (GUID) { 0x25dbce51, 0x6c8f, 0x4a72, { 0x8a, 0x6d, 0xb5, 0x4c, 0x2b, 0x4f, 0xc8, 0x35 } };
		RegisterDeviceNotificationW(hWnd, &DevBroadcastDeviceInterface, DEVICE_NOTIFY_WINDOW_HANDLE);
		static JOBOBJECT_EXTENDED_LIMIT_INFORMATION JobObject_ExtendedLimitInformation = { 0 };
		JobObject_ExtendedLimitInformation.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		SetInformationJobObject(hJob = CreateJobObjectW(NULL, L"JobForDiskSpy"), JobObjectExtendedLimitInformation, &JobObject_ExtendedLimitInformation, sizeof(JobObject_ExtendedLimitInformation));
		INITCOMMONCONTROLSEX InitCommonControls = { sizeof(InitCommonControls), ICC_WIN95_CLASSES };
		InitCommonControlsEx(&InitCommonControls);
		LoadLibraryW(L"Msftedit.dll");
		RECT rect;
		GetClientRect(hWnd, &rect);
		hWnd_TabControl = CreateWindowExW(0,
			WC_TABCONTROLW, NULL,
			WS_CHILD | WS_VISIBLE,
			0, 0, rect.right + ScaleX(4, iCurrentDPI_X), rect.bottom + ScaleY(4, iCurrentDPI_Y),
			hWnd, NULL, WndClass_MainWindow.hInstance, NULL);
		TCITEMW TabControlItem;
		TabControlItem.mask = TCIF_TEXT;
		for (BYTE byteIndex = 0; byteIndex < _countof(TabControlPagesData); byteIndex++)
		{
			TabControlItem.pszText = TabControlPagesData[byteIndex].lpwTitle;
			TabCtrl_InsertItem(hWnd_TabControl, byteIndex, &TabControlItem);
		}
		TabCtrl_SetItemSize(hWnd_TabControl, 0, ScaleY(22, iCurrentDPI_Y));
		GetClientRect(hWnd_TabControl, &rect);
		TabCtrl_AdjustRect(hWnd_TabControl, FALSE, &rect);
		for (BYTE byteIndex = 0; byteIndex < _countof(TabControlPagesData); byteIndex++)
		{
			SetWindowLongPtrW(TabControlPagesData[byteIndex].hWnd = CreateWindowExW(0,
				WC_TABCONTROLW, NULL,
				(byteIndex) ? (WS_CHILD) : (WS_CHILD | WS_VISIBLE),
				rect.left, rect.top, rect.right, rect.bottom,
				hWnd_TabControl, NULL, WndClass_MainWindow.hInstance, NULL),
				GWLP_WNDPROC, (LONG_PTR)TabControlPagesData[byteIndex].WndProc);
		}
		hWnd_TabControl_CurrentPage = TabControlPagesData[TabControl_Page_Home].hWnd;
		NONCLIENTMETRICSW NonClientMetrics;
		NonClientMetrics.cbSize = sizeof(NonClientMetrics);
		SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NonClientMetrics), &NonClientMetrics, 0);
		NonClientMetrics.lfMessageFont.lfHeight = -MulDiv(9, iCurrentDPI_Y, 72);
		hFonts[0] = CreateFontIndirectW(&NonClientMetrics.lfMessageFont);
		NonClientMetrics.lfMessageFont.lfHeight = -MulDiv(10, iCurrentDPI_Y, 72);
		hFonts[1] = CreateFontIndirectW(&NonClientMetrics.lfMessageFont);
		CharFormat.dwMask = CFM_CHARSET | CFM_COLOR | CFM_FACE | CFM_SIZE;
		CharFormat.yHeight = MulDiv(ScaleY(12, iCurrentDPI_Y), 20 * 72, iCurrentDPI_Y);
		CharFormat.crTextColor = RGB(0, 150, 136);
		CharFormat.bCharSet = ANSI_CHARSET;
		HWND hWnd_Button_BrowseStorageDirectory, hWnd_Button_Help, hWnd_Button_HideUserInterface, hWnd_Button_RunAsAdministrator, hWnd_Button_RunAtStartup,
			hWnd_RichEdit_Copyright,
			hWnd_UpDown[2];
		struct {
			HWND *hWnd;
			HFONT hFont;
			DWORD dwExStyle;
			LPCWSTR lpClassName, lpWindowName;
			DWORD dwStyle;
			INT X, Y, nWidth, nHeight;
			HWND hWndParent;
			BYTE byteMenu;
		} CreateWindowExData[] =
		{
			{
				&hWnd_Button_Help,
				hFonts[1],
				0,
				WC_BUTTONW, L"?",
				WS_CHILD | WS_VISIBLE | BS_FLAT | BS_CENTER | BS_VCENTER,
				407, 1, 22, 22,
				hWnd,
				Identifier_Button_Help
			},
		{
			&hWnd_ComboBox_SelectOption,
			hFonts[1],
			0,
			WC_COMBOBOXW, NULL,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED,
			0, 1, 130, 0,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_ComboBox_SelectOption
		},
		{
			&hWnd_ComboBox_SelectDrive[0],
			hFonts[1],
			0,
			WC_COMBOBOXW, NULL,
			WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED | CBS_SORT,
			132, 1, 58, 0,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_ComboBox_SelectDrive
		},
		{
			&hWnd_Button_StartCopy,
			hFonts[1],
			0,
			WC_BUTTONW, L"Start",
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | BS_FLAT | BS_CENTER | BS_VCENTER,
			191, 0, 52, 29,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_Button_StartCopy
		},
		{
			&hWnd_CheckBox_LimitFileSize,
			hFonts[1],
			0,
			WC_BUTTONW, L"Limit File Size",
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | BS_AUTOCHECKBOX | BS_CENTER | BS_VCENTER,
			0, 30, 130, 27,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_CheckBox_LimitFileSize
		},
		{
			&hWnd_Edit_LimitFileSize,
			hFonts[1],
			WS_EX_CLIENTEDGE,
			WC_EDITW, NULL,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | ES_CENTER | ES_NUMBER,
			132, 30, 58, 27,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_Edit_LimitFileSize
		},
		{
			&hWnd_UpDown[0],
			NULL,
			0,
			UPDOWN_CLASS, NULL,
			WS_CHILD | WS_VISIBLE | UDS_AUTOBUDDY | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_HOTTRACK,
			0, 0, 0, 0,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			0
		},
		{
			&hWnd_ComboBox_SelectFileSizeUnit,
			hFonts[1],
			0,
			WC_COMBOBOXW, NULL,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED,
			192, 30, 50, 0,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_ComboBox_SelectFileSizeUnit
		},
		{
			&hWnd_CheckBox_LimitCopyTime,
			hFonts[1],
			0,
			WC_BUTTONW, L"Limit Copy Time",
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | BS_AUTOCHECKBOX | BS_CENTER | BS_VCENTER,
			0, 59, 130, 27,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_CheckBox_LimitCopyTime
		},
		{
			&hWnd_Edit_LimitCopyTime,
			hFonts[1],
			WS_EX_CLIENTEDGE,
			WC_EDITW, NULL,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | ES_CENTER | ES_NUMBER,
			132, 59, 58, 27,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_Edit_LimitCopyTime
		},
		{
			&hWnd_UpDown[1],
			NULL,
			0,
			UPDOWN_CLASS, NULL,
			WS_CHILD | WS_VISIBLE | UDS_AUTOBUDDY | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_HOTTRACK,
			0, 0, 0, 0,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			0
		},
		{
			&hWnd_ComboBox_SelectTimeUnit,
			hFonts[1],
			0,
			WC_COMBOBOXW, NULL,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED,
			192, 59, 50, 0,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_ComboBox_SelectTimeUnit
		},
		{
			&hWnd_Button_ApplyAndSaveCurrentSettings,
			hFonts[1],
			0,
			WC_BUTTONW, L"Apply and Save Current Settings",
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | BS_FLAT | BS_CENTER | BS_VCENTER,
			0, 87, 243, 30,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_Button_ApplyAndSaveCurrentSettings
		},
		{
			&hWnd_Button_BrowseStorageDirectory,
			hFonts[1],
			0,
			WC_BUTTONW, L"Browse Storage Directory",
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_FLAT | BS_CENTER | BS_VCENTER,
			245, 0, 180, 29,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_Button_BrowseStorageDirectory
		},
		{
			&hWnd_Button_HideUserInterface,
			hFonts[1],
			0,
			WC_BUTTONW, L"Hide User Interface",
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_FLAT | BS_CENTER | BS_VCENTER,
			245, 29, 180, 29,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_Button_HideUserInterface
		},
		{
			&hWnd_Button_RunAtStartup,
			hFonts[1],
			0,
			WC_BUTTONW, NULL,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_FLAT | BS_CENTER | BS_VCENTER,
			245, 58, 180, 29,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			0
		},
		{
			&hWnd_Button_RunAsAdministrator,
			hFonts[1],
			0,
			WC_BUTTONW, L" Run as Administrator",
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | BS_FLAT | BS_CENTER | BS_VCENTER,
			245, 87, 180, 30,
			TabControlPagesData[TabControl_Page_Home].hWnd,
			Identifier_Button_RunAsAdministrator
		},
		{
			&hWnd_ToolTip,
			NULL,
			WS_EX_TOPMOST,
			TOOLTIPS_CLASSW, NULL,
			WS_POPUP,
			0, 0, 0, 0,
			hWnd,
			0
		},
		{
			&hWnd_ComboBox_SelectDrive[1],
			hFonts[0],
			0,
			WC_COMBOBOXW, NULL,
			WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED | CBS_SORT,
			0, 1, 42, 0,
			TabControlPagesData[TabControl_Page_Progress].hWnd,
			Identifier_ComboBox_SelectDrive
		},
		{
			&hWnd_ListView_DrivesList,
			hFonts[0],
			0,
			WC_LISTVIEWW, NULL,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT,
			0, 0, 424, 116,
			TabControlPagesData[TabControl_Page_Filters].hWnd,
			Identifier_ListView_DrivesList
		},
		{
			&hWnd_RichEdit_Copyright,
			hFonts[1],
			0,
			MSFTEDIT_CLASS, NULL,
			WS_CHILD | WS_VISIBLE | ES_CENTER | ES_READONLY,
			0, 90, MulDiv(rect.right - rect.left - ScaleX(4, iCurrentDPI_X), USER_DEFAULT_SCREEN_DPI, iCurrentDPI_X), 30,
			TabControlPagesData[TabControl_Page_About].hWnd,
			Identifier_RichEdit_Copyright
		}
		};
		for (BYTE byteIndex = 0; byteIndex < _countof(CreateWindowExData); byteIndex++)
		{
			SendMessageW(*CreateWindowExData[byteIndex].hWnd = DPIAware_CreateWindowExW(iCurrentDPI_X, iCurrentDPI_Y, CreateWindowExData[byteIndex].dwExStyle,
				CreateWindowExData[byteIndex].lpClassName, CreateWindowExData[byteIndex].lpWindowName,
				CreateWindowExData[byteIndex].dwStyle,
				CreateWindowExData[byteIndex].X, CreateWindowExData[byteIndex].Y, CreateWindowExData[byteIndex].nWidth, CreateWindowExData[byteIndex].nHeight,
				CreateWindowExData[byteIndex].hWndParent, (HMENU)CreateWindowExData[byteIndex].byteMenu, WndClass_MainWindow.hInstance, NULL),
				WM_SETFONT, (WPARAM)CreateWindowExData[byteIndex].hFont, TRUE);
		}
		ListView_SetExtendedListViewStyle(hWnd_ListView_DrivesList, LVS_EX_AUTOCHECKSELECT | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
		SetWindowTheme(hWnd_ListView_DrivesList, L"Explorer", NULL);
		struct {
			LPWSTR lpwText;
			INT iWidth;
		} ListViewColumnsData[ListView_NumberOfColumns] =
		{
			{ L"Drive", 90 },
			{ L"Letter", 47 },
			{ L"Total Space", 80 },
			{ L"Status", 60 },
			{ L"Initial Time", 130 }
		};
		LVCOLUMNW ListViewColumn;
		ListViewColumn.mask = LVCF_WIDTH | LVCF_TEXT;
		HWND hWnd_Header = ListView_GetHeader(hWnd_ListView_DrivesList);
		HDITEMW HeaderItem;
		HeaderItem.mask = HDI_FORMAT;
		for (BYTE byteIndex = 0; byteIndex < _countof(ListViewColumnsData); byteIndex++)
		{
			ListViewColumn.cx = MulDiv(ListViewColumnsData[byteIndex].iWidth, iCurrentDPI_X, USER_DEFAULT_SCREEN_DPI);
			ListViewColumn.pszText = (LPWSTR)ListViewColumnsData[byteIndex].lpwText;
			ListView_InsertColumn(hWnd_ListView_DrivesList, byteIndex, (LPARAM)&ListViewColumn);
			Header_GetItem(hWnd_Header, byteIndex, &HeaderItem);
			HeaderItem.fmt |= HDF_FIXEDWIDTH;
			Header_SetItem(hWnd_Header, byteIndex, &HeaderItem);
		}
		if (dwLastErrorCodes[0] == ERROR_ALREADY_EXISTS)
		{
			WORD wFileSize = (WORD)SetFilePointer(hFile_DrivesDat, 0, NULL, FILE_END);
			if (wFileSize)
			{
				PDRIVEINFO pDriveInfo;
				HANDLE hFileMap_DriveData = CreateFileMappingW(hFile_DrivesDat, NULL, PAGE_READONLY, 0, wFileSize, NULL);
				if (hFileMap_DriveData == NULL || (pDriveInfo = (PDRIVEINFO)MapViewOfFile(hFileMap_DriveData, FILE_MAP_READ, 0, 0, 0)) == NULL)
				{
					ExitProcess(0);
				}
				for (DWORD dwIndex = 0; dwIndex < wFileSize / sizeof(*pDriveInfo); dwIndex++)
				{
					GetLocalTime(&LocalTime);
					StrFormatByteSizeW(pDriveInfo[dwIndex].dw64TotalSpace, szBytesSize, _countof(szBytesSize));
					wnsprintfW(szTime, _countof(szTime), STRING_TIME_FORMATW, LocalTime.wYear, LocalTime.wMonth, LocalTime.wDay, LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond);
					wnsprintfW(szDriveInfo, _countof(szDriveInfo), L"%lX", pDriveInfo[dwIndex].dwDriveIdentifier);
					LVITEMW ListViewItem;
					ListViewItem.mask = LVIF_PARAM | LVIF_TEXT;
					ListViewItem.iItem = dwIndex;
					ListViewItem.iSubItem = 0;
					ListViewItem.pszText = szDriveInfo;
					ListViewItem.lParam = (LPARAM)(dwIndex * sizeof(*pDriveInfo));
					ListView_InsertItem(hWnd_ListView_DrivesList, &ListViewItem);
					ListView_SetItemText(hWnd_ListView_DrivesList, dwIndex, ListView_Column2_Letter, L"[N/A]");
					ListView_SetItemText(hWnd_ListView_DrivesList, dwIndex, ListView_Column3_TotalSpace, szBytesSize);
					ListView_SetItemText(hWnd_ListView_DrivesList, dwIndex, ListView_Column4_Status, (pDriveInfo[dwIndex].byteIsExcluded) ? (L"Excluded") : (L"Normal"));
					ListView_SetItemText(hWnd_ListView_DrivesList, dwIndex, ListView_Column5_InitialTime, szTime);
				}
				UnmapViewOfFile(pDriveInfo);
				CloseHandle(hFileMap_DriveData);
			}
		}
		for (BYTE byteIndex = 0; byteIndex < MAX_DRIVE_NUMBER; byteIndex++)
		{
			if (dwLastErrorCodes[1] != ERROR_ALREADY_EXISTS)
			{
				pSettingsData->byteLimitFileSize[byteIndex] = pSettingsData->byteFileSizeUnit[byteIndex] = 2;
				pSettingsData->wMaxCopyTime[byteIndex] = 300;
				pSettingsData->wMaxFileSize[byteIndex] = 50;
			}
			pSharedData->dwMaxCopyTime[byteIndex] = (pSettingsData->byteLimitCopyTime[byteIndex] == 0) ? (0) : (pSettingsData->wMaxCopyTime[byteIndex] * ((pSettingsData->byteTimeUnit[byteIndex]) ? (60UL * 1000UL) : (1000UL)));
			pSharedData->dw64MaxFileSize[byteIndex] = (pSettingsData->byteLimitFileSize[byteIndex] == 0) ? (ULLONG_MAX) : (((DWORD64)pSettingsData->wMaxFileSize[byteIndex]) * ((pSettingsData->byteFileSizeUnit[byteIndex]) ? ((pSettingsData->byteFileSizeUnit[byteIndex] == 1) ? (1024ULL) : ((pSettingsData->byteFileSizeUnit[byteIndex] == 2) ? (1024ULL * 1024ULL) : (1024ULL * 1024ULL * 1024ULL))) : (1ULL)));
			DWORD dwVolumeSerialNumber;
			DWORD64 dw64TotalSpace;
			if (GetVolumeInformationW((WCHAR[]) { byteIndex + L'A', L':', L'\\', 0 }, NULL, 0, &dwVolumeSerialNumber, NULL, NULL, NULL, 0)
				&& GetDiskFreeSpaceExW((WCHAR[]) { byteIndex + L'A', L':', 0 }, NULL, (PULARGE_INTEGER)&dw64TotalSpace, NULL)
				&& GetDriveTypeW((WCHAR[]) { byteIndex + L'A', L':', L'\\', 0 }) != DRIVE_CDROM)
			{
				wnsprintfW(szDriveInfo, _countof(szDriveInfo), L"%lX", pSharedData->dwDriveIdentifier[byteIndex] = GetEnhancedVolumeSerialNumberCRC32(byteIndex + L'A', dwVolumeSerialNumber));
				LVFINDINFOW ListViewFindInfo;
				ListViewFindInfo.flags = LVFI_STRING;
				ListViewFindInfo.psz = szDriveInfo;
				LVITEMW ListViewItem;
				ListViewItem.pszText = szDriveInfo;
				if ((ListViewItem.iItem = ListView_FindItem(hWnd_ListView_DrivesList, -1, &ListViewFindInfo)) == -1)
				{
					GetLocalTime(&LocalTime);
					StrFormatByteSizeW(dw64TotalSpace, szBytesSize, _countof(szBytesSize));
					wnsprintfW(szTime, _countof(szTime), STRING_TIME_FORMATW, LocalTime.wYear, LocalTime.wMonth, LocalTime.wDay, LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond);
					ListViewItem.mask = LVIF_PARAM;
					ListViewItem.iItem = 0;
					ListViewItem.iSubItem = 0;
					ListViewItem.lParam = (LPARAM)SetFilePointer(hFile_DrivesDat, 0, NULL, FILE_CURRENT);
					DRIVEINFO DriveInfo = { 0, pSharedData->dwDriveIdentifier[byteIndex], dw64TotalSpace, LocalTime };
					WriteFile(hFile_DrivesDat, &DriveInfo, sizeof(DriveInfo), &dwNumberOfBytesWritten, NULL);
					ListView_InsertItem(hWnd_ListView_DrivesList, &ListViewItem);
					ListView_SetItemText(hWnd_ListView_DrivesList, 0, ListView_Column3_TotalSpace, szBytesSize);
					ListView_SetItemText(hWnd_ListView_DrivesList, 0, ListView_Column4_Status, L"Normal");
					ListView_SetItemText(hWnd_ListView_DrivesList, 0, ListView_Column5_InitialTime, szTime);
				}
				ListView_SetItemText(hWnd_ListView_DrivesList, ListViewItem.iItem, ListView_Column1_Drives, szDriveInfo);
				ListView_SetItemText(hWnd_ListView_DrivesList, ListViewItem.iItem, ListView_Column2_Letter, ((WCHAR[]) { byteIndex + L'A', L':', 0 }));
			}
		}
		ListView_SortItemsEx(hWnd_ListView_DrivesList, ListViewSortCompareItemsEx, NULL);
		ToolTipInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		ToolTipInfo.uId = (UINT_PTR)hWnd_CheckBox_LimitCopyTime;
		ToolTipInfo.lpszText = L"Enable Copy Time Limit";
		SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
		ToolTipInfo.uId = (UINT_PTR)hWnd_CheckBox_LimitFileSize;
		ToolTipInfo.lpszText = L"Enable File Size Limit";
		SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
		ToolTipInfo.uId = (UINT_PTR)hWnd_ComboBox_SelectDrive[0];
		ToolTipInfo.lpszText = L"Select Drive";
		SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
		ToolTipInfo.uId = (UINT_PTR)hWnd_ComboBox_SelectDrive[1];
		SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
		ToolTipInfo.uId = (UINT_PTR)hWnd_ComboBox_SelectFileSizeUnit;
		ToolTipInfo.lpszText = L"Select File Size Unit";
		SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
		ToolTipInfo.uId = (UINT_PTR)hWnd_ComboBox_SelectTimeUnit;
		ToolTipInfo.lpszText = L"Select Time Unit";
		SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
		PSID AdministratorsGroup;
		SID_IDENTIFIER_AUTHORITY SID_IdentifierAuthority = SECURITY_NT_AUTHORITY;
		BOOL bIsRunAsAdministrator = AllocateAndInitializeSid(&SID_IdentifierAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
		if (bIsRunAsAdministrator)
		{
			if (!CheckTokenMembership(NULL, AdministratorsGroup, &bIsRunAsAdministrator))
			{
				bIsRunAsAdministrator = FALSE;
			}
			FreeSid(AdministratorsGroup);
		}
		EnableWindow(hWnd_Button_RunAsAdministrator, !bIsRunAsAdministrator);
		Button_SetElevationRequiredState(hWnd_Button_RunAsAdministrator, TRUE);
		ComboBox_SetMinVisible(hWnd_ComboBox_SelectDrive[0], 4);
		ComboBox_SetMinVisible(hWnd_ComboBox_SelectDrive[1], 4);
		ComboBox_AddString(hWnd_ComboBox_SelectFileSizeUnit, L"B");
		ComboBox_AddString(hWnd_ComboBox_SelectFileSizeUnit, L"KB");
		ComboBox_AddString(hWnd_ComboBox_SelectFileSizeUnit, L"MB");
		ComboBox_AddString(hWnd_ComboBox_SelectFileSizeUnit, L"GB");
		ComboBox_SetCurSel(hWnd_ComboBox_SelectOption, ComboBox_AddString(hWnd_ComboBox_SelectOption, L"Copy Data"));
		ComboBox_AddString(hWnd_ComboBox_SelectOption, L"Set Limits");
		ComboBox_AddString(hWnd_ComboBox_SelectTimeUnit, L"Sec");
		ComboBox_AddString(hWnd_ComboBox_SelectTimeUnit, L"Min");
		Edit_LimitText(hWnd_Edit_LimitCopyTime, 4);
		Edit_LimitText(hWnd_Edit_LimitFileSize, 4);
		SendMessageW(hWnd_TabControl, WM_SETFONT, (WPARAM)hFonts[0], TRUE);
		SendMessageW(hWnd_UpDown[0], UDM_SETRANGE, 0, MAKELPARAM(9999, 0));
		SendMessageW(hWnd_UpDown[1], UDM_SETRANGE, 0, MAKELPARAM(9999, 0));
		wnsprintfW(CharFormat.szFaceName, _countof(CharFormat.szFaceName), NonClientMetrics.lfMessageFont.lfFaceName);
		SETTEXTEX SetTextEx = { ST_UNICODE, 1200 };
		SendMessageW(hWnd_RichEdit_Copyright, EM_SETTEXTEX, (WPARAM)&SetTextEx, (LPARAM)"{\\rtf1{\\colortbl;\\red0\\green102\\blue204;}Copyright \xa9 2016 - 2018 {\\field{\\*\\fldinst{HYPERLINK \" https://github.com/Programmer-YangXun/ \"}}{\\fldrslt{\\cf1 Programmer-Yang_Xun@outlook.com}}}}");
		SendMessageW(hWnd_RichEdit_Copyright, EM_SETEDITSTYLEEX, 0, SES_EX_HANDLEFRIENDLYURL);
		SendMessageW(hWnd_RichEdit_Copyright, EM_SETEVENTMASK, 0, ENM_LINK);
		LSTATUS lStatus = RunAtStartup(RegistryOperationFlag_QueryValue);
		SetWindowLongPtrW(hWnd_Button_RunAtStartup, GWLP_ID, (lStatus == ERROR_SUCCESS) ? (Identifier_Button_DisableRunAtStartup) : (Identifier_Button_EnableRunAtStartup));
		SetWindowTextW(hWnd_Button_RunAtStartup, (lStatus == ERROR_SUCCESS) ? (L"Disable Run at Startup") : (L"Enable Run at Startup"));
		SetClassLongPtrW(hWnd_TabControl, GCLP_HBRBACKGROUND, (LONG_PTR)WndClass_MainWindow.hbrBackground);
		SetClassLongPtrW(hWnd_TabControl, GCLP_HCURSOR, (LONG_PTR)LoadCursorW(NULL, IDC_ARROW));
		SetFocus(hWnd_TabControl_CurrentPage);
		if (dwLastErrorCodes[1] == ERROR_ALREADY_EXISTS && !pSettingsData->byteIsNormalExit)
		{
			StartupInfo.wShowWindow = SW_HIDE;
		}
		pSettingsData->byteIsNormalExit = FALSE;
#ifdef _DEBUG
		if (IsDebuggerPresent())
		{
			StartupInfo.wShowWindow = SW_SHOW;
		}
#endif
		if (StartupInfo.wShowWindow)
		{
			WCHAR szSystemDirectory[MAX_PATH];
			GetSystemDirectoryW(szSystemDirectory, _countof(szSystemDirectory));
			if (StrCmpIW(szCurrentDirectory, szSystemDirectory))
			{
				CreateThread(NULL, 0, Thread_CheckForUpdate, NULL, 0, NULL);
				ShowWindow(hWnd, SW_SHOW);
			}
		}
	}
	break;
	case WM_MEASUREITEM:
	{
		if (((LPMEASUREITEMSTRUCT)lParam)->CtlType == ODT_COMBOBOX)
		{
			((LPMEASUREITEMSTRUCT)lParam)->itemWidth = 0;
			((LPMEASUREITEMSTRUCT)lParam)->itemHeight = (iCurrentDPI_Y == USER_DEFAULT_SCREEN_DPI) ? (21) : ScaleY(24, iCurrentDPI_Y);
			return TRUE;
		}
	}
	break;
	case WM_DRAWITEM: DrawComboBox((LPDRAWITEMSTRUCT)lParam); break;
	case WM_PAINT:
	{
		PAINTSTRUCT PaintStruct;
		RECT rect = { ScaleX(243, iCurrentDPI_X), ScaleY(1, iCurrentDPI_Y), ScaleX(245, iCurrentDPI_X), ScaleY(116, iCurrentDPI_Y) };
		HBRUSH hBrush = CreateSolidBrush(RGB(14, 151, 249));
		FillRect(BeginPaint(hWnd, &PaintStruct), &rect, hBrush);
		DeleteObject((HGDIOBJ)hBrush);
		EndPaint(hWnd, &PaintStruct);
	}
	break;
	case WM_NOTIFY:
	{
		switch (((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGE:
		{
			ShowWindow(hWnd_TabControl_CurrentPage = TabControlPagesData[TabCtrl_GetCurSel(((LPNMHDR)lParam)->hwndFrom)].hWnd, SW_SHOW);
			SetFocus(hWnd_TabControl_CurrentPage);
		}
		break;
		case TCN_SELCHANGING:
		{
			INT iIndex = TabCtrl_GetCurSel(((LPNMHDR)lParam)->hwndFrom);
			ShowWindow(TabControlPagesData[iIndex].hWnd, SW_HIDE);
			if (iIndex == TabControl_Page_Progress)
			{
				BYTE byteIsChildWindowsDestroyed = FALSE, byteRunningSubProcessIndex = MAX_DRIVE_NUMBER;
				for (BYTE byteIndex = 0; byteIndex < MAX_DRIVE_NUMBER; byteIndex++)
				{
					if (ProcessesInformation[byteIndex].hProcess)
					{
						byteRunningSubProcessIndex = (byteRunningSubProcessIndex == MAX_DRIVE_NUMBER) ? (byteIndex) : (byteRunningSubProcessIndex);
					}
					else if (ProgressData[byteIndex].hWnd_ProgressBar && IsWindow(ProgressData[byteIndex].hWnd_ProgressBar))
					{
						byteIsChildWindowsDestroyed = TRUE;
						pSharedData->hWnd_MainWindows[byteIndex] = NULL;
						ComboBox_DeleteString(hWnd_ComboBox_SelectDrive[1], ComboBox_FindString(hWnd_ComboBox_SelectDrive[1], -1, ((WCHAR[]) { byteIndex + L'A', L':', 0 })));
						DestroyWindow(ProgressData[byteIndex].hWnd_Button_StopCopy);
						DestroyWindow(ProgressData[byteIndex].hWnd_Button_PauseCopy);
						DestroyWindow(ProgressData[byteIndex].hWnd_Button_ContinueCopy);
						DestroyWindow(ProgressData[byteIndex].hWnd_Button_SkipCurrentFile);
						DestroyWindow(ProgressData[byteIndex].hWnd_RichEdit_OutputDirectoryName);
						DestroyWindow(ProgressData[byteIndex].hWnd_RichEdit_OutputFileName);
						DestroyWindow(ProgressData[byteIndex].hWnd_ProgressBar);
					}
				}
				if (!ComboBox_GetCount(hWnd_ComboBox_SelectDrive[1]))
				{
					ShowWindow(hWnd_ComboBox_SelectDrive[1], SW_HIDE);
				}
				else if (byteIsChildWindowsDestroyed && byteRunningSubProcessIndex != MAX_DRIVE_NUMBER)
				{
					byteSelectedDriveIndexes[0] = byteRunningSubProcessIndex;
					ShowSpecifiedChildWindows(byteRunningSubProcessIndex, SW_SHOW);
					ComboBox_SelectString(hWnd_ComboBox_SelectDrive[1], -1, ((WCHAR[]) { byteRunningSubProcessIndex + L'A', L':', 0 }));
				}
			}
		}
		break;
		}
	}
	break;
	case WM_HELP:
	{
	ShowHelpInformation:;
		if (hWnd == pSharedData->hWnd_MainWindows[byteLocalDriveIndex])
		{
			MessageBoxW(NULL,
				L"When a storage device is connected, this program does not automatically copy data if this program's user interface is visible or specified file exists in root path (this file name can be specified by editing key \"SpecifiedFileExistingInRootPath\" in file \"Config.ini\")",
				L"",
				MB_ICONQUESTION | MB_OK);
		}
		SetFocus(hWnd_TabControl_CurrentPage);
	}
	break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case Identifier_Button_ApplyAndSaveCurrentSettings:
		{
		ApplyAndSaveCurrentSettings:;
			pSettingsData->byteFileSizeUnit[byteSelectedDriveIndexes[1]] = (BYTE)ComboBox_GetCurSel(hWnd_ComboBox_SelectFileSizeUnit);
			pSettingsData->byteLimitCopyTime[byteSelectedDriveIndexes[1]] = ((pSettingsData->wMaxCopyTime[byteSelectedDriveIndexes[1]] = (WORD)GetDlgItemInt(TabControlPagesData[TabControl_Page_Home].hWnd, Identifier_Edit_LimitCopyTime, NULL, FALSE)) == 0) ? (FALSE) : ((BYTE)Button_GetCheck(hWnd_CheckBox_LimitCopyTime));
			pSettingsData->byteLimitFileSize[byteSelectedDriveIndexes[1]] = (BYTE)Button_GetCheck(hWnd_CheckBox_LimitFileSize);
			pSettingsData->byteTimeUnit[byteSelectedDriveIndexes[1]] = (BYTE)ComboBox_GetCurSel(hWnd_ComboBox_SelectTimeUnit);
			pSettingsData->wMaxFileSize[byteSelectedDriveIndexes[1]] = (WORD)GetDlgItemInt(TabControlPagesData[TabControl_Page_Home].hWnd, Identifier_Edit_LimitFileSize, NULL, FALSE);
			pSharedData->dwMaxCopyTime[byteSelectedDriveIndexes[1]] = (pSettingsData->byteLimitCopyTime[byteSelectedDriveIndexes[1]] == 0) ? (0) : (pSettingsData->wMaxCopyTime[byteSelectedDriveIndexes[1]] * ((pSettingsData->byteTimeUnit[byteSelectedDriveIndexes[1]]) ? (60UL * 1000UL) : (1000UL)));
			pSharedData->dw64MaxFileSize[byteSelectedDriveIndexes[1]] = (pSettingsData->byteLimitFileSize[byteSelectedDriveIndexes[1]] == 0) ? (ULLONG_MAX) : (((DWORD64)pSettingsData->wMaxFileSize[byteSelectedDriveIndexes[1]]) * ((pSettingsData->byteFileSizeUnit[byteSelectedDriveIndexes[1]]) ? ((pSettingsData->byteFileSizeUnit[byteSelectedDriveIndexes[1]] == 1) ? (1024ULL) : ((pSettingsData->byteFileSizeUnit[byteSelectedDriveIndexes[1]] == 2) ? (1024ULL * 1024ULL) : (1024ULL * 1024ULL * 1024ULL))) : (1ULL)));
			EnableWindow(hWnd_Edit_LimitCopyTime, (BOOL)pSettingsData->byteLimitCopyTime[byteSelectedDriveIndexes[1]]);
			EnableWindow(hWnd_Edit_LimitFileSize, (BOOL)pSettingsData->byteLimitFileSize[byteSelectedDriveIndexes[1]]);
			EnableWindow(hWnd_ComboBox_SelectFileSizeUnit, (BOOL)pSettingsData->byteLimitFileSize[byteSelectedDriveIndexes[1]]);
			EnableWindow(hWnd_ComboBox_SelectTimeUnit, (BOOL)pSettingsData->byteLimitCopyTime[byteSelectedDriveIndexes[1]]);
			Button_SetCheck(hWnd_CheckBox_LimitCopyTime, pSettingsData->byteLimitCopyTime[byteSelectedDriveIndexes[1]]);
			Button_SetCheck(hWnd_CheckBox_LimitFileSize, pSettingsData->byteLimitCopyTime[byteSelectedDriveIndexes[1]]);
		}
		break;
		case Identifier_Button_DisableRunAtStartup:
		case Identifier_Button_EnableRunAtStartup:
		{
			if (RunAtStartup((LOWORD(wParam) == Identifier_Button_EnableRunAtStartup) ? (RegistryOperationFlag_SetValue) : (RegistryOperationFlag_DeleteValue)) == ERROR_SUCCESS)
			{
				SetWindowLongPtrW((HWND)lParam, GWLP_ID, (LOWORD(wParam) == Identifier_Button_EnableRunAtStartup) ? (Identifier_Button_DisableRunAtStartup) : (Identifier_Button_EnableRunAtStartup));
				SetWindowTextW((HWND)lParam, (LOWORD(wParam) == Identifier_Button_EnableRunAtStartup) ? (L"Disable Run at Startup") : (L"Enable Run at Startup"));
			}
		}
		break;
		case Identifier_Button_BrowseStorageDirectory:
		{
			SHCreateDirectory(NULL, szStorageDirectoryName);
			ShellExecuteW(NULL, L"explore", szStorageDirectoryName, NULL, NULL, SW_SHOW);
		}
		break;
		case Identifier_Button_Help:
		{
			goto ShowHelpInformation;
		}
		break;
		case Identifier_Button_HideUserInterface: ShowWindow(pSharedData->hWnd_MainWindows[byteLocalDriveIndex], SW_HIDE); break;
		case Identifier_Button_RunAsAdministrator:
		{
			static SHELLEXECUTEINFOW ShellExecuteInfo = { sizeof(ShellExecuteInfo) };
			ShellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			ShellExecuteInfo.lpVerb = L"runas";
			ShellExecuteInfo.lpFile = szProgramFileName;
			ShellExecuteInfo.nShow = SW_SHOW;
			pSettingsData->byteIsNormalExit = TRUE;
			pSharedData->hWnd_MainWindows[byteLocalDriveIndex] = NULL;
			if (ShellExecuteExW(&ShellExecuteInfo))
			{
				ExitProcess(0);
			}
			pSettingsData->byteIsNormalExit = FALSE;
			pSharedData->hWnd_MainWindows[byteLocalDriveIndex] = hWnd;
		}
		break;
		case Identifier_Button_StartCopy:
		{
		StartCopy:;
			INT iIndex = ComboBox_GetCurSel(hWnd_ComboBox_SelectDrive[0]);
			BYTE byteDriveIndex = (BYTE)ComboBox_GetItemData(hWnd_ComboBox_SelectDrive[0], iIndex) - 'A';
			if (CreateProcessW(szProgramFileName, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &StartupInfo, &ProcessesInformation[byteDriveIndex]))
			{
				if (!CloseHandle(CreateThread(NULL, 0, Thread_WaitForSingleProcess, (LPVOID)byteDriveIndex, 0, NULL)))
				{
					TerminateProcess(ProcessesInformation[byteDriveIndex].hProcess, 0);
					CloseHandle(ProcessesInformation[byteDriveIndex].hProcess);
					ProcessesInformation[byteDriveIndex].hProcess = NULL;
				}
				else
				{
					ShowSpecifiedChildWindows(byteSelectedDriveIndexes[0], SW_HIDE);
					EnableWindow(hWnd_Button_StartCopy, FALSE);
					EnableSpecifiedChildWindows(FALSE);
					ComboBox_DeleteString(hWnd_ComboBox_SelectDrive[0], iIndex);
					struct {
						HWND *hWnd;
						LPCWSTR lpClassName, lpWindowName;
						DWORD dwStyle;
						int X, Y, nWidth, nHeight;
						HWND hWndParent;
						BYTE byteMenu;
					} CreateWindowExData[] =
					{
						{
							&ProgressData[byteSelectedDriveIndexes[0] = byteDriveIndex].hWnd_Button_ContinueCopy,
							WC_BUTTONW, L"\u25b6",
							WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED | BS_FLAT | BS_CENTER | BS_VCENTER,
							43, 0, 27, 27,
							TabControlPagesData[TabControl_Page_Progress].hWnd,
							Identifier_Button_ContinueCopy
						},
					{
						&ProgressData[byteDriveIndex].hWnd_Button_PauseCopy,
						WC_BUTTONW, L"\u275a\u275a",
						WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_FLAT | BS_CENTER | BS_VCENTER,
						70, 0, 27, 27,
						TabControlPagesData[TabControl_Page_Progress].hWnd,
						Identifier_Button_PauseCopy
					},
					{
						&ProgressData[byteDriveIndex].hWnd_Button_StopCopy,
						WC_BUTTONW, L"\u23f9",
						WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_FLAT | BS_CENTER | BS_VCENTER,
						97, 0, 27, 27,
						TabControlPagesData[TabControl_Page_Progress].hWnd,
						Identifier_Button_StopCopy,
					},
					{
						&ProgressData[byteDriveIndex].hWnd_Button_SkipCurrentFile,
						WC_BUTTONW, L"\u2794",
						WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_FLAT | BS_CENTER | BS_VCENTER,
						124, 0, 27, 27,
						TabControlPagesData[TabControl_Page_Progress].hWnd,
						Identifier_Button_SkipCurrentFile
					},
					{
						&ProgressData[byteDriveIndex].hWnd_ProgressBar,
						PROGRESS_CLASSW, NULL,
						WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
						152, 1, 272, 25,
						TabControlPagesData[TabControl_Page_Progress].hWnd,
						0
					},
					{
						&ProgressData[byteDriveIndex].hWnd_RichEdit_OutputDirectoryName,
						MSFTEDIT_CLASS, NULL,
						WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_DISABLENOSCROLL,
						0, 27, 424, 51,
						TabControlPagesData[TabControl_Page_Progress].hWnd,
						Identifier_RichEdit_OutputDirectoryName
					},
					{
						&ProgressData[byteDriveIndex].hWnd_RichEdit_OutputFileName,
						MSFTEDIT_CLASS, NULL,
						WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_DISABLENOSCROLL,
						0, 78, 424, 38,
						TabControlPagesData[TabControl_Page_Progress].hWnd,
						Identifier_RichEdit_OutputFileName
					}
					};
					for (BYTE byteIndex = 0; byteIndex < _countof(CreateWindowExData); byteIndex++)
					{
						SendMessageW(*CreateWindowExData[byteIndex].hWnd = DPIAware_CreateWindowExW(iCurrentDPI_X, iCurrentDPI_Y, 0,
							CreateWindowExData[byteIndex].lpClassName, CreateWindowExData[byteIndex].lpWindowName,
							CreateWindowExData[byteIndex].dwStyle,
							CreateWindowExData[byteIndex].X, CreateWindowExData[byteIndex].Y, CreateWindowExData[byteIndex].nWidth, CreateWindowExData[byteIndex].nHeight,
							CreateWindowExData[byteIndex].hWndParent, (HMENU)CreateWindowExData[byteIndex].byteMenu, WndClass_MainWindow.hInstance, NULL),
							WM_SETFONT, (WPARAM)hFonts[1], TRUE);
					}
					ToolTipInfo.uId = (UINT_PTR)ProgressData[byteDriveIndex].hWnd_Button_StopCopy;
					ToolTipInfo.lpszText = L"Stop Copy";
					SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
					ToolTipInfo.uId = (UINT_PTR)ProgressData[byteDriveIndex].hWnd_Button_PauseCopy;
					ToolTipInfo.lpszText = L"Pause Copy";
					SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
					ToolTipInfo.uId = (UINT_PTR)ProgressData[byteDriveIndex].hWnd_Button_ContinueCopy;
					ToolTipInfo.lpszText = L"Continue Copy";
					SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
					ToolTipInfo.uId = (UINT_PTR)ProgressData[byteDriveIndex].hWnd_Button_SkipCurrentFile;
					ToolTipInfo.lpszText = L"Skip Current File";
					SendMessageW(hWnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ToolTipInfo);
					SendMessageW(ProgressData[byteDriveIndex].hWnd_RichEdit_OutputDirectoryName, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&CharFormat);
					SendMessageW(ProgressData[byteDriveIndex].hWnd_RichEdit_OutputFileName, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&CharFormat);
					INT iIndex = ComboBox_AddString(hWnd_ComboBox_SelectDrive[1], ((WCHAR[]) { byteDriveIndex + L'A', L':', 0 }));
					ComboBox_SetCurSel(hWnd_ComboBox_SelectDrive[1], iIndex);
					ComboBox_SetItemData(hWnd_ComboBox_SelectDrive[1], iIndex, byteDriveIndex + L'A');
					ShowWindow(hWnd_ComboBox_SelectDrive[1], SW_SHOW);
					TabCtrl_SetCurFocus(hWnd_TabControl, TabControl_Page_Progress);
					pSharedData->byteCopyData[byteDriveIndex] = TRUE;
					AssignProcessToJobObject(hJob, ProcessesInformation[byteDriveIndex].hProcess);
					ResumeThread(ProcessesInformation[byteDriveIndex].hThread);
				}
				CloseHandle(ProcessesInformation[byteDriveIndex].hThread);
			}
		}
		break;
		case Identifier_CheckBox_LimitCopyTime:
		case Identifier_CheckBox_LimitFileSize:
		{
			BYTE byteCheckState = (BYTE)Button_GetCheck((HWND)lParam);
			EnableWindow((LOWORD(wParam) == Identifier_CheckBox_LimitCopyTime) ? (hWnd_Edit_LimitCopyTime) : (hWnd_Edit_LimitFileSize), byteCheckState);
			EnableWindow((LOWORD(wParam) == Identifier_CheckBox_LimitCopyTime) ? (hWnd_ComboBox_SelectTimeUnit) : (hWnd_ComboBox_SelectFileSizeUnit), byteCheckState);
		}
		break;
		case Identifier_ComboBox_SelectDrive:
		{
			switch (HIWORD(wParam))
			{
			case CBN_DROPDOWN:
			{
				INT iIndex = ComboBox_GetCurSel(hWnd_ComboBox_SelectOption);
				for (BYTE byteIndex = 0; byteIndex < MAX_DRIVE_NUMBER; byteIndex++)
				{
					LPWSTR lpwDriveName = (WCHAR[]) { byteIndex + L'A', L':', 0 };
					if (pSharedData->hWnd_MainWindows[byteIndex] == NULL
						&& (iIndex == 1 || pSharedData->dwDriveIdentifier[byteIndex])
						&& ComboBox_FindString((HWND)lParam, -1, lpwDriveName) == CB_ERR)
					{
						ComboBox_SetItemData((HWND)lParam,
							ComboBox_AddString((HWND)lParam, lpwDriveName),
							byteIndex + L'A');
					}
				}
			}
			break;
			case CBN_SELCHANGE:
			{
				EnableWindow(hWnd_Button_StartCopy, !ComboBox_GetCurSel(hWnd_ComboBox_SelectOption));
				EnableSpecifiedChildWindows(TRUE);
				Button_SetCheck(hWnd_CheckBox_LimitCopyTime, pSettingsData->byteLimitCopyTime[byteSelectedDriveIndexes[1] = (BYTE)ComboBox_GetItemData((HWND)lParam, ComboBox_GetCurSel((HWND)lParam)) - 'A']);
				Button_SetCheck(hWnd_CheckBox_LimitFileSize, pSettingsData->byteLimitFileSize[byteSelectedDriveIndexes[1]]);
				ComboBox_SetCurSel(hWnd_ComboBox_SelectFileSizeUnit, pSettingsData->byteFileSizeUnit[byteSelectedDriveIndexes[1]]);
				ComboBox_SetCurSel(hWnd_ComboBox_SelectTimeUnit, pSettingsData->byteTimeUnit[byteSelectedDriveIndexes[1]]);
				SetDlgItemInt(TabControlPagesData[TabControl_Page_Home].hWnd, Identifier_Edit_LimitCopyTime, (UINT)pSettingsData->wMaxCopyTime[byteSelectedDriveIndexes[1]], FALSE);
				SetDlgItemInt(TabControlPagesData[TabControl_Page_Home].hWnd, Identifier_Edit_LimitFileSize, (UINT)pSettingsData->wMaxFileSize[byteSelectedDriveIndexes[1]], FALSE);
				goto ApplyAndSaveCurrentSettings;
			}
			break;
			}
		}
		break;
		case Identifier_ComboBox_SelectOption:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				switch (ComboBox_GetCurSel((HWND)lParam))
				{
				case 0:
				{
					INT iIndex = ComboBox_GetCurSel(hWnd_ComboBox_SelectDrive[0]);
					if (iIndex != CB_ERR)
					{
						WCHAR wchDriveLetter = (BYTE)ComboBox_GetItemData(hWnd_ComboBox_SelectDrive[0], iIndex);
						if (wchDriveLetter != *szProgramFileName && pSharedData->dwDriveIdentifier[wchDriveLetter - L'A'])
						{
							EnableWindow(hWnd_Button_StartCopy, TRUE);
						}
						else
						{
							ComboBox_SetCurSel(hWnd_ComboBox_SelectDrive[0], -1);
							EnableWindow(hWnd_Button_StartCopy, FALSE);
							EnableSpecifiedChildWindows(FALSE);
						}
					}
					for (BYTE byteIndex = 0; byteIndex < MAX_DRIVE_NUMBER; byteIndex++)
					{
						if (!pSharedData->dwDriveIdentifier[byteIndex])
						{
							ComboBox_DeleteString(hWnd_ComboBox_SelectDrive[0], ComboBox_FindString(hWnd_ComboBox_SelectDrive[0], -1, ((WCHAR[]) { byteIndex + L'A', L':', 0 })));
						}
					}
				}
				break;
				case 1: EnableWindow(hWnd_Button_StartCopy, FALSE); break;
				}
			}
		}
		break;
		}
	}
	break;
	case WM_CLOSE:
	{
		if (MessageBoxW(hWnd, L"Are you sure you want to Exit?", L"Confirm", MB_ICONQUESTION | MB_YESNO) == IDYES)
		{
			pSettingsData->byteIsNormalExit = TRUE;
			ExitProcess(0);
		}
		return 0;
	}
	break;
	case WM_COPYDATA:
	{
		PCOPYDATASTRUCT pCopyDataStruct = (PCOPYDATASTRUCT)lParam;
		switch (HIWORD(pCopyDataStruct->dwData))
		{
#ifndef PAGE_PROGRESS_DEMO
		case WindowEvent_OutputDirectoryName: SetWindowTextW(ProgressData[LOWORD(pCopyDataStruct->dwData)].hWnd_RichEdit_OutputDirectoryName, (LPCWSTR)pCopyDataStruct->lpData); break;
		case WindowEvent_OutputFileName: SetWindowTextW(ProgressData[LOWORD(pCopyDataStruct->dwData)].hWnd_RichEdit_OutputFileName, (LPCWSTR)pCopyDataStruct->lpData); break;
		case WindowEvent_SetProgressBarPos: SendMessageW(ProgressData[LOWORD(pCopyDataStruct->dwData)].hWnd_ProgressBar, PBM_SETPOS, *(PBYTE)pCopyDataStruct->lpData, 0); break;
#else
		case WindowEvent_OutputDirectoryName: SetWindowTextW(ProgressData[LOWORD(pCopyDataStruct->dwData)].hWnd_RichEdit_OutputDirectoryName, L"D:\\Demos\\\n->\nC:\\Data\\3B8CACFB\\"); break;
		case WindowEvent_OutputFileName: SetWindowTextW(ProgressData[LOWORD(pCopyDataStruct->dwData)].hWnd_RichEdit_OutputFileName, L"Demo.dat\n(Size: 10.24 GB)"); break;
		case WindowEvent_SetProgressBarPos: SendMessageW(ProgressData[LOWORD(pCopyDataStruct->dwData)].hWnd_ProgressBar, PBM_SETPOS, 50, 0); break;
#endif
		}
	}
	break;
	case WM_DEVICECHANGE:
	{
		switch (wParam)
		{
		case DBT_DEVICEARRIVAL:
		case DBT_DEVICEREMOVECOMPLETE:
		{
			if (((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				WCHAR wchDriveLetter = L'A';
				for (; wchDriveLetter <= L'Z'; wchDriveLetter++)
				{
					if (((PDEV_BROADCAST_VOLUME)lParam)->dbcv_unitmask & 0x01)
					{
						break;
					}
					((PDEV_BROADCAST_VOLUME)lParam)->dbcv_unitmask >>= 1;
				}
				LPWSTR lpwDriveName = ((WCHAR[]) { wchDriveLetter, L':', 0 });
				BYTE byteDriveIndex = wchDriveLetter - L'A';
				if (wParam == DBT_DEVICEARRIVAL || (wParam == DBT_DEVICEREMOVECOMPLETE && pSharedData->dwDriveIdentifier[byteDriveIndex]))
				{
					WCHAR szDriveInfo[70], szLogFileName[] = L"\0:\\Data\\History.log", szTime[30];
					SYSTEMTIME LocalTime;
					GetLocalTime(&LocalTime);
					wnsprintfW(szTime, _countof(szTime), STRING_TIME_FORMATW, LocalTime.wYear, LocalTime.wMonth, LocalTime.wDay, LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond);
					SHCreateDirectory(NULL, szStorageDirectoryName);
					*szLogFileName = *szProgramFileName;
					static HANDLE hFile_HistoryLog = INVALID_HANDLE_VALUE;
					if (hFile_HistoryLog == INVALID_HANDLE_VALUE && (hFile_HistoryLog = CreateFileW(szLogFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE && GetLastError() != ERROR_ALREADY_EXISTS)
					{
						WriteFile(hFile_HistoryLog, "\xff\xfe", 2, &dwNumberOfBytesWritten, NULL);
					}
					SetFilePointer(hFile_HistoryLog, 0, NULL, FILE_END);
					if (wParam == DBT_DEVICEARRIVAL)
					{
						LPCWSTR lpcwRootPathName = (WCHAR[]) { wchDriveLetter, L':', L'\\', 0 };
						DWORD dwVolumeSerialNumber;
						DWORD64 dw64TotalSpace;
						if (GetVolumeInformationW(lpcwRootPathName, NULL, 0, &dwVolumeSerialNumber, NULL, NULL, NULL, 0)
							&& GetDiskFreeSpaceExW(lpwDriveName, NULL, (PULARGE_INTEGER)&dw64TotalSpace, NULL)
							&& GetDriveTypeW(lpcwRootPathName) != DRIVE_CDROM)
						{
							if (!ComboBox_GetCurSel(hWnd_ComboBox_SelectOption))
							{
								INT iIndex = ComboBox_AddString(hWnd_ComboBox_SelectDrive[0], lpwDriveName);
								ComboBox_SetCurSel(hWnd_ComboBox_SelectDrive[0], iIndex);
								ComboBox_SetItemData(hWnd_ComboBox_SelectDrive[0], iIndex, wchDriveLetter);
								SendMessageW(hWnd, WM_COMMAND, MAKEDWORD(CBN_SELCHANGE, Identifier_ComboBox_SelectDrive), (LPARAM)hWnd_ComboBox_SelectDrive[0]);
							}
							wnsprintfW(szDriveInfo, _countof(szDriveInfo), L"%lX", pSharedData->dwDriveIdentifier[byteDriveIndex] = GetEnhancedVolumeSerialNumberCRC32(wchDriveLetter, dwVolumeSerialNumber));
							LVFINDINFOW ListViewFindInfo;
							ListViewFindInfo.flags = LVFI_STRING;
							ListViewFindInfo.psz = szDriveInfo;
							LVITEMW ListViewItem;
							INT iIndex = ListViewItem.iItem = ListView_FindItem(hWnd_ListView_DrivesList, -1, &ListViewFindInfo);
							if (iIndex == -1)
							{
								WCHAR szBytesSize[11];
								StrFormatByteSizeW(dw64TotalSpace, szBytesSize, _countof(szBytesSize));
								ListViewItem.mask = LVIF_PARAM | LVIF_TEXT;
								ListViewItem.iItem = 0;
								ListViewItem.iSubItem = 0;
								ListViewItem.pszText = szDriveInfo;
								ListViewItem.lParam = (LPARAM)SetFilePointer(hFile_DrivesDat, 0, NULL, FILE_END);
								DRIVEINFO DriveInfo = { 0, pSharedData->dwDriveIdentifier[byteDriveIndex], dw64TotalSpace, LocalTime };
								WriteFile(hFile_DrivesDat, &DriveInfo, sizeof(DriveInfo), &dwNumberOfBytesWritten, NULL);
								ListView_InsertItem(hWnd_ListView_DrivesList, &ListViewItem);
								ListView_SetItemText(hWnd_ListView_DrivesList, 0, ListView_Column3_TotalSpace, szBytesSize);
								ListView_SetItemText(hWnd_ListView_DrivesList, 0, ListView_Column4_Status, L"Normal");
								ListView_SetItemText(hWnd_ListView_DrivesList, 0, ListView_Column5_InitialTime, szTime);
							}
							BYTE byteIsExcluded = (BYTE)ListView_GetCheckState(hWnd_ListView_DrivesList, iIndex);
							ListView_SetItemText(hWnd_ListView_DrivesList, ListViewItem.iItem, ListView_Column2_Letter, lpwDriveName);
							ListView_SetItemState(hWnd_ListView_DrivesList, ListViewItem.iItem, LVIS_FOCUSED | LVIS_SELECTED, 0x000f);
							ListView_SortItemsEx(hWnd_ListView_DrivesList, ListViewSortCompareItemsEx, NULL);
							WriteFile(hFile_HistoryLog, szDriveInfo, sizeof(WCHAR) * wnsprintfW(szDriveInfo, _countof(szDriveInfo), L"[%ls] Drive %lX: Connected\r\n", szTime, pSharedData->dwDriveIdentifier[byteDriveIndex]), &dwNumberOfBytesWritten, NULL);
							if (IsWindowVisible(hWnd))
							{
								FLASHWINFO FlashWindowInfo = { sizeof(FlashWindowInfo), hWnd, FLASHW_TRAY | FLASHW_TIMER, 3, 0 };
								FlashWindowEx(&FlashWindowInfo);
							}
							else if (iIndex == -1 || !byteIsExcluded)
							{
								WCHAR szBuffer[MAX_PATH], szSpecifiedFileName[MAX_PATH];
								LPWCH lpwchFound = StrRChrW(szProgramFileName, NULL, L'\\');
								*lpwchFound = 0;
								wnsprintfW(szBuffer, _countof(szBuffer), L"%ls\\Config.ini", szProgramFileName);
								*lpwchFound = L'\\';
								if (!GetPrivateProfileStringW(lpcwAppName, lpcwKeyName, NULL, szSpecifiedFileName, _countof(szSpecifiedFileName), szBuffer))
								{
									WritePrivateProfileStringW(lpcwAppName, lpcwKeyName, lpcwDefaultSpecifiedFileName, szBuffer);
									wnsprintfW(szSpecifiedFileName, _countof(szSpecifiedFileName), lpcwDefaultSpecifiedFileName);
								}
								wnsprintfW(szBuffer, _countof(szBuffer), L"%lc:\\%ls", wchDriveLetter, szSpecifiedFileName);
								if (!PathFileExistsW(szBuffer))
								{
									goto StartCopy;
								}
							}
						}
					}
					else
					{
						WriteFile(hFile_HistoryLog, szDriveInfo, sizeof(WCHAR) * wnsprintfW(szDriveInfo, _countof(szDriveInfo), L"[%ls] Drive %lX: Disconnected\r\n", szTime, pSharedData->dwDriveIdentifier[byteDriveIndex]), &dwNumberOfBytesWritten, NULL);
						if (!ComboBox_GetCurSel(hWnd_ComboBox_SelectOption))
						{
							INT iIndex = ComboBox_GetCurSel(hWnd_ComboBox_SelectDrive[0]);
							if (iIndex != CB_ERR && wchDriveLetter == (WCHAR)ComboBox_GetItemData(hWnd_ComboBox_SelectDrive[0], iIndex))
							{
								EnableWindow(hWnd_Button_StartCopy, FALSE);
								EnableSpecifiedChildWindows(FALSE);
								ComboBox_SetCurSel(hWnd_ComboBox_SelectDrive[0], -1);
							}
							ComboBox_DeleteString(hWnd_ComboBox_SelectDrive[0], ComboBox_FindString(hWnd_ComboBox_SelectDrive[0], -1, lpwDriveName));
						}
						wnsprintfW(szDriveInfo, _countof(szDriveInfo), L"%lX", pSharedData->dwDriveIdentifier[byteDriveIndex]);
						pSharedData->dwDriveIdentifier[byteDriveIndex] = 0;
						LVFINDINFOW ListViewFindInfo;
						ListViewFindInfo.flags = LVFI_STRING;
						ListViewFindInfo.psz = szDriveInfo;
						INT iIndex = ListView_FindItem(hWnd_ListView_DrivesList, -1, &ListViewFindInfo);
						if (iIndex != -1)
						{
							ListView_SetItemText(hWnd_ListView_DrivesList, iIndex, ListView_Column2_Letter, L"[N/A]");
							ListView_SortItemsEx(hWnd_ListView_DrivesList, ListViewSortCompareItemsEx, NULL);
						}
					}
				}
			}
		}
		break;
		}
	}
	break;
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WndProc_Page_Progress(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case Identifier_Button_StopCopy: TerminateProcess(ProcessesInformation[byteSelectedDriveIndexes[0]].hProcess, 0); break;
		case Identifier_Button_ContinueCopy:
		{
			EnableWindow(ProgressData[byteSelectedDriveIndexes[0]].hWnd_Button_ContinueCopy, FALSE);
			EnableWindow(ProgressData[byteSelectedDriveIndexes[0]].hWnd_Button_PauseCopy, TRUE);
			PostMessageW(pSharedData->hWnd_MainWindows[byteSelectedDriveIndexes[0]], AppMessage_ContinueCopy, 0, 0);
		}
		break;
		case Identifier_Button_PauseCopy:
		{
			EnableWindow(ProgressData[byteSelectedDriveIndexes[0]].hWnd_Button_ContinueCopy, TRUE);
			EnableWindow(ProgressData[byteSelectedDriveIndexes[0]].hWnd_Button_PauseCopy, FALSE);
			PostMessageW(pSharedData->hWnd_MainWindows[byteSelectedDriveIndexes[0]], AppMessage_PauseCopy, 0, 0);
		}
		break;
		case Identifier_Button_SkipCurrentFile:
		{
			pSharedData->bCancelCopy[byteSelectedDriveIndexes[0]] = TRUE;
			EnableWindow(ProgressData[byteSelectedDriveIndexes[0]].hWnd_Button_ContinueCopy, TRUE);
			EnableWindow(ProgressData[byteSelectedDriveIndexes[0]].hWnd_Button_PauseCopy, FALSE);
			PostMessageW(pSharedData->hWnd_MainWindows[byteSelectedDriveIndexes[0]], AppMessage_ContinueCopy, 0, 0);
		}
		break;
		case Identifier_ComboBox_SelectDrive:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				ShowSpecifiedChildWindows(byteSelectedDriveIndexes[0], SW_HIDE);
				ShowSpecifiedChildWindows(byteSelectedDriveIndexes[0] = (BYTE)ComboBox_GetItemData((HWND)lParam, ComboBox_GetCurSel((HWND)lParam)) - 'A', SW_SHOW);
			}
		}
		break;
		}
	}
	break;
	case WM_MEASUREITEM:
	{
		if (((LPMEASUREITEMSTRUCT)lParam)->CtlType == ODT_COMBOBOX)
		{
			((LPMEASUREITEMSTRUCT)lParam)->itemWidth = 0;
			((LPMEASUREITEMSTRUCT)lParam)->itemHeight = (iCurrentDPI_Y == USER_DEFAULT_SCREEN_DPI) ? (19) : ScaleY(22, iCurrentDPI_Y);
			return TRUE;
		}
	}
	break;
	case WM_DRAWITEM: DrawComboBox((LPDRAWITEMSTRUCT)lParam); break;
	case WM_PAINT:
	{
		PAINTSTRUCT PaintStruct;
		RECT rect;
		GetClientRect(hWnd, &rect);
		rect.top = ScaleY(rect.top + 40, iCurrentDPI_Y);
		HDC hDC = BeginPaint(hWnd, &PaintStruct);
		SelectObject(hDC, (HGDIOBJ)hFonts[1]);
		SetTextColor(hDC, RGB(0, 120, 215));
		DrawTextW(hDC, L"Not Copying Data Now.\nSelect Drive on Home Page and Start Copy.", -1, &rect, DT_CENTER);
		EndPaint(hWnd, &PaintStruct);
	}
	break;
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WndProc_Page_Filters(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case Identifier_Menu_Delete:
		{
		DeleteItems:;
			LVITEMW ListViewItems[2];
			ListViewItems[1].mask = ListViewItems[0].mask = LVIF_PARAM;
			ListViewItems[0].iItem = -1;
			ListViewItems[1].iSubItem = ListViewItems[0].iSubItem = 0;
			while ((ListViewItems[0].iItem = ListView_GetNextItem(hWnd_ListView_DrivesList, -1, LVNI_SELECTED)) != -1)
			{
				LVFINDINFOW ListViewFindInfo;
				ListViewFindInfo.flags = LVFI_PARAM;
				ListViewFindInfo.lParam = (LPARAM)(SetFilePointer(hFile_DrivesDat, 0, NULL, FILE_END) - sizeof(DRIVEINFO));
				if ((ListViewItems[1].iItem = ListView_FindItem(hWnd_ListView_DrivesList, -1, &ListViewFindInfo)) != -1)
				{
					DRIVEINFO DriveInfo;
					ListView_GetItem(hWnd_ListView_DrivesList, &ListViewItems[0]);
					ListView_GetItem(hWnd_ListView_DrivesList, &ListViewItems[1]);
					SetFilePointer(hFile_DrivesDat, (LONG)ListViewItems[1].lParam, NULL, FILE_BEGIN);
					ReadFile(hFile_DrivesDat, &DriveInfo, sizeof(DriveInfo), &dwNumberOfBytesRead, NULL);
					SetFilePointer(hFile_DrivesDat, (LONG)ListViewItems[1].lParam, NULL, FILE_BEGIN);
					SetEndOfFile(hFile_DrivesDat);
					if (ListViewItems[1].lParam != ListViewItems[0].lParam)
					{
						SetFilePointer(hFile_DrivesDat, (LONG)ListViewItems[0].lParam, NULL, FILE_BEGIN);
						WriteFile(hFile_DrivesDat, &DriveInfo, sizeof(DriveInfo), &dwNumberOfBytesRead, NULL);
						ListViewItems[1].lParam = ListViewItems[0].lParam;
						ListView_SetItem(hWnd_ListView_DrivesList, &ListViewItems[1]);
					}
					ListView_DeleteItem(hWnd_ListView_DrivesList, ListViewItems[0].iItem);
				}
			}
		}
		break;
		case Identifier_Menu_Exclude:
		case Identifier_Menu_Resume:
		{
			LVITEMW ListViewItem;
			ListViewItem.mask = LVIF_PARAM;
			ListViewItem.iItem = -1;
			ListViewItem.iSubItem = 0;
			DRIVEINFO DriveInfo;
			while ((ListViewItem.iItem = ListView_GetNextItem(hWnd_ListView_DrivesList, ListViewItem.iItem, LVNI_SELECTED)) != -1)
			{
				ListView_GetItem(hWnd_ListView_DrivesList, &ListViewItem);
				SetFilePointer(hFile_DrivesDat, (LONG)ListViewItem.lParam, NULL, FILE_BEGIN);
				ReadFile(hFile_DrivesDat, &DriveInfo, sizeof(DriveInfo), &dwNumberOfBytesRead, NULL);
				SetFilePointer(hFile_DrivesDat, (LONG)ListViewItem.lParam, NULL, FILE_BEGIN);
				DriveInfo.byteIsExcluded = (LOWORD(wParam) == Identifier_Menu_Exclude);
				WriteFile(hFile_DrivesDat, &DriveInfo, sizeof(DriveInfo), &dwNumberOfBytesWritten, NULL);
				ListView_SetItemText(hWnd_ListView_DrivesList, ListViewItem.iItem, ListView_Column4_Status, (DriveInfo.byteIsExcluded) ? (L"Excluded") : (L"Normal"));
			}
		}
		break;
		}
	}
	break;
	case WM_NOTIFY:
	{
		switch (((LPNMHDR)lParam)->code)
		{
		case LVN_KEYDOWN:
		{
			switch (((LPNMLVKEYDOWN)lParam)->wVKey)
			{
			case VK_APPS: goto PopupMenu; break;
			case VK_DELETE: goto DeleteItems; break;
			}
		}
		break;
		case NM_RCLICK:
		{
			if (((LPNMHDR)lParam)->hwndFrom != ListView_GetHeader(hWnd_ListView_DrivesList))
			{
			PopupMenu:;
				POINT point;
				GetCursorPos(&point);
				HMENU hMenu = CreatePopupMenu();
				AppendMenuW(hMenu, MF_STRING, Identifier_Menu_Exclude, L"Exclude");
				AppendMenuW(hMenu, MF_STRING, Identifier_Menu_Resume, L"Resume");
				AppendMenuW(hMenu, MF_STRING, Identifier_Menu_Delete, L"Delete");
				TrackPopupMenuEx(hMenu, TPM_LEFTALIGN, point.x, point.y, hWnd, NULL);
				DestroyMenu(hMenu);
			}
		}
		break;
		}
	}
	break;
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WndProc_Page_About(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT PaintStruct;
		RECT rect;
		GetClientRect(hWnd, &rect);
		rect.left = ScaleX(rect.left + 10, iCurrentDPI_X);
		rect.top = ScaleY(rect.top + 10, iCurrentDPI_Y);
		HDC hDC = BeginPaint(hWnd, &PaintStruct);
		SelectObject(hDC, (HGDIOBJ)hFonts[1]);
		SetTextColor(hDC, RGB(0, 120, 215));
		DrawTextW(hDC, L"Terms of Use:\nThis program is mainly designed to steal data from local disks.\nI am not responsible for any accident such as data loss and damage to disks.", -1, &rect, DT_WORDBREAK);
		EndPaint(hWnd, &PaintStruct);
	}
	break;
	case WM_NOTIFY:
	{
		if (((LPNMHDR)lParam)->code == EN_LINK && ((ENLINK*)lParam)->msg == WM_LBUTTONUP)
		{
			ShellExecuteW(NULL, L"open", L"https://github.com/Programmer-YangXun/", NULL, NULL, SW_SHOW);
		}
	}
	break;
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WndProc_MessageOnly(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		if ((hThread_CopyData = CreateThread(NULL, 0, Thread_CopyData, NULL, 0, NULL)) == NULL)
		{
			ExitProcess(0);
		}
	}
	case WM_DEVICECHANGE:
	{
		if (((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype == DBT_DEVTYP_HANDLE)
		{
			switch (wParam)
			{
			case DBT_DEVICEQUERYREMOVE:
			case DBT_DEVICEQUERYREMOVEFAILED:
			case DBT_DEVICEREMOVECOMPLETE:
			case DBT_DEVICEREMOVEPENDING: ExitProcess(0); break;
			}
		}
	}
	break;
	case AppMessage_ContinueCopy:
	{
		ResumeThread(hThread_CopyData);
		ResumeThread(hThread_Timer);
	}
	break;
	case AppMessage_PauseCopy:
	{
		SuspendThread(hThread_CopyData);
		SuspendThread(hThread_Timer);
	}
	break;
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

INT CALLBACK ListViewSortCompareItemsEx(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	WCHAR szBuffer[2][2];
	LVITEMW ListViewItems[2];
	ListViewItems[1].mask = ListViewItems[0].mask = LVIF_TEXT;
	ListViewItems[0].iItem = (INT)lParam1;
	ListViewItems[1].iItem = (INT)lParam2;
	ListViewItems[1].iSubItem = ListViewItems[0].iSubItem = ListView_Column2_Letter;
	ListViewItems[0].pszText = szBuffer[0];
	ListViewItems[1].pszText = szBuffer[1];
	ListViewItems[1].cchTextMax = ListViewItems[0].cchTextMax = _countof(*szBuffer);
	ListView_GetItem(hWnd_ListView_DrivesList, &ListViewItems[0]);
	ListView_GetItem(hWnd_ListView_DrivesList, &ListViewItems[1]);
	if ((*szBuffer[0] == L'[' && *szBuffer[1] != L'['))
	{
		return 1;
	}
	else if (*szBuffer[0] != L'[' && *szBuffer[1] == L'[')
	{
		return -1;
	}
	return StrCmpLogicalW(szBuffer[0], szBuffer[1]);
}

DWORD CALLBACK CopyProgressRoutine(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber, DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
	static BYTE byteProgressBarPos;
	static LONGLONG llongTotalBytesTransferred;
	if (!TotalBytesTransferred.QuadPart)
	{
		byteProgressBarPos = 0;
		llongTotalBytesTransferred = 0;
		SetEndOfFile(hDestinationFile);
	}
	else if (TotalBytesTransferred.QuadPart == TotalFileSize.QuadPart)
	{
		byteProgressBarPos = 100;
		SendMessage_WM_COPYDATA(MAKEDWORD(WindowEvent_SetProgressBarPos, byteSelectedDriveIndexes[0]),
			sizeof(byteProgressBarPos),
			&byteProgressBarPos);
		SetFileTime(hDestinationFile, &((LPWIN32_FIND_DATAW)lpData)->ftCreationTime, &((LPWIN32_FIND_DATAW)lpData)->ftLastAccessTime, &((LPWIN32_FIND_DATAW)lpData)->ftLastWriteTime);
	}
	else if (dwCallbackReason == CALLBACK_CHUNK_FINISHED && TotalBytesTransferred.QuadPart - llongTotalBytesTransferred >= TotalFileSize.QuadPart / 100)
	{
		llongTotalBytesTransferred = TotalBytesTransferred.QuadPart;
		byteProgressBarPos++;
		SendMessage_WM_COPYDATA(MAKEDWORD(WindowEvent_SetProgressBarPos, byteSelectedDriveIndexes[0]),
			sizeof(byteProgressBarPos),
			&byteProgressBarPos);
	}
	return PROGRESS_CONTINUE;
}

DWORD WINAPI Thread_CheckForUpdate(LPVOID lpvParam)
{
	DWORD dwFlags;
	if (InternetGetConnectedState(&dwFlags, 0))
	{
		HINTERNET hInternetOpen = InternetOpenW(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (hInternetOpen)
		{
			HINTERNET hInternetOpenUrl = InternetOpenUrlW(hInternetOpen, L"https://github.com/Programmer-YangXun/Disk-Spy/releases/download/Latest/Disk.Spy.exe", NULL, 0, INTERNET_FLAG_RELOAD, 0);
			if (hInternetOpenUrl)
			{
				WCHAR szTempFileName[MAX_PATH], szTempPath[MAX_PATH];
				GetTempPathW(_countof(szTempPath), szTempPath);
				GetTempFileNameW(szTempPath, L"", 0, szTempFileName);
				HANDLE hFile = CreateFileW(szTempFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					BYTE byteBuffer[1024];
					while (InternetReadFile(hInternetOpenUrl, byteBuffer, sizeof(byteBuffer), &dwNumberOfBytesRead))
					{
						WriteFile(hFile, byteBuffer, dwNumberOfBytesRead, &dwNumberOfBytesWritten, NULL);
						if (dwNumberOfBytesRead != sizeof(byteBuffer))
						{
							break;
						}
					}
					CloseHandle(hFile);
					WCHAR szFileProductVersions[2][20];
					if (GetFileProductVersion(szTempFileName, szFileProductVersions[0], _countof(*szFileProductVersions))
						&& GetFileProductVersion(szProgramFileName, szFileProductVersions[1], _countof(*szFileProductVersions))
						&& StrCmpLogicalW(szFileProductVersions[0], szFileProductVersions[1]) == 1
						&& MessageBoxW(pSharedData->hWnd_MainWindows[byteLocalDriveIndex], L"Found a new version, do you want to update now?", L"Confirm", MB_ICONQUESTION | MB_YESNO) == IDYES)
					{
						WCHAR szCommandLine[MAX_PATH * 2 + 20];
						SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
						SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
						SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szProgramFileName, NULL);
						wnsprintfW(szCommandLine, _countof(szCommandLine), L"/c copy \"%ls\" \"%ls\" & del \"%ls\"", szTempFileName, szProgramFileName, szTempFileName);
						ShellExecuteW(NULL, L"open", L"cmd.exe", szCommandLine, NULL, SW_HIDE);
						ExitProcess(0);
					}
					else
					{
						DeleteFileW(szTempFileName);
					}
				}
				InternetCloseHandle(hInternetOpenUrl);
			}
			InternetCloseHandle(hInternetOpen);
		}
	}
	return 0;
}

DWORD WINAPI Thread_CopyData(LPVOID lpvParam)
{
	LPCWSTR lpcwRootPathName = (WCHAR[]) { byteSelectedDriveIndexes[0] + L'A', L':', L'\\', 0 };
	DEV_BROADCAST_HANDLE DevBroadcastHandle = { sizeof(DevBroadcastHandle) };
	DevBroadcastHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
	if ((DevBroadcastHandle.dbch_handle = CreateFileW(lpcwRootPathName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL)) != INVALID_HANDLE_VALUE
		&& (DevBroadcastHandle.dbch_hdevnotify = RegisterDeviceNotificationW(pSharedData->hWnd_MainWindows[byteSelectedDriveIndexes[0]], &DevBroadcastHandle, DEVICE_NOTIFY_WINDOW_HANDLE)) != NULL)
	{
		WCHAR szLogFileName[70], szTargetPathName[40];
		SYSTEMTIME LocalTime;
		GetLocalTime(&LocalTime);
		wnsprintfW(szTargetPathName, _countof(szTargetPathName), L"\\\\?\\%ls%lX\\", szStorageDirectoryName, pSharedData->dwDriveIdentifier[byteSelectedDriveIndexes[0]]);
		SHCreateDirectory(NULL, szTargetPathName);
		wnsprintfW(szLogFileName, _countof(szLogFileName), L"%ls\\Logs\\", szStorageDirectoryName);
		SHCreateDirectory(NULL, szLogFileName);
		wnsprintfW(szLogFileName, _countof(szLogFileName), L"%ls%lX [%hu-%02hu-%02hu %02hu-%02hu-%02hu].log", szLogFileName, pSharedData->dwDriveIdentifier[byteSelectedDriveIndexes[0]], LocalTime.wYear, LocalTime.wMonth, LocalTime.wDay, LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond);
		if ((hFile_DataCopyDetailsLog = CreateFileW(szLogFileName, GENERIC_WRITE | DELETE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
		{
			HANDLE hHeap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
			if (hHeap)
			{
				LPWSTR lpwNewFileName = (LPWSTR)HeapAlloc(hHeap, HEAP_NO_SERIALIZE, MAX_UNICODE_PATH * sizeof(WCHAR)),
					lpwPathName = (LPWSTR)HeapAlloc(hHeap, HEAP_NO_SERIALIZE, MAX_UNICODE_PATH * sizeof(WCHAR)),
					lpwString = (LPWSTR)HeapAlloc(hHeap, HEAP_NO_SERIALIZE, MAX_UNICODE_PATH * 2 * sizeof(WCHAR));
				if (lpwNewFileName == NULL || lpwPathName == NULL || lpwString == NULL)
				{
					ExitProcess(0);
				}
				WriteFile(hFile_DataCopyDetailsLog, "\xff\xfe", 2, &dwNumberOfBytesWritten, NULL);
				WriteFile(hFile_DataCopyDetailsLog, lpwString, sizeof(WCHAR) * wnsprintfW(lpwString, MAX_UNICODE_PATH * 2, L"Drive %lX\r\n", pSharedData->dwDriveIdentifier[byteSelectedDriveIndexes[0]]), &dwNumberOfBytesWritten, NULL);
				if (pSharedData->dwMaxCopyTime[byteSelectedDriveIndexes[0]])
				{
					hThread_Timer = CreateThread(NULL, 0, Thread_Timer, NULL, 0, NULL);
				}
				wnsprintfW(lpwPathName, MAX_UNICODE_PATH, L"\\\\?\\%lc:", *lpcwRootPathName);
				DWORD64 dw64FileSize;
				WCHAR szByteSize[11];
				struct _DATA {
					BYTE byteIsFindFileJumped;
					INT iLengthOfPathName;
					HANDLE hDirectory, hFindFile;
					FILETIME FileTimes[3];
					struct _DATA *Previous, *Next;
				} FindData, *pFindData = &FindData;
				FindData.Previous = NULL;
				WIN32_FIND_DATAW Win32_FindData, Win32_FindData_Temp;
				*Win32_FindData.cFileName = 0;
				Win32_FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
				while (TRUE)
				{
					if (*Win32_FindData.cFileName != L'.' || (Win32_FindData.cFileName[1] && (Win32_FindData.cFileName[1] != L'.' || Win32_FindData.cFileName[2])))
					{
						if (Win32_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							if ((pFindData->Next = (struct _DATA*)HeapAlloc(hHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, sizeof(FindData))) == NULL)
							{
								ExitProcess(0);
							}
							pFindData->Next->Previous = pFindData;
							pFindData->Next->iLengthOfPathName = wnsprintfW(lpwPathName, MAX_UNICODE_PATH, L"%ls%ls\\*.*", lpwPathName, Win32_FindData.cFileName) - 3;
							pFindData->Next->hFindFile = FindFirstFileW(lpwPathName, &Win32_FindData);
							pFindData->Next->FileTimes[0] = Win32_FindData.ftCreationTime;
							pFindData->Next->FileTimes[1] = Win32_FindData.ftLastAccessTime;
							pFindData->Next->FileTimes[2] = Win32_FindData.ftLastWriteTime;
							lpwPathName[pFindData->Next->iLengthOfPathName] = 0;
							wnsprintfW(lpwNewFileName, MAX_UNICODE_PATH, L"%ls%ls", szTargetPathName, lpwPathName + 7);
							SendMessage_WM_COPYDATA(MAKEDWORD(WindowEvent_OutputDirectoryName, byteSelectedDriveIndexes[0]),
								sizeof(WCHAR) * (wnsprintfW(lpwString, MAX_UNICODE_PATH * 2, L"%ls\n->\n%ls", lpwPathName + 4, szTargetPathName + 4) + 1),
								lpwString);
							if (SHCreateDirectory(NULL, lpwNewFileName) == ERROR_SUCCESS)
							{
								byteIsDirectoryCreated = TRUE;
							}
							pFindData->Next->hDirectory = CreateFileW(lpwNewFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
							pFindData = pFindData->Next;
							if (pFindData->Previous == &FindData)
							{
								continue;
							}
							SetFileAttributesW(lpwNewFileName, Win32_FindData.dwFileAttributes);
							WriteFile(hFile_DataCopyDetailsLog, lpwString, sizeof(WCHAR) * wnsprintfW(lpwString, MAX_UNICODE_PATH * 2, L"\r\n[%ls]\r\n", lpwPathName + 6), &dwNumberOfBytesWritten, NULL);
						}
						else if ((dw64FileSize = MAKEDWORD64(Win32_FindData.nFileSizeHigh, Win32_FindData.nFileSizeLow)) <= pSharedData->dw64MaxFileSize[byteSelectedDriveIndexes[0]])
						{
							wnsprintfW(lpwNewFileName, MAX_UNICODE_PATH, L"%ls%ls%ls", szTargetPathName, lpwPathName + 7, Win32_FindData.cFileName);
							BYTE byteIsFileExisting = FindClose(FindFirstFileW(lpwNewFileName, &Win32_FindData_Temp));
							if (!byteIsFileExisting || MAKEDWORD64(Win32_FindData_Temp.nFileSizeHigh, Win32_FindData_Temp.nFileSizeLow) != dw64FileSize || MAKEDWORD64(Win32_FindData.ftLastWriteTime.dwHighDateTime, Win32_FindData.ftLastWriteTime.dwLowDateTime) != MAKEDWORD64(Win32_FindData_Temp.ftLastWriteTime.dwHighDateTime, Win32_FindData_Temp.ftLastWriteTime.dwLowDateTime))
							{
								if (pFindData->byteIsFindFileJumped)
								{
									pFindData->byteIsFindFileJumped = FALSE;
									SendMessage_WM_COPYDATA(MAKEDWORD(WindowEvent_OutputDirectoryName, byteSelectedDriveIndexes[0]),
										sizeof(WCHAR) * (wnsprintfW(lpwString, MAX_UNICODE_PATH * 2, L"%ls\n->\n%ls", lpwPathName + 4, szTargetPathName + 4) + 1),
										lpwString);
									WriteFile(hFile_DataCopyDetailsLog, lpwString, sizeof(WCHAR) * wnsprintfW(lpwString, MAX_UNICODE_PATH * 2, L"\r\n[%ls]\r\n", lpwPathName + 6), &dwNumberOfBytesWritten, NULL);
								}
								StrFormatByteSizeW(dw64FileSize, szByteSize, _countof(szByteSize));
								SendMessage_WM_COPYDATA(MAKEDWORD(WindowEvent_OutputFileName, byteSelectedDriveIndexes[0]),
									sizeof(WCHAR) * (wnsprintfW(lpwString, MAX_UNICODE_PATH * 2, L"%ls\n(Size: %ls)", Win32_FindData.cFileName, szByteSize) + 1),
									lpwString);
								SetFileAttributesW(lpwNewFileName, FILE_ATTRIBUTE_NORMAL);
								WriteFile(hFile_DataCopyDetailsLog, lpwString, sizeof(WCHAR) * wnsprintfW(lpwString, MAX_UNICODE_PATH * 2, L"\"%ls\" (Size: %ls)", Win32_FindData.cFileName, szByteSize), &dwNumberOfBytesWritten, NULL);
								if (byteIsFileExisting)
								{
									WriteFile(hFile_DataCopyDetailsLog, L"[Updated]", sizeof(L"[Updated]") - sizeof(WCHAR), &dwNumberOfBytesWritten, NULL);
								}
								WriteFile(hFile_DataCopyDetailsLog, L"[Unfinished]\r\n", sizeof(L"[Unfinished]") + sizeof(WCHAR), &dwNumberOfBytesWritten, NULL);
								wnsprintfW(lpwString, MAX_UNICODE_PATH * 2, L"%ls%ls", lpwPathName, Win32_FindData.cFileName);
								if (pSharedData->bCancelCopy[byteSelectedDriveIndexes[0]] == 2)
								{
									pSharedData->bCancelCopy[byteSelectedDriveIndexes[0]] = 0;
									SuspendThread(hThread_CopyData);
								}
								if (CopyFileExW(lpwString, lpwNewFileName, CopyProgressRoutine, &Win32_FindData, &pSharedData->bCancelCopy[byteSelectedDriveIndexes[0]], COPY_FILE_ALLOW_DECRYPTED_DESTINATION))
								{
									byteIsFileCopied = TRUE;
									SetFilePointer(hFile_DataCopyDetailsLog, -(INT)(sizeof(L"[Unfinished]") + sizeof(WCHAR)), NULL, FILE_CURRENT);
									WriteFile(hFile_DataCopyDetailsLog, L"\r\n", 4, &dwNumberOfBytesWritten, NULL);
								}
								else if (pSharedData->bCancelCopy[byteSelectedDriveIndexes[0]] == TRUE)
								{
									pSharedData->bCancelCopy[byteSelectedDriveIndexes[0]] = 2;
									SetFilePointer(hFile_DataCopyDetailsLog, -4, NULL, FILE_CURRENT);
									WriteFile(hFile_DataCopyDetailsLog, L"[Skipped]\r\n", sizeof(L"[Skipped]") + sizeof(WCHAR), &dwNumberOfBytesWritten, NULL);
								}
								SetEndOfFile(hFile_DataCopyDetailsLog);
							}
						}
					}
					while (!FindNextFileW(pFindData->hFindFile, &Win32_FindData))
					{
						FindClose(pFindData->hFindFile);
						SetFileTime(pFindData->hDirectory, &pFindData->FileTimes[0], &pFindData->FileTimes[1], &pFindData->FileTimes[2]);
						CloseHandle(pFindData->hDirectory);
						if ((pFindData = pFindData->Previous) == &FindData)
						{
							ProcessDataCopyDetailsLogFile();
							ExitProcess(0);
						}
						HeapFree(hHeap, HEAP_NO_SERIALIZE, pFindData->Next);
						lpwPathName[pFindData->iLengthOfPathName] = 0;
						pFindData->byteIsFindFileJumped = TRUE;
					}
				}
			}
		}
	}
	ExitProcess(0);
}

DWORD WINAPI Thread_Timer(LPVOID lpvParam)
{
	WaitForSingleObject(hThread_CopyData, pSharedData->dwMaxCopyTime[byteSelectedDriveIndexes[0]]);
	ProcessDataCopyDetailsLogFile();
	ExitProcess(0);
}

DWORD WINAPI Thread_WaitForSingleProcess(LPVOID lpvParam)
{
	WaitForSingleObject(ProcessesInformation[(BYTE)lpvParam].hProcess, INFINITE);
	CloseHandle(ProcessesInformation[(BYTE)lpvParam].hProcess);
	ProcessesInformation[(BYTE)lpvParam].hProcess = NULL;
	pSharedData->hWnd_MainWindows[(BYTE)lpvParam] = NULL;
	EnableWindow(ProgressData[(BYTE)lpvParam].hWnd_Button_StopCopy, FALSE);
	EnableWindow(ProgressData[(BYTE)lpvParam].hWnd_Button_PauseCopy, FALSE);
	EnableWindow(ProgressData[(BYTE)lpvParam].hWnd_Button_ContinueCopy, FALSE);
	EnableWindow(ProgressData[(BYTE)lpvParam].hWnd_Button_SkipCurrentFile, FALSE);
	SetWindowTextW(ProgressData[(BYTE)lpvParam].hWnd_RichEdit_OutputDirectoryName, NULL);
	SetWindowTextW(ProgressData[(BYTE)lpvParam].hWnd_RichEdit_OutputFileName, NULL);
	SendMessageW(ProgressData[(BYTE)lpvParam].hWnd_ProgressBar, PBM_SETPOS, 100, 0);
	return 0;
}

BOOL GetFileProductVersion(LPCWSTR lpcwFileName, LPWSTR lpwFileProductVersionBuffer, DWORD cchFileProductVersionBuffer)
{
	BOOL bIsSuccessful = FALSE;
	DWORD dwFileVersionInfoSize = GetFileVersionInfoSizeW(lpcwFileName, NULL);
	if (lpwFileProductVersionBuffer && cchFileProductVersionBuffer && dwFileVersionInfoSize)
	{
		HANDLE hHeap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
		if (hHeap)
		{
			PVOID pvFileVersionInfo = HeapAlloc(hHeap, HEAP_NO_SERIALIZE, dwFileVersionInfoSize);
			if (pvFileVersionInfo && GetFileVersionInfoW(lpcwFileName, 0, dwFileVersionInfoSize, pvFileVersionInfo))
			{
				struct {
					WORD wLanguage, wCodePage;
				} *lpTranslate;
				UINT cbTranslationSize;
				if (VerQueryValueW(pvFileVersionInfo, L"\\VarFileInfo\\Translation", &lpTranslate, &cbTranslationSize) && cbTranslationSize / sizeof(*lpTranslate) != 0)
				{
					UINT cbFileProductVersionSize;
					LPWSTR lpwFileProductVersion;
					WCHAR szSubBlock[50];
					wnsprintfW(szSubBlock, _countof(szSubBlock), L"\\StringFileInfo\\%04X%04X\\ProductVersion", lpTranslate->wLanguage, lpTranslate->wCodePage);
					if (VerQueryValueW(pvFileVersionInfo, szSubBlock, (PVOID*)&lpwFileProductVersion, &cbFileProductVersionSize))
					{
						wnsprintfW(lpwFileProductVersionBuffer, cchFileProductVersionBuffer, lpwFileProductVersion);
						bIsSuccessful = TRUE;
					}
				}
			}
			HeapDestroy(hHeap);
		}
	}
	return bIsSuccessful;
}

DWORD GetEnhancedVolumeSerialNumberCRC32(WCHAR wchDriveLetter, DWORD dwVolumeSerialNumber)
{
	BYTE byteBuffer[256];
	WCHAR szDeviceInfo[128];
	szDeviceInfo[0] = 0;
	HANDLE hDevice = CreateFileW((WCHAR[]) { L'\\', L'\\', L'.', L'\\', wchDriveLetter, L':', 0 }, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		DWORD dwBytesReturned;
		STORAGE_DEVICE_NUMBER StorageDeviceNumber;
		DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &StorageDeviceNumber, sizeof(StorageDeviceNumber), &dwBytesReturned, NULL);
		CloseHandle(hDevice);
		DWORD dwVolumeDeviceNumber = StorageDeviceNumber.DeviceNumber;
		GUID GUID_DEVINTERFACE_USB_DISK = { 0x53f56307L, 0xb6bf, 0x11d0,{ 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b } };
		HDEVINFO hDevInfo = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		PSP_DEVICE_INTERFACE_DETAIL_DATA_W pSP_DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)byteBuffer;
		SP_DEVICE_INTERFACE_DATA SP_DeviceInterfaceData = { sizeof(SP_DeviceInterfaceData) };
		DWORD dwIndex = 0;
		while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DISK, dwIndex++, &SP_DeviceInterfaceData))
		{
			DWORD cbSize = 0;
			SetupDiGetDeviceInterfaceDetailW(hDevInfo, &SP_DeviceInterfaceData, NULL, 0, &cbSize, NULL);
			if (cbSize && cbSize <= sizeof(byteBuffer))
			{
				pSP_DeviceInterfaceDetailData->cbSize = sizeof(*pSP_DeviceInterfaceDetailData);
				SP_DEVINFO_DATA SP_DevInfoData = { sizeof(SP_DevInfoData) };
				if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &SP_DeviceInterfaceData, pSP_DeviceInterfaceDetailData, cbSize, &cbSize, &SP_DevInfoData))
				{
					HANDLE hDrive = CreateFileW(pSP_DeviceInterfaceDetailData->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
					if (hDrive != INVALID_HANDLE_VALUE)
					{
						DeviceIoControl(hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &StorageDeviceNumber, sizeof(StorageDeviceNumber), &dwBytesReturned, NULL);
						CloseHandle(hDrive);
						if (StorageDeviceNumber.DeviceNumber == dwVolumeDeviceNumber)
						{
							wnsprintfW(szDeviceInfo, _countof(szDeviceInfo), pSP_DeviceInterfaceDetailData->DevicePath);
							break;
						}
					}
				}
			}
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
	DWORD dwCRC32 = 0xffffffff;
	static DWORD dwCRC32Table[256];
	if (!dwCRC32Table[1])
	{
		for (WORD wIndex = 0; wIndex < _countof(dwCRC32Table); wIndex++)
		{
			dwCRC32Table[wIndex] = wIndex;
			for (BYTE byteIndex = 0; byteIndex < 8; byteIndex++)
			{
				if (dwCRC32Table[wIndex] & 0x00000001)
				{
					dwCRC32Table[wIndex] = (dwCRC32Table[wIndex] >> 1) ^ 0xEDB88320;
				}
				else
				{
					dwCRC32Table[wIndex] >>= 1;
				}
			}
		}
	}
	wnsprintfW(szDeviceInfo, _countof(szDeviceInfo), L"%ls%lX", szDeviceInfo, dwVolumeSerialNumber);
	LPWSTR lpwCRC32Buffer = szDeviceInfo;
	while (*lpwCRC32Buffer)
	{
		dwCRC32 = (dwCRC32 >> 8) ^ dwCRC32Table[(dwCRC32 & 0xff) ^ *lpwCRC32Buffer++];
	}
	dwCRC32 ^= 0xffffffff;
	return dwCRC32;
}

LSTATUS RunAtStartup(BYTE byteParam)
{
	BOOL bIsWoW64Process;
	if (!IsWow64Process(GetCurrentProcess(), &bIsWoW64Process))
	{
		bIsWoW64Process = FALSE;
	}
	HKEY hKey;
	LSTATUS lStatus = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, ((bIsWoW64Process) ? (KEY_WOW64_64KEY) : (KEY_WOW64_32KEY)) | ((byteParam == RegistryOperationFlag_QueryValue) ? (KEY_READ) : (KEY_READ | KEY_WRITE)), NULL, &hKey, NULL);
	if (lStatus == ERROR_SUCCESS)
	{
		if (byteParam == RegistryOperationFlag_SetValue)
		{
			lStatus = RegSetValueExW(hKey, lpcwProjectName, 0, REG_SZ, (PBYTE)szProgramFileName, sizeof(WCHAR) * lstrlenW(szProgramFileName));
		}
		else
		{
			BYTE byteIsValueQueried = FALSE;
			DWORD dwIndex = 0;
			WCHAR szValueName[MAX_PATH], szData[MAX_PATH];
			do
			{
				DWORD cbSizeOfData = sizeof(szData), cchValueName = _countof(szValueName);
				if ((lStatus = RegEnumValueW(hKey, dwIndex++, szValueName, &cchValueName, NULL, NULL, (PBYTE)szData, &cbSizeOfData)) != ERROR_SUCCESS)
				{
					break;
				}
				if (!StrCmpIW(szData, szProgramFileName))
				{
					byteIsValueQueried = TRUE;
					if ((byteParam == RegistryOperationFlag_QueryValue) || (byteParam == RegistryOperationFlag_DeleteValue && (lStatus = RegDeleteValueW(hKey, szValueName)) != ERROR_SUCCESS))
					{
						break;
					}
				}
			} while (lStatus != ERROR_NO_MORE_ITEMS);
			lStatus = (byteIsValueQueried) ? (ERROR_SUCCESS) : (lStatus);
		}
		RegCloseKey(hKey);
	}
	return lStatus;
}

VOID DrawComboBox(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct->CtlType == ODT_COMBOBOX)
	{
		if (lpDrawItemStruct->itemState & ODS_DISABLED)
		{
			SetBkColor(lpDrawItemStruct->hDC, RGB(240, 240, 240));
		}
		else if (lpDrawItemStruct->itemID != -1)
		{
			SetBkColor(lpDrawItemStruct->hDC, GetSysColor((lpDrawItemStruct->itemState & ODS_SELECTED) ? (COLOR_HIGHLIGHT) : (COLOR_WINDOW)));
			SetTextColor(lpDrawItemStruct->hDC, GetSysColor((lpDrawItemStruct->itemState & ODS_SELECTED) ? (COLOR_HIGHLIGHTTEXT) : (COLOR_WINDOWTEXT)));
			if (lpDrawItemStruct->itemState & ODS_COMBOBOXEDIT)
			{
				SetBkColor(lpDrawItemStruct->hDC, RGB(255, 255, 255));
				SetTextColor(lpDrawItemStruct->hDC, RGB(0, 0, 0));
			}
			HANDLE hHeap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
			if (hHeap)
			{
				LPWSTR lpwBuffer = HeapAlloc(hHeap, HEAP_NO_SERIALIZE, ComboBox_GetLBTextLen(lpDrawItemStruct->hwndItem, lpDrawItemStruct->itemID));
				if (lpwBuffer)
				{
					TEXTMETRIC tm;
					GetTextMetricsW(lpDrawItemStruct->hDC, &tm);
					ExtTextOutW(lpDrawItemStruct->hDC,
						2 * LOWORD(GetDialogBaseUnits()) / 4, (lpDrawItemStruct->rcItem.bottom + lpDrawItemStruct->rcItem.top - tm.tmHeight) / 2,
						ETO_CLIPPED | ETO_OPAQUE, &lpDrawItemStruct->rcItem,
						lpwBuffer, ComboBox_GetLBText(lpDrawItemStruct->hwndItem, lpDrawItemStruct->itemID, lpwBuffer), NULL);
				}
				HeapDestroy(hHeap);
			}
			if (lpDrawItemStruct->itemState & ODS_COMBOBOXEDIT && lpDrawItemStruct->itemState & ODS_FOCUS)
			{
				DrawFocusRect(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem);
			}
		}
	}
}

VOID EnableSpecifiedChildWindows(BOOL bEnable)
{
	if (!bEnable)
	{
		Button_SetCheck(hWnd_CheckBox_LimitCopyTime, BST_UNCHECKED);
		Button_SetCheck(hWnd_CheckBox_LimitFileSize, BST_UNCHECKED);
		ComboBox_SetCurSel(hWnd_ComboBox_SelectFileSizeUnit, -1);
		ComboBox_SetCurSel(hWnd_ComboBox_SelectTimeUnit, -1);
		SetWindowTextW(hWnd_Edit_LimitCopyTime, L"0");
		SetWindowTextW(hWnd_Edit_LimitFileSize, L"0");
		EnableWindow(hWnd_ComboBox_SelectFileSizeUnit, FALSE);
		EnableWindow(hWnd_ComboBox_SelectTimeUnit, FALSE);
		EnableWindow(hWnd_Edit_LimitCopyTime, FALSE);
		EnableWindow(hWnd_Edit_LimitFileSize, FALSE);
	}
	EnableWindow(hWnd_Button_ApplyAndSaveCurrentSettings, bEnable);
	EnableWindow(hWnd_CheckBox_LimitCopyTime, bEnable);
	EnableWindow(hWnd_CheckBox_LimitFileSize, bEnable);
}

VOID ProcessDataCopyDetailsLogFile(VOID)
{
	if (!byteIsDirectoryCreated && !byteIsFileCopied)
	{
		FILE_DISPOSITION_INFO File_Disposition_Info = { TRUE };
		SetFileInformationByHandle(hFile_DataCopyDetailsLog, FileDispositionInfo, &File_Disposition_Info, sizeof(File_Disposition_Info));
	}
	else
	{
		WriteFile(hFile_DataCopyDetailsLog, L"\r\nComplete.\r\n", sizeof(L"\r\nComplete.") + sizeof(WCHAR), &dwNumberOfBytesWritten, NULL);
	}
}

VOID ShowSpecifiedChildWindows(BYTE byteIndex, INT iShowWindow)
{
	ShowWindow(ProgressData[byteIndex].hWnd_Button_StopCopy, iShowWindow);
	ShowWindow(ProgressData[byteIndex].hWnd_Button_PauseCopy, iShowWindow);
	ShowWindow(ProgressData[byteIndex].hWnd_Button_ContinueCopy, iShowWindow);
	ShowWindow(ProgressData[byteIndex].hWnd_Button_SkipCurrentFile, iShowWindow);
	ShowWindow(ProgressData[byteIndex].hWnd_RichEdit_OutputDirectoryName, iShowWindow);
	ShowWindow(ProgressData[byteIndex].hWnd_RichEdit_OutputFileName, iShowWindow);
	ShowWindow(ProgressData[byteIndex].hWnd_ProgressBar, iShowWindow);
}

VOID SendMessage_WM_COPYDATA(ULONG_PTR dwData, DWORD cbData, PVOID lpData)
{
	COPYDATASTRUCT CopyDataStruct = { dwData, cbData, lpData };
	SendMessageW(pSharedData->hWnd_MainWindows[byteLocalDriveIndex], WM_COPYDATA, 0, (LPARAM)&CopyDataStruct);
}
#pragma endregion