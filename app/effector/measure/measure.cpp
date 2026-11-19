// measure.cpp --- measureモジュールの中身
#include "measure.hpp"

Measure::Measure(IClock& clock, uint32_t budgetUs)
    : m_clock(clock),
      m_budgetUs(budgetUs),
      m_sumUs(0),
      m_blockCount(0),
      m_stats(),     // すべて0で始める
      m_result()     // すべて0で始める
{
}

// 処理の開始時刻を返す
uint64_t Measure::beginBlock()
{
    return m_clock.nowUs();
}

// 処理の終了時に呼ぶ。開始時刻との差が、かかった時間になる
void Measure::endBlock(uint64_t startUs)
{
    uint64_t endUs     = m_clock.nowUs();
    uint32_t elapsedUs = static_cast<uint32_t>(endUs - startUs);

    m_stats.lastUs = elapsedUs;

    if (elapsedUs > m_stats.maxUs) {
        m_stats.maxUs = elapsedUs;
    }

    m_sumUs += elapsedUs;
    m_blockCount++;
    m_stats.avgUs = static_cast<uint32_t>(m_sumUs / m_blockCount);

    if (elapsedUs > m_budgetUs) {
        m_stats.overrunCount++;
    }
}

// ---- ここから下は、まだ中身を作っていない(次のステップ以降) ----
void Measure::start() {}
void Measure::stop() {}
void Measure::process(const float* in, float* out, size_t frames)
{
    (void)in;
    (void)out;
    (void)frames;
}
bool Measure::done() const { return false; }

const ProcessStats& Measure::stats() const { return m_stats; }
const LatencyResult& Measure::result() const { return m_result; }