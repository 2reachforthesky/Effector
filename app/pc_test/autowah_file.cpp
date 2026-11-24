// autowah_file.cpp - 録音した WAV に AutoWah をかける道具(Pi には載せない)
//
// 使い方:  autowah_file 入力.wav 出力.wav [SENS Q LEVEL]
//   SENS / Q / LEVEL は 1 ～ 5(省略すると 3 5 5)
#include "wav.hpp"
#include "autowah.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static const int kBlock = 128;   // 本番と同じ 128 サンプルずつ

static void Measure(const std::vector<float>& v, double& rms, float& peak)
{
    double s = 0.0;
    peak = 0.0f;
    for (float x : v)
    {
        s += (double)x * x;
        if (std::fabs(x) > peak) peak = std::fabs(x);
    }
    rms = v.empty() ? 0.0 : std::sqrt(s / (double)v.size());
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::printf("usage: autowah_file in.wav out.wav [SENS Q LEVEL]\n");
        std::printf("  SENS, Q, LEVEL: 1-5 (default 3 5 5)\n");
        return 1;
    }

    const char* in_name  = argv[1];
    const char* out_name = argv[2];
    int sens  = (argc > 3) ? std::atoi(argv[3]) : 3;
    int q     = (argc > 4) ? std::atoi(argv[4]) : 5;
    int level = (argc > 5) ? std::atoi(argv[5]) : 5;

    // 1. WAV を読み込む(モノラルの float になる)
    std::vector<float> samples;
    int sample_rate = 0;
    if (!ReadWav(in_name, samples, sample_rate))
    {
        std::printf("ERROR: cannot read '%s' (16-bit PCM WAV only?)\n", in_name);
        return 1;
    }

    double rms_in;
    float peak_in;
    Measure(samples, rms_in, peak_in);
    std::printf("input : %s  %d Hz  %.1f sec  rms=%.3f  peak=%.2f\n",
                in_name, sample_rate, (double)samples.size() / sample_rate, rms_in, peak_in);

    if (peak_in < 0.1f)
        std::printf("WARN : input is quiet (peak < 0.1). wah may not react much.\n");
    if (peak_in > 0.99f)
        std::printf("WARN : input is clipped (peak ~ 1.0). re-record at a lower level.\n");

    // 2. AutoWah を準備する(ファイルのサンプリング周波数を渡す)
    AutoWah wah;
    wah.Prepare((float)sample_rate);
    wah.SetSens(sens);
    wah.SetQ(q);
    wah.SetLevel(level);

    // 3. 128 サンプルずつ処理する
    for (size_t pos = 0; pos < samples.size(); pos += kBlock)
    {
        int count = (int)(samples.size() - pos);
        if (count > kBlock) count = kBlock;
        wah.Process(&samples[pos], count);
    }

    // 4. 書き出す
    double rms_out;
    float peak_out;
    Measure(samples, rms_out, peak_out);
    if (!WriteWav(out_name, samples, sample_rate))
    {
        std::printf("ERROR: cannot write '%s'\n", out_name);
        return 1;
    }
    std::printf("output: %s  SENS=%d Q=%d LEVEL=%d  rms=%.3f  peak=%.2f  (out/in rms=%.2f)\n",
                out_name, sens, q, level, rms_out, peak_out, rms_in > 0.0 ? rms_out / rms_in : 0.0);
    return 0;
}