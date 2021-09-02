/*
 * Header File: Device.h
 * Last Update: 2021/09/02
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

namespace Hydr10n {
	namespace Device {
		constexpr int GetFirstDriveNumber(DWORD dwUnitMask) {
			int i = 0;
			while (i < 26 && !(dwUnitMask >> i & 1))
				i++;
			return i;
		}

		inline BOOL WINAPI GetDeviceNumber(LPCWSTR lpName, STORAGE_DEVICE_NUMBER& storageDeviceNumber) {
			const auto drive = CreateFileW(lpName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
			if (drive != INVALID_HANDLE_VALUE) {
				DWORD dwBytesReturned;
				const auto ret = DeviceIoControl(drive, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, &storageDeviceNumber, sizeof(storageDeviceNumber), &dwBytesReturned, nullptr);

				CloseHandle(drive);

				return ret;
			}

			return FALSE;
		};

		BOOL WINAPI GetPNPDeviceID(int iDriveNumber, LPWSTR lpPNPDeviceID, DWORD nCount);
	}
}
