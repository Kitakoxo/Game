#include "Misc.h"
#include "Audio/Audio.h"

Audio* Audio::instance = nullptr;

// コンストラクタ
Audio::Audio()
{
	// インスタンス設定
	_ASSERT_EXPR(instance == nullptr, "already instantiated");
	instance = this;

	HRESULT hr;

	// COMの初期化
	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	UINT32 createFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	//createFlags |= XAUDIO2_DEBUG_ENGINE;
#endif

	// XAudio初期化
	hr = XAudio2Create(&xaudio, createFlags);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// マスタリングボイス生成
	hr = xaudio->CreateMasteringVoice(&masteringVoice);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
}

// デストラクタ
Audio::~Audio()
{
	// マスタリングボイス破棄
	if (masteringVoice != nullptr)
	{
		masteringVoice->DestroyVoice();
		masteringVoice = nullptr;
	}

	// XAudio終了化
	if (xaudio != nullptr)
	{
		xaudio->Release();
		xaudio = nullptr;
	}

	// COM終了化
	CoUninitialize();
}

// オーディオソース読み込み
std::unique_ptr<AudioSource> Audio::LoadAudioSource(const char* filename)
{
	std::shared_ptr<AudioResource> resource = std::make_shared<AudioResource>(filename);
	return std::make_unique<AudioSource>(xaudio, resource);
}
