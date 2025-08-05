#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include "RenderState.h"
#include "PrimitiveRenderer.h"
#include "ShapeRenderer.h"
#include "ModelRenderer.h"

enum class WindowMode
{
	Windowed,
	Borderless,
};

// グラフィックス
class Graphics
{
private:
	Graphics() = default;
	~Graphics() = default;

public:
	// インスタンス取得
	static Graphics& Instance()
	{
		static Graphics instance;
		return instance;
	}

	// 初期化
	void Initialize(HWND hWnd);

	// リソース解放
	void Finalize();

	// クリア
	void Clear(float r, float g, float b, float a);

	// レンダーターゲット設定
	void SetRenderTargets();

	// ウィンドウモード設定
	void SetWindowMode(WindowMode mode);

	// ウィンドウサイズ設定
	void SetWindowSize(UINT width, UINT height);

	// ウィンドウサイズを変更したときに呼ぶ
	void OnWindowSizeChanged(UINT width, UINT height);

	// ウィンドウモード取得
	WindowMode GetWindowMode() const { return windowMode; }

	// 画面表示
	void Present(UINT syncInterval);

	// ウインドウハンドル取得
	HWND GetWindowHandle() { return hWnd; }

	// デバイス取得
	ID3D11Device* GetDevice() { return device.Get(); }

	// デバイスコンテキスト取得
	ID3D11DeviceContext* GetDeviceContext() { return immediateContext.Get(); }

	// スクリーン幅取得
	float GetScreenWidth() const { return screenWidth; }

	// スクリーン高さ取得
	float GetScreenHeight() const { return screenHeight; }

	// レンダーステート取得
	RenderState* GetRenderState() { return renderState.get(); }

	// プリミティブレンダラ取得
	PrimitiveRenderer* GetPrimitiveRenderer() const { return primitiveRenderer.get(); }

	// シェイプレンダラ取得
	ShapeRenderer* GetShapeRenderer() const { return shapeRenderer.get(); }

	// モデルレンダラ取得
	ModelRenderer* GetModelRenderer() const { return modelRenderer.get(); }

	//スワップチェイン取得
	IDXGISwapChain* GetSwapChain() const { return swapchain.Get(); }

	//ミューテックス取得
	std::mutex& GetMutex() { return mutex; }

	ID3D11RenderTargetView* GetRenderTargetView() const { return renderTargetView.Get(); }
	ID3D11DepthStencilView* GetDepthStencilView() const { return depthStencilView.Get(); }
	ID3D11ShaderResourceView* GetDepthSRV() const { return depthSRV.Get(); }
	uint64_t GetFrameCount() const { return frameCount; }
private:
	void Resize(UINT width, UINT height);

private:
	HWND											hWnd = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device>			device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		immediateContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain>			swapchain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  depthSRV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>         depthStencil;
	D3D11_VIEWPORT                                  viewport{};
	WindowMode windowMode = WindowMode::Windowed;
	RECT windowRect{};

	float	screenWidth = 0;
	float	screenHeight = 0;

	std::unique_ptr<RenderState>					renderState;
	std::unique_ptr<PrimitiveRenderer>				primitiveRenderer;
	std::unique_ptr<ShapeRenderer>					shapeRenderer;
	std::unique_ptr<ModelRenderer>					modelRenderer;
	std::mutex mutex;
	uint64_t frameCount = 0;
};
