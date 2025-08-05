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

// �O���t�B�b�N�X
class Graphics
{
private:
	Graphics() = default;
	~Graphics() = default;

public:
	// �C���X�^���X�擾
	static Graphics& Instance()
	{
		static Graphics instance;
		return instance;
	}

	// ������
	void Initialize(HWND hWnd);

	// ���\�[�X���
	void Finalize();

	// �N���A
	void Clear(float r, float g, float b, float a);

	// �����_�[�^�[�Q�b�g�ݒ�
	void SetRenderTargets();

	// �E�B���h�E���[�h�ݒ�
	void SetWindowMode(WindowMode mode);

	// �E�B���h�E�T�C�Y�ݒ�
	void SetWindowSize(UINT width, UINT height);

	// �E�B���h�E�T�C�Y��ύX�����Ƃ��ɌĂ�
	void OnWindowSizeChanged(UINT width, UINT height);

	// �E�B���h�E���[�h�擾
	WindowMode GetWindowMode() const { return windowMode; }

	// ��ʕ\��
	void Present(UINT syncInterval);

	// �E�C���h�E�n���h���擾
	HWND GetWindowHandle() { return hWnd; }

	// �f�o�C�X�擾
	ID3D11Device* GetDevice() { return device.Get(); }

	// �f�o�C�X�R���e�L�X�g�擾
	ID3D11DeviceContext* GetDeviceContext() { return immediateContext.Get(); }

	// �X�N���[�����擾
	float GetScreenWidth() const { return screenWidth; }

	// �X�N���[�������擾
	float GetScreenHeight() const { return screenHeight; }

	// �����_�[�X�e�[�g�擾
	RenderState* GetRenderState() { return renderState.get(); }

	// �v���~�e�B�u�����_���擾
	PrimitiveRenderer* GetPrimitiveRenderer() const { return primitiveRenderer.get(); }

	// �V�F�C�v�����_���擾
	ShapeRenderer* GetShapeRenderer() const { return shapeRenderer.get(); }

	// ���f�������_���擾
	ModelRenderer* GetModelRenderer() const { return modelRenderer.get(); }

	//�X���b�v�`�F�C���擾
	IDXGISwapChain* GetSwapChain() const { return swapchain.Get(); }

	//�~���[�e�b�N�X�擾
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
