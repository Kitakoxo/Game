#include "Skinning.hlsli"
#include "Basic.hlsli"

VS_OUT main(
	float4 position		: POSITION,
	float4 boneWeights	: BONE_WEIGHTS,
	uint4  boneIndices	: BONE_INDICES,
	float2 texcoord		: TEXCOORD)
{
	VS_OUT vout = (VS_OUT)0;

	position = SkinningPosition(position, boneWeights, boneIndices);
	vout.vertex = mul(position, viewProjection);
	vout.texcoord = texcoord;

	return vout;
}
