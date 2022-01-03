/*
 * Header File: AppData.h
 * Last Update: 2022/01/03
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

#include <Shlwapi.h>

namespace Hydr10n::Data {
	struct AppData {
		AppData(const AppData&) = delete;
		AppData& operator=(const AppData&) = delete;

		AppData() = default;

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
			data = static_cast<INT>(GetPrivateProfileIntW(lpSection, lpKey, 0, m_path));
			return GetLastError() == ERROR_SUCCESS;
		}

	private:
		LPWSTR m_path;
	};
}
