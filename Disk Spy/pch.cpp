#include "pch.h"

#include "Device.h"

#include <initializer_list>

using namespace std;
using namespace Hydr10n::Device;

void* operator new(size_t size) { return HeapAlloc(GetProcessHeap(), 0, size); }

void operator delete(void* p) { HeapFree(GetProcessHeap(), 0, p); }

DWORD WINAPI GenerateDriveID(int iDriveNumber) {
	DWORD dwHash;
	struct {
		DWORD VolumeSerialNumber;
		WCHAR PNPDeviceID[200];
	} driveInfo;
	return GetVolumeInformationW(initializer_list<WCHAR>({ static_cast<WCHAR>('A' + iDriveNumber), ':', '\\', 0 }).begin(), nullptr, 0, &driveInfo.VolumeSerialNumber, nullptr, nullptr, nullptr, 0)
		&& GetPNPDeviceID(iDriveNumber, driveInfo.PNPDeviceID, ARRAYSIZE(driveInfo.PNPDeviceID))
		&& SUCCEEDED(HashData(reinterpret_cast<PBYTE>(&driveInfo), sizeof(driveInfo.VolumeSerialNumber) + lstrlenW(driveInfo.PNPDeviceID), reinterpret_cast<PBYTE>(&dwHash), sizeof(dwHash))) ? dwHash : 0;
}
