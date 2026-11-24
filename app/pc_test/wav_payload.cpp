// wav_payload.cpp - WAV の音のデータだけにエフェクトをかける
#include "wav_payload.hpp"
#include "wav.hpp"          // GetU16 / GetU32 を使う

#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
    // ---- WAV ファイルの決まり(数字に名前を付ける) ----
    const size_t kRiffHeaderSize  = 12;   // "RIFF" + 大きさ + "WAVE"
    const size_t kChunkHeaderSize = 8;    // チャンク名(4) + 大きさ(4)
    const size_t kChunkNameSize   = 4;
    const size_t kFmtMinSize      = 16;   // fmt チャンクの最小の大きさ
    const size_t kFmtExtSize      = 26;   // 拡張形式で、本当の形式まで含む大きさ

    // fmt チャンクの中身の位置(チャンクの中身の先頭から)
    const size_t kFmtOffsetFormat   = 0;
    const size_t kFmtOffsetChannels = 2;
    const size_t kFmtOffsetRate     = 4;
    const size_t kFmtOffsetBits     = 14;
    const size_t kFmtOffsetSubtype  = 24; // 拡張形式のときの、本当の形式

    const uint16_t kFormatPcm        = 1;
    const uint16_t kFormatFloat      = 3;
    const uint16_t kFormatExtensible = 0xFFFE;

    const unsigned kBitsPerByte = 8;
    const uint16_t kBits16 = 16;
    const uint16_t kBits24 = 24;
    const uint16_t kBits32 = 32;

    // 整数の音を -1.0 ～ +1.0 にするときの割り算の値(2 の (ビット数 - 1) 乗)
    float FullScale(uint16_t bits)
    {
        return std::ldexp(1.0f, static_cast<int>(bits) - 1);
    }

    // バイト列 p から、符号付きの整数を 1 つ組み立てる(下の桁が先)
    int32_t LoadInt(const uint8_t* p, uint16_t bits)
    {
        uint32_t v = 0;
        const unsigned n = bits / kBitsPerByte;
        for (unsigned i = 0; i < n; ++i)
            v |= static_cast<uint32_t>(p[i]) << (kBitsPerByte * i);

        // 符号を延ばす(24 ビットで、先頭のビットが 1 なら負)
        if (bits < kBits32)
        {
            const uint32_t sign_bit = 1u << (bits - 1);
            if (v & sign_bit)
                v |= ~((1u << bits) - 1u);
        }
        return static_cast<int32_t>(v);
    }

    // 整数 v を、bits ビットぶんのバイトにして p に書く(下の桁が先)
    void StoreInt(uint8_t* p, int32_t v, uint16_t bits)
    {
        const uint32_t u = static_cast<uint32_t>(v);
        const unsigned n = bits / kBitsPerByte;
        for (unsigned i = 0; i < n; ++i)
            p[i] = static_cast<uint8_t>(u >> (kBitsPerByte * i));
    }

    // サンプルを 1 個読んで、-1.0 ～ +1.0 の float にする
    float DecodeSample(const uint8_t* p, uint16_t format, uint16_t bits)
    {
        if (format == kFormatFloat)
        {
            float f;
            std::memcpy(&f, p, sizeof(f));
            return f;
        }
        return static_cast<float>(LoadInt(p, bits)) / FullScale(bits);
    }

    // float を、元のビット数の形に戻して p に書く(範囲外は端に収める)
    void EncodeSample(uint8_t* p, float x, uint16_t format, uint16_t bits)
    {
        if (format == kFormatFloat)
        {
            std::memcpy(p, &x, sizeof(x));
            return;
        }
        const double scale = static_cast<double>(FullScale(bits));
        const double max_v = scale - 1.0;
        const double min_v = -scale;

        double d = static_cast<double>(x) * scale;
        if (!(d == d))            // NaN は 0 にする
            d = 0.0;
        if (d > max_v) d = max_v;   // 範囲外は端に収める
        if (d < min_v) d = min_v;

        // 四捨五入。wav.cpp の WriteWav と同じ丸め方(0.5 は 0 から遠いほうへ)にそろえる
        const long long v = std::llround(d);
        StoreInt(p, static_cast<int32_t>(v), bits);
    }
}

size_t WavPayload::Frames() const
{
    const size_t frame_bytes = BytesPerSample() * channels;
    return frame_bytes == 0 ? 0 : data_size / frame_bytes;
}

void ChannelStats::Add(float x)
{
    const float a = std::fabs(x);
    if (a > peak) peak = a;
    sum_sq += static_cast<double>(x) * x;
    ++count;
}

double ChannelStats::Rms() const
{
    return count == 0 ? 0.0 : std::sqrt(sum_sq / static_cast<double>(count));
}

