/*
 * Header File: File.h
 * Last Update: 2021/08/31
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

namespace Hydr10n {
	namespace File {
		constexpr bool IsCurrentOrParentDirectory(LPCWSTR lpFileName) { return lpFileName[0] == '.' && (!lpFileName[1] || (lpFileName[1] == '.' && !lpFileName[2])); }

		inline BOOL WINAPI SetFileTime(LPCWSTR lpPath, const FILETIME* lpCreationTime, const FILETIME* lpLastAccessTime, const FILETIME* lpLastWriteTime) {
			BOOL ret = FALSE;

			const HANDLE hFile = CreateFileW(lpPath, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
			if (hFile != INVALID_HANDLE_VALUE) {
				ret = SetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);

				CloseHandle(hFile);
			}

			return ret;
		}
	}
}
