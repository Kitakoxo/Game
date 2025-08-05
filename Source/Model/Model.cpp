#include <filesystem>
#include <fstream>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include "Misc.h"
#include "GLTFImporter.h"
#include "GpuResourceUtils.h"
#include "Model.h"

const std::vector<D3D11_INPUT_ELEMENT_DESC> Model::InputElementDescs =
{
	{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

namespace DirectX
{
	template<class Archive>
	void serialize(Archive& archive, XMUINT4& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z),
			cereal::make_nvp("w", v.w)
		);
	}

	template<class Archive>
	void serialize(Archive& archive, XMFLOAT2& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y)
		);
	}

	template<class Archive>
	void serialize(Archive& archive, XMFLOAT3& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z)
		);
	}

	template<class Archive>
	void serialize(Archive& archive, XMFLOAT4& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z),
			cereal::make_nvp("w", v.w)
		);
	}

	template<class Archive>
	void serialize(Archive& archive, XMFLOAT4X4& m)
	{
		archive(
			cereal::make_nvp("_11", m._11),
			cereal::make_nvp("_12", m._12),
			cereal::make_nvp("_13", m._13),
			cereal::make_nvp("_14", m._14),
			cereal::make_nvp("_21", m._21),
			cereal::make_nvp("_22", m._22),
			cereal::make_nvp("_23", m._23),
			cereal::make_nvp("_24", m._24),
			cereal::make_nvp("_31", m._31),
			cereal::make_nvp("_32", m._32),
			cereal::make_nvp("_33", m._33),
			cereal::make_nvp("_34", m._34),
			cereal::make_nvp("_41", m._41),
			cereal::make_nvp("_42", m._42),
			cereal::make_nvp("_43", m._43),
			cereal::make_nvp("_44", m._44)
		);
	}
}

template<class Archive>
void Model::Node::serialize(Archive& archive)
{
	archive(
		CEREAL_NVP(name),
		CEREAL_NVP(parentIndex),
		CEREAL_NVP(position),
		CEREAL_NVP(rotation),
		CEREAL_NVP(scale)
	);
}

template<class Archive>
void Model::Material::serialize(Archive& archive)
{
	archive(
		CEREAL_NVP(name),
		CEREAL_NVP(baseTextureFileName),
		CEREAL_NVP(normalTextureFileName),
		CEREAL_NVP(emissiveTextureFileName),
		CEREAL_NVP(occlusionTextureFileName),
		CEREAL_NVP(metalnessRoughnessTextureFileName),
		CEREAL_NVP(baseColor),
		CEREAL_NVP(emissiveColor),
		CEREAL_NVP(metalness),
		CEREAL_NVP(roughness),
		CEREAL_NVP(occlusionStrength),
		CEREAL_NVP(alphaCutoff),
		CEREAL_NVP(alphaMode)
	);
}

template<class Archive>
void Model::Vertex::serialize(Archive& archive)
{
	archive(
		CEREAL_NVP(position),
		CEREAL_NVP(boneWeight),
		CEREAL_NVP(boneIndex),
		CEREAL_NVP(texcoord),
		CEREAL_NVP(normal),
		CEREAL_NVP(tangent)
	);
}

template<class Archive>
void Model::Bone::serialize(Archive& archive)
{
	archive(
		CEREAL_NVP(nodeIndex),
		CEREAL_NVP(offsetTransform)
	);
}

template<class Archive>
void Model::Mesh::serialize(Archive& archive)
{
	archive(
		CEREAL_NVP(vertices),
		CEREAL_NVP(indices),
		CEREAL_NVP(bones),
		CEREAL_NVP(nodeIndex),
		CEREAL_NVP(materialIndex)
	);
}

template<class Archive>
void Model::VectorKeyframe::serialize(Archive& archive)
{
	archive(
		CEREAL_NVP(seconds),
		CEREAL_NVP(value)
	);
}

template<class Archive>
void Model::QuaternionKeyframe::serialize(Archive& archive)
{
	archive(
		CEREAL_NVP(seconds),
		CEREAL_NVP(value)
	);
}

template<class Archive>
void Model::NodeAnim::serialize(Archive& archive)
{
	archive(
		CEREAL_NVP(positionKeyframes),
		CEREAL_NVP(rotationKeyframes),
		CEREAL_NVP(scaleKeyframes)
	);
}

