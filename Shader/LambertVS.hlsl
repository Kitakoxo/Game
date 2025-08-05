#include "Skinning.hlsli"
#include "Lambert.hlsli"

VS_OUT main(
	float4 position		: POSITION,
	float4 boneWeights	: BONE_WEIGHTS,
	uint4  boneIndices	: BONE_INDICES,
	float2 texcoord		: TEXCOORD,
	float3 normal		: NORMAL)
{
	VS_OUT vout = (VS_OUT)0;

	position = SkinningPosition(position, boneWeights, boneIndices);
	vout.vertex = mul(position, viewProjection);
	vout.texcoord = texcoord;
	vout.normal = SkinningVector(normal, boneWeights, boneIndices);

	return vout;
}
