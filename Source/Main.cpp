#include <windows.h>
#include <memory>
#include <assert.h>
#include <tchar.h>

#include "Framework.h"

// 画面サイズの定義
const LONG SCREEN_WIDTH = 1280;
const LONG SCREEN_HEIGHT = 720;

// ウィンドウプロシージャ（ウィンドウに送られてくるメッセージの処理）
LRESULT CALLBACK fnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    // ウィンドウに紐づけた Framework インスタンスを取得
    Framework* f = reinterpret_cast<Framework*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    // Frameworkがあればその HandleMessage 関数でメッセージ処理、なければデフォルト処理
    return f ? f->HandleMessage(hwnd, msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);
}

// エントリーポイント（アプリのメイン関数）
INT WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd_line, INT cmd_show)
{
#if defined(DEBUG) | defined(_DEBUG)
    // メモリリーク検出用（デバッグ時のみ）
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(237); // 特定のアロケーション番号でブレーク（必要なら）
#endif

    // ウィンドウクラスの登録
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW; // リサイズ時に再描画
    wcex.lpfnWndProc = fnWndProc;         // メッセージ処理関数
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);  // デフォルトの矢印カーソル
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // 背景色
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T("Game");      // クラス名
    wcex.hIconSm = 0;
    RegisterClassEx(&wcex);               // 登録実行

    // ウィンドウサイズをクライアント領域サイズに調整
    RECT rc = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    // ウィンドウの作成
    HWND hWnd = CreateWindow(
        _T("Game"), _T(""),
        WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, instance, NULL
    );

    // ウィンドウ表示
    ShowWindow(hWnd, cmd_show);

    // Framework インスタンス生成（ゲームのメイン処理クラス）
    Framework f(hWnd);

    // ウィンドウに Framework のポインタを登録
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&f));

    // メインループの実行
    return f.Run();
}
