#include <fstream>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "Misc.h"
#include "GpuResourceUtils.h"
#include "GLTFImporter.h"

bool LoadImageData(tinygltf::Image*, const int, std::string*,
	std::string*, int, int,
	const unsigned char*, int,
	void* user_pointer)
{
	return true;
}

// コンストラクタ
GLTFImporter::GLTFImporter(const char* filename)
	: filepath(filename) 
{
	// 拡張子取得
	std::string extension = filepath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), tolower);	// 小文字化

	tinygltf::TinyGLTF gltf;

	// 勝手にイメージを読み込まないようにする
	gltf.SetImageLoader(LoadImageData, this);

	std::string error, warning;
	bool result = false;
	if (extension == ".glb")
	{
		result = gltf.LoadBinaryFromFile(&gltfModel, &error, &warning, filepath.u8string());
	}
	else if (extension == ".gltf")
	{
		result = gltf.LoadASCIIFromFile(&gltfModel, &error, &warning, filepath.u8string());
	}
	if (!warning.empty())
	{
		OutputDebugStringA(warning.c_str());
		OutputDebugStringA("\n");
	}
	if (!result)
	{
		OutputDebugStringA(error.c_str());
		OutputDebugStringA("\n");
		_ASSERT_EXPR_A(result, error.c_str());
	}
}

// ノードデータを読み込み
void GLTFImporter::LoadNodes(NodeList& nodes)
{
	Model::Node& node = nodes.emplace_back();
	node.name = filepath.filename().stem().string();

	nodes.resize(gltfModel.nodes.size());
	for (size_t gltfNodeIndex = 0; gltfNodeIndex < nodes.size(); ++gltfNodeIndex)
	{
		const tinygltf::Node& gltfNode = gltfModel.nodes.at(gltfNodeIndex);
		Model::Node& node = nodes.at(gltfNodeIndex);

		// データ取得
		node.name = gltfNode.name;
		
		for (int gltfChildNodeIndex : gltfNode.children)
		{
			nodes.at(gltfChildNodeIndex).parentIndex = static_cast<int>(gltfNodeIndex);
		}

		if (!gltfNode.matrix.empty())
		{
			DirectX::XMFLOAT4X4 m = gltfMatrixToXMFLOAT4X4(gltfNode.matrix);

			DirectX::XMVECTOR S, R, T;
			DirectX::XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&m));

			DirectX::XMStoreFloat3(&node.scale, S);
			DirectX::XMStoreFloat4(&node.rotation, R);
			DirectX::XMStoreFloat3(&node.position, T);
		}
		else
		{
			if (gltfNode.scale.size() > 0)
			{
				node.scale = gltfVector3ToXMFLOAT3(gltfNode.scale);
			}
			if (gltfNode.rotation.size() > 0)
			{
				node.rotation = gltfQuaternionToXMFLOAT4(gltfNode.rotation);
			}
			if (gltfNode.translation.size() > 0)
			{
				node.position = gltfVector3ToXMFLOAT3(gltfNode.translation);
			}
		}
		// 座標系変換
		ConvertNodeAxisSystem(node);
	}
}

