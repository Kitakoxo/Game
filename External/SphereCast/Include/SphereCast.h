#pragma once

#include <DirectXMath.h>

// スフィアキャスト結果
struct SphereCastResult
{
	DirectX::XMVECTOR	position = { 0, 0, 0 };	// レイとポリゴンの交点
	DirectX::XMVECTOR	normal = { 0, 0, 0 };	// 衝突したポリゴンの法線ベクトル
	float				distance = 0.0f; 		// レイの始点から交点までの距離
	DirectX::XMVECTOR	verts[3];
	int					materialIndex = -1; 	// 衝突したポリゴンのマテリアル番号
};

// 外部の点に対する三角形内部の最近点を算出する
bool GetClosestPos_PointTriangle(
	const  DirectX::XMVECTOR& Point,
	const  DirectX::XMVECTOR TrianglePos[3],
	DirectX::XMVECTOR& nearPos,
	DirectX::XMVECTOR* nearTriPos1 = {},		// 外部の点に対して一番近い頂点
	DirectX::XMVECTOR* nearTriPos2 = {});		// 外部の点に対してnearTriPos1と合わせて一番近い辺を構成する頂点

// レイVs三角形
bool IntersectRayVsTriangle(
	const DirectX::XMVECTOR& rayStart,
	const DirectX::XMVECTOR& rayDirection,		// 要正規化
	const float rayDist,
	const DirectX::XMVECTOR trianglePos[3],
	SphereCastResult* result = {});

// 球Vs三角形
bool IntersectSphereVsTriangle(
	const DirectX::XMVECTOR& spherePos,
	const float radius,
	const DirectX::XMVECTOR trianglePos[3],
	SphereCastResult* result = {});

// レイVs球
bool IntersectRayVsSphere(
	const DirectX::XMVECTOR& start,
	const DirectX::XMVECTOR& end,
	const DirectX::XMVECTOR& spherePos,
	const float radius,
	SphereCastResult* result = {});

// レイVs円柱
bool IntersectRayVsCylinder(
	const DirectX::XMVECTOR& start,
	const DirectX::XMVECTOR& end,
	const DirectX::XMVECTOR& startCylinder,
	const DirectX::XMVECTOR& endCylinder,
	float radius,
	SphereCastResult* result = {});

// スフィアキャストVs三角形 簡易版
bool IntersectSphereCastVsTriangleEST(
	const DirectX::XMVECTOR& start,			// スフィアキャストのスタートの位置の球の中心
	const DirectX::XMVECTOR& end,  			// スフィアキャストのエンドの位置の球の中心
	const float radius,
	const DirectX::XMVECTOR trianglePos[3],
	SphereCastResult* result = {},
	const float angle = -1.0f);				// 三角形とスフィアキャストの間の角度が指定以下の場合、交差判定を行わないようにする値。プラスの値をラジアンで指定。

// スフィアキャストVs三角形 完全版
bool IntersectSphereCastVsTriangle(
	const DirectX::XMVECTOR& start,			// スフィアキャストのスタートの位置の球の中心
	const DirectX::XMVECTOR& end,			// スフィアキャストのエンドの位置の球の中心
	const float radius,
	const DirectX::XMVECTOR trianglePos[3],
	SphereCastResult* result = {},
	const float angle = -1.0f);				// 三角形とスフィアキャストの間の角度が指定以下の場合、交差判定を行わないようにする値。プラスの値をラジアンで指定。

// スフィアキャストVs球
bool IntersectSphereCastVsSphere(
	const DirectX::XMVECTOR& start,			// スフィアキャストのスタートの位置の球の中心
	const DirectX::XMVECTOR& end,  			// スフィアキャストのエンドの位置の球の中心
	const float sphereCastRadius,			// スフィアキャストの半径
	const DirectX::XMVECTOR& spherePos,
	const float sphereRadius,				// 球の半径
	SphereCastResult* result = {});

// スフィアキャストVsカプセル
bool IntersectSphereCastVsCapsule(
	const DirectX::XMVECTOR& start,			// スフィアキャストのスタートの位置の球の中心
	const DirectX::XMVECTOR& end,  			// スフィアキャストのエンドの位置の球の中心
	const float sphereCastRadius,			// スフィアキャストの半径
	const DirectX::XMVECTOR& startCapsule,
	const DirectX::XMVECTOR& endCapsule,
	const float capsuleRadius,				// カプセルの半径
	SphereCastResult* result = {});

// スフィアキャストVsAABB
bool IntersectSphereCastVsAABB(
	const DirectX::XMVECTOR& start,			// スフィアキャストのスタートの位置の球の中心
	const DirectX::XMVECTOR& end,  			// スフィアキャストのエンドの位置の球の中心
	const float sphereCastRadius,
	const DirectX::XMVECTOR& aabbPos,
	const DirectX::XMVECTOR& aabbRadii,
	SphereCastResult* result = {});
