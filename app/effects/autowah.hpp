// autowah.hpp - オートワウ
//
// 入力の音の大きさ(包絡線)を追いかけて、バンドパスフィルタの
// 中心周波数を自動で動かす。強く弾くと高く、弱くなると低くなる。
#pragma once
#include "effect.hpp"

class AutoWah : public Effect
{
public:
    const char* Name() const override { return "AutoWah"; }

    // 使い始める前に 1 回だけ。係数の計算と状態のリセットをする
    void Prepare(float sample_rate) override;

    // 音の処理。samples を直接書き換える
    void Process(float* samples, int count) override;

    // 覚えている状態(前の音の残り)を消す。パラメータは変えない
    void Reset();

    // パラメータ設定。stage は 1 ～ 5 の段階値(設計書 8.1 の表)
    void SetSens(int stage);   // 感度:   20/40/60/80/100 %
    void SetQ(int stage);      // 鋭さ:   0.5/0.8/1.2/2.0/3.0
    void SetLevel(int stage);  // 音量:   20/40/60/80/100 %

    // 確認用(テストで使う。処理には影響しない)
    float Envelope() const   { return env_; }
    float CenterFreq() const { return fc_; }

private:
    // ---- 設定値(ボタンで変わる。「目標」の値) ----
    // 初期値は Step 8 で確定した設定(SENS 3 / Q 5 / LEVEL 5)
    float sens_  = 0.3f;   // 感度 0.0 ～ 1.0
    float q_     = 1.0f;   // フィルタの鋭さ
    float level_ = 0.7f;   // 出力の音量の目標 0.0 ～ 1.0

    // ---- 準備で決まる値(Prepare で計算) ----
    float sample_rate_  = 48000.0f;
    float env_attack_   = 0.0f;  // 包絡線が上がる速さの係数
    float env_release_  = 0.0f;  // 包絡線が下がる速さの係数
    float fc_smooth_    = 0.0f;  // 中心周波数を滑らかにする係数
    float level_smooth_ = 0.0f;  // 音量を滑らかにする係数

    // ---- 覚えておく状態(ブロックをまたぐ) ----
    float env_       = 0.0f;    // 現在の包絡線
    float fc_        = 400.0f;  // 現在の中心周波数 (Hz)
    float low_       = 0.0f;    // フィルタ内部状態 1
    float band_      = 0.0f;    // フィルタ内部状態 2
    float level_cur_ = 1.0f;    // 現在実際に使っている音量(level_ に近づく)
    float f_         = 0.0f;    // フィルタの係数(16 サンプルに 1 回だけ更新)
    int   control_count_ = 0;   // 係数を更新してから、何サンプル経ったか
};