template<class Archive>
void Model::Animation::serialize(Archive& archive)
{
	archive(
		CEREAL_NVP(name),
		CEREAL_NVP(secondsLength),
		CEREAL_NVP(nodeAnims)
	);
}

// コンストラクタ
Model::Model(ID3D11Device* device, const char* filename, float sampleRate)
{
	std::filesystem::path filepath(filename);
	std::filesystem::path dirpath(filepath.parent_path());

	std::filesystem::path extension = filepath.extension();

	// 独自形式のモデルファイルの存在確認
	filepath.replace_extension(".cereal");
	if (std::filesystem::exists(filepath))
	{
		// 独自形式のモデルファイルの読み込み
		Deserialize(filepath.string().c_str());
	}
	else if (extension == ".gltf" || extension == ".glb")
	{
		// 汎用モデルファイルの読み込み
		GLTFImporter importer(filename);

		// マテリアルデータ読み取り
		importer.LoadMaterials(materials, device);

		// ノードデータ読み取り
		importer.LoadNodes(nodes);

		// メッシュデータ読み取り
		importer.LoadMeshes(meshes, nodes);

		// アニメーションデータ読み取り
		importer.LoadAnimations(animations, nodes, sampleRate);

		// 独自形式のモデルファイルを保存
		//Serialize(filepath.string().c_str());
	}
	else
	{
		_ASSERT_EXPR_A(false, "found not model file");
	}

	// マテリアル構築
	for (Material& material : materials)
	{
		if (material.baseMap == nullptr)
		{
			if (material.baseTextureFileName.empty())
			{
				// ダミーテクスチャ作成
				HRESULT hr = GpuResourceUtils::CreateDummyTexture(device, 0xFFFFFFFF,
					material.baseMap.GetAddressOf());
				_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
			}
			else
			{
				// ベーステクスチャ読み込み
				std::filesystem::path diffuseTexturePath(dirpath / material.baseTextureFileName);
				HRESULT hr = GpuResourceUtils::LoadTexture(device, diffuseTexturePath.string().c_str(),
					material.baseMap.GetAddressOf());
				_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
			}
		}

		if (material.normalMap == nullptr)
		{
			if (material.normalTextureFileName.empty())
			{
				// 法線ダミーテクスチャ作成
				HRESULT hr = GpuResourceUtils::CreateDummyTexture(device, 0xFFFF7F7F,
					material.normalMap.GetAddressOf());
				_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
			}
			else
			{
				// 法線テクスチャ読み込み
				std::filesystem::path texturePath(dirpath / material.normalTextureFileName);
				HRESULT hr = GpuResourceUtils::LoadTexture(device, texturePath.string().c_str(),
					material.normalMap.GetAddressOf());
				_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
			}
		}
	}

	// ノード構築
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		Node& node = nodes.at(nodeIndex);

		// 親子関係を構築
		node.parent = node.parentIndex >= 0 ? &nodes.at(node.parentIndex) : nullptr;
		if (node.parent != nullptr)
		{
			node.parent->children.emplace_back(&node);
		}
	}

	// メッシュ構築
	for (Mesh& mesh : meshes)
	{
		// 参照マテリアル設定
		mesh.material = &materials.at(mesh.materialIndex);

		// 参照ノード設定
		mesh.node = &nodes.at(mesh.nodeIndex);

		// 頂点バッファ
		{
			D3D11_BUFFER_DESC bufferDesc = {};
			D3D11_SUBRESOURCE_DATA subresourceData = {};

			bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.vertices.size());
			bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;
			subresourceData.pSysMem = mesh.vertices.data();
			subresourceData.SysMemPitch = 0;
			subresourceData.SysMemSlicePitch = 0;

			HRESULT hr = device->CreateBuffer(&bufferDesc, &subresourceData, mesh.vertexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}

		// インデックスバッファ
		{
			D3D11_BUFFER_DESC bufferDesc = {};
			D3D11_SUBRESOURCE_DATA subresourceData = {};

			bufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.indices.size());
			bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;
			subresourceData.pSysMem = mesh.indices.data();
			subresourceData.SysMemPitch = 0;
			subresourceData.SysMemSlicePitch = 0;
			HRESULT hr = device->CreateBuffer(&bufferDesc, &subresourceData, mesh.indexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}

		// ボーン構築
		for (Bone& bone : mesh.bones)
		{
			// 参照ノード設定
			bone.node = &nodes.at(bone.nodeIndex);
		}
	}

	// 行列初期化
	DirectX::XMFLOAT4X4 worldTransform;
	DirectX::XMStoreFloat4x4(&worldTransform, DirectX::XMMatrixIdentity());
	UpdateTransform(worldTransform);
}

