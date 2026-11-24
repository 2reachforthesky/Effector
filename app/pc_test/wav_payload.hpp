// wav_payload.hpp - WAV ファイルの「音のデータ(ペイロード)」だけにエフェクトをかける
//
// PC でテストするための道具(Pi には載せない)。
//
// 読み込んでモノラルに直して書き直すのではなく、ファイルのバイト列の中の
// 音のデータの部分だけを、その場で書き換える。
// ヘッダ、チャンネル数、ビット数、LIST などの他のチャンクは、そのまま残る。
#pragma once
#include "effect.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

// fmt チャンクと data チャンクから分かったこと
struct WavPayload
{
    uint16_t format      = 0;   // 1 = 整数 (PCM)、3 = 浮動小数点
    uint16_t channels    = 0;   // チャンネル数
    uint32_t sample_rate = 0;   // サンプリング周波数 (Hz)
    uint16_t bits        = 0;   // 1 サンプルのビット数
    size_t   data_pos    = 0;   // バイト列の中で、音のデータが始まる位置
    size_t   data_size   = 0;   // 音のデータのバイト数

    size_t BytesPerSample() const { return bits / 8u; }
    size_t Frames() const;      // 1 チャンネルあたりのサンプル数
};

// チャンネルごとの、音の大きさの記録
struct ChannelStats
{
    double sum_sq = 0.0;   // サンプルの 2 乗の合計
    float  peak   = 0.0f;  // 絶対値の最大
    size_t count  = 0;     // サンプル数

    void Add(float x);
    double Rms() const;
};

// ファイルの中身を全部読む / 全部書く
bool ReadFileBytes(const char* filename, std::vector<uint8_t>& bytes);
bool WriteFileBytes(const char* filename, const std::vector<uint8_t>& bytes);

// bytes の中から fmt と data を探す。見つからなければ false
bool FindWavPayload(const std::vector<uint8_t>& bytes, WavPayload& payload);

// この道具で扱える形式か(16/24/32 ビット整数、32 ビット浮動小数点)
bool IsSupportedPayload(const WavPayload& payload);

// 音のデータに、チャンネルごとのエフェクトを block_size サンプルずつかける。
//   effects[c] がチャンネル c を担当する(要素数 = チャンネル数)。
//   エフェクトは状態を持つので、チャンネルごとに別のものを渡すこと。
//   in_stats / out_stats は、nullptr でもよい。
bool ProcessWavPayload(std::vector<uint8_t>& bytes,
                       const WavPayload& payload,
                       const std::vector<Effect*>& effects,
                       int block_size,
                       std::vector<ChannelStats>* in_stats,
                       std::vector<ChannelStats>* out_stats);