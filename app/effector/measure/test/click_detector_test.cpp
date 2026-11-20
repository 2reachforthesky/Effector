// click_detector_test.cpp --- 「偽のケーブル」を使って、クリック検出をテストする
#include "click.hpp"
#include "click_detector.hpp"
#include <cstdint>
#include <cstdio>

static const size_t kBlock     = 128;            // 1ブロックのサンプル数
static const size_t kStreamLen = kBlock * 20;    // 偽の入力の長さ(2560サンプル)

static int g_failed = 0;

static void check(bool ok, const char* name)
{
    std::printf("%s : %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) {
        g_failed++;
    }
}

// 疑似乱数(毎回同じ並びになるので、テスト結果がぶれない)
static uint32_t g_seed = 12345;
static float randomNoise(float amp)
{
    g_seed = g_seed * 1664525u + 1013904223u;
    float r = static_cast<float>(g_seed >> 8) / 16777216.0f;   // 0〜1
    return (r * 2.0f - 1.0f) * amp;                            // -amp〜+amp
}

// 偽のケーブル: ノイズだけの入力に、クリックを delay サンプル遅れ・gain 倍で重ねる
static void buildStream(float* stream, size_t len,
                        const float* click, size_t clickLen,
                        size_t delay, float gain, float noiseAmp)
{
    for (size_t i = 0; i < len; i++) {
        stream[i] = randomNoise(noiseAmp);
    }
    for (size_t i = 0; i < clickLen; i++) {
        if (delay + i < len) {
            stream[delay + i] += gain * click[i];
        }
    }
}

// 実機と同じように、128サンプルずつ検出器に渡す
static bool runDetector(ClickDetector& d, const float* stream, size_t len)
{
    for (size_t pos = 0; pos < len; pos += kBlock) {
        d.feed(stream + pos, kBlock);
    }
    return d.detected();
}

static int64_t diff(uint64_t a, uint64_t b)
{
    return static_cast<int64_t>(a) - static_cast<int64_t>(b);
}

int main()
{
    float click[kClickLength];
    generateClick(click, kClickLength, kClickFreqHz, kClickSampleRate, kClickAmplitude);

    float stream[kStreamLen];
    ClickDetector det(kClickDetectThreshold);

    // 1. ノイズだけ(クリックなし)なら、見つけない
    buildStream(stream, kStreamLen, click, kClickLength, kStreamLen, 1.0f, 0.02f);
    check(!runDetector(det, stream, kStreamLen), "noise only: not detected");

    // 2. 遅れ0・ノイズなしで、クリックが「しきい値を超える位置」を調べる
    det.reset();
    buildStream(stream, kStreamLen, click, kClickLength, 0, 1.0f, 0.0f);
    check(runDetector(det, stream, kStreamLen), "clean click: detected");
    uint64_t offset = det.position();
    std::printf("rise offset = %llu samples\n", static_cast<unsigned long long>(offset));
    check(offset > 0 && offset < kClickLength / 2, "offset is inside the first half of the click");

    // 3. 1000サンプル遅れ: 位置は offset + 1000 になるはず
    det.reset();
    buildStream(stream, kStreamLen, click, kClickLength, 1000, 1.0f, 0.0f);
    runDetector(det, stream, kStreamLen);
    check(det.position() == offset + 1000, "delay 1000: position = offset + 1000");

    // 4. reset すると、また最初から検出できる
    det.reset();
    check(!det.detected(), "after reset: not detected");
    runDetector(det, stream, kStreamLen);
    check(det.position() == offset + 1000, "after reset: same position again");

    // 5. ノイズ入り: 数サンプルのずれは許す
    det.reset();
    buildStream(stream, kStreamLen, click, kClickLength, 1000, 1.0f, 0.02f);
    runDetector(det, stream, kStreamLen);
    int64_t d = diff(det.position(), offset + 1000);
    check(det.detected() && d >= -2 && d <= 2, "delay 1000 + noise: within 2 samples");

    // 6. ブロックの境目(384番目のサンプル = 3ブロック目の先頭)
    det.reset();
    buildStream(stream, kStreamLen, click, kClickLength, static_cast<size_t>(384 - offset), 1.0f, 0.0f);
    runDetector(det, stream, kStreamLen);
    check(det.position() == 384, "block boundary: first sample of block");

    // 7. ブロックの境目(383番目のサンプル = 2ブロック目の最後)
    det.reset();
    buildStream(stream, kStreamLen, click, kClickLength, static_cast<size_t>(383 - offset), 1.0f, 0.0f);
    runDetector(det, stream, kStreamLen);
    check(det.position() == 383, "block boundary: last sample of block");

    // 8. 音が小さく(半分に)なっても見つかる
    det.reset();
    buildStream(stream, kStreamLen, click, kClickLength, 1000, 0.5f, 0.0f);
    check(runDetector(det, stream, kStreamLen), "half-size click: detected");

    return g_failed == 0 ? 0 : 1;
}