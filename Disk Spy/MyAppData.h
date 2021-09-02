#pragma once

#include "AppData.h"

#include <algorithm>

#include <memory>

#pragma warning(push)
#pragma warning(disable: 26495)

struct MyAppData {
	MyAppData() {}

	class FilterData {
	public:
		enum class FileSizeUnit { KB, MB, GB };

		enum class TimeUnit { Sec, Min };

		struct CopyLimitsData {
			static constexpr INT MaxValue = 9999;

			INT MaxFileSize{};
			FileSizeUnit FileSizeUnit = FileSizeUnit::MB;

			INT MaxDuration{};
			TimeUnit TimeUnit = TimeUnit::Min;

			INT ReservedStorageSpaceGB{};
		} CopyLimits;

		struct WhitelistData {
			WCHAR SpecialFileName[9] = L"*";

			UINT DriveCount{};
			struct DriveData {
				DWORD ID;
				BOOL Excluded;
			} Drives[64];
		} Whitelist;

		FilterData() {
			constexpr WCHAR szFileName[] = L"Filter.ini";
			std::unique_ptr<WCHAR> exePath(new WCHAR[UNICODE_STRING_MAX_CHARS + ARRAYSIZE(szFileName) - 1]);
			if (exePath != nullptr && GetModuleFileNameW(nullptr, exePath.get(), UNICODE_STRING_MAX_CHARS)) {
				PathRemoveFileSpecW(exePath.get());
				PathAppendW(exePath.get(), szFileName);
				m_appData.SetPath(exePath.get());
			}
		}

		void Clamp() {
			using namespace std;
			CopyLimits.MaxFileSize = clamp(CopyLimits.MaxFileSize, 0, CopyLimitsData::MaxValue);
			CopyLimits.FileSizeUnit = clamp(CopyLimits.FileSizeUnit, FileSizeUnit::KB, FileSizeUnit::GB);
			CopyLimits.MaxDuration = clamp(CopyLimits.MaxDuration, 0, CopyLimitsData::MaxValue);
			CopyLimits.TimeUnit = clamp(CopyLimits.TimeUnit, TimeUnit::Sec, TimeUnit::Min);
			CopyLimits.ReservedStorageSpaceGB = clamp(CopyLimits.ReservedStorageSpaceGB, 0, CopyLimitsData::MaxValue);
		}

		BOOL Save() {
			Clamp();

			if (m_appData.Save(Sections::CopyLimits, Keys::MaxFileSize, CopyLimits.MaxFileSize)
				&& m_appData.Save(Sections::CopyLimits, Keys::FileSizeUnit, static_cast<INT>(CopyLimits.FileSizeUnit))
				&& m_appData.Save(Sections::CopyLimits, Keys::MaxDuration, CopyLimits.MaxDuration)
				&& m_appData.Save(Sections::CopyLimits, Keys::TimeUnit, static_cast<INT>(CopyLimits.TimeUnit))
				&& m_appData.Save(Sections::CopyLimits, Keys::ReservedStorageSpaceGB, CopyLimits.ReservedStorageSpaceGB)
				&& m_appData.Save(Sections::Whitelist, Keys::SpecialFileName, Whitelist.SpecialFileName)) {
				WCHAR szDrives[ARRAYSIZE(Whitelist.Drives) * 13 + 1];
				*szDrives = 0;

				for (UINT i = 0; i < min(ARRAYSIZE(Whitelist.Drives), Whitelist.DriveCount); i++) {
					Whitelist.Drives[i].Excluded = Whitelist.Drives[i].Excluded ? TRUE : FALSE;

					WCHAR szDrive[14];
					wsprintfW(szDrive, L"0x%lX,%d;", Whitelist.Drives[i].ID, Whitelist.Drives[i].Excluded);
					lstrcatW(szDrives, szDrive);
				}

				return m_appData.Save(Sections::Whitelist, Keys::Drives, szDrives);
			}

			return FALSE;
		}

		BOOL Load() {
			const auto ret = m_appData.Load(Sections::CopyLimits, Keys::MaxFileSize, CopyLimits.MaxFileSize)
				&& m_appData.Load(Sections::CopyLimits, Keys::FileSizeUnit, reinterpret_cast<INT&>(CopyLimits.FileSizeUnit))
				&& m_appData.Load(Sections::CopyLimits, Keys::MaxDuration, CopyLimits.MaxDuration)
				&& m_appData.Load(Sections::CopyLimits, Keys::TimeUnit, reinterpret_cast<INT&>(CopyLimits.TimeUnit))
				&& m_appData.Load(Sections::CopyLimits, Keys::ReservedStorageSpaceGB, CopyLimits.ReservedStorageSpaceGB);

			Clamp();

			if (ret && m_appData.Load(Sections::Whitelist, Keys::SpecialFileName, Whitelist.SpecialFileName, ARRAYSIZE(Whitelist.SpecialFileName))) {
				WCHAR szDrives[ARRAYSIZE(Whitelist.Drives) * 13 + 1];
				if (m_appData.Load(Sections::Whitelist, Keys::Drives, szDrives, ARRAYSIZE(szDrives))) {
					WCHAR szDrive[13]{};

					const auto ConvertString = [&] {
						if (Whitelist.DriveCount < ARRAYSIZE(WhitelistData::Drives)
							&& StrToIntExW(szDrive, STIF_SUPPORT_HEX, reinterpret_cast<int*>(&Whitelist.Drives[Whitelist.DriveCount].ID))) {
							const auto substr = StrStrW(szDrive, L",");
							if (substr != nullptr) {
								if (StrToIntExW(substr + 1, STIF_SUPPORT_HEX, reinterpret_cast<int*>(&Whitelist.Drives[Whitelist.DriveCount].Excluded))) {
									Whitelist.Drives[Whitelist.DriveCount].Excluded = Whitelist.Drives[Whitelist.DriveCount].Excluded ? TRUE : FALSE;
									Whitelist.DriveCount++;

									return TRUE;
								}
							}
						}

						SetLastError(ERROR_INVALID_DATA);

						return FALSE;
					};

					int i{};
					for (const auto ch : szDrives) {
						if (!ch)
							break;

						if (ch != ';') {
							if (i >= ARRAYSIZE(szDrive) - 1) {
								SetLastError(ERROR_INVALID_DATA);

								return FALSE;
							}

							szDrive[i++] = ch;
							szDrive[i] = 0;
						}
						else if (i) {
							if (!ConvertString())
								return FALSE;

							i = 0;
						}
					}

					if (i && !ConvertString())
						return FALSE;

					return TRUE;
				}
			}

			return FALSE;
		}

	private:
		struct Sections { static constexpr LPCWSTR CopyLimits = L"Copy Limits", Whitelist = L"Whitelist"; };

		struct Keys { static constexpr LPCWSTR MaxFileSize = L"MaxFileSize", FileSizeUnit = L"FileSizeUnit", MaxDuration = L"MaxDuration", TimeUnit = L"TimeUnit", ReservedStorageSpaceGB = L"ReservedStorageSpaceGB", SpecialFileName = L"SpecialFileName(8)", Drives = L"(DriveID,Excluded)[64]"; };

		Hydr10n::Data::AppData m_appData;
	} Filter;
};

#pragma warning(pop)
