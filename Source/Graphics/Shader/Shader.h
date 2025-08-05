#pragma once

#include "RenderContext.h"
#include "Model.h"

class Shader
{
public:
	Shader() {}
	virtual ~Shader() {}

	// 開始処理
	virtual void Begin(const RenderContext& rc) = 0;

	// 更新処理
	virtual void Update(const RenderContext& rc, const Model::Mesh& mesh) = 0;

	// 終了処理
	virtual void End(const RenderContext& rc) = 0;
};
