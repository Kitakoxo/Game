#include <windows.h>
#include <memory>
#include <assert.h>
#include <tchar.h>

#include "Framework.h"

// ��ʃT�C�Y�̒�`
const LONG SCREEN_WIDTH = 1280;
const LONG SCREEN_HEIGHT = 720;

// �E�B���h�E�v���V�[�W���i�E�B���h�E�ɑ����Ă��郁�b�Z�[�W�̏����j
LRESULT CALLBACK fnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    // �E�B���h�E�ɕR�Â��� Framework �C���X�^���X���擾
    Framework* f = reinterpret_cast<Framework*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    // Framework������΂��� HandleMessage �֐��Ń��b�Z�[�W�����A�Ȃ���΃f�t�H���g����
    return f ? f->HandleMessage(hwnd, msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);
}

// �G���g���[�|�C���g�i�A�v���̃��C���֐��j
INT WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd_line, INT cmd_show)
{
#if defined(DEBUG) | defined(_DEBUG)
    // ���������[�N���o�p�i�f�o�b�O���̂݁j
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(237); // ����̃A���P�[�V�����ԍ��Ńu���[�N�i�K�v�Ȃ�j
#endif

    // �E�B���h�E�N���X�̓o�^
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW; // ���T�C�Y���ɍĕ`��
    wcex.lpfnWndProc = fnWndProc;         // ���b�Z�[�W�����֐�
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);  // �f�t�H���g�̖��J�[�\��
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // �w�i�F
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T("Game");      // �N���X��
    wcex.hIconSm = 0;
    RegisterClassEx(&wcex);               // �o�^���s

    // �E�B���h�E�T�C�Y���N���C�A���g�̈�T�C�Y�ɒ���
    RECT rc = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    // �E�B���h�E�̍쐬
    HWND hWnd = CreateWindow(
        _T("Game"), _T(""),
        WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, instance, NULL
    );

    // �E�B���h�E�\��
    ShowWindow(hWnd, cmd_show);

    // Framework �C���X�^���X�����i�Q�[���̃��C�������N���X�j
    Framework f(hWnd);

    // �E�B���h�E�� Framework �̃|�C���^��o�^
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&f));

    // ���C�����[�v�̎��s
    return f.Run();
}
