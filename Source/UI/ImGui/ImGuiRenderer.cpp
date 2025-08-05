#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <ImGuizmo.h>
#include "ImGuiRenderer.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 初期化
void ImGuiRenderer::Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* dc)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;
	//io.ConfigViewportsNoDefaultParent = true;
	//io.ConfigDockingAlwaysTabBar = true;
	//io.ConfigDockingTransparentPayload = true;
#if 1
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI
#endif

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device, dc);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	ImFont* font = io.Fonts->AddFontFromFileTTF("Data/Font/ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	IM_ASSERT(font != NULL);
}

// 終了化
void ImGuiRenderer::Finalize()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// フレーム開始処理
void ImGuiRenderer::NewFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImVec2 size = ImGui::GetIO().DisplaySize;
	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

#if 0
	// Docking
	const ImGuiWindowFlags window_flags = ImGuiWindowFlags_None
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoBackground
		;
	const ImGuiDockNodeFlags docspace_flags = ImGuiDockNodeFlags_None
		//| ImGuiDockNodeFlags_KeepAliveOnly
		| ImGuiDockNodeFlags_PassthruCentralNode
		;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	bool dock_open = true;
	if (ImGui::Begin("MainDockspace", &dock_open, window_flags))
	{
		ImGui::PopStyleVar(3);

		ImGuiID dockspaceId = ImGui::GetID("MyDockspace");
		ImGui::DockSpace(dockspaceId, ImVec2(0, 0), docspace_flags);
	}
	ImGui::End();
#endif
}

// 描画
void ImGuiRenderer::Render(ID3D11DeviceContext* context)
{
	// Rendering
	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

// WIN32メッセージハンドラー
LRESULT ImGuiRenderer::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
