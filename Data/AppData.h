/*
 * Header File: AppData.h
 * Last Update: 2021/08/31
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

#include <Shlwapi.h>

namespace Hydr10n {
	namespace Data {
		class AppData {
		public:
			AppData() = default;

			AppData(LPCWSTR lpPath) : m_path(StrDupW(lpPath)) {}

			~AppData() { LocalFree(m_path); }

			void SetPath(LPCWSTR lpPath) {
				LocalFree(m_path);
				m_path = StrDupW(lpPath);
			}

			LPCWSTR GetPath() const { return m_path; }

			BOOL Save(LPCWSTR lpSection, LPCWSTR lpKey, LPCWSTR data) const { return WritePrivateProfileStringW(lpSection, lpKey, data, m_path); }

			BOOL Save(LPCWSTR lpSection, LPCWSTR lpKey, INT data) const {
				WCHAR str[11];
				wsprintfW(str, L"%d", data);
				return Save(lpSection, lpKey, str);
			}

			BOOL Load(LPCWSTR lpSection, LPCWSTR lpKey, LPWSTR data, DWORD nSize) const { return GetPrivateProfileStringW(lpSection, lpKey, nullptr, data, nSize, m_path), GetLastError() == ERROR_SUCCESS; }

			BOOL Load(LPCWSTR lpSection, LPCWSTR lpKey, INT& data) const {
				data = GetPrivateProfileIntW(lpSection, lpKey, 0, m_path);
				return GetLastError() == ERROR_SUCCESS;
			}

		private:
			LPWSTR m_path;
		};
	}
}
