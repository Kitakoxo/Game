#pragma once

#include <map>
#include <filesystem>
#include <tiny_gltf.h>
#include "Model.h"

class GLTFImporter
{
private:
	using MeshList = std::vector<Model::Mesh>;
	using MaterialList = std::vector<Model::Material>;
	using NodeList = std::vector<Model::Node>;
	using AnimationList = std::vector<Model::Animation>;

public:
	GLTFImporter(const char* filename);

	// ノードデータを読み込み
	void LoadNodes(NodeList& nodes);

	// メッシュデータを読み込み
	void LoadMeshes(MeshList& meshes, const NodeList& nodes);

	// マテリアルデータを読み込み
	void LoadMaterials(MaterialList& materials, ID3D11Device* device = nullptr);

	// アニメーションデータを読み込み
	void LoadAnimations(AnimationList& animations, const NodeList& nodes, float sampleRate = 60);

private:
	// gltfVector3 → XMFLOAT3
	static DirectX::XMFLOAT3 gltfVector3ToXMFLOAT3(const std::vector<double>& gltfValue);

	// gltfQuaternion → XMFLOAT4
	static DirectX::XMFLOAT4 gltfQuaternionToXMFLOAT4(const std::vector<double>& gltfValue);

	// gltfMatrix → XMFLOAT4X4
	static DirectX::XMFLOAT4X4 gltfMatrixToXMFLOAT4X4(const std::vector<double>& gltfValue);

	// 座標系変換
	static void ConvertPositionAxisSystem(DirectX::XMFLOAT3& v);
	static void ConvertPositionAxisSystem(DirectX::XMFLOAT4& v);
	static void ConvertRotationAxisSystem(DirectX::XMFLOAT4& q);
	static void ConvertMatrixAxisSystem(DirectX::XMFLOAT4X4& m);
	static void ConvertNodeAxisSystem(Model::Node& node);
	static void ConvertMeshAxisSystem(Model::Mesh& mesh);
	static void ConvertAnimationAxisSystem(Model::Animation& animation);

	// タンジェント計算
	static void ComputeTangents(std::vector<Model::Vertex>& vertices, const std::vector<uint32_t>& indices);

private:
	std::filesystem::path			filepath;
	tinygltf::Model					gltfModel;
};