bool ReadFileBytes(const char* filename, std::vector<uint8_t>& bytes)
{
    FILE* file = std::fopen(filename, "rb");
    if (!file)
        return false;
    uint8_t buffer[4096];
    size_t n;
    bytes.clear();
    while ((n = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
        bytes.insert(bytes.end(), buffer, buffer + n);
    std::fclose(file);
    return true;
}

bool WriteFileBytes(const char* filename, const std::vector<uint8_t>& bytes)
{
    FILE* file = std::fopen(filename, "wb");
    if (!file)
        return false;
    const size_t written = std::fwrite(bytes.data(), 1, bytes.size(), file);
    std::fclose(file);
    return written == bytes.size();
}

bool FindWavPayload(const std::vector<uint8_t>& bytes, WavPayload& payload)
{
    if (bytes.size() < kRiffHeaderSize ||
        std::memcmp(&bytes[0], "RIFF", kChunkNameSize) != 0 ||
        std::memcmp(&bytes[8], "WAVE", kChunkNameSize) != 0)
    {
        std::printf("not a WAV file\n");
        return false;
    }

    WavPayload found;
    bool found_fmt  = false;
    bool found_data = false;

    // チャンク(「名前 + 大きさ + 中身」)を順に見て、fmt と data を探す
    size_t pos = kRiffHeaderSize;
    while (pos + kChunkHeaderSize <= bytes.size())
    {
        size_t size = GetU32(&bytes[pos + kChunkNameSize]);
        const size_t body = pos + kChunkHeaderSize;
        if (size > bytes.size() - body)
            size = bytes.size() - body;   // 大きさが嘘のファイル(録音中に切れたものなど)への備え

        if (std::memcmp(&bytes[pos], "fmt ", kChunkNameSize) == 0 && size >= kFmtMinSize)
        {
            found.format      = GetU16(&bytes[body + kFmtOffsetFormat]);
            found.channels    = GetU16(&bytes[body + kFmtOffsetChannels]);
            found.sample_rate = GetU32(&bytes[body + kFmtOffsetRate]);
            found.bits        = GetU16(&bytes[body + kFmtOffsetBits]);
            if (found.format == kFormatExtensible && size >= kFmtExtSize)
                found.format = GetU16(&bytes[body + kFmtOffsetSubtype]);
            found_fmt = true;
        }
        else if (std::memcmp(&bytes[pos], "data", kChunkNameSize) == 0 && !found_data)
        {
            found.data_pos  = body;
            found.data_size = size;
            found_data = true;
        }
        pos = body + size + (size & 1u);  // 大きさが奇数なら、1 バイト飛ばす決まり
    }

    if (!found_fmt || !found_data || found.channels == 0)
    {
        std::printf("fmt or data chunk not found\n");
        return false;
    }
    payload = found;
    return true;
}

bool IsSupportedPayload(const WavPayload& payload)
{
    if (payload.format == kFormatFloat)
        return payload.bits == kBits32;
    if (payload.format == kFormatPcm)
        return payload.bits == kBits16 || payload.bits == kBits24 || payload.bits == kBits32;
    return false;
}

bool ProcessWavPayload(std::vector<uint8_t>& bytes,
                       const WavPayload& payload,
                       const std::vector<Effect*>& effects,
                       int block_size,
                       std::vector<ChannelStats>* in_stats,
                       std::vector<ChannelStats>* out_stats)
{
    if (!IsSupportedPayload(payload) || block_size <= 0)
        return false;
    if (effects.size() != payload.channels)
        return false;
    if (payload.data_pos + payload.data_size > bytes.size())
        return false;

    const size_t channels     = payload.channels;
    const size_t sample_bytes = payload.BytesPerSample();
    const size_t frame_bytes  = sample_bytes * channels;
    const size_t frames       = payload.Frames();

    if (in_stats)  in_stats->assign(channels, ChannelStats());
    if (out_stats) out_stats->assign(channels, ChannelStats());

    std::vector<float> block(static_cast<size_t>(block_size));

    for (size_t start = 0; start < frames; start += static_cast<size_t>(block_size))
    {
        size_t n = frames - start;
        if (n > static_cast<size_t>(block_size))
            n = static_cast<size_t>(block_size);

        for (size_t c = 0; c < channels; ++c)
        {
            // 1) このチャンネルの n サンプルを取り出す(チャンネルは交互に並んでいる)
            for (size_t i = 0; i < n; ++i)
            {
                const size_t at = payload.data_pos + (start + i) * frame_bytes + c * sample_bytes;
                block[i] = DecodeSample(&bytes[at], payload.format, payload.bits);
                if (in_stats) (*in_stats)[c].Add(block[i]);
            }

            // 2) エフェクトをかける
            effects[c]->Process(block.data(), static_cast<int>(n));

            // 3) 元の場所に、元のビット数で書き戻す
            for (size_t i = 0; i < n; ++i)
            {
                const size_t at = payload.data_pos + (start + i) * frame_bytes + c * sample_bytes;
                EncodeSample(&bytes[at], block[i], payload.format, payload.bits);
                if (out_stats) (*out_stats)[c].Add(block[i]);
            }
        }
    }
    return true;
}