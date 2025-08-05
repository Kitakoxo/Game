#pragma once

#include <windows.h>
#include <crtdbg.h>

// ───────────────────────────────────────
// アサートマクロ（デバッグ時のみ）
// expr が false のときにメッセージ付きアサートを出す
// Release ビルド時は無効化される
// ───────────────────────────────────────
#if defined( DEBUG ) || defined( _DEBUG )
#define _ASSERT_EXPR_A(expr, msg) \
    (void)((!!(expr)) || \
    (1 != _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, "%s", msg)) || \
    (_CrtDbgBreak(), 0))
#else
#define  _ASSERT_EXPR_A(expr, expr_str) ((void)0)
#endif

// ───────────────────────────────────────
// HRESULT からエラーメッセージ文字列を取得（Windows 専用）
// 使用後は LocalFree でメモリ解放が必要
// ───────────────────────────────────────
inline LPWSTR HRTrace(HRESULT hr)
{
    LPWSTR msg;
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&msg), 0, NULL);
    return msg;
}

// ───────────────────────────────────────
// 高精度タイマーによる簡易計測クラス
// begin() → end() で秒数を取得可能
// ───────────────────────────────────────
class Benchmark
{
    LARGE_INTEGER ticksPerSecond;
    LARGE_INTEGER startTicks;
    LARGE_INTEGER currentTicks;

public:
    Benchmark()
    {
        QueryPerformanceFrequency(&ticksPerSecond);
        QueryPerformanceCounter(&startTicks);
        QueryPerformanceCounter(&currentTicks);
    }
    void begin()
    {
        QueryPerformanceCounter(&startTicks);
    }
    float end()
    {
        QueryPerformanceCounter(&currentTicks);
        return static_cast<float>(currentTicks.QuadPart - startTicks.QuadPart)
            / static_cast<float>(ticksPerSecond.QuadPart);
    }
};
