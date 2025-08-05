#include "Misc.h"
#include "Audio/AudioResource.h"

// WAVEタグ作成マクロ
#define MAKE_WAVE_TAG_VALUE(c1, c2, c3, c4)  ( c1 | (c2<<8) | (c3<<16) | (c4<<24) )

// コンストラクタ
AudioResource::AudioResource(const char* filename)
{
	// WAVファイル読み込み
	{
		FILE* fp = nullptr;
		errno_t error = fopen_s(&fp, filename, "rb");
		_ASSERT_EXPR_A(error == 0, "WAV File not found");

		// ファイルのサイズを求める
		fseek(fp, 0, SEEK_END);
		size_t size = static_cast<size_t>(ftell(fp));
		fseek(fp, 0, SEEK_SET);

		size_t readBytes = 0;

		// RIFFヘッダ
		fread(&riff, sizeof(riff), 1, fp);
		readBytes += sizeof(riff);

		// "RIFF" との一致を確認
		_ASSERT_EXPR_A(riff.tag == MAKE_WAVE_TAG_VALUE('R', 'I', 'F', 'F'), "not in RIFF format");

		// "WAVE" との一致を確認
		_ASSERT_EXPR_A(riff.type == MAKE_WAVE_TAG_VALUE('W', 'A', 'V', 'E'), "not in WAVE format");

		while (size > readBytes)
		{
			Chunk chunk;
			fread(&chunk, sizeof(chunk), 1, fp);
			readBytes += sizeof(chunk);

			// 'fmt '
			if (chunk.tag == MAKE_WAVE_TAG_VALUE('f', 'm', 't', ' '))
			{
				fread(&fmt, sizeof(fmt), 1, fp);
				readBytes += sizeof(fmt);

				// 拡張領域があれば読み捨て
				if (chunk.size > sizeof(Fmt))
				{
					UINT16 extSize;
					fread(&extSize, sizeof(extSize), 1, fp);
					readBytes += sizeof(extSize);

					if (readBytes + chunk.size == size)
					{
						break;
					}

					fseek(fp, extSize, SEEK_CUR);
					readBytes += extSize;
				}
			}
			// 'data'
			else if (chunk.tag == MAKE_WAVE_TAG_VALUE('d', 'a', 't', 'a'))
			{
				data.resize(chunk.size);
				fread(data.data(), chunk.size, 1, fp);
				readBytes += chunk.size;

				// 8-bit wav ファイルの場合は unsigned -> signed の変換が必要
				if (fmt.quantumBits == 8)
				{
					for (UINT32 i = 0; i < chunk.size; ++i)
					{
						data[i] -= 128;
					}
				}
			}
			// それ以外
			else
			{
				if (readBytes + chunk.size == size) break;

				// 読み飛ばす
				fseek(fp, chunk.size, SEEK_CUR);
				readBytes += chunk.size;
			}
		}

		fclose(fp);
	}

	// WAV フォーマットをセットアップ
	{
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = fmt.channel;
		wfx.nSamplesPerSec = fmt.sampleRate;
		wfx.wBitsPerSample = fmt.quantumBits;
		wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
		wfx.cbSize = sizeof(WAVEFORMATEX);
	}
}

// デストラクタ
AudioResource::~AudioResource()
{

}
