#pragma once

#include "Camera.h"
#include "RenderState.h"
#include "Light.h"

struct RenderContext
{
	ID3D11DeviceContext*	deviceContext;
	const RenderState*		renderState;
	const Camera*			camera;
	const LightManager*		lightManager = nullptr;
};
