#pragma once

#include <memory>
#include "Scene.h"
#include "Camera.h"
#include "FreeCameraController.h"
#include "Model.h"
#include "Light.h"
#include <ImGuizmo.h>

// モデルビューアシーン
class ModelViewerScene : public Scene
{
public:
	ModelViewerScene();
	~ModelViewerScene() override = default;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(float elapsedTime) override;

	// GUI描画処理
	void DrawGUI() override;

private:
	// メニューGUI描画
	void DrawMenuGUI();

	// ヒエラルキーGUI描画
	void DrawHierarchyGUI();

	// プロパティGUI描画
	void DrawPropertyGUI();

	// アニメーションGUI描画
	void DrawAnimationGUI();

	// マテリアルGUI描画
	void DrawMaterialGUI();

private:
	Camera								camera;
	FreeCameraController				cameraController;
	LightManager						lightManager;
	std::shared_ptr<Model>				model;
	Model::Node* selectionNode = nullptr;
	std::vector<Model::NodePose>		nodePoses;
	bool								animationPlaying = false;
	bool								animationLoop = false;
	float								animationSamplingRate = 60;
	float								animationBlendSeconds = 0;
	float								animationSpeed = 1.0f;
	float								currentAnimationSeconds = 0;
	int									currentAnimationIndex = -1;
	int									shaderId;
	ImGuizmo::OPERATION	gizmoOperation = ImGuizmo::TRANSLATE;
	bool	gizmoEnable = false;
};
