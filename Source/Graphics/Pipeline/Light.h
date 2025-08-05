#pragma once

#include <DirectXMath.h>

struct DirectionalLight
{
	DirectX::XMFLOAT3	direction = { 0, -1, 0 };
	DirectX::XMFLOAT3	color = { 1, 1, 1 };
};

class LightManager
{
public:
	// ディレクショナルライト設定
	void SetDirectionalLight(DirectionalLight& light) { directionalLight = light; }

	// ディレクショナルライト取得
	const DirectionalLight& GetDirectionalLight() const { return directionalLight; }

private:
	DirectionalLight	directionalLight;
};
