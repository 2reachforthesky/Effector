// measure_latency_test.cpp --- 「偽のケーブル」で、往復遅延の測定をテストする
#include "measure.hpp"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

static const size_t kBlock = 128;   // 1ブロックのサンプル数

static int g_failed = 0;

static void check(bool ok, const char* name)
{
    std::printf("%s : %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) {
        g_failed++;
    }
}

// Measure のコンストラクタに必要な、偽の時計(この測定では使わない)
class FakeClock : public IClock {
public:
    uint64_t nowUs() override { return 0; }
};

// 疑似乱数(毎回同じ並びになる)
static uint32_t g_seed = 12345;
static float randomNoise(float amp)
{
    g_seed = g_seed * 1664525u + 1013904223u;
    float r = static_cast<float>(g_seed >> 8) / 16777216.0f;
    return (r * 2.0f - 1.0f) * amp;
}

// 偽のケーブルでつないで、測定が終わるまでブロックを回し続ける。
//   delay : 出した音が戻ってくるまでのサンプル数(128以上にすること)
//   gain  : 戻ってくる音の大きさ(0なら、何も戻ってこない)
static bool runLoop(Measure& m, size_t delay, float gain, float noiseAmp, float* maxOutAbs)
{
    std::vector<float> sent;     // Measure がこれまでに出した全サンプル
    float in[kBlock];
    float out[kBlock];
    float maxOut = 0.0f;

    for (size_t n = 0; n < 200000 && !m.done(); n++) {
        for (size_t i = 0; i < kBlock; i++) {
            size_t t = n * kBlock + i;          // 通算のサンプル番号
            float v = randomNoise(noiseAmp);
            if (t >= delay) {
                v += gain * sent[t - delay];    // 出した音が、delay サンプル遅れて戻ってくる
            }
            in[i] = v;
        }

        m.process(in, out, kBlock);

        for (size_t i = 0; i < kBlock; i++) {
            sent.push_back(out[i]);
            if (std::fabs(out[i]) > maxOut) {
                maxOut = std::fabs(out[i]);
            }
        }
    }

    if (maxOutAbs != nullptr) {
        *maxOutAbs = maxOut;
    }
    return m.done();
}

static bool near(float a, float b, float tol)
{
    return std::fabs(a - b) <= tol;
}

int main()
{
    const float usPerSample = 1000000.0f / 48000.0f;   // 約20.83マイクロ秒

    FakeClock clock;
    Measure m(clock);

    // 1. 測定を始める前は、無音を出す
    {
        float in[kBlock];
        float out[kBlock];
        for (size_t i = 0; i < kBlock; i++) { in[i] = 0.5f; out[i] = 9.0f; }
        m.process(in, out, kBlock);
        bool silent = true;
        for (size_t i = 0; i < kBlock; i++) { if (out[i] != 0.0f) { silent = false; } }
        check(silent, "idle: output is silent");
    }

    // 2. 1000サンプル遅れ・ノイズなし
    {
        float maxOut = 0.0f;
        m.start();
        check(runLoop(m, 1000, 1.0f, 0.0f, &maxOut), "delay 1000: finished");
        check(m.result().count == kMeasureTrials, "delay 1000: all trials measured");
        check(m.result().timeoutCount == 0, "delay 1000: no timeout");
        check(near(m.result().avgUs, 1000 * usPerSample, 1.0f), "delay 1000: avg = 20833 us");
        check(near(m.result().minUs, 1000 * usPerSample, 1.0f), "delay 1000: min = 20833 us");
        check(near(m.result().maxUs, 1000 * usPerSample, 1.0f), "delay 1000: max = 20833 us");
        check(m.result().jitterUs < 1.0f, "delay 1000: jitter = 0");
        check(maxOut > 0.29f && maxOut <= 0.3001f, "click level is about 0.3");
    }

    // 3. 300サンプル遅れ(ブロックの長さの倍数ではない)
    {
        m.start();
        runLoop(m, 300, 1.0f, 0.0f, nullptr);
        check(near(m.result().avgUs, 300 * usPerSample, 1.0f), "delay 300: avg = 6250 us");
    }

    // 4. ノイズ入り: 数サンプルのずれは許す
    {
        m.start();
        runLoop(m, 1000, 1.0f, 0.02f, nullptr);
        check(m.result().count == kMeasureTrials, "noise: all trials measured");
        check(near(m.result().avgUs, 1000 * usPerSample, 3 * usPerSample), "noise: avg within 3 samples");
        check(m.result().jitterUs <= 5 * usPerSample, "noise: jitter within 5 samples");
        std::printf("  (noise case: avg=%.1f us, jitter=%.1f us)\n", m.result().avgUs, m.result().jitterUs);
    }

    // 5. 何も戻ってこない: すべてタイムアウト
    {
        m.start();
        check(runLoop(m, 1000, 0.0f, 0.0f, nullptr), "no return: finished");
        check(m.result().count == 0, "no return: nothing measured");
        check(m.result().timeoutCount == kMeasureTrials, "no return: all timeouts");
    }

    // 6. もう一度 start すると、前回の結果が消えて、数え直しになる
    {
        m.start();
        runLoop(m, 1000, 1.0f, 0.0f, nullptr);
        check(m.result().count == kMeasureTrials, "restart: count is reset");
        check(m.result().timeoutCount == 0, "restart: timeoutCount is reset");
    }

    // 7. stop すると、無音になり、終了扱いにはならない
    {
        m.start();
        float in[kBlock] = {0};
        float out[kBlock];
        for (int i = 0; i < 20; i++) {
            m.process(in, out, kBlock);
        }
        m.stop();
        for (size_t i = 0; i < kBlock; i++) { out[i] = 9.0f; }
        m.process(in, out, kBlock);
        bool silent = true;
        for (size_t i = 0; i < kBlock; i++) { if (out[i] != 0.0f) { silent = false; } }
        check(silent && !m.done(), "stop: silent and not done");
    }

    return g_failed == 0 ? 0 : 1;
}