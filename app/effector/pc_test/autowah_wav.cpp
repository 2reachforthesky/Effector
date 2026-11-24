// autowah_wav.cpp - 録音した WAV のデータ(ペイロード)に AutoWah をかける道具(Pi には載せない)
//
// 使い方:  autowah_wav 入力.wav 出力.wav [SENS Q LEVEL]
//   SENS / Q / LEVEL は 1 ～ 5(省略すると 3 5 5。Step 8 で確定した設定)
//
// ヘッダやチャンネル数、ビット数は変えずに、音のデータだけを書き換える。
// チャンネルごとに別の AutoWah を使う(ステレオなら左右を別々に処理する)。
#include "autowah.hpp"
#include "wav_payload.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
    // 本番と同じ処理単位(config.hpp の BLOCK_SIZE と同じ値にする)
    const int kBlockSize = 128;

    // 段階(SENS / Q / LEVEL)の範囲と、省略したときの値
    const int kStageMin     = 1;
    const int kStageMax     = 5;
    const int kDefaultSens  = 3;
    const int kDefaultQ     = 5;
    const int kDefaultLevel = 5;

    // 録音の大きさの目安(ピーク)
    const float kQuietPeak = 0.1f;    // これより小さいと、ワウがあまり動かない
    const float kClipPeak  = 0.99f;   // これ以上は、録音が割れている

    // 段階の文字列を数にする。1 ～ 5 でなければ false
    bool ParseStage(const char* text, int& stage)
    {
        char* end = nullptr;
        const long v = std::strtol(text, &end, 10);
        if (end == text || *end != '\0' || v < kStageMin || v > kStageMax)
            return false;
        stage = static_cast<int>(v);
        return true;
    }

    void PrintStats(const char* label, const std::vector<ChannelStats>& stats)
    {
        for (size_t c = 0; c < stats.size(); ++c)
            std::printf("  %s ch%zu : rms=%.3f  peak=%.3f\n",
                        label, c, stats[c].Rms(), static_cast<double>(stats[c].peak));
    }
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::printf("usage: autowah_wav in.wav out.wav [SENS Q LEVEL]\n");
        std::printf("  SENS, Q, LEVEL: %d-%d (default %d %d %d)\n",
                    kStageMin, kStageMax, kDefaultSens, kDefaultQ, kDefaultLevel);
        return 1;
    }

    const char* in_name  = argv[1];
    const char* out_name = argv[2];
    if (std::strcmp(in_name, out_name) == 0)
    {
        std::printf("ERROR: output must be a different file from input\n");
        return 1;
    }

    int sens  = kDefaultSens;
    int q     = kDefaultQ;
    int level = kDefaultLevel;
    if ((argc > 3 && !ParseStage(argv[3], sens)) ||
        (argc > 4 && !ParseStage(argv[4], q)) ||
        (argc > 5 && !ParseStage(argv[5], level)))
    {
        std::printf("ERROR: SENS / Q / LEVEL must be %d-%d\n", kStageMin, kStageMax);
        return 1;
    }

    // 1. ファイルを丸ごと読んで、音のデータの場所を調べる
    std::vector<uint8_t> bytes;
    if (!ReadFileBytes(in_name, bytes))
    {
        std::printf("ERROR: cannot open '%s'\n", in_name);
        return 1;
    }
    WavPayload payload;
    if (!FindWavPayload(bytes, payload))
        return 1;

    std::printf("input : %s\n", in_name);
    std::printf("  format=%u channels=%u rate=%u Hz bits=%u  frames=%zu (%.1f sec)\n",
                payload.format, payload.channels, payload.sample_rate, payload.bits,
                payload.Frames(),
                static_cast<double>(payload.Frames()) / payload.sample_rate);

    if (!IsSupportedPayload(payload))
    {
        std::printf("ERROR: unsupported format (16/24/32-bit PCM or 32-bit float only)\n");
        return 1;
    }

    // 2. チャンネルごとに、別の AutoWah を用意する
    std::vector<AutoWah> wahs(payload.channels);
    std::vector<Effect*> effects;
    for (AutoWah& wah : wahs)
    {
        wah.SetSens(sens);
        wah.SetQ(q);
        wah.SetLevel(level);
        wah.Prepare(static_cast<float>(payload.sample_rate));   // ファイルの周波数を渡す
        effects.push_back(&wah);
    }

    // 3. 音のデータだけを書き換える
    std::vector<ChannelStats> in_stats;
    std::vector<ChannelStats> out_stats;
    if (!ProcessWavPayload(bytes, payload, effects, kBlockSize, &in_stats, &out_stats))
    {
        std::printf("ERROR: processing failed\n");
        return 1;
    }

    PrintStats("in ", in_stats);
    for (size_t c = 0; c < in_stats.size(); ++c)
    {
        if (in_stats[c].peak >= kClipPeak)
            std::printf("WARN : ch%zu is clipped. re-record at a lower level.\n", c);
        else if (in_stats[c].peak > 0.0f && in_stats[c].peak < kQuietPeak)
            std::printf("WARN : ch%zu is quiet (peak < %.1f). wah may not react much.\n",
                        c, static_cast<double>(kQuietPeak));
    }

    // 4. 書き出す
    if (!WriteFileBytes(out_name, bytes))
    {
        std::printf("ERROR: cannot write '%s'\n", out_name);
        return 1;
    }
    std::printf("output: %s  SENS=%d Q=%d LEVEL=%d\n", out_name, sens, q, level);
    PrintStats("out", out_stats);
    return 0;
}