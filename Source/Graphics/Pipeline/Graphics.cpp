#include "DirectWriteCustomFont.h"
#include "Misc.h"
#include "Graphics.h"
#include <d3d11sdklayers.h>
#include <dxgidebug.h>

// 初期化
void Graphics::Initialize(HWND hWnd)
{
	this->hWnd = hWnd;
	// ウィンドウモード設定
	windowMode = WindowMode::Windowed;
	GetWindowRect(hWnd, &windowRect);
	// 画面のサイズを取得する。
	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT screenWidth = rc.right - rc.left;
	UINT screenHeight = rc.bottom - rc.top;

	this->screenWidth = static_cast<float>(screenWidth);
	this->screenHeight = static_cast<float>(screenHeight);

	HRESULT hr = S_OK;

	// デバイス＆スワップチェーンの生成
	{
		UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1,
		};

		// スワップチェーンを作成するための設定オプション
		DXGI_SWAP_CHAIN_DESC swapchainDesc;
		{
			swapchainDesc.BufferDesc.Width = screenWidth;
			swapchainDesc.BufferDesc.Height = screenHeight;
			swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
			swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
			swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			swapchainDesc.SampleDesc.Count = 1;
			swapchainDesc.SampleDesc.Quality = 0;
			swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapchainDesc.BufferCount = 2;
			swapchainDesc.OutputWindow = hWnd;
			swapchainDesc.Windowed = TRUE;
			swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapchainDesc.Flags = 0;
		}

		D3D_FEATURE_LEVEL featureLevel;

		// デバイス＆スワップチェーンの生成
		hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&swapchainDesc,
			swapchain.GetAddressOf(),
			device.GetAddressOf(),
			&featureLevel,
			immediateContext.GetAddressOf()
		);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// レンダーターゲットビューの生成
	{
		// スワップチェーンからバックバッファテクスチャを取得する。
		// ※スワップチェーンに内包されているバックバッファテクスチャは'色'を書き込むテクスチャ。
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		hr = swapchain->GetBuffer(
			0,
			__uuidof(ID3D11Texture2D),
			reinterpret_cast<void**>(texture2d.GetAddressOf()));
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// バックバッファテクスチャへの書き込みの窓口となるレンダーターゲットビューを生成する。
		hr = device->CreateRenderTargetView(texture2d.Get(), nullptr, renderTargetView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// 深度ステンシルビューの生成
	{
		// 深度ステンシル情報を書き込むためのテクスチャを作成する。
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		D3D11_TEXTURE2D_DESC texture2dDesc;
		texture2dDesc.Width = screenWidth;
		texture2dDesc.Height = screenHeight;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = 1;
		texture2dDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;
		hr = device->CreateTexture2D(&texture2dDesc, nullptr, texture2d.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		depthStencil = texture2d;
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		hr = device->CreateShaderResourceView(texture2d.Get(), &srvDesc, depthSRV.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// 深度ステンシルテクスチャへの書き込みに窓口になる深度ステンシルビューを作成する。
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		hr = device->CreateDepthStencilView(texture2d.Get(), &dsvDesc, depthStencilView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// ビューポート
	{
		viewport.Width = static_cast<float>(screenWidth);
		viewport.Height = static_cast<float>(screenHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		DirectWriteCustomFont::RecreateAllRenderTargets(swapchain.Get());
	}

	// レンダーステート生成
	renderState = std::make_unique<RenderState>(device.Get());

	// レンダラ生成
	primitiveRenderer = std::make_unique<PrimitiveRenderer>(device.Get());
	shapeRenderer = std::make_unique<ShapeRenderer>(device.Get());
	modelRenderer = std::make_unique<ModelRenderer>(device.Get());

}

// リソース解放
void Graphics::Finalize()
{
	modelRenderer.reset();
	shapeRenderer.reset();
	primitiveRenderer.reset();
	renderState.reset();
	depthSRV.Reset();
	depthStencil.Reset();
	depthStencilView.Reset();
	renderTargetView.Reset();

	Microsoft::WRL::ComPtr<ID3D11Debug> debug;
#if defined(DEBUG) || defined(_DEBUG)
	if (immediateContext)
	{
		immediateContext->ClearState();
		immediateContext->Flush();
		DirectWriteCustomFont::ReleaseAllRenderTargets();
		immediateContext->QueryInterface(IID_PPV_ARGS(debug.GetAddressOf()));
	}
#else
	if (immediateContext)
	{
		immediateContext->ClearState();
		immediateContext->Flush();
		DirectWriteCustomFont::ReleaseAllRenderTargets();
	}
#endif
	swapchain.Reset();
	immediateContext.Reset();
	device.Reset();
#if defined(DEBUG) || defined(_DEBUG)
	if (debug)
	{
		debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
	}
	{
		HMODULE module = GetModuleHandleW(L"dxgidebug.dll");
		bool shouldFree = false;
		if (!module) {
			module = LoadLibraryW(L"dxgidebug.dll");
			shouldFree = true;
		}
		if (module)
		{
			using DXGIGetDebugInterfaceProc = HRESULT(WINAPI*)(UINT, REFIID, void**);
			auto getDebugInterface = reinterpret_cast<DXGIGetDebugInterfaceProc>(GetProcAddress(module, "DXGIGetDebugInterface1"));
			if (getDebugInterface)
			{
				Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
				if (SUCCEEDED(getDebugInterface(0, IID_PPV_ARGS(dxgiDebug.GetAddressOf()))))
				{
					dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
				}
			}
			if (shouldFree)
			{
				FreeLibrary(module);
			}
		}
	}
#endif

	swapchain.Reset();
	immediateContext.Reset();
	device.Reset();
}

// クリア
void Graphics::Clear(float r, float g, float b, float a)
{
	float color[4]{ r, g, b, a };
	immediateContext->ClearRenderTargetView(renderTargetView.Get(), color);
	immediateContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

// レンダーターゲット設定
void Graphics::SetRenderTargets()
{
	immediateContext->RSSetViewports(1, &viewport);
	immediateContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
}

// ウィンドウモード設定
void Graphics::SetWindowMode(WindowMode mode)
{
	if (windowMode == mode) return;
	if (mode == WindowMode::Windowed)
	{
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		SetWindowPos(hWnd, HWND_NOTOPMOST,
			windowRect.left, windowRect.top,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			SWP_FRAMECHANGED);
		ShowWindow(hWnd, SW_SHOW);
	}
	else /* Borderless */
	{
		if (windowMode == WindowMode::Windowed)
		{
			GetWindowRect(hWnd, &windowRect);
		}
		MONITORINFO mi{ sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi);
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(hWnd, HWND_NOTOPMOST,
			mi.rcMonitor.left, mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_FRAMECHANGED);
	}

	RECT rc;
	GetClientRect(hWnd, &rc);
	Resize(rc.right - rc.left, rc.bottom - rc.top);
	windowMode = mode;
}

// ウィンドウサイズ設定
void Graphics::SetWindowSize(UINT width, UINT height)
{
	if (windowMode == WindowMode::Windowed)
	{
		RECT rc{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		SetWindowPos(hWnd, HWND_NOTOPMOST,
			0, 0,
			rc.right - rc.left,
			rc.bottom - rc.top,
			SWP_NOMOVE | SWP_NOZORDER);
		GetWindowRect(hWnd, &windowRect);
	}
	Resize(width, height);
}

// ウィンドウサイズを変更したときに呼ぶ
void Graphics::OnWindowSizeChanged(UINT width, UINT height)
{
	Resize(width, height);
	++frameCount;
}

// 画面表示
void Graphics::Present(UINT syncInterval)
{
	swapchain->Present(syncInterval, 0);
	++frameCount;
}

// ウィンドウリサイズ
void Graphics::Resize(UINT width, UINT height)
{
	if (!swapchain) return;
	if (screenWidth == width && screenHeight == height) return;
	if (width == 0 || height == 0) return;

	screenWidth = static_cast<float>(width);
	screenHeight = static_cast<float>(height);

	immediateContext->OMSetRenderTargets(0, nullptr, nullptr);
	immediateContext->ClearState();
	immediateContext->Flush();
	DirectWriteCustomFont::ReleaseAllRenderTargets();
	renderTargetView.Reset();
	depthSRV.Reset();
	depthStencil.Reset();
	depthStencilView.Reset();

	HRESULT hr = swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBuffer;
	D3D11_TEXTURE2D_DESC depthDesc{};
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;
	hr = device->CreateTexture2D(&depthDesc, nullptr, depthBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	hr = device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, depthStencilView.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	hr = device->CreateShaderResourceView(depthBuffer.Get(), &srvDesc, depthSRV.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	depthStencil = depthBuffer;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	DirectWriteCustomFont::RecreateAllRenderTargets(swapchain.Get());
}
