// wav.hpp - WAV ファイルの読み書き
#pragma once
#include <cstdint>
#include <vector>

// 数値を 1 バイトずつに分けて、下の桁から順に v の後ろに足す
void PutU16(std::vector<uint8_t>& v, uint16_t x);
void PutU32(std::vector<uint8_t>& v, uint32_t x);

// p から始まるバイトを、数値に組み立て直す
uint16_t GetU16(const uint8_t* p);
uint32_t GetU32(const uint8_t* p);

// 16 ビット、モノラルの WAV を書き出す。失敗したら false
bool WriteWav(const char* filename, const std::vector<float>& samples, int sample_rate);

// WAV を読み込んで、モノラルの float の並びにする。
// sample_rate には、ファイルに書いてあったサンプリング周波数が入る。失敗したら false
bool ReadWav(const char* filename, std::vector<float>& samples, int& sample_rate);
