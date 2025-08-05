#include "Graphics.h"
#include "Effect.h"
#include "EffectManager.h"
#include <stdexcept>

//�R���X�g���N�^
Effect::Effect(const char* filename)
{
    //�G�t�F�N�g��ǂݍ��݂���O�Ƀ��b�N����
    //�}���`�X���b�h��Effect���쐬�����DeviceContext�𓯎��A�N�Z�X����
    //�t���[�Y����\��������̂Ŕr�����䂷��
    std::lock_guard<std::mutex> lock(Graphics::Instance().GetMutex());

    //Effekseer�̃��\�[�X��ǂݍ���
    //Effekseer��UTF-16�̃t�@�C���p�X�ȊO�͑Ή����Ă��Ȃ����ߕ����R�[�h�ϊ����K�v
    //char16_t utf16Filename[256];
    //Effekseer::ConvertUtf8ToUtf16(utf16Filename, 256, filename);
    char16_t utf16Filename[256] = {};
    if (!Effekseer::ConvertUtf8ToUtf16(utf16Filename, sizeof(utf16Filename) / sizeof(char16_t), filename))
    {
        throw std::runtime_error("Failed to convert filename to UTF-16.");
    }

    //Effekseer::Manager���擾
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    //Effekseer�G�t�F�N�g��ǂݍ���
    effekseerEffect = Effekseer::Effect::Create(effekseerManager, (EFK_CHAR*)utf16Filename);
}

//�Đ�
Effekseer::Handle Effect::Play(const DirectX::XMFLOAT3& position, float scale, bool loop)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    Effekseer::Handle handle = effekseerManager->Play(effekseerEffect, position.x, position.y, position.z);
    effekseerManager->SetScale(handle, scale, scale, scale);

    return handle;
}

//�p�x��������Đ�
Effekseer::Handle Effect::Play(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& angle, float scale)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    Effekseer::Handle handle = effekseerManager->Play(effekseerEffect, position.x, position.y, position.z);
    effekseerManager->SetScale(handle, scale, scale, scale);
    effekseerManager->SetRotation(handle, angle.x, angle.y, angle.z);
    return handle;
}

//��~
void Effect::stop(Effekseer::Handle handle)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    effekseerManager->StopEffect(handle);
}

//���W�ݒ�
void Effect::SetPosition(Effekseer::Handle handle, const DirectX::XMFLOAT3& position)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    effekseerManager->SetLocation(handle, position.x, position.y, position.z);
}

//�X�P�[���ݒ�
void Effect::SetScale(Effekseer::Handle handle, const DirectX::XMFLOAT3& scale)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    effekseerManager->SetScale(handle, scale.x, scale.y, scale.z);
}

//�p�x�ݒ�
void Effect::SetRotation(Effekseer::Handle handle, const DirectX::XMFLOAT3& angle)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    effekseerManager->SetRotation(handle, angle.x, angle.y, angle.z);
}

//�G�t�F�N�g�Đ������ǂ������擾����
bool Effect::IsPlaying(Effekseer::Handle handle)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    // Effekseer �� Exists �֐����g�p���ăn���h���̏�Ԃ��m�F
    return effekseerManager->Exists(handle);
}
