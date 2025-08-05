#pragma once

#include "DirectWriteCustomFont.h"

// フォントコレクションローダー
WRL::ComPtr <CustomFontCollectionLoader> pFontCollectionLoader = nullptr;

std::vector<DirectWriteCustomFont*> DirectWriteCustomFont::instances;
DirectWriteCustomFont::DirectWriteCustomFont() {
	instances.push_back(this);
}

DirectWriteCustomFont::DirectWriteCustomFont(FontData* set) : Setting(*set) {
	instances.push_back(this);
}

//=============================================================================
//		カスタムファイルローダー
//=============================================================================
class CustomFontFileEnumerator : public IDWriteFontFileEnumerator
{
public:
	CustomFontFileEnumerator(IDWriteFactory* factory, const std::vector<std::wstring>& fontFilePaths)
		: refCount_(0), factory_(factory), fontFilePaths_(fontFilePaths), currentFileIndex_(-1)
	{
		factory_->AddRef();
	}

	~CustomFontFileEnumerator()
	{
		factory_->Release();
	}

	IFACEMETHODIMP QueryInterface(REFIID iid, void** ppvObject) override
	{
		if (iid == __uuidof(IUnknown) || iid == __uuidof(IDWriteFontCollectionLoader))
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		else
		{
			*ppvObject = nullptr;
			return E_NOINTERFACE;
		}
	}

	IFACEMETHODIMP_(ULONG) AddRef() override
	{
		return InterlockedIncrement(&refCount_);
	}

	IFACEMETHODIMP_(ULONG) Release() override
	{
		ULONG newCount = InterlockedDecrement(&refCount_);
		if (newCount == 0)
			delete this;

		return newCount;
	}

	IFACEMETHODIMP MoveNext(OUT BOOL* hasCurrentFile) override {
		if (++currentFileIndex_ < static_cast<int>(fontFilePaths_.size())) {
			*hasCurrentFile = TRUE;
			return S_OK;
		}
		else {
			*hasCurrentFile = FALSE;
			return S_OK;
		}
	}

	IFACEMETHODIMP GetCurrentFontFile(OUT IDWriteFontFile** fontFile) override
	{
		// フォントファイルを読み込む
		return factory_->CreateFontFileReference(fontFilePaths_[currentFileIndex_].c_str(), nullptr, fontFile);
	}

private:
	ULONG refCount_;

	// DirectWriteファクトリ
	IDWriteFactory* factory_;

	// フォントファイルのパス
	std::vector<std::wstring> fontFilePaths_;

	// 現在のファイルインデックス
	int currentFileIndex_;
};

//=============================================================================
//		カスタムフォントコレクションローダー
//=============================================================================
class CustomFontCollectionLoader : public IDWriteFontCollectionLoader
{
public:
	// コンストラクタ
	CustomFontCollectionLoader() : refCount_(0) {}

	// IUnknown メソッド
	IFACEMETHODIMP QueryInterface(REFIID iid, void** ppvObject) override
	{
		if (iid == __uuidof(IUnknown) || iid == __uuidof(IDWriteFontCollectionLoader))
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		else
		{
			*ppvObject = nullptr;
			return E_NOINTERFACE;
		}
	}

	IFACEMETHODIMP_(ULONG) AddRef() override
	{
		return InterlockedIncrement(&refCount_);
	}

	IFACEMETHODIMP_(ULONG) Release() override
	{
		ULONG newCount = InterlockedDecrement(&refCount_);
		if (newCount == 0)
			delete this;

		return newCount;
	}

	// IDWriteFontCollectionLoader メソッド
	IFACEMETHODIMP CreateEnumeratorFromKey
	(
		IDWriteFactory* factory,
		void const* collectionKey,
		UINT32 collectionKeySize,
		OUT IDWriteFontFileEnumerator** fontFileEnumerator) override
	{
		// 読み込むフォントファイルのパスを渡す
		std::vector<std::wstring> fontFilePaths(std::begin(FontList::FontPath), std::end(FontList::FontPath));

		// カスタムフォントファイル列挙子の作成
		*fontFileEnumerator = new (std::nothrow) CustomFontFileEnumerator(factory, fontFilePaths);

		// メモリ不足の場合はエラーを返す
		if (*fontFileEnumerator == nullptr) { return E_OUTOFMEMORY; }

		return S_OK;
	}

private:
	ULONG refCount_;
};