// アニメーション追加読み込み
void Model::AppendAnimations(const char* filename)
{
	std::filesystem::path filepath(filename);
	std::filesystem::path dirpath(filepath.parent_path());

	if (filepath.extension() == ".gltf" ||
		filepath.extension() == ".glb")
	{
		// 汎用モデルファイルの読み込み
		GLTFImporter importer(filename);

		// アニメーションデータ読み取り
		importer.LoadAnimations(animations, nodes);
	}
	else
	{
		_ASSERT_EXPR_A(false, "found not model file");
	}
}

// アニメーションインデックス取得
int Model::GetAnimationIndex(const char* name) const
{
	for (size_t animationIndex = 0; animationIndex < animations.size(); ++animationIndex)
	{
		if (animations.at(animationIndex).name == name)
		{
			return static_cast<int>(animationIndex);
		}
	}
	return -1;
}

// ノードインデックス取得
int Model::GetNodeIndex(const char* name) const
{
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		if (nodes.at(nodeIndex).name == name)
		{
			return static_cast<int>(nodeIndex);
		}
	}
	return -1;
}

// トランスフォーム更新処理
void Model::UpdateTransform(const DirectX::XMFLOAT4X4& worldTransform)
{
	DirectX::XMMATRIX ParentWorldTransform = DirectX::XMLoadFloat4x4(&worldTransform);

	for (Node& node : nodes)
	{
		// ローカル行列算出
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&node.rotation));
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.position.x, node.position.y, node.position.z);
		DirectX::XMMATRIX LocalTransform = S * R * T;

		// グローバル行列算出
		DirectX::XMMATRIX ParentGlobalTransform;
		if (node.parent != nullptr)
		{
			ParentGlobalTransform = DirectX::XMLoadFloat4x4(&node.parent->globalTransform);
		}
		else
		{
			ParentGlobalTransform = DirectX::XMMatrixIdentity();
		}
		DirectX::XMMATRIX GlobalTransform = LocalTransform * ParentGlobalTransform;

		// ワールド行列算出
		DirectX::XMMATRIX WorldTransform = GlobalTransform * ParentWorldTransform;

		// 計算結果を格納
		DirectX::XMStoreFloat4x4(&node.localTransform, LocalTransform);
		DirectX::XMStoreFloat4x4(&node.globalTransform, GlobalTransform);
		DirectX::XMStoreFloat4x4(&node.worldTransform, WorldTransform);
	}
}

