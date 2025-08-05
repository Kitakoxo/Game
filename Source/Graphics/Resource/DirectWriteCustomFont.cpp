#pragma once

#include "DirectWriteCustomFont.h"

// �t�H���g�R���N�V�������[�_�[
WRL::ComPtr <CustomFontCollectionLoader> pFontCollectionLoader = nullptr;

std::vector<DirectWriteCustomFont*> DirectWriteCustomFont::instances;
DirectWriteCustomFont::DirectWriteCustomFont() {
	instances.push_back(this);
}

DirectWriteCustomFont::DirectWriteCustomFont(FontData* set) : Setting(*set) {
	instances.push_back(this);
}

//=============================================================================
//		�J�X�^���t�@�C�����[�_�[
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
		// �t�H���g�t�@�C����ǂݍ���
		return factory_->CreateFontFileReference(fontFilePaths_[currentFileIndex_].c_str(), nullptr, fontFile);
	}

private:
	ULONG refCount_;

	// DirectWrite�t�@�N�g��
	IDWriteFactory* factory_;

	// �t�H���g�t�@�C���̃p�X
	std::vector<std::wstring> fontFilePaths_;

	// ���݂̃t�@�C���C���f�b�N�X
	int currentFileIndex_;
};

//=============================================================================
//		�J�X�^���t�H���g�R���N�V�������[�_�[
//=============================================================================
class CustomFontCollectionLoader : public IDWriteFontCollectionLoader
{
public:
	// �R���X�g���N�^
	CustomFontCollectionLoader() : refCount_(0) {}

	// IUnknown ���\�b�h
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

	// IDWriteFontCollectionLoader ���\�b�h
	IFACEMETHODIMP CreateEnumeratorFromKey
	(
		IDWriteFactory* factory,
		void const* collectionKey,
		UINT32 collectionKeySize,
		OUT IDWriteFontFileEnumerator** fontFileEnumerator) override
	{
		// �ǂݍ��ރt�H���g�t�@�C���̃p�X��n��
		std::vector<std::wstring> fontFilePaths(std::begin(FontList::FontPath), std::end(FontList::FontPath));

		// �J�X�^���t�H���g�t�@�C���񋓎q�̍쐬
		*fontFileEnumerator = new (std::nothrow) CustomFontFileEnumerator(factory, fontFilePaths);

		// �������s���̏ꍇ�̓G���[��Ԃ�
		if (*fontFileEnumerator == nullptr) { return E_OUTOFMEMORY; }

		return S_OK;
	}

private:
	ULONG refCount_;
};

//�f�X�g���N�^
DirectWriteCustomFont::~DirectWriteCustomFont()
{
	instances.erase(std::remove(instances.begin(), instances.end(), this), instances.end());

	//----------------------------------------------------
	// �@ DirectWrite �n �� FontFile �ɊԐڎQ�Ƃ�����
	//----------------------------------------------------
	pTextLayout.Reset();              // �� �ŏ��Ƀ��C�A�E�g
	pTextFormat.Reset();              //    ���Ƀt�H�[�}�b�g
	fontCollection.Reset();           // �������� IDWriteFontFile �̎Q�Ƃ� 0 �ɂȂ�

	//----------------------------------------------------
	// �A FontFile ��ێ����Ă��� vector �����
	//----------------------------------------------------
	pFontFileList.clear();            // ComPtr<IDWriteFontFile> �� Release

	//----------------------------------------------------
	// �B ���[�_�[�� Factory �������
	//----------------------------------------------------
	if (pDWriteFactory && pFontCollectionLoader)
		pDWriteFactory->UnregisterFontCollectionLoader(pFontCollectionLoader.Get());
	pFontCollectionLoader.Reset();

	//----------------------------------------------------
	// �C D2D / DX ���\�[�X
	//----------------------------------------------------
	pBrush.Reset();
	pShadowBrush.Reset();
	if (pRenderTarget) pRenderTarget->Flush();  // �`��L���[��f���o��
	pRenderTarget.Reset();
	pBackBuffer.Reset();

	//----------------------------------------------------
	// �D Factory ��
	//----------------------------------------------------
	pDWriteFactory.Reset();
	pD2DFactory.Reset();
}

