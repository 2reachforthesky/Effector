// autowah_test.cpp - AutoWah を耳と数字で確かめる道具(Pi には載せない)
#include "wav.hpp"
#include "autowah.hpp"
#include <cmath>
#include <cstdio>
#include <vector>

static const double kPi = 3.14159265358979;
static const int kRate  = 48000;
static const int kBlock = 128;

// ベースらしい音を作る: 24 本の倍音を重ねる
static void AddNote(std::vector<float>& out, double freq, double amp, double sec)
{
    const int n = (int)(sec * kRate);
    const int kHarm = 24;
    double norm = 0.0;
    for (int h = 1; h <= kHarm; ++h) norm += 1.0 / std::pow((double)h, 1.1);

    for (int i = 0; i < n; ++i)
    {
        const double t = (double)i / kRate;
        const double attack = (t < 0.003) ? (t / 0.003) : 1.0;   // 3ms で立ち上げて、プチ音を防ぐ
        double s = 0.0;
        for (int h = 1; h <= kHarm; ++h)
        {
            // 高い倍音ほど早く消える(弾いた直後は明るく、だんだん丸くなる)
            const double decay = std::exp(-t / (0.5 / (1.0 + 0.25 * h)));
            s += decay / std::pow((double)h, 1.1) * std::sin(2.0 * kPi * freq * h * t);
        }
        out.push_back((float)(amp * attack * s / norm * 2.2));
    }
}

static std::vector<float> MakeBassLine()
{
    std::vector<float> v;
    // 強弱をつけたフレーズ: (周波数Hz, 強さ, 長さ秒)
    AddNote(v, 41.20, 0.50, 0.75);   // E1 強く
    AddNote(v, 55.00, 0.15, 0.75);   // A1 弱く
    AddNote(v, 73.42, 0.40, 0.75);   // D2 中くらい
    AddNote(v, 98.00, 0.60, 0.75);   // G2 強く
    AddNote(v, 82.41, 0.10, 0.75);   // E2 とても弱く
    AddNote(v, 55.00, 0.55, 0.75);   // A1 強く
    AddNote(v, 41.20, 0.25, 1.00);   // E1 やや弱く(長め)
    return v;
}

static double Rms(const std::vector<float>& v)
{
    double s = 0.0;
    for (float x : v) s += (double)x * x;
    return std::sqrt(s / (double)v.size());
}

static void RunOne(const std::vector<float>& in, const char* file, int sens, int q, int level)
{
    AutoWah wah;
    wah.Prepare((float)kRate);
    wah.SetSens(sens);
    wah.SetQ(q);
    wah.SetLevel(level);

    std::vector<float> out = in;
    float fc_min = 1.0e9f, fc_max = 0.0f, peak = 0.0f;

    // 128 サンプルずつ渡す(本番と同じ)
    for (size_t pos = 0; pos < out.size(); pos += kBlock)
    {
        int count = (int)(out.size() - pos);
        if (count > kBlock) count = kBlock;
        wah.Process(&out[pos], count);

        if (wah.CenterFreq() < fc_min) fc_min = wah.CenterFreq();
        if (wah.CenterFreq() > fc_max) fc_max = wah.CenterFreq();
    }
    for (float x : out)
        if (std::fabs(x) > peak) peak = std::fabs(x);

    WriteWav(file, out, kRate);
    std::printf("%-22s SENS=%d Q=%d LEVEL=%d | fc %4.0f-%4.0f Hz | out/in rms=%.2f | peak=%.2f\n",
                file, sens, q, level, fc_min, fc_max, Rms(out) / Rms(in), peak);
}

int main()
{
    std::vector<float> in = MakeBassLine();
    WriteWav("v3_bass_in.wav", in, kRate);
    std::printf("v3_bass_in.wav         rms=%.3f (%.1f sec)\n", Rms(in), (double)in.size() / kRate);

    RunOne(in, "v3_s3_q3.wav", 3, 3, 5);   // 標準
    RunOne(in, "v3_s1_q3.wav", 1, 3, 5);   // 感度を低く
    RunOne(in, "v3_s5_q3.wav", 5, 3, 5);   // 感度を高く
    RunOne(in, "v3_s3_q1.wav", 3, 1, 5);   // Q を低く(広い)
    RunOne(in, "v3_s3_q5.wav", 3, 5, 5);   // Q を高く(鋭い)
    return 0;
}