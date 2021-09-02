#include "pch.h"

#include "MainDialog.h"

#ifdef _DEBUG
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
#else
#pragma comment(linker, "/ENTRY:Main")
int Main()
#endif
{
	const auto window = FindWindowW(L"#32770", AppName);
	if (window != nullptr) {
		ShowWindow(window, SW_SHOW);
		SetForegroundWindow(window);

		return ERROR_SUCCESS;
	}

	const auto dialog = CreateDialog(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_DIALOG_MAIN), nullptr, MainWindowProc);

	int argc;
	const auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv != nullptr && lstrcmpW(argv[1], Args.Background))
		ShowWindow(dialog, SW_SHOW);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
		if (!IsDialogMessage(dialog, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	return static_cast<int>(msg.wParam);
}