// ����������
HRESULT DirectWriteCustomFont::Init(IDXGISwapChain* swapChain)
{
	HRESULT result = S_OK;

	// Direct2D�t�@�N�g�����̏�����
	result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pD2DFactory.GetAddressOf());
	if (FAILED(result)) { return result; }

	// �o�b�N�o�b�t�@�̎擾
	result = swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (FAILED(result)) { return result; }

	// dpi�̐ݒ�
	//FLOAT dpiX;
	//FLOAT dpiY;
	//pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);

		// SwapChain �`���E�B���h�E�� HWND �𓾂�
	DXGI_SWAP_CHAIN_DESC scDesc = {};
	result = swapChain->GetDesc(&scDesc);
	if (FAILED(result)) return result;
	HWND hwnd = scDesc.OutputWindow;

	// Windows10 �ȍ~: �E�B���h�E�P�ʂ� DPI ���擾
	// (�A�v�����Ń}�j�t�F�X�g�� per-monitor DPI awareness ��錾���Ă���O��)
	//UINT dpi = ::GetDpiForWindow(hwnd);
	//FLOAT dpiX = static_cast<FLOAT>(dpi);
	//FLOAT dpiY = dpiX;

	const FLOAT dpiX = 96.0f;
	const FLOAT dpiY = dpiX;

	// �����_�[�^�[�Q�b�g�̍쐬
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);

	// �T�[�t�F�X�ɕ`�悷�郌���_�[�^�[�Q�b�g���쐬
	result = pD2DFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer.Get(), &props, pRenderTarget.GetAddressOf());
	if (FAILED(result)) { return result; }

	// �A���`�G�C���A�V���O���[�h�̐ݒ�
	pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

	// IDWriteFactory�̍쐬
	result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(pDWriteFactory.GetAddressOf()));
	if (FAILED(result)) { return result; }

	// �J�X�^���t�H���g�R���N�V�������[�_�[
	pFontCollectionLoader = new CustomFontCollectionLoader();

	// �J�X�^���t�H���g�R���N�V�������[�_�[�̍쐬
	result = pDWriteFactory->RegisterFontCollectionLoader(pFontCollectionLoader.Get());
	if (FAILED(result)) { return result; }

	// �t�H���g�t�@�C���̓ǂݍ���
	result = FontLoader();
	if (FAILED(result)) { return result; }

	// �t�H���g��ݒ�
	result = SetFont(Setting);
	if (FAILED(result)) { return result; }

	return result;
}

// �w�肳�ꂽ�p�X�̃t�H���g��ǂݍ���
HRESULT DirectWriteCustomFont::FontLoader()
{
	pFontFileList.clear();                 // ���񃊃Z�b�g

	// �@ �t�H���g�t�@�C���� ComPtr<IDWriteFontFile> �ɂ��� push
	for (const wchar_t* path : FontList::FontPath)
	{
		WRL::ComPtr<IDWriteFontFile> f;
		HRESULT hr = pDWriteFactory->CreateFontFileReference(
			path, nullptr, f.GetAddressOf());
		if (FAILED(hr)) return hr;
		pFontFileList.push_back(f);
	}

	// �A ���̃��X�g�ŃJ�X�^���R���N�V��������
	HRESULT hr = pDWriteFactory->CreateCustomFontCollection(
		pFontCollectionLoader.Get(),
		pFontFileList.data(),
		static_cast<UINT32>(pFontFileList.size()),
		&fontCollection);
	if (FAILED(hr)) return hr;

	// �B ���O���L���b�V��
	return GetFontFamilyName(fontCollection.Get());
}

