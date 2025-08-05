#pragma once

#include <windows.h>

// ─────────────────────────────────────
// 高精度タイマー（QueryPerformanceCounter 使用）
// 経過時間の取得・一時停止・再開などに対応
// ─────────────────────────────────────
class HighResolutionTimer
{
public:
	// コンストラクタ：初期化とタイマーのスタート
	HighResolutionTimer() : delta_time(-1.0), paused_time(0), stopped(false)
	{
		// 秒あたりのカウント数（≒CPUクロック）取得
		LONGLONG counts_per_sec;
		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&counts_per_sec));
		seconds_per_count = 1.0 / static_cast<double>(counts_per_sec);

		// 現在時刻を初期化
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&this_time));
		base_time = this_time;
		last_time = this_time;
	}

	// 経過時間の取得（秒単位） ※停止時間は除外
	float TimeStamp() const
	{
		if (stopped)
		{
			// 停止中：停止した時刻までの差分からポーズ分を引く
			return static_cast<float>(((stop_time - paused_time) - base_time) * seconds_per_count);
		}
		else
		{
			// 動作中：現在時刻から開始時刻を引き、ポーズ時間を除外
			return static_cast<float>(((this_time - paused_time) - base_time) * seconds_per_count);
		}
	}

	// 前フレームとの時間差（Tickで更新された値）
	float TimeInterval() const
	{
		return static_cast<float>(delta_time);
	}

	// タイマーのリセット（ゲーム開始時など）
	void Reset()
	{
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&this_time));
		base_time = this_time;
		last_time = this_time;

		stop_time = 0;
		stopped = false;
	}

	// タイマーの再開（一時停止からの再開）
	void Start()
	{
		LONGLONG start_time;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&start_time));

		if (stopped)
		{
			// 停止していた時間を記録し、再開
			paused_time += (start_time - stop_time);
			last_time = start_time;
			stop_time = 0;
			stopped = false;
		}
	}

	// タイマーの一時停止
	void Stop()
	{
		if (!stopped)
		{
			QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&stop_time));
			stopped = true;
		}
	}

	// 毎フレーム呼び出すことで delta_time を更新
	void Tick()
	{
		if (stopped)
		{
			delta_time = 0.0;
			return;
		}

		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&this_time));

		// 前フレームとの時間差（秒）を計算
		delta_time = (this_time - last_time) * seconds_per_count;

		// 次フレーム用に時刻更新
		last_time = this_time;

		// 稀にマイナスになる場合があるので保護
		if (delta_time < 0.0)
		{
			delta_time = 0.0;
		}
	}

private:
	// 1カウントあたりの秒数（= 1 / カウント/秒）
	double seconds_per_count;

	// 前フレームとの時間差（秒）
	double delta_time;

	// 各種時刻（すべてQueryPerformanceCounterの値）
	LONGLONG base_time;    // タイマー開始時刻
	LONGLONG paused_time;  // 累計停止時間
	LONGLONG stop_time;    // 一時停止した時刻
	LONGLONG last_time;    // 前フレーム時刻
	LONGLONG this_time;    // 現在時刻

	// 停止状態かどうか
	bool stopped;
};
