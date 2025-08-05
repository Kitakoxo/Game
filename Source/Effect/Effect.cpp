#include "Graphics.h"
#include "Effect.h"
#include "EffectManager.h"
#include <stdexcept>

//コンストラクタ
Effect::Effect(const char* filename)
{
    //エフェクトを読み込みする前にロックする
    //マルチスレッドでEffectを作成するとDeviceContextを同時アクセスして
    //フリーズする可能性があるので排他制御する
    std::lock_guard<std::mutex> lock(Graphics::Instance().GetMutex());

    //Effekseerのリソースを読み込む
    //EffekseerはUTF-16のファイルパス以外は対応していないため文字コード変換が必要
    //char16_t utf16Filename[256];
    //Effekseer::ConvertUtf8ToUtf16(utf16Filename, 256, filename);
    char16_t utf16Filename[256] = {};
    if (!Effekseer::ConvertUtf8ToUtf16(utf16Filename, sizeof(utf16Filename) / sizeof(char16_t), filename))
    {
        throw std::runtime_error("Failed to convert filename to UTF-16.");
    }

    //Effekseer::Managerを取得
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    //Effekseerエフェクトを読み込み
    effekseerEffect = Effekseer::Effect::Create(effekseerManager, (EFK_CHAR*)utf16Filename);
}

//再生
Effekseer::Handle Effect::Play(const DirectX::XMFLOAT3& position, float scale, bool loop)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    Effekseer::Handle handle = effekseerManager->Play(effekseerEffect, position.x, position.y, position.z);
    effekseerManager->SetScale(handle, scale, scale, scale);

    return handle;
}

//角度もつけられる再生
Effekseer::Handle Effect::Play(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& angle, float scale)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    Effekseer::Handle handle = effekseerManager->Play(effekseerEffect, position.x, position.y, position.z);
    effekseerManager->SetScale(handle, scale, scale, scale);
    effekseerManager->SetRotation(handle, angle.x, angle.y, angle.z);
    return handle;
}

//停止
void Effect::stop(Effekseer::Handle handle)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    effekseerManager->StopEffect(handle);
}

//座標設定
void Effect::SetPosition(Effekseer::Handle handle, const DirectX::XMFLOAT3& position)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    effekseerManager->SetLocation(handle, position.x, position.y, position.z);
}

//スケール設定
void Effect::SetScale(Effekseer::Handle handle, const DirectX::XMFLOAT3& scale)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    effekseerManager->SetScale(handle, scale.x, scale.y, scale.z);
}

//角度設定
void Effect::SetRotation(Effekseer::Handle handle, const DirectX::XMFLOAT3& angle)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    effekseerManager->SetRotation(handle, angle.x, angle.y, angle.z);
}

//エフェクト再生中かどうかを取得する
bool Effect::IsPlaying(Effekseer::Handle handle)
{
    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

    // Effekseer の Exists 関数を使用してハンドルの状態を確認
    return effekseerManager->Exists(handle);
}