// �t�H���g�����擾����
std::wstring DirectWriteCustomFont::GetFontName(int num)
{
	// �t�H���g���̃��X�g���󂾂����ꍇ
	if (fontNamesList.empty())
	{
		return nullptr;
	}

	// ���X�g�̃T�C�Y�𒴂��Ă����ꍇ
	if (num >= static_cast<int>(fontNamesList.size()))
	{
		return fontNamesList[0];
	}

	return fontNamesList[num];
}

// �ǂݍ��񂾃t�H���g���̐����擾����
int DirectWriteCustomFont::GetFontNameNum()
{
	return fontNamesList.size();
}

// �t�H���g�ݒ�
// ��1�����F�t�H���g�f�[�^�\����
HRESULT DirectWriteCustomFont::SetFont(FontData set)
{
	HRESULT result = S_OK;

	// �ݒ���R�s�[
	Setting = set;

	std::wstring fileName = GetFontFileNameWithoutExtension(Setting.font);
	result = pDWriteFactory->CreateTextFormat(
		fileName.c_str(),          // �� .c_str() �œn��
		fontCollection.Get(),
		Setting.fontWeight,
		Setting.fontStyle,
		Setting.fontStretch,
		Setting.fontSize,
		Setting.localeName,
		pTextFormat.GetAddressOf());
	if (FAILED(result)) { return result; }

	//�֐�SetTextAlignment()
	//��1�����F�e�L�X�g�̔z�u�iDWRITE_TEXT_ALIGNMENT_LEADING�F�O, DWRITE_TEXT_ALIGNMENT_TRAILING�F��, DWRITE_TEXT_ALIGNMENT_CENTER�F����,
	//                         DWRITE_TEXT_ALIGNMENT_JUSTIFIED�F�s�����ς��j
	result = pTextFormat->SetTextAlignment(Setting.textAlignment);
	if (FAILED(result)) { return result; }

	//�֐�CreateSolidColorBrush()
	//��1�����F�t�H���g�F�iD2D1::ColorF(D2D1::ColorF::Black)�F��, D2D1::ColorF(D2D1::ColorF(0.0f, 0.2f, 0.9f, 1.0f))�FRGBA�w��j
	result = pRenderTarget->CreateSolidColorBrush(Setting.Color, pBrush.GetAddressOf());
	if (FAILED(result)) { return result; }

	// �e�p�̃u���V���쐬
	result = pRenderTarget->CreateSolidColorBrush(Setting.shadowColor, pShadowBrush.GetAddressOf());
	if (FAILED(result)) { return result; }

	return result;
}

