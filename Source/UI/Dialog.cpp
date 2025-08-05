#include "Dialog.h"

static char pathBuffer[MAX_PATH];

// [ファイルを開く]ダイアログボックスを表示
DialogResult Dialog::OpenFileName(char* filepath, int size, const char* filter, const char* title, HWND hWnd, bool multiSelect)
{
	// 初期パス設定
	char dirname[MAX_PATH];
	if (filepath[0] != '0')
	{
		// ディレクトリパス取得
		::_splitpath_s(filepath, nullptr, 0, dirname, MAX_PATH, nullptr, 0, nullptr, 0);
	}
	else
	{
		filepath[0] = dirname[0] = '\0';
	}
	if ((dirname[0] == '\0'))
	{
		strcpy_s(dirname, MAX_PATH, pathBuffer);
	}
	// lpstrInitialDir は \ でないと受け付けない
	for (char* p = dirname; *p != '\0'; p++)
	{
		if (*p == '/')
			* p = '\\';
	}

	if (filter == nullptr)
	{
		filter = "All Files\0*.*\0\0";
	}

	// 構造体セット
	OPENFILENAMEA	ofn;
	memset(&ofn, 0, sizeof(OPENFILENAMEA));
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filepath;
	ofn.nMaxFile = size;
	ofn.lpstrTitle = title;
	ofn.lpstrInitialDir = (dirname[0] != '\0') ? dirname : nullptr;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	if (multiSelect)
	{
		ofn.Flags |= OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	}

	// カレントディレクトリ取得
	char currentDir[MAX_PATH];
	if (!::GetCurrentDirectoryA(MAX_PATH, currentDir))
	{
		currentDir[0] = '\0';
	}

	// ダイアログオープン
	if (::GetOpenFileNameA(&ofn) == FALSE)
	{
		return DialogResult::Cancel;
	}

	// カレントディレクトリ復帰
	if (currentDir[0] != '\0')
	{
		::SetCurrentDirectoryA(currentDir);
	}

	// 最終パスを記憶
	strcpy_s(pathBuffer, MAX_PATH, filepath);

	return DialogResult::OK;
}

// [ファイルを保存]ダイアログボックスを表示
DialogResult Dialog::SaveFileName(char* filepath, int size, const char* filter, const char* title, const char* ext, HWND hWnd)
{
	// 初期パス設定
	char dirname[MAX_PATH];
	if (filepath[0] != '0')
	{
		// ディレクトリパス取得
		::_splitpath_s(filepath, nullptr, 0, dirname, MAX_PATH, nullptr, 0, nullptr, 0);
	}
	else
	{
		filepath[0] = dirname[0] = '\0';
	}
	if ((dirname[0] == '\0'))
	{
		strcpy_s(dirname, MAX_PATH, pathBuffer);
	}
	// lpstrInitialDir は \ でないと受け付けない
	for (char* p = dirname; *p != '\0'; p++)
	{
		if (*p == '/')
			* p = '\\';
	}

	if (filter == nullptr)
	{
		filter = "All Files\0*.*\0\0";
	}

	// 構造体セット
	OPENFILENAMEA	ofn;
	memset(&ofn, 0, sizeof(OPENFILENAMEA));
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filepath;
	ofn.nMaxFile = size;
	ofn.lpstrTitle = title;
	ofn.lpstrInitialDir = (dirname[0] != '\0') ? dirname : nullptr;
	ofn.lpstrDefExt = ext;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

	// カレントディレクトリ取得
	char current_dir[MAX_PATH];
	if (!::GetCurrentDirectoryA(MAX_PATH, current_dir))
	{
		current_dir[0] = '\0';
	}

	// ダイアログオープン
	if (::GetSaveFileNameA(&ofn) == FALSE)
	{
		return DialogResult::Cancel;
	}

	// カレントディレクトリ復帰
	if (current_dir[0] != '\0')
	{
		::SetCurrentDirectoryA(current_dir);
	}

	// 最終パスを記憶
	strcpy_s(pathBuffer, MAX_PATH, filepath);

	return DialogResult::OK;
}
