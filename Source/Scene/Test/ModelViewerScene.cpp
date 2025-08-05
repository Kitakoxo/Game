#include <functional>
#include <imgui.h>
#include "ModelViewerScene.h"
#include "Graphics.h"
#include "TransformUtils.h"
#include "Dialog.h"

// コンストラクタ
ModelViewerScene::ModelViewerScene()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// カメラ設定
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),	// 画角
		screenWidth / screenHeight,			// 画面アスペクト比
		0.1f,								// ニアクリップ
		1000.0f								// ファークリップ
	);
	camera.SetLookAt(
		{ 0, 3, 5 },		// 視点
		{ 0, 0, 0 },		// 注視点
		{ 0, 1, 0 }			// 上ベクトル
	);
	cameraController.SyncCameraToController(camera);

	shaderId = static_cast<int>(ShaderId::Basic);

	// ライト設定
	DirectionalLight directionalLight;
	directionalLight.direction = { 0, -1, -1 };
	directionalLight.color = { 1, 1, 1 };
	lightManager.SetDirectionalLight(directionalLight);
}

// 更新処理
void ModelViewerScene::Update(float elapsedTime)
{
	// カメラ更新処理
	cameraController.Update();
	cameraController.SyncControllerToCamera(camera);

	if (model != nullptr)
	{
		// アニメーション更新
		if (animationPlaying && currentAnimationIndex >= 0)
		{
			model->ComputeAnimation(currentAnimationIndex, currentAnimationSeconds, nodePoses);
			model->SetNodePoses(nodePoses);

			// 時間更新
			const Model::Animation& animation = model->GetAnimations().at(currentAnimationIndex);
			currentAnimationSeconds += elapsedTime * animationSpeed;
			if (currentAnimationSeconds > animation.secondsLength)
			{
				if (animationLoop)
				{
					currentAnimationSeconds -= animation.secondsLength;
				}
				else
				{
					currentAnimationSeconds = animation.secondsLength;
				}
			}
		}

		// トランスフォーム更新
		DirectX::XMFLOAT4X4 worldTransform;
		DirectX::XMStoreFloat4x4(&worldTransform, DirectX::XMMatrixIdentity());
		model->UpdateTransform(worldTransform);
	}

}

// 描画処理
void ModelViewerScene::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

	// グリッド描画
	primitiveRenderer->DrawGrid(20, 1);
	primitiveRenderer->Render(dc, camera.GetView(), camera.GetProjection(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	rc.lightManager = &lightManager;

	// 描画
	if (model != nullptr)
	{
		// モデル描画
		modelRenderer->Draw(static_cast<ShaderId>(shaderId), model);
		modelRenderer->Render(rc);

		// レンダーステート設定
		dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
		dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
		dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

		// 軸描画
		const std::vector<Model::Node>& nodes = model->GetNodes();
		if (selectionNode != nullptr)
		{
			DirectX::XMFLOAT3 p, x, y, z;
			const float length = 0.1f;
			DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&selectionNode->worldTransform);
			DirectX::XMVECTOR X = DirectX::XMVector3Transform(DirectX::XMVectorSet(length, 0, 0, 0), W);
			DirectX::XMVECTOR Y = DirectX::XMVector3Transform(DirectX::XMVectorSet(0, length, 0, 0), W);
			DirectX::XMVECTOR Z = DirectX::XMVector3Transform(DirectX::XMVectorSet(0, 0, length, 0), W);
			DirectX::XMStoreFloat3(&p, W.r[3]);
			DirectX::XMStoreFloat3(&x, X);
			DirectX::XMStoreFloat3(&y, Y);
			DirectX::XMStoreFloat3(&z, Z);
			primitiveRenderer->AddVertex(p, { 1, 0, 0, 1 });
			primitiveRenderer->AddVertex(x, { 1, 0, 0, 1 });
			primitiveRenderer->AddVertex(p, { 0, 1, 0, 1 });
			primitiveRenderer->AddVertex(y, { 0, 1, 0, 1 });
			primitiveRenderer->AddVertex(p, { 0, 0, 1, 1 });
			primitiveRenderer->AddVertex(z, { 0, 0, 1, 1 });
		}
		primitiveRenderer->Render(dc, camera.GetView(), camera.GetProjection(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	}
}

// GUI描画処理
void ModelViewerScene::DrawGUI()
{
	DrawMenuGUI();
	DrawHierarchyGUI();
	DrawPropertyGUI();
	DrawAnimationGUI();
	DrawMaterialGUI();
}

// メニューGUI描画
void ModelViewerScene::DrawMenuGUI()
{
	if (ImGui::BeginMainMenuBar())
	{
		// ファイルメニュー
		if (ImGui::BeginMenu("File"))
		{
			bool check = false;
			if (ImGui::MenuItem("Open Model", "", &check))
			{
				static const char* filter = "Model Files(*.gltf;*.glb)\0*.gltf;*.glb;\0All Files(*.*)\0*.*;\0\0";

				char filename[256] = { 0 };
				HWND hWnd = Graphics::Instance().GetWindowHandle();
				DialogResult result = Dialog::OpenFileName(filename, sizeof(filename), filter, nullptr, hWnd);
				if (result == DialogResult::OK)
				{
					ID3D11Device* device = Graphics::Instance().GetDevice();
					model = std::make_shared<Model>(device, filename, animationSamplingRate);
					animationSpeed = 1.0f;
					currentAnimationSeconds = 0.0f;
					currentAnimationIndex = -1;
				}
			}


			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

// ヒエラルキーGUI描画
void ModelViewerScene::DrawHierarchyGUI()
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 10, pos.y + 30), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Once);

	if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_None))
	{
		// ノードツリーを再帰的に描画する関数
		std::function<void(Model::Node*)> drawNodeTree = [&](Model::Node* node)
			{
				// 矢印をクリック、またはノードをダブルクリックで階層を開く
				ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
					| ImGuiTreeNodeFlags_OpenOnDoubleClick;

				// 子がいない場合は矢印をつけない
				size_t childCount = node->children.size();
				if (childCount == 0)
				{
					nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				}

				// 選択フラグ
				if (selectionNode == node)
				{
					nodeFlags |= ImGuiTreeNodeFlags_Selected;
				}

				// ツリーノードを表示
				bool opened = ImGui::TreeNodeEx(node, nodeFlags, node->name.c_str());

				// フォーカスされたノードを選択する
				if (ImGui::IsItemFocused())
				{
					selectionNode = node;
				}

				// 開かれている場合、子階層も同じ処理を行う
				if (opened && childCount > 0)
				{
					for (Model::Node* child : node->children)
					{
						drawNodeTree(child);
					}
					ImGui::TreePop();
				}
			};
		// 再帰的にノードを描画
		if (model != nullptr)
		{
			drawNodeTree(model->GetRootNode());
		}
	}
	ImGui::End();
}

