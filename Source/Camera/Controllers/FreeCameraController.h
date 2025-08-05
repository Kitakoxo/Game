#pragma once
#include "Camera.h"

class FreeCameraController
{
public:
	// カメラからコントローラーへパラメータを同期する
	void SyncCameraToController(const Camera& camera);

	// コントローラーからカメラへパラメータを同期する
	void SyncControllerToCamera(Camera& camera);

	// 更新処理
	void Update();

private:
	DirectX::XMFLOAT3		eye;
	DirectX::XMFLOAT3		focus;
	DirectX::XMFLOAT3		up;
	DirectX::XMFLOAT3		right;
	float					distance;

	float					angleX;
	float					angleY;
};
