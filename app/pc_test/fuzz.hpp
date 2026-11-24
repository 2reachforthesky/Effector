// fuzz.hpp - ファズ
//
// 音を大きくして、-1.0 ～ +1.0 からはみ出した部分を切り落とす。
// 波の頭が平らに削られて、歪んだ音になる。
#pragma once
#include "effect.hpp"

class Fuzz : public Effect
{
public:
    const char* Name() const override { return "Fuzz"; }

    void Prepare(float) override {}

    void Process(float* samples, int count) override
    {
        for (int i = 0; i < count; ++i)
        {
            float x = samples[i] * gain_; // 大きくして
            if (x > 1.0f)                 // はみ出した分を切り落とす
                x = 1.0f;
            if (x < -1.0f)
                x = -1.0f;
            samples[i] = x * level_; // 最後に音量を下げる
        }
    }

private:
    float gain_ = 20.0f; // 歪みの強さ
    float level_ = 0.5f; // 出力の音量
};
