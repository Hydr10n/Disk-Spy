/*
 * Header File: Registry.cpp
 * Last Update: 2022/01/03
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#include "Registry.h"

#include <Shlwapi.h>

#include <memory>

namespace Hydr10n::Registry {
	LSTATUS WINAPI RunAtStartup(RegistryOperation operation, LPCWSTR lpName, LPCWSTR lpPath, LPCWSTR lpCommandLine, BOOL localMachineIfTrueElseCurrentUser) {
		BOOL bIsWoW64Process;
		if (!IsWow64Process(GetCurrentProcess(), &bIsWoW64Process)) bIsWoW64Process = FALSE;

		HKEY hKey;
		auto status = RegCreateKeyExW(localMachineIfTrueElseCurrentUser ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\CurrentVersion\Run)", 0, nullptr, 0, (bIsWoW64Process ? KEY_WOW64_64KEY : KEY_WOW64_32KEY) | KEY_READ | (operation == RegistryOperation::QueryValue ? 0 : KEY_WRITE), nullptr, &hKey, nullptr);
		if (status == ERROR_SUCCESS) {
			switch (operation) {
			case RegistryOperation::DeleteValue: status = RegDeleteValueW(hKey, lpName); break;

			case RegistryOperation::SetValue: {
				if (lpCommandLine == nullptr) {
					status = RegSetValueExW(hKey, lpName, 0, REG_SZ, reinterpret_cast<const BYTE*>(lpPath), sizeof(*lpPath) * lstrlenW(lpPath));
				}
				else {
					const auto StrLen = lstrlenW(lpPath) + lstrlenW(lpCommandLine) + 4;
					const std::unique_ptr<WCHAR> buf(new WCHAR[StrLen]);
					const auto str = buf.get();
					status = str == nullptr ? static_cast<LSTATUS>(GetLastError()) : RegSetValueExW(hKey, lpName, 0, REG_SZ, reinterpret_cast<PBYTE>(str), sizeof(*str) * wnsprintfW(str, StrLen, LR"("%ls" %ls)", lpPath, lpCommandLine));
				}
			}	break;

			case RegistryOperation::QueryValue: {
				DWORD dwType, cbData;
				if ((status = RegQueryValueExW(hKey, lpName, nullptr, &dwType, nullptr, &cbData)) == ERROR_SUCCESS) {
					if (dwType == REG_SZ) {
						const std::unique_ptr<WCHAR> buf(new WCHAR[cbData]);
						const auto str = buf.get();
						if (str == nullptr) status = static_cast<LSTATUS>(GetLastError());
						else if ((status = RegQueryValueExW(hKey, lpName, nullptr, &dwType, reinterpret_cast<PBYTE>(str), &cbData)) == ERROR_SUCCESS) {
							PathRemoveArgsW(str);
							PathUnquoteSpacesW(str);
							if (lstrcmpiW(str, lpPath)) status = ERROR_FILE_NOT_FOUND;
						}
					}
					else status = ERROR_FILE_NOT_FOUND;
				}
			}	break;

			default: status = ERROR_BAD_ARGUMENTS; break;
			}

			RegCloseKey(hKey);
		}

		return status;
	}
}
