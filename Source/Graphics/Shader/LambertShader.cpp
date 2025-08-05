#include "Misc.h"
#include "GpuResourceUtils.h"
#include "LambertShader.h"

LambertShader::LambertShader(ID3D11Device* device)
{
	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/LambertVS.cso",
		Model::InputElementDescs.data(),
		static_cast<UINT>(Model::InputElementDescs.size()),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

	// ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/LambertPS.cso",
		pixelShader.GetAddressOf());

	// メッシュ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbMesh),
		meshConstantBuffer.GetAddressOf());
}

// 開始処理
void LambertShader::Begin(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->IASetInputLayout(inputLayout.Get());
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	// 定数バッファ設定
	ID3D11Buffer* cbs[] =
	{
		meshConstantBuffer.Get(), 
	};
	dc->PSSetConstantBuffers(0, _countof(cbs), cbs);
}

// 更新処理
void LambertShader::Update(const RenderContext& rc, const Model::Mesh& mesh)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// メッシュ用定数バッファ更新
	CbMesh cbMesh{};
	cbMesh.materialColor = mesh.material->baseColor;
	dc->UpdateSubresource(meshConstantBuffer.Get(), 0, 0, &cbMesh, 0, 0);

	// シェーダーリソースビュー設定
	ID3D11ShaderResourceView* srvs[] =
	{
		mesh.material->baseMap.Get(),
	};
	dc->PSSetShaderResources(0, _countof(srvs), srvs);
}

// 描画終了
void LambertShader::End(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定解除
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	// 定数バッファ設定解除
	ID3D11Buffer* cbs[] = { nullptr };
	dc->PSSetConstantBuffers(1, _countof(cbs), cbs);

	// シェーダーリソースビュー設定解除
	ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);
}
