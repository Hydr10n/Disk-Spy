/*
 * Header File: FindFiles.cpp
 * Last Update: 2021/09/03
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#include "FindFiles.h"

#include "File.h"

#include <Shlwapi.h>

#include <memory>

using namespace std;
using namespace Hydr10n::File;

namespace Hydr10n {
	namespace File {
		BOOL WINAPI FindFiles(LPCWSTR lpPath, FileFoundEventHandler fileFoundEventHandler, EnterDirectoryEventHandler enterDirectoryEventHandler, LeaveDirectoryEventHandler leaveDirectoryEventHandler, LPVOID lpParam) {
			const unique_ptr<WCHAR> buf(new WCHAR[UNICODE_STRING_MAX_CHARS]);
			const auto str = buf.get();
			if (str == nullptr)
				return FALSE;

			const auto FindFiles = [&](const auto& FindFiles, FileFoundEventHandler fileFoundEventHandler, EnterDirectoryEventHandler enterDirectoryEventHandler, LeaveDirectoryEventHandler leaveDirectoryEventHandler, LPVOID lpParam) {
				const int i = wnsprintfW(str, UNICODE_STRING_MAX_CHARS, L"%ls*.*", str) - 3;

				WIN32_FIND_DATAW findData;

				const struct HandleWrapper {
					const HANDLE Handle;

					~HandleWrapper() { FindClose(Handle); }
				} wrapper{ FindFirstFileW(str, &findData) };

				str[i] = 0;

				if (wrapper.Handle != INVALID_HANDLE_VALUE) {
					do {
						if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
							if (enterDirectoryEventHandler != nullptr && !enterDirectoryEventHandler(str, findData, lpParam)) {
								SetLastError(ERROR_CANCELLED);

								return FALSE;
							}

							if (!IsCurrentOrParentDirectory(findData.cFileName)) {
								wnsprintfW(str, UNICODE_STRING_MAX_CHARS, L"%ls%ls\\", str, findData.cFileName);

								if (!FindFiles(FindFiles, fileFoundEventHandler, enterDirectoryEventHandler, leaveDirectoryEventHandler, lpParam))
									return FALSE;

								str[i] = 0;
							}

							if (leaveDirectoryEventHandler != nullptr && !leaveDirectoryEventHandler(str, findData, lpParam)) {
								SetLastError(ERROR_CANCELLED);

								return FALSE;
							}
						}
						else if (fileFoundEventHandler != nullptr && !fileFoundEventHandler(str, findData, lpParam)) {
							SetLastError(ERROR_CANCELLED);

							return FALSE;
						}
					} while (FindNextFileW(wrapper.Handle, &findData));

					return TRUE;
				}

				return FALSE;
			};

			(void)lstrcpynW(str, lpPath, UNICODE_STRING_MAX_CHARS);

			return FindFiles(FindFiles, fileFoundEventHandler, enterDirectoryEventHandler, leaveDirectoryEventHandler, lpParam);
		}
	}
}
