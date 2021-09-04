/*
 * Header File: FindFiles.h
 * Last Update: 2021/09/03
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

namespace Hydr10n {
	namespace File {
		using FileFoundEventHandler = BOOL CALLBACK(LPCWSTR lpPath, const WIN32_FIND_DATAW& findData, LPVOID lpParam);

		using EnterDirectoryEventHandler = FileFoundEventHandler;

		using LeaveDirectoryEventHandler = EnterDirectoryEventHandler;

		BOOL WINAPI FindFiles(LPCWSTR lpPath, FileFoundEventHandler fileFoundEventHandler, EnterDirectoryEventHandler enterDirectoryEventHandler, LeaveDirectoryEventHandler leaveDirectoryEventHandler, LPVOID lpParam);
	}
}