// プロパティGUI描画
void ModelViewerScene::DrawPropertyGUI()
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 970, pos.y + 30), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Once);

	if (ImGui::Begin("Property", nullptr, ImGuiWindowFlags_None))
	{
		if (selectionNode != nullptr)
		{
			if (ImGui::CollapsingHeader("Node", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// 位置
				if (ImGui::DragFloat3("Local Position", &selectionNode->position.x, 0.1f))
				{
					animationPlaying = false;
				}
				// 位置
				DirectX::XMFLOAT3 globalPosition =
				{
					selectionNode->globalTransform._41,
					selectionNode->globalTransform._42,
					selectionNode->globalTransform._43,
				};
				if (ImGui::DragFloat3("Global Position", &globalPosition.x, 0.1f))
				{
					if (selectionNode->parent != nullptr)
					{
						DirectX::XMMATRIX ParentGlobalTransform = DirectX::XMLoadFloat4x4(&selectionNode->parent->globalTransform);
						DirectX::XMMATRIX InverseParentGlobalTransform = DirectX::XMMatrixInverse(nullptr, ParentGlobalTransform);
						DirectX::XMVECTOR GlobalPosition = DirectX::XMLoadFloat3(&globalPosition);
						DirectX::XMVECTOR LocalPosition = DirectX::XMVector3Transform(GlobalPosition, InverseParentGlobalTransform);
						DirectX::XMStoreFloat3(&selectionNode->position, LocalPosition);
					}
					else
					{
						selectionNode->position = globalPosition;
					}
					animationPlaying = false;
				}

				// 回転
				DirectX::XMFLOAT3 angle;
				TransformUtils::QuaternionToRollPitchYaw(selectionNode->rotation, angle.x, angle.y, angle.z);
				angle.x = DirectX::XMConvertToDegrees(angle.x);
				angle.y = DirectX::XMConvertToDegrees(angle.y);
				angle.z = DirectX::XMConvertToDegrees(angle.z);
				if (ImGui::DragFloat3("Local Rotation", &angle.x, 1.0f))
				{
					angle.x = DirectX::XMConvertToRadians(angle.x);
					angle.y = DirectX::XMConvertToRadians(angle.y);
					angle.z = DirectX::XMConvertToRadians(angle.z);
					DirectX::XMVECTOR Rotation = DirectX::XMQuaternionRotationRollPitchYaw(angle.x, angle.y, angle.z);
					DirectX::XMStoreFloat4(&selectionNode->rotation, Rotation);
				}

				TransformUtils::MatrixToRollPitchYaw(selectionNode->globalTransform, angle.x, angle.y, angle.z);
				angle.x = DirectX::XMConvertToDegrees(angle.x);
				angle.y = DirectX::XMConvertToDegrees(angle.y);
				angle.z = DirectX::XMConvertToDegrees(angle.z);
				if (ImGui::DragFloat3("Global Rotation", &angle.x, 0.1f))
				{
					angle.x = DirectX::XMConvertToRadians(angle.x);
					angle.y = DirectX::XMConvertToRadians(angle.y);
					angle.z = DirectX::XMConvertToRadians(angle.z);
					DirectX::XMVECTOR GlobalRotation = DirectX::XMQuaternionRotationRollPitchYaw(angle.x, angle.y, angle.z);

					if (selectionNode->parent != nullptr)
					{
						DirectX::XMMATRIX ParentGlobalTransform = DirectX::XMLoadFloat4x4(&selectionNode->parent->globalTransform);
						ParentGlobalTransform.r[0] = DirectX::XMVector3Normalize(ParentGlobalTransform.r[0]);
						ParentGlobalTransform.r[1] = DirectX::XMVector3Normalize(ParentGlobalTransform.r[1]);
						ParentGlobalTransform.r[2] = DirectX::XMVector3Normalize(ParentGlobalTransform.r[2]);
						DirectX::XMVECTOR ParentGlobalRotation = DirectX::XMQuaternionRotationMatrix(ParentGlobalTransform);
						DirectX::XMVECTOR InverseParentGlobalRotation = DirectX::XMQuaternionInverse(ParentGlobalRotation);
						DirectX::XMVECTOR LocalRotation = DirectX::XMQuaternionMultiply(GlobalRotation, InverseParentGlobalRotation);
						DirectX::XMStoreFloat4(&selectionNode->rotation, LocalRotation);
					}
					else
					{
						DirectX::XMStoreFloat4(&selectionNode->rotation, GlobalRotation);
					}
					animationPlaying = false;
				}

				// スケール
				if (ImGui::DragFloat3("Local Scale", &selectionNode->scale.x, 0.01f))
				{
					animationPlaying = false;
				}

				DirectX::XMMATRIX GlobalTransform = DirectX::XMLoadFloat4x4(&selectionNode->globalTransform);
				DirectX::XMFLOAT3 globalScale =
				{
					DirectX::XMVectorGetX(DirectX::XMVector3Length(GlobalTransform.r[0])),
					DirectX::XMVectorGetX(DirectX::XMVector3Length(GlobalTransform.r[1])),
					DirectX::XMVectorGetX(DirectX::XMVector3Length(GlobalTransform.r[2]))
				};
				if (ImGui::DragFloat3("Global Scale", &globalScale.x, 0.1f))
				{
				}

			}
		}
	}

	ImGui::End();
}

