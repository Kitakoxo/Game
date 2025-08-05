#pragma once

#include <vector>
#include <Windows.h>

// オーディオリソース
class AudioResource
{
public:
	AudioResource(const char* filename);
	~AudioResource();

	// データ取得
	UINT8* GetAudioData() { return data.data(); }

	// データサイズ取得
	UINT32 GetAudioBytes() const { return static_cast<UINT32>(data.size()); }

	// WAVEフォーマット取得
	const WAVEFORMATEX& GetWaveFormat() const { return wfx; }

private:
	// RIFFヘッダ
	struct Riff
	{
		UINT32				tag;			// RIFF形式の識別子 'RIFF'
		UINT32				size;			// これ以降のファイルサイズ(ファイルサイズ - 8)
		UINT32				type;			// RIFFの種類を表す識別子 'WAVE'
	};

	// チャンク
	struct Chunk
	{
		UINT32				tag;			// チャンク形式の識別子 'fmt ' 'data'
		UINT32				size;			// データサイズ('fmt 'リニアPCMならば16 'data'波形データサイズ)
	};

	// fmt チャンク
	struct Fmt
	{
		UINT16				fmtId;			// フォーマットID(リニアPCMならば1)
		UINT16				channel;		// チャンネル数(モノラル:1 ステレオ:2)
		UINT32				sampleRate;		// サンプリングレート(44.1kHzなら44100)
		UINT32				transRate;		// データ速度(Byte/sec) [ 44.1kHz 16bit ステレオ 44100×2×2 ]
		UINT16				blockSize;		// ブロックサイズ(Byte/sample×チャンネル数)
		UINT16				quantumBits;	// サンプルあたりのビット数(bit/sample)
	};

	Riff					riff;
	Fmt						fmt;
	std::vector<UINT8>		data;
	WAVEFORMATEX			wfx;

};
