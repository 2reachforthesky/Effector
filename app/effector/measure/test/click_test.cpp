// click_test.cpp --- クリック波形のテスト
#include "click.hpp"
#include <cmath>
#include <cstdio>

static int g_failed = 0;

static void check(bool ok, const char* name)
{
    std::printf("%s : %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) {
        g_failed++;
    }
}

int main()
{
    float buf[kClickLength];
    for (size_t i = 0; i < kClickLength; i++) {
        buf[i] = 9.0f;   // わざと大きな値で埋めておく(上書きされるか確認するため)
    }

    generateClick(buf, kClickLength, kClickFreqHz, kClickSampleRate, kClickAmplitude);

    // 両端は0に近いはず
    check(std::fabs(buf[0]) < 1e-6f, "first sample is 0");
    check(std::fabs(buf[kClickLength - 1]) < 1e-3f, "last sample is about 0");

    // 最大値・範囲・合計(直流成分)を調べる
    float peak = 0.0f;
    float sum = 0.0f;
    bool inRange = true;
    for (size_t i = 0; i < kClickLength; i++) {
        float a = std::fabs(buf[i]);
        if (a > peak) {
            peak = a;
        }
        if (a > kClickAmplitude) {
            inRange = false;
        }
        sum += buf[i];
    }
    check(inRange, "never exceeds amplitude");
    check(peak > kClickAmplitude * 0.5f, "peak is large enough");
    check(std::fabs(sum) < 0.01f, "DC (sum) is about 0");

    // 振幅0なら、完全な無音になるはず
    float quiet[kClickLength];
    generateClick(quiet, kClickLength, kClickFreqHz, kClickSampleRate, 0.0f);
    bool allZero = true;
    for (size_t i = 0; i < kClickLength; i++) {
        if (quiet[i] != 0.0f) {
            allZero = false;
        }
    }
    check(allZero, "amplitude 0 gives silence");

    return g_failed == 0 ? 0 : 1;
}