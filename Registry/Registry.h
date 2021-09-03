/*
 * Header File: Registry.h
 * Last Update: 2021/09/03
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

namespace Hydr10n {
	namespace Registry {
		enum class RegistryOperation { SetValue, DeleteValue, QueryValue };

		LSTATUS WINAPI RunAtStartup(RegistryOperation operation, LPCWSTR lpName, LPCWSTR lpPath, LPCWSTR lpCommandLine, BOOL localMachineIfTrueElseCurrentUser);
	}
}
