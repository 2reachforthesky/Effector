//click.hpp ---クリック音(往復遅延の測定に使う音)を作る
#pragma once
#include <cstddef>

//クリック音の仕様

const size_t kClickLength = 96; //サンプル数(2ミリ秒)
const float kClickFreqHz = 3000.0f; //サイン波の周波数
const float kClickSampleRate = 48000.0f; //サンプリング周波数
const float kClickAmplitude = 0.3f; //振幅(最大1.0) 

//bufにlength個のクリック波形を書き込む
void generateClick(float* buf, size_t length, float freqHz, float sampleRate, float amplitude);