//デストラクタ
DirectWriteCustomFont::~DirectWriteCustomFont()
{
	instances.erase(std::remove(instances.begin(), instances.end(), this), instances.end());

	//----------------------------------------------------
	// ① DirectWrite 系 → FontFile に間接参照を持つ
	//----------------------------------------------------
	pTextLayout.Reset();              // ← 最初にレイアウト
	pTextFormat.Reset();              //    次にフォーマット
	fontCollection.Reset();           // ★ここで IDWriteFontFile の参照が 0 になる

	//----------------------------------------------------
	// ② FontFile を保持していた vector を空に
	//----------------------------------------------------
	pFontFileList.clear();            // ComPtr<IDWriteFontFile> を Release

	//----------------------------------------------------
	// ③ ローダーを Factory から解除
	//----------------------------------------------------
	if (pDWriteFactory && pFontCollectionLoader)
		pDWriteFactory->UnregisterFontCollectionLoader(pFontCollectionLoader.Get());
	pFontCollectionLoader.Reset();

	//----------------------------------------------------
	// ④ D2D / DX リソース
	//----------------------------------------------------
	pBrush.Reset();
	pShadowBrush.Reset();
	if (pRenderTarget) pRenderTarget->Flush();  // 描画キューを吐き出し
	pRenderTarget.Reset();
	pBackBuffer.Reset();

	//----------------------------------------------------
	// ⑤ Factory 類
	//----------------------------------------------------
	pDWriteFactory.Reset();
	pD2DFactory.Reset();
}

// 初期化処理
HRESULT DirectWriteCustomFont::Init(IDXGISwapChain* swapChain)
{
	HRESULT result = S_OK;

	// Direct2Dファクトリ情報の初期化
	result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pD2DFactory.GetAddressOf());
	if (FAILED(result)) { return result; }

	// バックバッファの取得
	result = swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (FAILED(result)) { return result; }

	// dpiの設定
	//FLOAT dpiX;
	//FLOAT dpiY;
	//pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);

		// SwapChain 描画先ウィンドウの HWND を得る
	DXGI_SWAP_CHAIN_DESC scDesc = {};
	result = swapChain->GetDesc(&scDesc);
	if (FAILED(result)) return result;
	HWND hwnd = scDesc.OutputWindow;

	// Windows10 以降: ウィンドウ単位の DPI を取得
	// (アプリ側でマニフェストに per-monitor DPI awareness を宣言している前提)
	//UINT dpi = ::GetDpiForWindow(hwnd);
	//FLOAT dpiX = static_cast<FLOAT>(dpi);
	//FLOAT dpiY = dpiX;

	const FLOAT dpiX = 96.0f;
	const FLOAT dpiY = dpiX;

	// レンダーターゲットの作成
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);

	// サーフェスに描画するレンダーターゲットを作成
	result = pD2DFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer.Get(), &props, pRenderTarget.GetAddressOf());
	if (FAILED(result)) { return result; }

	// アンチエイリアシングモードの設定
	pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

	// IDWriteFactoryの作成
	result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(pDWriteFactory.GetAddressOf()));
	if (FAILED(result)) { return result; }

	// カスタムフォントコレクションローダー
	pFontCollectionLoader = new CustomFontCollectionLoader();

	// カスタムフォントコレクションローダーの作成
	result = pDWriteFactory->RegisterFontCollectionLoader(pFontCollectionLoader.Get());
	if (FAILED(result)) { return result; }

	// フォントファイルの読み込み
	result = FontLoader();
	if (FAILED(result)) { return result; }

	// フォントを設定
	result = SetFont(Setting);
	if (FAILED(result)) { return result; }

	return result;
}