void Model::ComputeAnimation(int animationIndex, int nodeIndex, float time, NodePose& nodePose) const
{
	const Animation& animation = animations.at(animationIndex);
	const NodeAnim& nodeAnim = animation.nodeAnims.at(nodeIndex);

	// 位置
	for (size_t index = 0; index < nodeAnim.positionKeyframes.size() - 1; ++index)
	{
		// 現在の時間がどのキーフレームの間にいるか判定する
		const VectorKeyframe& keyframe0 = nodeAnim.positionKeyframes.at(index);
		const VectorKeyframe& keyframe1 = nodeAnim.positionKeyframes.at(index + 1);
		if (time >= keyframe0.seconds && time <= keyframe1.seconds)
		{
			// 再生時間とキーフレームの時間から補完率を算出する
			float rate = (time - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

			// 前のキーフレームと次のキーフレームの姿勢を補完
			DirectX::XMVECTOR V0 = DirectX::XMLoadFloat3(&keyframe0.value);
			DirectX::XMVECTOR V1 = DirectX::XMLoadFloat3(&keyframe1.value);
			DirectX::XMVECTOR V = DirectX::XMVectorLerp(V0, V1, rate);
			// 計算結果をノードに格納
			DirectX::XMStoreFloat3(&nodePose.position, V);
		}
	}
	// 回転
	for (size_t index = 0; index < nodeAnim.rotationKeyframes.size() - 1; ++index)
	{
		// 現在の時間がどのキーフレームの間にいるか判定する
		const QuaternionKeyframe& keyframe0 = nodeAnim.rotationKeyframes.at(index);
		const QuaternionKeyframe& keyframe1 = nodeAnim.rotationKeyframes.at(index + 1);
		if (time >= keyframe0.seconds && time <= keyframe1.seconds)
		{
			// 再生時間とキーフレームの時間から補完率を算出する
			float rate = (time - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

			// 前のキーフレームと次のキーフレームの姿勢を補完
			DirectX::XMVECTOR Q0 = DirectX::XMLoadFloat4(&keyframe0.value);
			DirectX::XMVECTOR Q1 = DirectX::XMLoadFloat4(&keyframe1.value);
			DirectX::XMVECTOR Q = DirectX::XMQuaternionSlerp(Q0, Q1, rate);
			// 計算結果をノードに格納
			DirectX::XMStoreFloat4(&nodePose.rotation, Q);
		}
	}
	// スケール
	for (size_t index = 0; index < nodeAnim.scaleKeyframes.size() - 1; ++index)
	{
		// 現在の時間がどのキーフレームの間にいるか判定する
		const VectorKeyframe& keyframe0 = nodeAnim.scaleKeyframes.at(index);
		const VectorKeyframe& keyframe1 = nodeAnim.scaleKeyframes.at(index + 1);
		if (time >= keyframe0.seconds && time <= keyframe1.seconds)
		{
			// 再生時間とキーフレームの時間から補完率を算出する
			float rate = (time - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

			// 前のキーフレームと次のキーフレームの姿勢を補完
			DirectX::XMVECTOR V0 = DirectX::XMLoadFloat3(&keyframe0.value);
			DirectX::XMVECTOR V1 = DirectX::XMLoadFloat3(&keyframe1.value);
			DirectX::XMVECTOR V = DirectX::XMVectorLerp(V0, V1, rate);
			// 計算結果をノードに格納
			DirectX::XMStoreFloat3(&nodePose.scale, V);
		}
	}
}

// アニメーション計算
void Model::ComputeAnimation(int animationIndex, float time, std::vector<NodePose>& nodePoses) const
{
	if (nodePoses.size() != nodes.size())
	{
		nodePoses.resize(nodes.size());
	}
	for (size_t nodeIndex = 0; nodeIndex < nodePoses.size(); ++nodeIndex)
	{
		ComputeAnimation(animationIndex, static_cast<int>(nodeIndex), time, nodePoses.at(nodeIndex));
	}
}

// ノードポーズ設定
void Model::SetNodePoses(const std::vector<NodePose>& nodePoses)
{
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		const NodePose& pose = nodePoses.at(nodeIndex);
		Node& node = nodes.at(nodeIndex);

		node.position = pose.position;
		node.rotation = pose.rotation;
		node.scale = pose.scale;
	}
}

// ノードポーズ取得
void Model::GetNodePoses(std::vector<NodePose>& nodePoses) const
{
	if (nodePoses.size() != nodes.size())
	{
		nodePoses.resize(nodes.size());
	}
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		const Node& node = nodes.at(nodeIndex);
		NodePose& pose = nodePoses.at(nodeIndex);

		pose.position = node.position;
		pose.rotation = node.rotation;
		pose.scale = node.scale;
	}
}

// シリアライズ
void Model::Serialize(const char* filename)
{
	std::ofstream ostream(filename, std::ios::binary);
	if (ostream.is_open())
	{
		cereal::BinaryOutputArchive archive(ostream);

		try
		{
			archive(
				CEREAL_NVP(nodes),
				CEREAL_NVP(materials),
				CEREAL_NVP(meshes),
				CEREAL_NVP(animations)
			);
		}
		catch (...)
		{
			_ASSERT_EXPR_A(false, "Model serialize failed.");
		}
	}
}

// デシリアライズ
void Model::Deserialize(const char* filename)
{
	std::ifstream istream(filename, std::ios::binary);
	if (istream.is_open())
	{
		cereal::BinaryInputArchive archive(istream);

		try
		{
			archive(
				CEREAL_NVP(nodes),
				CEREAL_NVP(materials),
				CEREAL_NVP(meshes),
				CEREAL_NVP(animations)
			);
		}
		catch (...)
		{
			_ASSERT_EXPR_A(false, "Model deserialize failed.");
		}
	}
	else
	{
		_ASSERT_EXPR_A(false, "Model File not found.");
	}
}