// アニメーションGUI描画
void ModelViewerScene::DrawAnimationGUI()
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 10, pos.y + 350), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Once);

	if (ImGui::Begin("Animation", nullptr, ImGuiWindowFlags_None))
	{
		ImGui::Checkbox("Loop", &animationLoop); ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::InputFloat("SamplingRate", &animationSamplingRate);
		ImGui::DragFloat("BlendSeconds", &animationBlendSeconds, 0.01f);

		if (model != nullptr)
		{
			float secondsLength = currentAnimationIndex >= 0 ? model->GetAnimations().at(currentAnimationIndex).secondsLength : 0;
			int currentFrame = static_cast<int>(currentAnimationSeconds * 60.0f);
			int frameLength = static_cast<int>(secondsLength * 60);

			ImGui::SetNextItemWidth(50);
			ImGui::PushID(u8"フレーム");
			if (ImGui::DragInt("##v", &currentFrame, 1, 0, frameLength))
			{
				animationPlaying = true;
				currentAnimationSeconds = currentFrame / 60.0f;
				animationSpeed = 0.0f;
			}
			ImGui::PopID();

			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::PushID(u8"タイムライン");
			if (ImGui::SliderFloat("##v", &currentAnimationSeconds, 0, secondsLength, "%.3f"))
			{
				animationPlaying = true;
				animationSpeed = 0.0f;
			}
			ImGui::PopID();

			int index = 0;
			for (const Model::Animation& animation : model->GetAnimations())
			{
				ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf;

				ImGui::TreeNodeEx(&animation, nodeFlags, animation.name.c_str());

				// ダブルクリックでアニメーション再生
				if (ImGui::IsItemClicked())
				{
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						animationPlaying = true;
						currentAnimationIndex = index;
						currentAnimationSeconds = 0.0f;
						animationSpeed = 1.0f;
					}
				}

				ImGui::TreePop();

				index++;
			}
		}
	}

	ImGui::End();
}

// マテリアルGUI描画
void ModelViewerScene::DrawMaterialGUI()
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 970, pos.y + 350), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Once);

	if (ImGui::Begin("Material", nullptr, ImGuiWindowFlags_None))
	{
		if (model != nullptr)
		{
			const char* shaderNames[] =
			{
				"Basic",
				"Lambert",
			};
			ImGui::Combo("Shader", &shaderId, shaderNames, _countof(shaderNames));

			int index = 0;
			for (const Model::Material& material : model->GetMaterials())
			{
				ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
					| ImGuiTreeNodeFlags_OpenOnDoubleClick;

				if (ImGui::TreeNodeEx(&material, nodeFlags, material.name.c_str()))
				{
					ImGui::Text("BaseMap");
					ImGui::Image(material.baseMap.Get(), ImVec2(50, 50));
					DirectX::XMFLOAT4 baseColor = material.baseColor;
					ImGui::ColorEdit4("BaseColor", &baseColor.x, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs);

					ImGui::TreePop();
				}

				index++;
			}
		}
	}

	ImGui::End();
}
