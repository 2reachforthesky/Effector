// click_detector.cpp --- クリック音の検出
#include "click_detector.hpp"

ClickDetector::ClickDetector(float threshold)
    : m_threshold(threshold),
      m_detected(false),
      m_position(0),
      m_samplesSeen(0)
{
}

void ClickDetector::reset()
{
    m_detected    = false;
    m_position    = 0;
    m_samplesSeen = 0;
}

bool ClickDetector::feed(const float* in, size_t frames)
{
    if (in == nullptr) {
        return m_detected;
    }

    for (size_t i = 0; i < frames; i++) {
        float v = in[i];

        // 最初にしきい値を超えた場所を記録する(プラス側でもマイナス側でもよい)
        if (!m_detected && (v > m_threshold || v < -m_threshold)) {
            m_detected = true;
            m_position = m_samplesSeen + i;
        }
    }

    m_samplesSeen += frames;   // 見つかったあとも、数え続ける
    return m_detected;
}

bool ClickDetector::detected() const { return m_detected; }
uint64_t ClickDetector::position() const { return m_position; }