// 指定されたパスのフォントを読み込む
HRESULT DirectWriteCustomFont::FontLoader()
{
	pFontFileList.clear();                 // 毎回リセット

	// ① フォントファイルを ComPtr<IDWriteFontFile> にして push
	for (const wchar_t* path : FontList::FontPath)
	{
		WRL::ComPtr<IDWriteFontFile> f;
		HRESULT hr = pDWriteFactory->CreateFontFileReference(
			path, nullptr, f.GetAddressOf());
		if (FAILED(hr)) return hr;
		pFontFileList.push_back(f);
	}

	// ② そのリストでカスタムコレクション生成
	HRESULT hr = pDWriteFactory->CreateCustomFontCollection(
		pFontCollectionLoader.Get(),
		pFontFileList.data(),
		static_cast<UINT32>(pFontFileList.size()),
		&fontCollection);
	if (FAILED(hr)) return hr;

	// ③ 名前をキャッシュ
	return GetFontFamilyName(fontCollection.Get());
}

// フォント名を取得する
std::wstring DirectWriteCustomFont::GetFontName(int num)
{
	// フォント名のリストが空だった場合
	if (fontNamesList.empty())
	{
		return nullptr;
	}

	// リストのサイズを超えていた場合
	if (num >= static_cast<int>(fontNamesList.size()))
	{
		return fontNamesList[0];
	}

	return fontNamesList[num];
}

// 読み込んだフォント名の数を取得する
int DirectWriteCustomFont::GetFontNameNum()
{
	return fontNamesList.size();
}

// フォント設定
// 第1引数：フォントデータ構造体
HRESULT DirectWriteCustomFont::SetFont(FontData set)
{
	HRESULT result = S_OK;

	// 設定をコピー
	Setting = set;

	std::wstring fileName = GetFontFileNameWithoutExtension(Setting.font);
	result = pDWriteFactory->CreateTextFormat(
		fileName.c_str(),          // ← .c_str() で渡す
		fontCollection.Get(),
		Setting.fontWeight,
		Setting.fontStyle,
		Setting.fontStretch,
		Setting.fontSize,
		Setting.localeName,
		pTextFormat.GetAddressOf());
	if (FAILED(result)) { return result; }

	//関数SetTextAlignment()
	//第1引数：テキストの配置（DWRITE_TEXT_ALIGNMENT_LEADING：前, DWRITE_TEXT_ALIGNMENT_TRAILING：後, DWRITE_TEXT_ALIGNMENT_CENTER：中央,
	//                         DWRITE_TEXT_ALIGNMENT_JUSTIFIED：行いっぱい）
	result = pTextFormat->SetTextAlignment(Setting.textAlignment);
	if (FAILED(result)) { return result; }

	//関数CreateSolidColorBrush()
	//第1引数：フォント色（D2D1::ColorF(D2D1::ColorF::Black)：黒, D2D1::ColorF(D2D1::ColorF(0.0f, 0.2f, 0.9f, 1.0f))：RGBA指定）
	result = pRenderTarget->CreateSolidColorBrush(Setting.Color, pBrush.GetAddressOf());
	if (FAILED(result)) { return result; }

	// 影用のブラシを作成
	result = pRenderTarget->CreateSolidColorBrush(Setting.shadowColor, pShadowBrush.GetAddressOf());
	if (FAILED(result)) { return result; }

	return result;
}

