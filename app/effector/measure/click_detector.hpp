// click_detector.hpp --- 戻ってきた音の中から、クリック音を見つける
#pragma once

#include <cstddef>
#include <cstdint>

// 検出のしきい値(この大きさを超えたら「クリックが来た」とみなす)
const float kClickDetectThreshold = 0.1f;

class ClickDetector {
public:
    explicit ClickDetector(float threshold);

    // 検出待ちの状態に戻す(サンプルの数え方も0から)
    void reset();

    // 1ブロック(frames個のサンプル)を調べる。見つかっていたら true
    bool feed(const float* in, size_t frames);

    bool     detected() const;   // もう見つかったか
    uint64_t position() const;   // 見つけたサンプルの番号(reset後の最初のサンプルが0番)

private:
    float    m_threshold;
    bool     m_detected;
    uint64_t m_position;
    uint64_t m_samplesSeen;      // これまでに調べたサンプルの数
};