// メッシュデータを読み込み
void GLTFImporter::LoadMeshes(MeshList& meshes, const NodeList& nodes)
{
	for (int gltfNodeIndex = 0; gltfNodeIndex < gltfModel.nodes.size(); ++gltfNodeIndex)
	{
		const tinygltf::Node& gltfNode = gltfModel.nodes.at(gltfNodeIndex);
		if (gltfNode.mesh < 0) continue;

		const tinygltf::Mesh& gltfMesh = gltfModel.meshes.at(gltfNode.mesh);
				
		for (const tinygltf::Primitive& gltfPrimitive : gltfMesh.primitives)
		{
			Model::Mesh& mesh = meshes.emplace_back();
			mesh.nodeIndex = gltfNodeIndex;
			mesh.materialIndex = gltfPrimitive.material;

			// ボーン
			if (gltfNode.skin >= 0)
			{
				const tinygltf::Skin& gltfSkin = gltfModel.skins.at(gltfNode.skin);
				const tinygltf::Accessor& gltfAccessor = gltfModel.accessors.at(gltfSkin.inverseBindMatrices);
				const tinygltf::BufferView& gltfBufferView = gltfModel.bufferViews.at(gltfAccessor.bufferView);

				for (int i = 0; i < gltfAccessor.count; ++i)
				{
					Model::Bone& bone = mesh.bones.emplace_back();
					const DirectX::XMFLOAT4X4* offsetTransforms = reinterpret_cast<const DirectX::XMFLOAT4X4*>(gltfModel.buffers.at(gltfBufferView.buffer).data.data() + gltfBufferView.byteOffset + gltfAccessor.byteOffset);
					bone.offsetTransform = offsetTransforms[i];
					bone.nodeIndex = gltfSkin.joints.at(i);
				}
			}

			// インデックスバッファ
			{
				const tinygltf::Accessor& gltfAccessor = gltfModel.accessors.at(gltfPrimitive.indices);
				const tinygltf::BufferView& gltfBufferView = gltfModel.bufferViews.at(gltfAccessor.bufferView);
				const void* indices = gltfModel.buffers.at(gltfBufferView.buffer).data.data() + gltfBufferView.byteOffset + gltfAccessor.byteOffset;
				mesh.indices.resize(gltfAccessor.count);
				if (gltfAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					const uint32_t* indicesU32 = static_cast<const uint32_t*>(indices);
					for (size_t i = 0; i < gltfAccessor.count; ++i)
					{
						mesh.indices.at(i) = indicesU32[i];
					}
				}
				else if (gltfAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					const uint16_t* indicesU16 = static_cast<const uint16_t*>(indices);
					for (size_t i = 0; i < gltfAccessor.count; ++i)
					{
						mesh.indices.at(i) = indicesU16[i];
					}
				}
				else
				{
					_ASSERT_EXPR_A(false, "This accessor component type is not supported.");;
				}
			}

			// 頂点バッファ領域確保
			{
				std::map<std::string, int>::const_iterator gltfAttribute = gltfPrimitive.attributes.find("POSITION");
				_ASSERT_EXPR(gltfAttribute != gltfPrimitive.attributes.end(), "");
				const tinygltf::Accessor& gltfAccessor = gltfModel.accessors.at(gltfAttribute->second);
				mesh.vertices.resize(gltfAccessor.count);
			}

			// 頂点バッファ
			for (std::map<std::string, int>::const_reference gltfAttribute : gltfPrimitive.attributes)
			{
				const tinygltf::Accessor& gltfAccessor = gltfModel.accessors.at(gltfAttribute.second);
				const tinygltf::BufferView& gltfBufferView = gltfModel.bufferViews.at(gltfAccessor.bufferView);
				void* vertexBuffer = gltfModel.buffers.at(gltfBufferView.buffer).data.data() + gltfBufferView.byteOffset + gltfAccessor.byteOffset;
				if (gltfAttribute.first == "POSITION")
				{
					const float* p = static_cast<const float*>(vertexBuffer);
					for (size_t i = 0; i < gltfAccessor.count; ++i)
					{
						DirectX::XMFLOAT3& position = mesh.vertices.at(i).position;
						position.x = p[0];
						position.y = p[1];
						position.z = p[2];
						p += 3;
					}
				}
				else if (gltfAttribute.first == "NORMAL")
				{
					const float* p = static_cast<const float*>(vertexBuffer);
					for (size_t i = 0; i < gltfAccessor.count; ++i)
					{
						DirectX::XMFLOAT3& normal = mesh.vertices.at(i).normal;
						normal.x = p[0];
						normal.y = p[1];
						normal.z = p[2];
						p += 3;
					}
				}
				else if (gltfAttribute.first == "TANGENT")
				{
					const float* p = static_cast<const float*>(vertexBuffer);
					for (size_t i = 0; i < gltfAccessor.count; ++i)
					{
						DirectX::XMFLOAT4& tangent = mesh.vertices.at(i).tangent;
						tangent.x = p[0];
						tangent.y = p[1];
						tangent.z = p[2];
						tangent.w = p[3];
						p += 4;
					}
				}
				else if (gltfAttribute.first == "TEXCOORD_0")
				{
					switch (gltfAccessor.componentType)
					{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
						{
							const float* p = static_cast<const float*>(vertexBuffer);
							for (size_t i = 0; i < gltfAccessor.count; ++i)
							{
								DirectX::XMFLOAT2& texcoord = mesh.vertices.at(i).texcoord;
								texcoord.x = p[0];
								texcoord.y = p[1];
								p += 2;
							}
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						{
							const uint8_t* p = static_cast<const uint8_t*>(vertexBuffer);
							for (size_t i = 0; i < gltfAccessor.count; ++i)
							{
								DirectX::XMFLOAT2& texcoord = mesh.vertices.at(i).texcoord;
								texcoord.x = p[0] / static_cast<float>(0xFF);
								texcoord.y = p[1] / static_cast<float>(0xFF);
								p += 2;
							}
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						{
							const uint16_t* p = static_cast<const uint16_t*>(vertexBuffer);
							for (size_t i = 0; i < gltfAccessor.count; ++i)
							{
								DirectX::XMFLOAT2& texcoord = mesh.vertices.at(i).texcoord;
								texcoord.x = p[0] / static_cast<float>(0xFFFF);
								texcoord.y = p[1] / static_cast<float>(0xFFFF);
								p += 2;
							}
							break;
						}
						default:
						{
							_ASSERT_EXPR(0, L"");
							continue;
						}
					}
				}
				else if (gltfAttribute.first == "JOINTS_0")
				{
					switch (gltfAccessor.componentType)
					{
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						{
							const uint8_t* p = static_cast<const uint8_t*>(vertexBuffer);
							for (size_t i = 0; i < gltfAccessor.count; ++i)
							{
								DirectX::XMUINT4& boneIndex = mesh.vertices.at(i).boneIndex;
								boneIndex.x = p[0];
								boneIndex.y = p[1];
								boneIndex.z = p[2];
								boneIndex.w = p[3];
								p += 4;
							}
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						{
							const uint16_t* p = static_cast<const uint16_t*>(vertexBuffer);
							for (size_t i = 0; i < gltfAccessor.count; ++i)
							{
								DirectX::XMUINT4& boneIndex = mesh.vertices.at(i).boneIndex;
								boneIndex.x = p[0];
								boneIndex.y = p[1];
								boneIndex.z = p[2];
								boneIndex.w = p[3];
								p += 4;
							}
							break;
						}
						default:
						{
							_ASSERT_EXPR(0, L"");
							continue;
						}
					}
				}
				else if (gltfAttribute.first == "WEIGHTS_0")
				{
					switch (gltfAccessor.componentType)
					{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
						{
							const float* p = static_cast<const float*>(vertexBuffer);
							for (size_t i = 0; i < gltfAccessor.count; ++i)
							{
								DirectX::XMFLOAT4& boneWeights = mesh.vertices.at(i).boneWeight;
								boneWeights.x = p[0];
								boneWeights.y = p[1];
								boneWeights.z = p[2];
								boneWeights.w = p[3];
								p += 4;
							}
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						{
							const uint8_t* p = static_cast<const uint8_t*>(vertexBuffer);
							for (size_t i = 0; i < gltfAccessor.count; ++i)
							{
								DirectX::XMFLOAT4& boneWeights = mesh.vertices.at(i).boneWeight;
								boneWeights.x = p[0] / static_cast<float>(0xFF);
								boneWeights.y = p[1] / static_cast<float>(0xFF);
								boneWeights.z = p[2] / static_cast<float>(0xFF);
								boneWeights.w = p[3] / static_cast<float>(0xFF);
								p += 4;
							}
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						{
							const uint16_t* p = static_cast<const uint16_t*>(vertexBuffer);
							for (size_t i = 0; i < gltfAccessor.count; ++i)
							{
								DirectX::XMFLOAT4& boneWeights = mesh.vertices.at(i).boneWeight;
								boneWeights.x = p[0] / static_cast<float>(0xFFFF);
								boneWeights.y = p[1] / static_cast<float>(0xFFFF);
								boneWeights.z = p[2] / static_cast<float>(0xFFFF);
								boneWeights.w = p[3] / static_cast<float>(0xFFFF);
								p += 4;
							}
							break;
						}
						default:
						{
							_ASSERT_EXPR(0, L"");
							continue;
						}
					}
				}
			}

			// タンジェントがなかった場合は自力で計算
			if (gltfPrimitive.attributes.find("TANGENT") == gltfPrimitive.attributes.end() &&
				gltfPrimitive.attributes.find("POSITION") != gltfPrimitive.attributes.end() &&
				gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end())
			{
				ComputeTangents(mesh.vertices, mesh.indices);
			}

			// 座標系変換
			ConvertMeshAxisSystem(mesh);
		}
	}

}

// マテリアルデータを読み込み
void GLTFImporter::LoadMaterials(MaterialList& materials, ID3D11Device* device)
{
	// ディレクトリパス取得
	std::filesystem::path dirpath(filepath.parent_path());

	for (const tinygltf::Material& gltfMaterial : gltfModel.materials)
	{
		Model::Material& material = materials.emplace_back();
		material.name = gltfMaterial.name;
		material.baseColor.x = static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor.at(0));
		material.baseColor.y = static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor.at(1));
		material.baseColor.z = static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor.at(2));
		material.baseColor.w = static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor.at(3));
		material.emissiveColor.x = static_cast<float>(gltfMaterial.emissiveFactor.at(0));
		material.emissiveColor.y = static_cast<float>(gltfMaterial.emissiveFactor.at(1));
		material.emissiveColor.z = static_cast<float>(gltfMaterial.emissiveFactor.at(2));
		material.metalness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
		material.roughness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);
		material.occlusionStrength = static_cast<float>(gltfMaterial.occlusionTexture.strength);
		material.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
		if (gltfMaterial.alphaMode == "BLEND")
		{
			material.alphaMode = Model::AlphaMode::Blend;
		}
		else if (gltfMaterial.alphaMode == "MASK")
		{
			material.alphaMode = Model::AlphaMode::Mask;
		}
		else
		{
			material.alphaMode = Model::AlphaMode::Opaque;
		}

		auto loadTexture = [&](int gltfTextureIndex, const char* textureType, std::string& textureFilename, ID3D11ShaderResourceView** srv)
		{
			if (gltfTextureIndex < 0) return;

			const tinygltf::Texture& gltfTexture = gltfModel.textures.at(gltfTextureIndex);
			const tinygltf::Image& gltfImage = gltfModel.images.at(gltfTexture.source);
			if (gltfImage.bufferView >= 0 || !gltfImage.image.empty())
			{
				if (device != nullptr)
				{
					if (gltfImage.bufferView >= 0)
					{
						const tinygltf::BufferView& gltfBufferView = gltfModel.bufferViews.at(gltfImage.bufferView);
						const tinygltf::Buffer& gltfBuffer = gltfModel.buffers.at(gltfBufferView.buffer);
						const byte* data = gltfBuffer.data.data() + gltfBufferView.byteOffset;
						GpuResourceUtils::LoadTexture(device, data, gltfBufferView.byteLength, srv);
					}
					else
					{
						HRESULT hr = GpuResourceUtils::LoadTexture(device, gltfImage.image.data(), gltfImage.image.size(), srv);
						if (FAILED(hr))
						{
							D3D11_TEXTURE2D_DESC desc = { 0 };
							desc.Width = gltfImage.width;
							desc.Height = gltfImage.height;
							desc.MipLevels = 1;
							desc.ArraySize = 1;
							desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
							desc.SampleDesc.Count = 1;
							desc.SampleDesc.Quality = 0;
							desc.Usage = D3D11_USAGE_DEFAULT;
							desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
							desc.CPUAccessFlags = 0;
							desc.MiscFlags = 0;
							D3D11_SUBRESOURCE_DATA data;
							::memset(&data, 0, sizeof(data));
							data.pSysMem = gltfImage.image.data();
							data.SysMemPitch = gltfImage.width * (gltfImage.bits / 8);

							Microsoft::WRL::ComPtr<ID3D11Texture2D>	texture;
							HRESULT hr = device->CreateTexture2D(&desc, &data, texture.GetAddressOf());
							_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

							hr = device->CreateShaderResourceView(texture.Get(), nullptr, srv);
							_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
						}
					}
				}
				else
				{
					// テクスチャファイルパス作成
					std::filesystem::path textureFilePath(gltfImage.uri);
					if (textureFilePath == "")
					{
						// テクスチャファイル名がなかった場合はマテリアル名とテクスチャタイプから作成
						std::string extension(std::filesystem::path(gltfImage.mimeType).filename().string());
						textureFilePath = material.name + "_" + textureType + "." + extension;
					}
					textureFilePath = "Textures" / textureFilePath.filename();

					// 埋め込みテクスチャを出力するディレクトリを確認
					std::filesystem::path outputDirPath(dirpath / textureFilePath.parent_path());
					if (!std::filesystem::exists(outputDirPath))
					{
						// なかったらディレクトリ作成
						std::filesystem::create_directories(outputDirPath);
					}
					// 出力ディレクトリに画像ファイルを保存
					std::filesystem::path outputFilePath(dirpath / textureFilePath);
					if (!std::filesystem::exists(outputFilePath))
					{
						if (gltfImage.bufferView >= 0)
						{
							const tinygltf::BufferView& gltfBufferView = gltfModel.bufferViews.at(gltfImage.bufferView);
							const tinygltf::Buffer& gltfBuffer = gltfModel.buffers.at(gltfBufferView.buffer);
							const byte* data = gltfBuffer.data.data() + gltfBufferView.byteOffset;

							std::ofstream os(outputFilePath.string().c_str(), std::ios::binary);
							os.write(reinterpret_cast<const char*>(data), gltfBufferView.byteLength);
						}
						else
						{
							// リニアな画像データは.pngで出力
							textureFilePath = textureFilePath.replace_extension(".png");
							stbi_write_png(
								outputFilePath.string().c_str(),
								static_cast<int>(gltfImage.width),
								static_cast<int>(gltfImage.height),
								static_cast<int>(sizeof(uint32_t)),
								gltfImage.image.data(), 0);
						}
					}
					// テクスチャファイルパスを格納
					textureFilename = textureFilePath.string();
				}
			}
			else
			{
				textureFilename = gltfImage.uri;
			}
		};
		loadTexture(gltfMaterial.pbrMetallicRoughness.baseColorTexture.index, "Base", material.baseTextureFileName, material.baseMap.GetAddressOf());
		loadTexture(gltfMaterial.normalTexture.index, "Normal", material.normalTextureFileName, material.normalMap.GetAddressOf());
		loadTexture(gltfMaterial.emissiveTexture.index, "Emissive", material.emissiveTextureFileName, material.emissiveMap.GetAddressOf());
		loadTexture(gltfMaterial.occlusionTexture.index, "Occlusion", material.occlusionTextureFileName, material.occlusionMap.GetAddressOf());
		loadTexture(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index, "MetallicRoughness", material.metalnessRoughnessTextureFileName, material.metalnessRoughnessMap.GetAddressOf());
	}
}

