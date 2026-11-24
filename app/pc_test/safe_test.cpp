// safe_test.cpp - AutoWah の安全対策のテスト(Pi には載せない)
//
// 確かめること:
//   A) NaN / Inf が混ざっても、出力が壊れず、次のブロックも正常に動く
//   B) 満杯の入力や桁外れの入力でも、出力が ±1.0 を超えない
//   C) Prepare し直すと、前の音の残りが消える
//   D) 長い無音のあと、微小な値が残らない
//
// 注意: Process は渡した配列をその場で書き換える。
//       同じ配列を使い回すと、出力をもう一度入力に戻すことになるので、
//       ブロックごとに入力を作り直すこと。
#include "autowah.hpp"

#include <cmath>
#include <cstdio>
#include <limits>

namespace
{
    // ---- テストの条件(数字に名前を付ける) ----
    const double kPi         = 3.14159265358979;
    const float  kSampleRate = 48000.0f;   // Hz
    const int    kBlockSize  = 128;        // config.hpp の BLOCK_SIZE と同じ
    const float  kToneHz     = 2500.0f;    // 試験音の高さ

    const int   kStageMax    = 5;          // SENS / Q / LEVEL の最大の段階

    // A) 異常な値を混ぜる
    const float kToneAmp        = 0.3f;    // 普通の音の大きさ
    const float kNormalValue    = 0.2f;    // 異常値の間に混ぜる、普通の値
    const int   kBadPatternSize = 4;       // NaN, +Inf, -Inf, 普通の値 の繰り返し

    // B) 満杯の矩形波と、桁外れの入力
    const float kFullScale      = 1.0f;
    const int   kSquareHalfPeriod = 10;    // 矩形波の半周期(サンプル数)
    const float kHugeValue      = 5.0e30f; // 有限だが、桁外れに大きい値
    const float kOverRange      = -3.0f;   // ±1.0 を超える値
    const float kPeakLimit      = 1.0f;    // 出力の上限

    // C) / D) 音を流してから、確認する
    const float kLoudAmp        = 0.5f;
    const int   kLoudBlocks     = 100;     // 音を流すブロック数
    const int   kSilenceSeconds = 15;      // 無音の長さ

    // 1 秒ぶんのブロック数(切り捨て)
    const int kBlocksPerSecond = static_cast<int>(kSampleRate) / kBlockSize;

    // ---- 道具 ----

    // buf[i] = amp × sin(2π × hz × (start + i) / サンプリング周波数)
    void FillSine(float* buf, int n, int start, float hz, float amp)
    {
        for (int i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(start + i) / kSampleRate;
            buf[i] = amp * static_cast<float>(std::sin(2.0 * kPi * hz * t));
        }
    }

