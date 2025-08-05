#include <imgui_impl_win32.h>        // ImGuiのWin32バックエンド
#include <imgui_impl_dx11.h>        // ImGuiのDirectX11バックエンド
#include <ImGuizmo.h>               // ギズモ描画用（トランスフォーム操作用）
#include "ImGuiRenderer.h"          // 自作ImGuiラッパークラス

// Win32 メッセージ処理のラッパー
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ===========================
// ImGui 初期化
// ===========================
void ImGuiRenderer::Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* dc)
{
	// ImGuiコンテキストの作成
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// 各種機能の有効化
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // キーボード操作を有効化
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // ゲームパッド操作（必要に応じて）
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // ドッキング（ウィンドウ合体）機能
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // 複数ウィンドウをOS上に表示

#if 1
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // DPIスケーリング（未安定）
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // DPIスケーリング（未安定）
#endif

	// -------------------------------
	// スタイル設定（見た目の初期化）
	// -------------------------------
	ImGui::StyleColorsDark();  // 元のダークスタイルをベースにする

	// カスタムスタイルの調整
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6.0f;                       // ウィンドウの角丸
	style.FrameRounding = 4.0f;                        // ボタンなどの角丸
	style.GrabRounding = 4.0f;                         // スライダーの丸み
	style.ScrollbarRounding = 6.0f;                    // スクロールバーの角丸
	style.FrameBorderSize = 1.0f;                      // ボーダー線を少し追加
	style.WindowBorderSize = 1.0f;
	style.IndentSpacing = 15.0f;                       // インデント間隔

	// 色のカスタマイズ（一例）
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.12f, 0.15f, 0.95f);  // ウィンドウ背景
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 0.85f);  // ヘッダー背景
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.40f, 0.50f, 0.90f);  // ヘッダー・ホバー時
	colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.45f, 0.55f, 1.00f);  // ヘッダー・アクティブ時
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.30f, 0.85f);  // ボタン
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.40f, 0.50f, 0.90f);  // ボタン・ホバー
	colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.45f, 0.55f, 1.00f);  // ボタン・クリック時
	colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);  // 入力欄・スライダー背景
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.30f, 0.35f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);  // タイトルバー背景
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);

	// Viewport 有効時の背景も合わせる
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 6.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}


	// -------------------------------
	// バックエンド（Win32 + DX11）初期化
	// -------------------------------
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device, dc);

	// -------------------------------
	// フォント読み込み（日本語対応）
	// -------------------------------
	ImFont* font = io.Fonts->AddFontFromFileTTF(
		"Data/Font/ArialUni.ttf",         // 日本語対応フォントパス
		18.0f,                             // フォントサイズ
		nullptr,
		io.Fonts->GetGlyphRangesJapanese() // 日本語グリフを対象に
	);
	IM_ASSERT(font != NULL); // フォント読み込み失敗時にAssert
}

// ===========================
// ImGui 終了処理
// ===========================
void ImGuiRenderer::Finalize()
{
	ImGui_ImplDX11_Shutdown();    // DX11バックエンド解放
	ImGui_ImplWin32_Shutdown();   // Win32バックエンド解放
	ImGui::DestroyContext();      // ImGuiコンテキスト破棄
}

// ===========================
// フレームの開始処理
// ===========================
void ImGuiRenderer::NewFrame()
{
	// バックエンドの新フレーム処理
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	// ImGuiフレームの開始
	ImGui::NewFrame();

	// ImGuizmoの初期化（トランスフォーム操作用）
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImVec2 size = ImGui::GetIO().DisplaySize;
	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);         // 透視投影を使用
	ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y); // 描画範囲を設定

#if 0
	// ↓ ドッキングスペースを設置するサンプル（必要に応じて有効化）

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
		ImGui::PopStyleVar(3); // StyleVarのリセット
		ImGuiID dockspaceId = ImGui::GetID("MyDockspace");
		ImGui::DockSpace(dockspaceId, ImVec2(0, 0), docspace_flags);
	}
	ImGui::End();
#endif
}

// ===========================
// ImGui描画の実行
// ===========================
void ImGuiRenderer::Render(ID3D11DeviceContext* context)
{
	// ImGuiによる描画コマンドの生成
	ImGui::Render();

	// DX11バックエンドに描画データを渡す
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// マルチビューポートの描画（ウィンドウをOS上に分離）
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();         // サブウィンドウの更新
		ImGui::RenderPlatformWindowsDefault();  // サブウィンドウの描画
	}
}

// ===========================
// Win32 メッセージ処理
// ===========================
LRESULT ImGuiRenderer::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// ImGuiが使うイベントなら処理、そうでなければ次へ
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