// アニメーションデータを読み込み
void GLTFImporter::LoadAnimations(AnimationList& animations, const NodeList& nodes, float sampleRate)
{
	DirectX::XMVECTOR Epsilon = DirectX::XMVectorReplicate(0.00001f);

	for (const tinygltf::Animation& gltfAnimation : gltfModel.animations)
	{
		Model::Animation& animation = animations.emplace_back();
		animation.name = gltfAnimation.name;
		animation.nodeAnims.resize(nodes.size());
		animation.secondsLength = 0;

		float minTime = FLT_MAX;
		float maxTime = 0;
		for (const tinygltf::AnimationChannel& gltfAnimationChannel : gltfAnimation.channels)
		{
			// ノードアニメーションデータを読み取り開始
			Model::NodeAnim& nodeAnim = animation.nodeAnims.at(gltfAnimationChannel.target_node);
			const tinygltf::AnimationSampler& gltfAnimationSampler = gltfAnimation.samplers.at(gltfAnimationChannel.sampler);
			const tinygltf::Accessor& gltfInputAccessor = gltfModel.accessors.at(gltfAnimationSampler.input);
			const tinygltf::Accessor& gltfOutputAccessor = gltfModel.accessors.at(gltfAnimationSampler.output);
			const tinygltf::BufferView& gltfInputBufferView{ gltfModel.bufferViews.at(gltfInputAccessor.bufferView) };
			const tinygltf::BufferView& gltfOutputBufferView = gltfModel.bufferViews.at(gltfOutputAccessor.bufferView);

			const float* gltfKeyframeTimes = reinterpret_cast<const float*>(gltfModel.buffers.at(gltfInputBufferView.buffer).data.data() + gltfInputBufferView.byteOffset + gltfInputAccessor.byteOffset);
			minTime = (std::min)(minTime, gltfKeyframeTimes[0]);
			maxTime = (std::max)(animation.secondsLength, gltfKeyframeTimes[gltfInputAccessor.count - 1]);

			if (gltfAnimationChannel.target_path == "scale")
			{
				// キーフレームデータ取得
				const DirectX::XMFLOAT3* gltfKeyframeValues = reinterpret_cast<const DirectX::XMFLOAT3*>(gltfModel.buffers.at(gltfOutputBufferView.buffer).data.data() + gltfOutputBufferView.byteOffset + gltfOutputAccessor.byteOffset);
				for (int i = 0; i < gltfInputAccessor.count; ++i)
				{
					Model::VectorKeyframe& keyframe = nodeAnim.scaleKeyframes.emplace_back();
					keyframe.seconds = gltfKeyframeTimes[i];
					keyframe.value = gltfKeyframeValues[i];
				}
				// キーフレームの値が全て同じなら最初のキーフレーム以外省く
				bool result = true;
				DirectX::XMVECTOR A = DirectX::XMLoadFloat3(&nodeAnim.scaleKeyframes.at(0).value);
				for (size_t i = 1; i < nodeAnim.scaleKeyframes.size(); ++i)
				{
					DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&nodeAnim.scaleKeyframes.at(i).value);
					if (!DirectX::XMVector3NearEqual(A, B, Epsilon))
					{
						result = false;
						break;
					}
				}
				if (result)
				{
					nodeAnim.scaleKeyframes.resize(1);
				}
			}
			else if (gltfAnimationChannel.target_path == "rotation")
			{
				// キーフレームデータ取得
				const DirectX::XMFLOAT4* gltfKeyframeValues = reinterpret_cast<const DirectX::XMFLOAT4*>(gltfModel.buffers.at(gltfOutputBufferView.buffer).data.data() + gltfOutputBufferView.byteOffset + gltfOutputAccessor.byteOffset);
				for (int i = 0; i < gltfInputAccessor.count; ++i)
				{
					// なぜかUnityで出力したアニメーションデータにはゴミと思われるキーフレームが存在している場合がある。
					// 小数点が存在するフレーム（時間）がゴミデータっぽいので除外する。
					float frame = gltfKeyframeTimes[i] * sampleRate;
					if (fabs(std::round(frame) - frame) > 0.001) continue;

					Model::QuaternionKeyframe& keyframe = nodeAnim.rotationKeyframes.emplace_back();
					keyframe.seconds = gltfKeyframeTimes[i];
					keyframe.value = gltfKeyframeValues[i];
				}

				// キーフレームの値が全て同じなら最初のキーフレーム以外省く
				bool result = true;
				DirectX::XMVECTOR A = DirectX::XMLoadFloat4(&nodeAnim.rotationKeyframes.at(0).value);
				for (size_t i = 1; i < nodeAnim.rotationKeyframes.size(); ++i)
				{
					DirectX::XMVECTOR B = DirectX::XMLoadFloat4(&nodeAnim.rotationKeyframes.at(i).value);
					if (!DirectX::XMVector4NearEqual(A, B, Epsilon))
					{
						result = false;
						break;
					}
				}
				if (result)
				{
					nodeAnim.rotationKeyframes.resize(1);
				}
			}
			else if (gltfAnimationChannel.target_path == "translation")
			{
				// キーフレームデータ取得
				const DirectX::XMFLOAT3* gltfKeyframeValues = reinterpret_cast<const DirectX::XMFLOAT3*>(gltfModel.buffers.at(gltfOutputBufferView.buffer).data.data() + gltfOutputBufferView.byteOffset + gltfOutputAccessor.byteOffset);
				for (int i = 0; i < gltfInputAccessor.count; ++i)
				{
					Model::VectorKeyframe& keyframe = nodeAnim.positionKeyframes.emplace_back();
					keyframe.seconds = gltfKeyframeTimes[i];
					keyframe.value = gltfKeyframeValues[i];
				}

				// キーフレームの値が全て同じなら最初のキーフレーム以外省く
				bool result = true;
				DirectX::XMVECTOR A = DirectX::XMLoadFloat3(&nodeAnim.positionKeyframes.at(0).value);
				for (size_t i = 1; i < nodeAnim.positionKeyframes.size(); ++i)
				{
					DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&nodeAnim.positionKeyframes.at(i).value);
					if (!DirectX::XMVector3NearEqual(A, B, Epsilon))
					{
						result = false;
						break;
					}
				}
				if (result)
				{
					nodeAnim.positionKeyframes.resize(1);
				}
			}
		}

		// 先頭キーフレームの時間が0じゃない場合があるので調整する
		for (Model::NodeAnim& nodeAnim : animation.nodeAnims)
		{
			for (Model::VectorKeyframe& keyframe : nodeAnim.positionKeyframes)
			{
				keyframe.seconds -= minTime;
			}
			for (Model::QuaternionKeyframe& keyframe : nodeAnim.rotationKeyframes)
			{
				keyframe.seconds -= minTime;
			}
			for (Model::VectorKeyframe& keyframe : nodeAnim.scaleKeyframes)
			{
				keyframe.seconds -= minTime;
			}
		}
		// アニメーション再生時間
		animation.secondsLength = maxTime - minTime;

		// 座標系変換
		ConvertAnimationAxisSystem(animation);

		// アニメーションがなかったノードに対して初期姿勢のキーフレームを追加する
		for (size_t nodeIndex = 0; nodeIndex < animation.nodeAnims.size(); ++nodeIndex)
		{
			const Model::Node& node = nodes.at(nodeIndex);
			Model::NodeAnim& nodeAnim = animation.nodeAnims.at(nodeIndex);
			// 移動
			if (nodeAnim.positionKeyframes.size() == 0)
			{
				Model::VectorKeyframe& keyframe = nodeAnim.positionKeyframes.emplace_back();
				keyframe.seconds = 0.0f;
				keyframe.value = node.position;
			}
			if (nodeAnim.positionKeyframes.size() == 1)
			{
				Model::VectorKeyframe& keyframe = nodeAnim.positionKeyframes.emplace_back();
				keyframe.seconds = animation.secondsLength;
				keyframe.value = nodeAnim.positionKeyframes.at(0).value;
			}
			// 回転
			if (nodeAnim.rotationKeyframes.size() == 0)
			{
				Model::QuaternionKeyframe& keyframe = nodeAnim.rotationKeyframes.emplace_back();
				keyframe.seconds = 0.0f;
				keyframe.value = node.rotation;
			}
			if (nodeAnim.rotationKeyframes.size() == 1)
			{
				Model::QuaternionKeyframe& keyframe = nodeAnim.rotationKeyframes.emplace_back();
				keyframe.seconds = animation.secondsLength;
				keyframe.value = nodeAnim.rotationKeyframes.at(0).value;
			}
			// スケール
			if (nodeAnim.scaleKeyframes.size() == 0)
			{
				Model::VectorKeyframe& keyframe = nodeAnim.scaleKeyframes.emplace_back();
				keyframe.seconds = 0.0f;
				keyframe.value = node.scale;
			}
			if (nodeAnim.scaleKeyframes.size() == 1)
			{
				Model::VectorKeyframe& keyframe = nodeAnim.scaleKeyframes.emplace_back();
				keyframe.seconds = animation.secondsLength;
				keyframe.value = nodeAnim.scaleKeyframes.at(0).value;
			}
		}
	}
}

