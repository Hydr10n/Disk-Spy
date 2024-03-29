/*
 * Header File: Device.cpp
 * Last Update: 2022/01/03
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#include "Device.h"

#include <SetupAPI.h>

#include <memory>

using namespace std;

namespace Hydr10n::Device {
	BOOL WINAPI GetPNPDeviceID(int iDriveNumber, LPWSTR lpPNPDeviceID, UINT nCount) {
		BOOL ret = FALSE;
		STORAGE_DEVICE_NUMBER storageDeviceNumber;
		if (GetDeviceNumber(initializer_list<WCHAR>({ '\\', '\\', '.', '\\', static_cast<WCHAR>('A' + iDriveNumber), ':', 0 }).begin(), storageDeviceNumber)) {
			const auto classDevs = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
			if (classDevs != INVALID_HANDLE_VALUE) {
				SP_DEVICE_INTERFACE_DATA deviceInterfaceData{ sizeof(deviceInterfaceData) };
				for (DWORD i = 0; SetupDiEnumDeviceInterfaces(classDevs, nullptr, &GUID_DEVINTERFACE_DISK, i, &deviceInterfaceData); i++) {
					DWORD dwRequiredSize;
					SetupDiGetDeviceInterfaceDetailW(classDevs, &deviceInterfaceData, nullptr, 0, &dwRequiredSize, nullptr);
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
						const shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData(reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(new BYTE[dwRequiredSize]));
						if (deviceInterfaceDetailData != nullptr) {
							deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
							if (SetupDiGetDeviceInterfaceDetailW(classDevs, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, nullptr, nullptr)) {
								STORAGE_DEVICE_NUMBER storageDeviceNumber2;
								if (GetDeviceNumber(deviceInterfaceDetailData->DevicePath, storageDeviceNumber2)) {
									if (storageDeviceNumber.DeviceNumber == storageDeviceNumber2.DeviceNumber) {
										if (nCount <= static_cast<DWORD>(lstrlenW(deviceInterfaceDetailData->DevicePath))) {
											SetLastError(ERROR_INSUFFICIENT_BUFFER);
										}
										else {
											ret = TRUE;
											(void)lstrcpynW(lpPNPDeviceID, deviceInterfaceDetailData->DevicePath, static_cast<int>(nCount));
										}
									}
								}
							}
						}
					}
				}
				SetupDiDestroyDeviceInfoList(classDevs);
			}
		}
		return ret;
	}
}