//=================================================================================================================================
// フォント設定
// 第1引数：フォント名（L"メイリオ", L"Arial", L"Meiryo UI"等）
// 第2引数：フォントの太さ（DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_WEIGHT_BOLD等）
// 第3引数：フォントスタイル（DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STYLE_OBLIQUE, DWRITE_FONT_STYLE_ITALIC）
// 第4引数：フォントの幅（DWRITE_FONT_STRETCH_NORMAL,DWRITE_FONT_STRETCH_EXTRA_EXPANDED等）
// 第5引数：フォントサイズ（20, 30等）
// 第6引数：ロケール名（L"ja-jp"等）
// 第7引数：テキストの配置（DWRITE_TEXT_ALIGNMENT_LEADING：前, 等）
// 第8引数：フォントの色（D2D1::ColorF(D2D1::ColorF::Black)：黒, D2D1::ColorF(D2D1::ColorF(0.0f, 0.2f, 0.9f, 1.0f))：RGBA指定等）
// 第9引数：影の色（D2D1::ColorF(D2D1::ColorF::Black)：黒, D2D1::ColorF(D2D1::ColorF(0.0f, 0.2f, 0.9f, 1.0f))：RGBA指定等）
// 第10引数：影のオフセット（D2D1::Point2F(2.0f, 2.0f)：右下に2ピクセルずらす）
//=================================================================================================================================
HRESULT DirectWriteCustomFont::SetFont(WCHAR const* fontname, DWRITE_FONT_WEIGHT fontWeight, DWRITE_FONT_STYLE fontStyle,
	DWRITE_FONT_STRETCH fontStretch, FLOAT fontSize, WCHAR const* localeName,
	DWRITE_TEXT_ALIGNMENT textAlignment, D2D1_COLOR_F Color, D2D1_COLOR_F shadowColor, D2D1_POINT_2F shadowOffset)
{
	HRESULT result = S_OK;

	//pDWriteFactory->CreateTextFormat(GetFontFileNameWithoutExtension(fontname), fontCollection.Get(), fontWeight, fontStyle, fontStretch, fontSize, localeName, &pTextFormat);
	std::wstring fileName = GetFontFileNameWithoutExtension(fontname);
	result = pDWriteFactory->CreateTextFormat(
		fileName.c_str(),
		fontCollection.Get(),
		fontWeight,
		fontStyle,
		fontStretch,
		fontSize,
		localeName,
		pTextFormat.GetAddressOf());
	if (FAILED(result)) { return result; }

	pTextFormat->SetTextAlignment(textAlignment);
	if (FAILED(result)) { return result; }

	pRenderTarget->CreateSolidColorBrush(Color, pBrush.GetAddressOf());
	if (FAILED(result)) { return result; }

	pRenderTarget->CreateSolidColorBrush(shadowColor, pShadowBrush.GetAddressOf());
	if (FAILED(result)) { return result; }

	return result;
}

//====================================
// 文字描画
// string：文字列
// pos：描画ポジション
// options：テキストの整形
//====================================
HRESULT DirectWriteCustomFont::DrawString(std::string str, D2D1_POINT_2F pos, D2D1_DRAW_TEXT_OPTIONS options, bool shadow)
{
	HRESULT result = S_OK;

	// 文字列の変換
	std::wstring wstr = StringToWString(str.c_str());

	// ターゲットサイズの取得
	D2D1_SIZE_F TargetSize = pRenderTarget->GetSize();

	// テキストレイアウトを作成
	result = pDWriteFactory->CreateTextLayout(wstr.c_str(), wstr.size(), pTextFormat.Get(), TargetSize.width, TargetSize.height, pTextLayout.GetAddressOf());
	if (FAILED(result)) { return result; }

	// 描画位置の確定
	D2D1_POINT_2F pounts;
	pounts.x = pos.x;
	pounts.y = pos.y;

	// 描画の開始
	pRenderTarget->BeginDraw();

	// 縁取る場合
	if (Setting.enableOutline && Setting.outlineThickness > 0.0f) {
		pBrush->SetColor(Setting.outlineColor);
		const float t = Setting.outlineThickness;
		const D2D1_POINT_2F offsets[8] = {
			{-t, -t}, {0, -t}, {t, -t},
			{t, 0}, {t, t}, {0, t},
			{-t, t}, {-t, 0}
		};
		for (const auto& offset : offsets) {
			D2D1_POINT_2F outlinePos = D2D1::Point2F(pos.x + offset.x, pos.y + offset.y);
			pRenderTarget->DrawTextLayout(outlinePos, pTextLayout.Get(), pBrush.Get(), options);
		}

		pBrush->SetColor(Setting.Color);
	}

	// 影を描画する場合
	if (shadow)
	{
		// 影の描画
		pRenderTarget->DrawTextLayout(D2D1::Point2F(pounts.x - Setting.shadowOffset.x, pounts.y - Setting.shadowOffset.y),
			pTextLayout.Get(),
			pShadowBrush.Get(),
			options);
	}

	// 描画処理
	pRenderTarget->DrawTextLayout(pounts, pTextLayout.Get(), pBrush.Get(), options);

	// 描画の終了
	result = pRenderTarget->EndDraw();
	if (FAILED(result)) { return result; }

	return S_OK;
}

D2D1_SIZE_F DirectWriteCustomFont::MeasureString(const std::string& str)
{
	std::wstring w = StringToWString(str);
	D2D1_SIZE_F size = pRenderTarget->GetSize();
	Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
	if (FAILED(pDWriteFactory->CreateTextLayout(w.c_str(), static_cast<UINT32>(w.size()), pTextFormat.Get(), size.width, size.height, layout.GetAddressOf())))
		return { 0,0 };
	DWRITE_TEXT_METRICS metrics{};
	if (FAILED(layout->GetMetrics(&metrics))) return { 0,0 };
	return { metrics.width, metrics.height };
}