// gltfVector3 → XMFLOAT3
DirectX::XMFLOAT3 GLTFImporter::gltfVector3ToXMFLOAT3(const std::vector<double>& gltfValue)
{
	return DirectX::XMFLOAT3(
		static_cast<float>(gltfValue.at(0)),
		static_cast<float>(gltfValue.at(1)),
		static_cast<float>(gltfValue.at(2))
	);
}

// gltfQuaternion → XMFLOAT4
DirectX::XMFLOAT4 GLTFImporter::gltfQuaternionToXMFLOAT4(const std::vector<double>& gltfValue)
{
	return DirectX::XMFLOAT4(
		static_cast<float>(gltfValue.at(0)),
		static_cast<float>(gltfValue.at(1)),
		static_cast<float>(gltfValue.at(2)),
		static_cast<float>(gltfValue.at(3))
	);
}

// gltfMatrix → XMFLOAT4X4
DirectX::XMFLOAT4X4 GLTFImporter::gltfMatrixToXMFLOAT4X4(const std::vector<double>& gltfValue)
{
	return DirectX::XMFLOAT4X4(
		static_cast<float>(gltfValue.at(0)),
		static_cast<float>(gltfValue.at(1)),
		static_cast<float>(gltfValue.at(2)),
		static_cast<float>(gltfValue.at(3)),
		static_cast<float>(gltfValue.at(4)),
		static_cast<float>(gltfValue.at(5)),
		static_cast<float>(gltfValue.at(6)),
		static_cast<float>(gltfValue.at(7)),
		static_cast<float>(gltfValue.at(8)),
		static_cast<float>(gltfValue.at(9)),
		static_cast<float>(gltfValue.at(10)),
		static_cast<float>(gltfValue.at(11)),
		static_cast<float>(gltfValue.at(12)),
		static_cast<float>(gltfValue.at(13)),
		static_cast<float>(gltfValue.at(14)),
		static_cast<float>(gltfValue.at(15))
	);
}

