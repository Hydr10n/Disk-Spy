/*
 * Header File: RegistryHelpers.h
 * Last Update: 2021/08/31
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

namespace Hydr10n {
	namespace RegistryHelpers {
		enum class RegistryOperation { SetValue, DeleteValue, QueryValue };

		LSTATUS WINAPI RunAtStartup(RegistryOperation operation, LPCWSTR lpName, LPCWSTR lpPath, LPCWSTR lpCommandLine, BOOL localMachineIfTrueElseCurrentUser);
	}
}
