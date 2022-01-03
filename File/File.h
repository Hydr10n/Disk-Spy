/*
 * Header File: File.h
 * Last Update: 2022/01/03
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

namespace Hydr10n::File {
	constexpr bool IsCurrentOrParentDirectory(LPCWSTR lpFileName) { return lpFileName[0] == '.' && (!lpFileName[1] || (lpFileName[1] == '.' && !lpFileName[2])); }

	inline BOOL WINAPI SetFileTime(LPCWSTR lpPath, const FILETIME* lpCreationTime, const FILETIME* lpLastAccessTime, const FILETIME* lpLastWriteTime) {
		auto ret = FALSE;
		const auto file = CreateFileW(lpPath, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
		if (file != INVALID_HANDLE_VALUE) {
			ret = SetFileTime(file, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
			CloseHandle(file);
		}
		return ret;
	}
}