// 座標系変換
void GLTFImporter::ConvertPositionAxisSystem(DirectX::XMFLOAT3& v)
{
	v.x = -v.x;
}

void GLTFImporter::ConvertPositionAxisSystem(DirectX::XMFLOAT4& v)
{
	v.x = -v.x;
}

void GLTFImporter::ConvertRotationAxisSystem(DirectX::XMFLOAT4& q)
{
	q.x = -q.x;
	q.w = -q.w;
}

void GLTFImporter::ConvertMatrixAxisSystem(DirectX::XMFLOAT4X4& m)
{
	m._12 = -m._12;
	m._13 = -m._13;
	m._21 = -m._21;
	m._31 = -m._31;
	m._41 = -m._41;
}

void GLTFImporter::ConvertNodeAxisSystem(Model::Node& node)
{
	ConvertPositionAxisSystem(node.position);
	ConvertRotationAxisSystem(node.rotation);
}

void GLTFImporter::ConvertMeshAxisSystem(Model::Mesh& mesh)
{
	for (Model::Vertex& v : mesh.vertices)
	{
		ConvertPositionAxisSystem(v.position);
		ConvertPositionAxisSystem(v.normal);
		ConvertPositionAxisSystem(v.tangent);
	}

	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		uint32_t* p = &mesh.indices.at(i);
		uint32_t temp = p[1];
		p[1] = p[2];
		p[2] = temp;
	}

	for (Model::Bone& bone : mesh.bones)
	{
		ConvertMatrixAxisSystem(bone.offsetTransform);
	}
}

