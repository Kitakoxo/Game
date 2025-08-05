#include <imgui_impl_win32.h>        // ImGui��Win32�o�b�N�G���h
#include <imgui_impl_dx11.h>        // ImGui��DirectX11�o�b�N�G���h
#include <ImGuizmo.h>               // �M�Y���`��p�i�g�����X�t�H�[������p�j
#include "ImGuiRenderer.h"          // ����ImGui���b�p�[�N���X

// Win32 ���b�Z�[�W�����̃��b�p�[
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ===========================
// ImGui ������
// ===========================
void ImGuiRenderer::Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* dc)
{
	// ImGui�R���e�L�X�g�̍쐬
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// �e��@�\�̗L����
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // �L�[�{�[�h�����L����
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // �Q�[���p�b�h����i�K�v�ɉ����āj
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // �h�b�L���O�i�E�B���h�E���́j�@�\
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // �����E�B���h�E��OS��ɕ\��

#if 1
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // DPI�X�P�[�����O�i������j
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // DPI�X�P�[�����O�i������j
#endif

	// -------------------------------
	// �X�^�C���ݒ�i�����ڂ̏������j
	// -------------------------------
	ImGui::StyleColorsDark();  // ���̃_�[�N�X�^�C�����x�[�X�ɂ���

	// �J�X�^���X�^�C���̒���
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6.0f;                       // �E�B���h�E�̊p��
	style.FrameRounding = 4.0f;                        // �{�^���Ȃǂ̊p��
	style.GrabRounding = 4.0f;                         // �X���C�_�[�̊ۂ�
	style.ScrollbarRounding = 6.0f;                    // �X�N���[���o�[�̊p��
	style.FrameBorderSize = 1.0f;                      // �{�[�_�[���������ǉ�
	style.WindowBorderSize = 1.0f;
	style.IndentSpacing = 15.0f;                       // �C���f���g�Ԋu

	// �F�̃J�X�^�}�C�Y�i���j
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.12f, 0.15f, 0.95f);  // �E�B���h�E�w�i
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 0.85f);  // �w�b�_�[�w�i
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.40f, 0.50f, 0.90f);  // �w�b�_�[�E�z�o�[��
	colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.45f, 0.55f, 1.00f);  // �w�b�_�[�E�A�N�e�B�u��
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.30f, 0.85f);  // �{�^��
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.40f, 0.50f, 0.90f);  // �{�^���E�z�o�[
	colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.45f, 0.55f, 1.00f);  // �{�^���E�N���b�N��
	colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);  // ���͗��E�X���C�_�[�w�i
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.30f, 0.35f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);  // �^�C�g���o�[�w�i
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);

	// Viewport �L�����̔w�i�����킹��
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 6.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}


	// -------------------------------
	// �o�b�N�G���h�iWin32 + DX11�j������
	// -------------------------------
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device, dc);

	// -------------------------------
	// �t�H���g�ǂݍ��݁i���{��Ή��j
	// -------------------------------
	ImFont* font = io.Fonts->AddFontFromFileTTF(
		"Data/Font/ArialUni.ttf",         // ���{��Ή��t�H���g�p�X
		18.0f,                             // �t�H���g�T�C�Y
		nullptr,
		io.Fonts->GetGlyphRangesJapanese() // ���{��O���t��Ώۂ�
	);
	IM_ASSERT(font != NULL); // �t�H���g�ǂݍ��ݎ��s����Assert
}

// ===========================
// ImGui �I������
// ===========================
void ImGuiRenderer::Finalize()
{
	ImGui_ImplDX11_Shutdown();    // DX11�o�b�N�G���h���
	ImGui_ImplWin32_Shutdown();   // Win32�o�b�N�G���h���
	ImGui::DestroyContext();      // ImGui�R���e�L�X�g�j��
}

// ===========================
// �t���[���̊J�n����
// ===========================
void ImGuiRenderer::NewFrame()
{
	// �o�b�N�G���h�̐V�t���[������
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	// ImGui�t���[���̊J�n
	ImGui::NewFrame();

	// ImGuizmo�̏������i�g�����X�t�H�[������p�j
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImVec2 size = ImGui::GetIO().DisplaySize;
	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);         // �������e���g�p
	ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y); // �`��͈͂�ݒ�

#if 0
	// �� �h�b�L���O�X�y�[�X��ݒu����T���v���i�K�v�ɉ����ėL�����j

	const ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	const ImGuiDockNodeFlags docspace_flags =
		ImGuiDockNodeFlags_PassthruCentralNode;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	bool dock_open = true;
	if (ImGui::Begin("MainDockspace", &dock_open, window_flags))
	{
		ImGui::PopStyleVar(3); // StyleVar�̃��Z�b�g
		ImGuiID dockspaceId = ImGui::GetID("MyDockspace");
		ImGui::DockSpace(dockspaceId, ImVec2(0, 0), docspace_flags);
	}
	ImGui::End();
#endif
}

// ===========================
// ImGui�`��̎��s
// ===========================
void ImGuiRenderer::Render(ID3D11DeviceContext* context)
{
	// ImGui�ɂ��`��R�}���h�̐���
	ImGui::Render();

	// DX11�o�b�N�G���h�ɕ`��f�[�^��n��
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// �}���`�r���[�|�[�g�̕`��i�E�B���h�E��OS��ɕ����j
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();         // �T�u�E�B���h�E�̍X�V
		ImGui::RenderPlatformWindowsDefault();  // �T�u�E�B���h�E�̕`��
	}
}

// ===========================
// Win32 ���b�Z�[�W����
// ===========================
LRESULT ImGuiRenderer::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// ImGui���g���C�x���g�Ȃ珈���A�����łȂ���Ύ���
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