//====================================
// 文字描画
// string：文字列
// rect：領域指定
// options：テキストの整形
	//====================================
HRESULT DirectWriteCustomFont::DrawString(std::string str, D2D1_RECT_F rect, D2D1_DRAW_TEXT_OPTIONS options, bool shadow)
{
	HRESULT result = S_OK;

	// 文字列の変換
	std::wstring wstr = StringToWString(str.c_str());

	// 描画の開始
	pRenderTarget->BeginDraw();

	if (Setting.enableOutline && Setting.outlineThickness > 0.0f) {
		pBrush->SetColor(Setting.outlineColor);
		const float t = Setting.outlineThickness;
		const D2D1_POINT_2F offsets[8] = {
			{-t, -t}, {0, -t}, {t, -t},
			{t, 0}, {t, t}, {0, t},
			{-t, t}, {-t, 0}
		};
		for (const auto& offset : offsets) {
			D2D1_RECT_F outlineRect = D2D1::RectF(
				rect.left + offset.x, rect.top + offset.y,
				rect.right + offset.x, rect.bottom + offset.y
			);
			pRenderTarget->DrawText(wstr.c_str(), wstr.size(), pTextFormat.Get(), outlineRect, pBrush.Get(), options);
		}

		pBrush->SetColor(Setting.Color);
	}

	if (shadow)
	{
		// 影の描画
		pRenderTarget->DrawText(wstr.c_str(),
			wstr.size(),
			pTextFormat.Get(),
			D2D1::RectF(rect.left - Setting.shadowOffset.x, rect.top - Setting.shadowOffset.y, rect.right - Setting.shadowOffset.x, rect.bottom - Setting.shadowOffset.y),
			pShadowBrush.Get(), options);
	}

	// 描画処理
	pRenderTarget->DrawText(wstr.c_str(), wstr.size(), pTextFormat.Get(), rect, pBrush.Get(), options);

	// 描画の終了
	result = pRenderTarget->EndDraw();
	if (FAILED(result)) { return result; }


	return S_OK;
}

// フォント名を取得
HRESULT DirectWriteCustomFont::GetFontFamilyName(IDWriteFontCollection* customFontCollection, const WCHAR* locale)
{
	HRESULT result = S_OK;

	// フォントファミリー名一覧をリセット
	std::vector<std::wstring>().swap(fontNamesList);

	// フォントの数を取得
	UINT32 familyCount = customFontCollection->GetFontFamilyCount();

	for (UINT32 i = 0; i < familyCount; i++)
	{
		// フォントファミリーの取得
		WRL::ComPtr <IDWriteFontFamily> fontFamily = nullptr;
		result = customFontCollection->GetFontFamily(i, fontFamily.GetAddressOf());
		if (FAILED(result)) { return result; }

		// フォントファミリー名の一覧を取得
		WRL::ComPtr <IDWriteLocalizedStrings> familyNames = nullptr;
		result = fontFamily->GetFamilyNames(familyNames.GetAddressOf());
		if (FAILED(result)) { return result; }

		// 指定されたロケールに対応するインデックスを検索
		UINT32 index = 0;
		BOOL exists = FALSE;
		result = familyNames->FindLocaleName(locale, &index, &exists);
		if (FAILED(result)) { return result; }

		// 指定されたロケールが見つからなかった場合は、デフォルトのロケールを使用
		if (!exists)
		{
			result = familyNames->FindLocaleName(L"en-us", &index, &exists);
			if (FAILED(result)) { return result; }
		}

		// フォントファミリー名の長さを取得
		UINT32 length = 0;
		result = familyNames->GetStringLength(index, &length);
		if (FAILED(result)) { return result; }

		// フォントファミリー名の取得
		WCHAR* name = new WCHAR[length + 1];
		result = familyNames->GetString(index, name, length + 1);
		if (FAILED(result)) { return result; }

		// フォントファミリー名を追加
		fontNamesList.push_back(name);

		// フォントファミリー名の破棄
		delete[] name;
	}

	return result;
}