    // n 個すべてが有限か調べ、絶対値の最大を peak に入れる
    bool AllFinite(const float* buf, int n, float& peak)
    {
        peak = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            if (!std::isfinite(buf[i]))
                return false;
            if (std::fabs(buf[i]) > peak)
                peak = std::fabs(buf[i]);
        }
        return true;
    }

    // 使う前に、SENS / Q / LEVEL を設定して準備する
    void Setup(AutoWah& wah, int sens, int q, int level)
    {
        wah.SetSens(sens);
        wah.SetQ(q);
        wah.SetLevel(level);
        wah.Prepare(kSampleRate);
    }

    // ---- 各テスト ----

    void TestNanInf()
    {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float inf = std::numeric_limits<float>::infinity();

        AutoWah wah;
        Setup(wah, 3, kStageMax, kStageMax);

        float buf[kBlockSize];
        FillSine(buf, kBlockSize, 0, kToneHz, kToneAmp);
        wah.Process(buf, kBlockSize);            // まず普通の音を流す

        // 4 個ごとに、NaN, +Inf, -Inf, 普通の値 を並べる
        for (int i = 0; i < kBlockSize; ++i)
        {
            switch (i % kBadPatternSize)
            {
            case 0:  buf[i] = nan;          break;
            case 1:  buf[i] = inf;          break;
            case 2:  buf[i] = -inf;         break;
            default: buf[i] = kNormalValue; break;
            }
        }
        wah.Process(buf, kBlockSize);
        float peak1;
        const bool ok1 = AllFinite(buf, kBlockSize, peak1);

        FillSine(buf, kBlockSize, 0, kToneHz, kToneAmp);   // 次のブロックは普通の音
        wah.Process(buf, kBlockSize);
        float peak2;
        const bool ok2 = AllFinite(buf, kBlockSize, peak2);

        std::printf("A) NaN/Inf block : finite=%d | next block finite=%d\n", ok1, ok2);
    }

    void TestRange()
    {
        AutoWah wah;
        Setup(wah, kStageMax, kStageMax, kStageMax);   // 一番反応が強い設定

        // 満杯の矩形波を 1 秒流して、出力の最大を調べる
        float peak = 0.0f;
        for (int b = 0; b < kBlocksPerSecond; ++b)
        {
            float buf[kBlockSize];
            for (int i = 0; i < kBlockSize; ++i)
            {
                const int n = b * kBlockSize + i;
                buf[i] = ((n / kSquareHalfPeriod) % 2) ? kFullScale : -kFullScale;
            }
            wah.Process(buf, kBlockSize);
            float p;
            AllFinite(buf, kBlockSize, p);
            if (p > peak) peak = p;
        }

        // 桁外れの値と、±1.0 を超える値を交互に入れる
        float huge[kBlockSize];
        for (int i = 0; i < kBlockSize; ++i)
            huge[i] = (i % 2) ? kHugeValue : kOverRange;
        wah.Process(huge, kBlockSize);
        float huge_peak;
        const bool huge_ok = AllFinite(huge, kBlockSize, huge_peak);

        std::printf("B) square peak=%.3f (<=%.1f) | huge input finite=%d peak=%.3f\n",
                    static_cast<double>(peak), static_cast<double>(kPeakLimit),
                    huge_ok, static_cast<double>(huge_peak));
    }

    void TestPrepareClears()
    {
        AutoWah wah;
        Setup(wah, 3, kStageMax, kStageMax);

        for (int b = 0; b < kLoudBlocks; ++b)
        {
            float buf[kBlockSize];
            FillSine(buf, kBlockSize, b * kBlockSize, kToneHz, kLoudAmp);
            wah.Process(buf, kBlockSize);
        }

        wah.Prepare(kSampleRate);                      // 準備し直す
        float silence[kBlockSize] = {};
        wah.Process(silence, kBlockSize);
        float peak;
        AllFinite(silence, kBlockSize, peak);

        std::printf("C) after Prepare, silence in -> out peak=%.6f env=%.6f fc=%.1f\n",
                    static_cast<double>(peak), static_cast<double>(wah.Envelope()),
                    static_cast<double>(wah.CenterFreq()));
    }

    void TestLongSilence()
    {
        AutoWah wah;
        Setup(wah, 3, kStageMax, kStageMax);

        for (int b = 0; b < kLoudBlocks; ++b)           // 音を流す(ブロックごとに新しい入力)
        {
            float buf[kBlockSize];
            FillSine(buf, kBlockSize, b * kBlockSize, kToneHz, kLoudAmp);
            wah.Process(buf, kBlockSize);
        }
        for (int b = 0; b < kBlocksPerSecond * kSilenceSeconds; ++b)   // 無音(ブロックごとに 0 を作る)
        {
            float silence[kBlockSize] = {};
            wah.Process(silence, kBlockSize);
        }
        std::printf("D) %ds silence: env=%g\n", kSilenceSeconds, static_cast<double>(wah.Envelope()));
    }
}

int main()
{
    TestNanInf();
    TestRange();
    TestPrepareClears();
    TestLongSilence();
    return 0;
}