//=================================================================================================================================
// �t�H���g�ݒ�
// ��1�����F�t�H���g���iL"���C���I", L"Arial", L"Meiryo UI"���j
// ��2�����F�t�H���g�̑����iDWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_WEIGHT_BOLD���j
// ��3�����F�t�H���g�X�^�C���iDWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STYLE_OBLIQUE, DWRITE_FONT_STYLE_ITALIC�j
// ��4�����F�t�H���g�̕��iDWRITE_FONT_STRETCH_NORMAL,DWRITE_FONT_STRETCH_EXTRA_EXPANDED���j
// ��5�����F�t�H���g�T�C�Y�i20, 30���j
// ��6�����F���P�[�����iL"ja-jp"���j
// ��7�����F�e�L�X�g�̔z�u�iDWRITE_TEXT_ALIGNMENT_LEADING�F�O, ���j
// ��8�����F�t�H���g�̐F�iD2D1::ColorF(D2D1::ColorF::Black)�F��, D2D1::ColorF(D2D1::ColorF(0.0f, 0.2f, 0.9f, 1.0f))�FRGBA�w�蓙�j
// ��9�����F�e�̐F�iD2D1::ColorF(D2D1::ColorF::Black)�F��, D2D1::ColorF(D2D1::ColorF(0.0f, 0.2f, 0.9f, 1.0f))�FRGBA�w�蓙�j
// ��10�����F�e�̃I�t�Z�b�g�iD2D1::Point2F(2.0f, 2.0f)�F�E����2�s�N�Z�����炷�j
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
// �����`��
// string�F������
// pos�F�`��|�W�V����
// options�F�e�L�X�g�̐��`
//====================================
HRESULT DirectWriteCustomFont::DrawString(std::string str, D2D1_POINT_2F pos, D2D1_DRAW_TEXT_OPTIONS options, bool shadow)
{
	HRESULT result = S_OK;

	// ������̕ϊ�
	std::wstring wstr = StringToWString(str.c_str());

	// �^�[�Q�b�g�T�C�Y�̎擾
	D2D1_SIZE_F TargetSize = pRenderTarget->GetSize();

	// �e�L�X�g���C�A�E�g���쐬
	result = pDWriteFactory->CreateTextLayout(wstr.c_str(), wstr.size(), pTextFormat.Get(), TargetSize.width, TargetSize.height, pTextLayout.GetAddressOf());
	if (FAILED(result)) { return result; }

	// �`��ʒu�̊m��
	D2D1_POINT_2F pounts;
	pounts.x = pos.x;
	pounts.y = pos.y;

	// �`��̊J�n
	pRenderTarget->BeginDraw();

	// �����ꍇ
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

	// �e��`�悷��ꍇ
	if (shadow)
	{
		// �e�̕`��
		pRenderTarget->DrawTextLayout(D2D1::Point2F(pounts.x - Setting.shadowOffset.x, pounts.y - Setting.shadowOffset.y),
			pTextLayout.Get(),
			pShadowBrush.Get(),
			options);
	}

	// �`�揈��
	pRenderTarget->DrawTextLayout(pounts, pTextLayout.Get(), pBrush.Get(), options);

	// �`��̏I��
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
// �����`��
// string�F������
// rect�F�̈�w��
// options�F�e�L�X�g�̐��`
	//====================================
HRESULT DirectWriteCustomFont::DrawString(std::string str, D2D1_RECT_F rect, D2D1_DRAW_TEXT_OPTIONS options, bool shadow)
{
	HRESULT result = S_OK;

	// ������̕ϊ�
	std::wstring wstr = StringToWString(str.c_str());

	// �`��̊J�n
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
		// �e�̕`��
		pRenderTarget->DrawText(wstr.c_str(),
			wstr.size(),
			pTextFormat.Get(),
			D2D1::RectF(rect.left - Setting.shadowOffset.x, rect.top - Setting.shadowOffset.y, rect.right - Setting.shadowOffset.x, rect.bottom - Setting.shadowOffset.y),
			pShadowBrush.Get(), options);
	}

	// �`�揈��
	pRenderTarget->DrawText(wstr.c_str(), wstr.size(), pTextFormat.Get(), rect, pBrush.Get(), options);

	// �`��̏I��
	result = pRenderTarget->EndDraw();
	if (FAILED(result)) { return result; }


	return S_OK;
}

