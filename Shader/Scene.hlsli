cbuffer CbScene : register(b7)
{
	row_major float4x4	viewProjection;
	float4				lightDirection;
	float4				lightColor;
	float4				cameraPosition;
};
