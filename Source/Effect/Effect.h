#pragma once

#include <DirectXMath.h>
#include <Effekseer.h>

//�G�t�F�N�g
class Effect
{
public:
    Effect(const char* filename);
    ~Effect() { effekseerEffect.Reset(); };      //���\�[�X�J��

    //�Đ�
    Effekseer::Handle Play(const DirectX::XMFLOAT3& position, float scale = 1.0f, const bool loop = false);

    Effekseer::Handle Play(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& angle, float scale = 1.0f);

    //��~
    void stop(Effekseer::Handle handle);

    //���W�ݒ�
    void SetPosition(Effekseer::Handle handle, const DirectX::XMFLOAT3& position);

    //�X�P�[���ݒ�
    void SetScale(Effekseer::Handle handle, const DirectX::XMFLOAT3& scale);

    //�p�x�ݒ�
    void SetRotation(Effekseer::Handle handle, const DirectX::XMFLOAT3& angle);

    // �Đ������ǂ����m�F
    bool IsPlaying(Effekseer::Handle handle);
private:
    Effekseer::EffectRef effekseerEffect;
};
