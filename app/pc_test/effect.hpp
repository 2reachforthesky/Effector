// effect.hpp - エフェクトの約束事
//
// すべてのエフェクトは、このクラスを受け継いで作る。
//
// 音の表し方:
//   - 1 個のサンプルは float で、範囲は -1.0 ～ +1.0
//   - モノラル (1 チャンネル)
//   - 128 個くらいをまとめて Process() に渡す
//
// Process() の中でやってはいけないこと:
//   - 時間のかかる処理
//   - new や delete
//   - printf などの表示
#pragma once

class Effect
{
public:
    virtual ~Effect() = default;

    // エフェクトの名前を返す
    virtual const char* Name() const = 0;

    // 使い始める前に 1 回だけ呼ばれる。サンプリング周波数 (例: 48000) が渡される
    virtual void Prepare(float sample_rate) = 0;

    // 音の処理。samples[0] ～ samples[count - 1] を書き換える
    virtual void Process(float* samples, int count) = 0;
};
