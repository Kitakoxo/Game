#include "Basic.hlsli"

cbuffer CbMesh : register(b0)
{
	float4		materialColor;
};

Texture2D DiffuseMap		: register(t0);
SamplerState LinearSampler	: register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
	return DiffuseMap.Sample(LinearSampler, pin.texcoord) * materialColor;
}
