/*
 * Header File: FindFiles.cpp
 * Last Update: 2021/09/07
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#include "FindFiles.h"

#include "File.h"

#include <Shlwapi.h>

#include <memory>

using namespace Hydr10n::File;
using namespace std;

#define STOP() { SetLastError(ERROR_CANCELLED); return FALSE; }

namespace Hydr10n {
	namespace File {
		BOOL WINAPI FindFiles(LPCWSTR lpPath, ErrorOccuredEventHandler errorOccuredEventHandler, FileFoundEventHandler fileFoundEventHandler, EnterDirectoryEventHandler enterDirectoryEventHandler, LeaveDirectoryEventHandler leaveDirectoryEventHandler, LPVOID lpParam) {
			const unique_ptr<WCHAR> buf(new WCHAR[UNICODE_STRING_MAX_CHARS]);
			const auto str = buf.get();
			if (str == nullptr)
				return FALSE;

			const auto FindFiles = [&](const auto& FindFiles, DWORD dwDepth, ErrorOccuredEventHandler errorOccuredEventHandler, FileFoundEventHandler fileFoundEventHandler, EnterDirectoryEventHandler enterDirectoryEventHandler, LeaveDirectoryEventHandler leaveDirectoryEventHandler, LPVOID lpParam) {
				const auto strLen = wnsprintfW(str, UNICODE_STRING_MAX_CHARS, L"%ls*.*", str) - 3;

				WIN32_FIND_DATAW findData;

				const struct HandleWrapper {
					const HANDLE Handle;

					~HandleWrapper() { FindClose(Handle); }
				} wrapper{ FindFirstFileW(str, &findData) };

				str[strLen] = 0;

				BOOL ret = wrapper.Handle != INVALID_HANDLE_VALUE;
				if (ret) {
					do {
						if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
							if (enterDirectoryEventHandler != nullptr && !enterDirectoryEventHandler(str, findData, lpParam))
								STOP();

							if (!IsCurrentOrParentDirectory(findData.cFileName)) {
								wnsprintfW(str, UNICODE_STRING_MAX_CHARS, L"%ls%ls\\", str, findData.cFileName);

								if (!FindFiles(FindFiles, dwDepth + 1, errorOccuredEventHandler, fileFoundEventHandler, enterDirectoryEventHandler, leaveDirectoryEventHandler, lpParam))
									return FALSE;

								str[strLen] = 0;
							}

							if (leaveDirectoryEventHandler != nullptr && !leaveDirectoryEventHandler(str, findData, lpParam))
								STOP();
						}
						else if (fileFoundEventHandler != nullptr && !fileFoundEventHandler(str, findData, lpParam))
							STOP();
					} while (FindNextFileW(wrapper.Handle, &findData));

					if (GetLastError() == ERROR_NO_MORE_FILES)
						SetLastError(ERROR_SUCCESS);
					else
						ret = FALSE;
				}

				if (!ret && errorOccuredEventHandler != nullptr && !errorOccuredEventHandler(lpPath, lpParam) && dwDepth)
					STOP();

				return ret;
			};

			(void)lstrcpynW(str, lpPath, UNICODE_STRING_MAX_CHARS);

			return FindFiles(FindFiles, 0, errorOccuredEventHandler, fileFoundEventHandler, enterDirectoryEventHandler, leaveDirectoryEventHandler, lpParam);
		}
	}
}