void GLTFImporter::ConvertAnimationAxisSystem(Model::Animation& animation)
{
	for (Model::NodeAnim& nodeAnim : animation.nodeAnims)
	{
		for (Model::VectorKeyframe& keyframe : nodeAnim.positionKeyframes)
		{
			ConvertPositionAxisSystem(keyframe.value);
		}
		for (Model::QuaternionKeyframe& keyframe : nodeAnim.rotationKeyframes)
		{
			ConvertRotationAxisSystem(keyframe.value);
		}
	}
}

// タンジェント計算
void GLTFImporter::ComputeTangents(std::vector<Model::Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	size_t vertexCount = vertices.size();
	std::unique_ptr<DirectX::XMFLOAT3[]> tan1 = std::make_unique<DirectX::XMFLOAT3[]>(vertexCount);
	std::unique_ptr<DirectX::XMFLOAT3[]> tan2 = std::make_unique<DirectX::XMFLOAT3[]>(vertexCount);
	std::unique_ptr<DirectX::XMFLOAT4[]> tangent = std::make_unique<DirectX::XMFLOAT4[]>(vertexCount);

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		const uint32_t i1 = indices[i + 0];
		const uint32_t i2 = indices[i + 1];
		const uint32_t i3 = indices[i + 2];

		const Model::Vertex& v1 = vertices[i1];
		const Model::Vertex& v2 = vertices[i2];
		const Model::Vertex& v3 = vertices[i3];

		const float x1 = v2.position.x - v1.position.x;
		const float x2 = v3.position.x - v1.position.x;
		const float y1 = v2.position.y - v1.position.y;
		const float y2 = v3.position.y - v1.position.y;
		const float z1 = v2.position.z - v1.position.z;
		const float z2 = v3.position.z - v1.position.z;
		const float s1 = v2.texcoord.x - v1.texcoord.x;
		const float s2 = v3.texcoord.x - v1.texcoord.x;
		const float t1 = v2.texcoord.y - v1.texcoord.y;
		const float t2 = v3.texcoord.y - v1.texcoord.y;
		const float r = 1.0f / (s1 * t2 - s2 * t1);
		const DirectX::XMFLOAT3 sdir = { (t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r };
		const DirectX::XMFLOAT3 tdir = { (s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r };
		tan1[i1] = { tan1[i1].x + sdir.x, tan1[i1].y + sdir.y, tan1[i1].z + sdir.z };
		tan1[i2] = { tan1[i2].x + sdir.x, tan1[i2].y + sdir.y, tan1[i2].z + sdir.z };
		tan1[i3] = { tan1[i3].x + sdir.x, tan1[i3].y + sdir.y, tan1[i3].z + sdir.z };
		tan2[i1] = { tan2[i1].x + sdir.x, tan2[i1].y + sdir.y, tan2[i1].z + sdir.z };
		tan2[i2] = { tan2[i2].x + sdir.x, tan2[i2].y + sdir.y, tan2[i2].z + sdir.z };
		tan2[i3] = { tan2[i3].x + sdir.x, tan2[i3].y + sdir.y, tan2[i3].z + sdir.z };
	}

	for (size_t i = 0; i < vertexCount; ++i)
	{
		Model::Vertex& v = vertices[i];

		DirectX::XMVECTOR N = DirectX::XMLoadFloat3(&v.normal);
		DirectX::XMVECTOR T1 = DirectX::XMLoadFloat3(&tan1[i]);

		// Gram-Schmidt orthogonalize        
		DirectX::XMVECTOR T = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(T1, DirectX::XMVectorScale(N, DirectX::XMVectorGetX(DirectX::XMVector3Dot(N, T1)))));

		// Calculate handedness        
		DirectX::XMVECTOR T2 = DirectX::XMLoadFloat3(&tan2[i]);
		float handedness = (DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(N, T1), T2)) < 0.0f) ? -1.0f : 1.0f;
		DirectX::XMStoreFloat4(&v.tangent, DirectX::XMVectorSetW(T, handedness));
	}
}
