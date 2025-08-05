#pragma once

#include <windows.h>
#include <crtdbg.h>

// ������������������������������������������������������������������������������
// �A�T�[�g�}�N���i�f�o�b�O���̂݁j
// expr �� false �̂Ƃ��Ƀ��b�Z�[�W�t���A�T�[�g���o��
// Release �r���h���͖����������
// ������������������������������������������������������������������������������
#if defined( DEBUG ) || defined( _DEBUG )
#define _ASSERT_EXPR_A(expr, msg) \
    (void)((!!(expr)) || \
    (1 != _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, "%s", msg)) || \
    (_CrtDbgBreak(), 0))
#else
#define  _ASSERT_EXPR_A(expr, expr_str) ((void)0)
#endif

// ������������������������������������������������������������������������������
// HRESULT ����G���[���b�Z�[�W��������擾�iWindows ��p�j
// �g�p��� LocalFree �Ń�����������K�v
// ������������������������������������������������������������������������������
inline LPWSTR HRTrace(HRESULT hr)
{
    LPWSTR msg;
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&msg), 0, NULL);
    return msg;
}

// ������������������������������������������������������������������������������
// �����x�^�C�}�[�ɂ��ȈՌv���N���X
// begin() �� end() �ŕb�����擾�\
// ������������������������������������������������������������������������������
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
