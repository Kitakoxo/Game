#pragma once

#include <DirectXMath.h>
#include <Effekseer.h>

//エフェクト
class Effect
{
public:
    Effect(const char* filename);
    ~Effect() { effekseerEffect.Reset(); };      //リソース開放

    //再生
    Effekseer::Handle Play(const DirectX::XMFLOAT3& position, float scale = 1.0f, const bool loop = false);

    Effekseer::Handle Play(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& angle, float scale = 1.0f);

    //停止
    void stop(Effekseer::Handle handle);

    //座標設定
    void SetPosition(Effekseer::Handle handle, const DirectX::XMFLOAT3& position);

    //スケール設定
    void SetScale(Effekseer::Handle handle, const DirectX::XMFLOAT3& scale);

    //角度設定
    void SetRotation(Effekseer::Handle handle, const DirectX::XMFLOAT3& angle);

    // 再生中かどうか確認
    bool IsPlaying(Effekseer::Handle handle);
private:
    Effekseer::EffectRef effekseerEffect;
};
