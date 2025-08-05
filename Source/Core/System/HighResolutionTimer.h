#pragma once

#include <windows.h>

// ��������������������������������������������������������������������������
// �����x�^�C�}�[�iQueryPerformanceCounter �g�p�j
// �o�ߎ��Ԃ̎擾�E�ꎞ��~�E�ĊJ�ȂǂɑΉ�
// ��������������������������������������������������������������������������
class HighResolutionTimer
{
public:
	// �R���X�g���N�^�F�������ƃ^�C�}�[�̃X�^�[�g
	HighResolutionTimer() : delta_time(-1.0), paused_time(0), stopped(false)
	{
		// �b������̃J�E���g���i��CPU�N���b�N�j�擾
		LONGLONG counts_per_sec;
		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&counts_per_sec));
		seconds_per_count = 1.0 / static_cast<double>(counts_per_sec);

		// ���ݎ�����������
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&this_time));
		base_time = this_time;
		last_time = this_time;
	}

	// �o�ߎ��Ԃ̎擾�i�b�P�ʁj ����~���Ԃ͏��O
	float TimeStamp() const
	{
		if (stopped)
		{
			// ��~���F��~���������܂ł̍�������|�[�Y��������
			return static_cast<float>(((stop_time - paused_time) - base_time) * seconds_per_count);
		}
		else
		{
			// ���쒆�F���ݎ�������J�n�����������A�|�[�Y���Ԃ����O
			return static_cast<float>(((this_time - paused_time) - base_time) * seconds_per_count);
		}
	}

	// �O�t���[���Ƃ̎��ԍ��iTick�ōX�V���ꂽ�l�j
	float TimeInterval() const
	{
		return static_cast<float>(delta_time);
	}

	// �^�C�}�[�̃��Z�b�g�i�Q�[���J�n���Ȃǁj
	void Reset()
	{
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&this_time));
		base_time = this_time;
		last_time = this_time;

		stop_time = 0;
		stopped = false;
	}

	// �^�C�}�[�̍ĊJ�i�ꎞ��~����̍ĊJ�j
	void Start()
	{
		LONGLONG start_time;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&start_time));

		if (stopped)
		{
			// ��~���Ă������Ԃ��L�^���A�ĊJ
			paused_time += (start_time - stop_time);
			last_time = start_time;
			stop_time = 0;
			stopped = false;
		}
	}

	// �^�C�}�[�̈ꎞ��~
	void Stop()
	{
		if (!stopped)
		{
			QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&stop_time));
			stopped = true;
		}
	}

	// ���t���[���Ăяo�����Ƃ� delta_time ���X�V
	void Tick()
	{
		if (stopped)
		{
			delta_time = 0.0;
			return;
		}

		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&this_time));

		// �O�t���[���Ƃ̎��ԍ��i�b�j���v�Z
		delta_time = (this_time - last_time) * seconds_per_count;

		// ���t���[���p�Ɏ����X�V
		last_time = this_time;

		// �H�Ƀ}�C�i�X�ɂȂ�ꍇ������̂ŕی�
		if (delta_time < 0.0)
		{
			delta_time = 0.0;
		}
	}

private:
	// 1�J�E���g������̕b���i= 1 / �J�E���g/�b�j
	double seconds_per_count;

	// �O�t���[���Ƃ̎��ԍ��i�b�j
	double delta_time;

	// �e�펞���i���ׂ�QueryPerformanceCounter�̒l�j
	LONGLONG base_time;    // �^�C�}�[�J�n����
	LONGLONG paused_time;  // �݌v��~����
	LONGLONG stop_time;    // �ꎞ��~��������
	LONGLONG last_time;    // �O�t���[������
	LONGLONG this_time;    // ���ݎ���

	// ��~��Ԃ��ǂ���
	bool stopped;
};
