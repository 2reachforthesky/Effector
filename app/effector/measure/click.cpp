// click.cpp --- クリック音の生成
#include "click.hpp"
#include <cmath>

void generateClick(float* buf, size_t length, float freqHz, float sampleRate, float amplitude)
{
    const float kPi = 3.14159265358979f;

    if (buf == nullptr || length == 0) {
        return;
    }
    if (length == 1) {
        buf[0] = 0.0f;
        return;
    }

    for (size_t n = 0; n < length; n++) {
        // 窓: 両端が0、真ん中が1のなめらかな山
        float window = 0.5f * (1.0f - std::cos(2.0f * kPi * static_cast<float>(n)
                                                / static_cast<float>(length - 1)));
        // サイン波
        float wave = std::sin(2.0f * kPi * freqHz * static_cast<float>(n) / sampleRate);

        buf[n] = amplitude * window * wave;
    }
}