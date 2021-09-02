#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <Dbt.h>

#include "resource.h"

#pragma comment(lib, "Comctl32")
#pragma comment(lib, "Setupapi")
#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "UxTheme")

constexpr auto AppName = L"Disk Spy";

constexpr struct { LPCWSTR Background; } Args{ L"--background" };

DWORD WINAPI GenerateDriveID(int iDriveNumber);
