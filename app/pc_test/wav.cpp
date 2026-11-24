// wav.cpp - WAV ファイルの読み書き
#include "wav.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

void PutU16(std::vector<uint8_t>& v, uint16_t x)
{
    v.push_back(static_cast<uint8_t>(x));      // 0～7 ビット目
    v.push_back(static_cast<uint8_t>(x >> 8)); // 8～15 ビット目
}

void PutU32(std::vector<uint8_t>& v, uint32_t x)
{
    v.push_back(static_cast<uint8_t>(x));       // 0～7 ビット目
    v.push_back(static_cast<uint8_t>(x >> 8));  // 8～15 ビット目
    v.push_back(static_cast<uint8_t>(x >> 16)); // 16～23 ビット目
    v.push_back(static_cast<uint8_t>(x >> 24)); // 24～31 ビット目
}

uint16_t GetU16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

uint32_t GetU32(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

bool WriteWav(const char* filename, const std::vector<float>& samples, int sample_rate)
{
    std::vector<uint8_t> v;
    uint32_t data_size = static_cast<uint32_t>(samples.size() * 2); // 1 サンプル 2 バイト

    // ヘッダ (44 バイト)
    v.insert(v.end(), {'R', 'I', 'F', 'F'});
    PutU32(v, 36 + data_size); // ファイル全体の大きさ - 8
    v.insert(v.end(), {'W', 'A', 'V', 'E'});
    v.insert(v.end(), {'f', 'm', 't', ' '});
    PutU32(v, 16);             // fmt の中身の大きさ
    PutU16(v, 1);              // 1 = 整数の形式 (PCM)
    PutU16(v, 1);              // チャンネル数 (モノラル)
    PutU32(v, sample_rate);     // サンプリング周波数
    PutU32(v, sample_rate * 2); // 1 秒あたりのバイト数
    PutU16(v, 2);              // 1 サンプルのバイト数
    PutU16(v, 16);             // ビット数
    v.insert(v.end(), {'d', 'a', 't', 'a'});
    PutU32(v, data_size); // サンプルのデータの大きさ

    // サンプル。-1.0～+1.0 を -32768～32767 の整数にする
    for (float s : samples)
    {
        long x = std::lround(s * 32768.0f); // 四捨五入
        if (x > 32767)
            x = 32767;
        if (x < -32768)
            x = -32768;
        PutU16(v, static_cast<uint16_t>(static_cast<int16_t>(x)));
    }

    FILE* file = std::fopen(filename, "wb"); // "wb" = 書き込み用、バイナリ
    if (!file)
        return false;
    std::fwrite(v.data(), 1, v.size(), file);
    std::fclose(file);
    return true;
}

// ファイルの中身を全部、bytes に読み込む
static bool ReadAllBytes(const char* filename, std::vector<uint8_t>& bytes)
{
    FILE* file = std::fopen(filename, "rb"); // "rb" = 読み込み用、バイナリ
    if (!file)
        return false;
    uint8_t buffer[4096];
    size_t n;
    while ((n = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        bytes.insert(bytes.end(), buffer, buffer + n);
    }
    std::fclose(file);
    return true;
}

// サンプルを 1 個読んで、-1.0～+1.0 の float にする
static float ReadSample(const uint8_t* p, uint16_t format, uint16_t bits)
{
    if (format == 1 && bits == 16) // 16 ビットの整数
    {
        int16_t s = static_cast<int16_t>(GetU16(p));
        return s / 32768.0f;
    }
    if (format == 3 && bits == 32) // 32 ビットの小数
    {
        float f;
        std::memcpy(&f, p, 4);
        return f;
    }
    return 0.0f;
}

bool ReadWav(const char* filename, std::vector<float>& samples, int& sample_rate)
{
    std::vector<uint8_t> bytes;
    if (!ReadAllBytes(filename, bytes))
    {
        std::printf("cannot open: %s\n", filename);
        return false;
    }
    if (bytes.size() < 12 ||
        std::memcmp(&bytes[0], "RIFF", 4) != 0 ||
        std::memcmp(&bytes[8], "WAVE", 4) != 0)
    {
        std::printf("not a WAV file: %s\n", filename);
        return false;
    }

    // チャンク (「4 文字の名前 + 4 バイトの大きさ + 中身」) を順に見て、fmt と data を探す
    uint16_t format = 0;
    uint16_t channels = 0;
    uint16_t bits = 0;
    uint32_t rate = 0;
    bool found_fmt = false;
    bool found_data = false;
    size_t data_pos = 0;
    size_t data_size = 0;

    size_t pos = 12;
    while (pos + 8 <= bytes.size())
    {
        size_t size = GetU32(&bytes[pos + 4]);
        size_t body = pos + 8; // 中身の始まり
        if (size > bytes.size() - body)
            size = bytes.size() - body; // 途中で切れているファイルへの備え

        if (std::memcmp(&bytes[pos], "fmt ", 4) == 0 && size >= 16)
        {
            format = GetU16(&bytes[body]);
            channels = GetU16(&bytes[body + 2]);
            rate = GetU32(&bytes[body + 4]);
            bits = GetU16(&bytes[body + 14]);
            if (format == 0xFFFE && size >= 26) // 拡張形式。本当の形式は 24 バイト目にある
                format = GetU16(&bytes[body + 24]);
            found_fmt = true;
        }
        else if (std::memcmp(&bytes[pos], "data", 4) == 0)
        {
            data_pos = body;
            data_size = size;
            found_data = true;
        }
        pos = body + size + (size & 1); // 大きさが奇数なら、1 バイト飛ばす決まり
    }

    if (!found_fmt || !found_data || channels == 0)
    {
        std::printf("fmt or data not found: %s\n", filename);
        return false;
    }
    std::printf("format=%u channels=%u rate=%u bits=%u\n", format, channels, rate, bits);
    bool supported = (format == 1 && bits == 16) || (format == 3 && bits == 32);
    if (!supported)
    {
        std::printf("unsupported format\n");
        return false;
    }

    // 左右などのチャンネルを平均して、モノラルにする
    size_t bytes_per_sample = bits / 8;
    size_t frames = data_size / (bytes_per_sample * channels);
    samples.clear();
    for (size_t i = 0; i < frames; ++i)
    {
        float sum = 0.0f;
        for (size_t c = 0; c < channels; ++c)
        {
            size_t at = data_pos + (i * channels + c) * bytes_per_sample;
            sum += ReadSample(&bytes[at], format, bits);
        }
        samples.push_back(sum / channels);
    }
    sample_rate = static_cast<int>(rate);
    return true;
}