// �t�H���g�����擾
HRESULT DirectWriteCustomFont::GetFontFamilyName(IDWriteFontCollection* customFontCollection, const WCHAR* locale)
{
	HRESULT result = S_OK;

	// �t�H���g�t�@�~���[���ꗗ�����Z�b�g
	std::vector<std::wstring>().swap(fontNamesList);

	// �t�H���g�̐����擾
	UINT32 familyCount = customFontCollection->GetFontFamilyCount();

	for (UINT32 i = 0; i < familyCount; i++)
	{
		// �t�H���g�t�@�~���[�̎擾
		WRL::ComPtr <IDWriteFontFamily> fontFamily = nullptr;
		result = customFontCollection->GetFontFamily(i, fontFamily.GetAddressOf());
		if (FAILED(result)) { return result; }

		// �t�H���g�t�@�~���[���̈ꗗ���擾
		WRL::ComPtr <IDWriteLocalizedStrings> familyNames = nullptr;
		result = fontFamily->GetFamilyNames(familyNames.GetAddressOf());
		if (FAILED(result)) { return result; }

		// �w�肳�ꂽ���P�[���ɑΉ�����C���f�b�N�X������
		UINT32 index = 0;
		BOOL exists = FALSE;
		result = familyNames->FindLocaleName(locale, &index, &exists);
		if (FAILED(result)) { return result; }

		// �w�肳�ꂽ���P�[����������Ȃ������ꍇ�́A�f�t�H���g�̃��P�[�����g�p
		if (!exists)
		{
			result = familyNames->FindLocaleName(L"en-us", &index, &exists);
			if (FAILED(result)) { return result; }
		}

		// �t�H���g�t�@�~���[���̒������擾
		UINT32 length = 0;
		result = familyNames->GetStringLength(index, &length);
		if (FAILED(result)) { return result; }

		// �t�H���g�t�@�~���[���̎擾
		WCHAR* name = new WCHAR[length + 1];
		result = familyNames->GetString(index, name, length + 1);
		if (FAILED(result)) { return result; }

		// �t�H���g�t�@�~���[����ǉ�
		fontNamesList.push_back(name);

		// �t�H���g�t�@�~���[���̔j��
		delete[] name;
	}

	return result;
}

// �S�Ẵt�H���g�����擾
HRESULT DirectWriteCustomFont::GetAllFontFamilyName(IDWriteFontCollection* customFontCollection)
{
	HRESULT result = S_OK;

	// �t�H���g�t�@�~���[���ꗗ�����Z�b�g
	std::vector<std::wstring>().swap(fontNamesList);

	// �t�H���g�t�@�~���[�̐����擾
	UINT32 familyCount = customFontCollection->GetFontFamilyCount();

	for (UINT32 i = 0; i < familyCount; i++)
	{
		// �t�H���g�t�@�~���[�̎擾
		WRL::ComPtr <IDWriteFontFamily> fontFamily = nullptr;
		result = customFontCollection->GetFontFamily(i, fontFamily.GetAddressOf());
		if (FAILED(result)) { return result; }

		// �t�H���g�t�@�~���[���̈ꗗ���擾
		WRL::ComPtr <IDWriteLocalizedStrings> familyNames = nullptr;
		result = fontFamily->GetFamilyNames(familyNames.GetAddressOf());
		if (FAILED(result)) { return result; }

		// �t�H���g�t�@�~���[���̐����擾
		UINT32 nameCount = familyNames->GetCount();

		// �t�H���g�t�@�~���[���̐������J��Ԃ�
		for (UINT32 j = 0; j < nameCount; ++j)
		{
			// �t�H���g�t�@�~���[���̒������擾
			UINT32 length = 0;
			result = familyNames->GetStringLength(j, &length);
			if (FAILED(result)) { return result; }

			// �t�H���g�t�@�~���[���̎擾
			WCHAR* name = new WCHAR[length + 1];
			result = familyNames->GetString(j, name, length + 1);
			if (FAILED(result)) { return result; }

			// �t�H���g�t�@�~���[����ǉ�
			fontNamesList.push_back(name);

			// �t�H���g�t�@�~���[���̔j��
			delete[] name;
		}
	}

	return result;
}

std::wstring DirectWriteCustomFont::GetFontFileNameWithoutExtension(const std::wstring& filePath)
{
	const size_t start = filePath.find_last_of(L"/\\") + 1;
	const size_t end = filePath.find_last_of(L'.');
	return filePath.substr(start, end - start);   // �� new[] ���� std::wstring ��Ԃ�
}

// string��wstring�֕ϊ�����
std::wstring DirectWriteCustomFont::StringToWString(std::string oString)
{
	// UTF-8 �� UTF-16 �ϊ�
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, oString.c_str(), -1, nullptr, 0);
	if (sizeNeeded == 0) return L"";

	std::wstring wstr(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, oString.c_str(), -1, &wstr[0], sizeNeeded);

	// null����������
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