// 全てのフォント名を取得
HRESULT DirectWriteCustomFont::GetAllFontFamilyName(IDWriteFontCollection* customFontCollection)
{
	HRESULT result = S_OK;

	// フォントファミリー名一覧をリセット
	std::vector<std::wstring>().swap(fontNamesList);

	// フォントファミリーの数を取得
	UINT32 familyCount = customFontCollection->GetFontFamilyCount();

	for (UINT32 i = 0; i < familyCount; i++)
	{
		// フォントファミリーの取得
		WRL::ComPtr <IDWriteFontFamily> fontFamily = nullptr;
		result = customFontCollection->GetFontFamily(i, fontFamily.GetAddressOf());
		if (FAILED(result)) { return result; }

		// フォントファミリー名の一覧を取得
		WRL::ComPtr <IDWriteLocalizedStrings> familyNames = nullptr;
		result = fontFamily->GetFamilyNames(familyNames.GetAddressOf());
		if (FAILED(result)) { return result; }

		// フォントファミリー名の数を取得
		UINT32 nameCount = familyNames->GetCount();

		// フォントファミリー名の数だけ繰り返す
		for (UINT32 j = 0; j < nameCount; ++j)
		{
			// フォントファミリー名の長さを取得
			UINT32 length = 0;
			result = familyNames->GetStringLength(j, &length);
			if (FAILED(result)) { return result; }

			// フォントファミリー名の取得
			WCHAR* name = new WCHAR[length + 1];
			result = familyNames->GetString(j, name, length + 1);
			if (FAILED(result)) { return result; }

			// フォントファミリー名を追加
			fontNamesList.push_back(name);

			// フォントファミリー名の破棄
			delete[] name;
		}
	}

	return result;
}

std::wstring DirectWriteCustomFont::GetFontFileNameWithoutExtension(const std::wstring& filePath)
{
	const size_t start = filePath.find_last_of(L"/\\") + 1;
	const size_t end = filePath.find_last_of(L'.');
	return filePath.substr(start, end - start);   // ← new[] せず std::wstring を返す
}

// stringをwstringへ変換する
std::wstring DirectWriteCustomFont::StringToWString(std::string oString)
{
	// UTF-8 → UTF-16 変換
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, oString.c_str(), -1, nullptr, 0);
	if (sizeNeeded == 0) return L"";

	std::wstring wstr(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, oString.c_str(), -1, &wstr[0], sizeNeeded);

	// null文字を除去
	wstr.pop_back();
	return wstr;
}

void DirectWriteCustomFont::ReleaseRenderTarget()
{
	if (pRenderTarget) pRenderTarget->Flush();
	pBrush.Reset();
	pShadowBrush.Reset();
	pRenderTarget.Reset();
	pBackBuffer.Reset();
}

HRESULT DirectWriteCustomFont::RecreateRenderTarget(IDXGISwapChain* swapChain)
{
	ReleaseRenderTarget();
	if (!swapChain) return E_INVALIDARG;
	HRESULT result = swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (FAILED(result)) return result;
	DXGI_SWAP_CHAIN_DESC scDesc{};
	result = swapChain->GetDesc(&scDesc);
	if (FAILED(result)) return result;
	//UINT dpi = ::GetDpiForWindow(scDesc.OutputWindow);
	//FLOAT dpiX = static_cast<FLOAT>(dpi);
	//FLOAT dpiY = dpiX;
	const FLOAT dpiX = 96.0f;
	const FLOAT dpiY = dpiX;
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);
	result = pD2DFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer.Get(), &props, pRenderTarget.GetAddressOf());
	if (FAILED(result)) return result;
	pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
	pRenderTarget->CreateSolidColorBrush(Setting.Color, pBrush.GetAddressOf());
	pRenderTarget->CreateSolidColorBrush(Setting.shadowColor, pShadowBrush.GetAddressOf());
	return result;
}

void DirectWriteCustomFont::ReleaseAllRenderTargets()
{
	for (auto* f : instances)
	{
		if (f) f->ReleaseRenderTarget();
	}
}

void DirectWriteCustomFont::RecreateAllRenderTargets(IDXGISwapChain* swapChain)
{
	for (auto* f : instances)
	{
		if (f) f->RecreateRenderTarget(swapChain);
	}
}