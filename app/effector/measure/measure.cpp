// measure.cpp --- measureモジュールの中身
#include "measure.hpp"

Measure::Measure(IClock& clock, uint32_t budgetUs)
    : m_clock(clock),
      m_budgetUs(budgetUs),
      m_sumUs(0),
      m_blockCount(0),
      m_stats(),
      m_result(),
      m_state(kIdle),
      m_click(),
      m_riseOffset(0),
      m_detector(kClickDetectThreshold),
      m_trial(0),
      m_gapLeft(0),
      m_waitedSamples(0),
      m_minSamples(0),
      m_maxSamples(0),
      m_sumSamples(0)
{
    // 出すクリック音を作る
    generateClick(m_click, kClickLength, kClickFreqHz, kClickSampleRate, kClickAmplitude);

    // クリック音そのものを検出器にかけて、「しきい値を超える位置」を調べておく
    ClickDetector probe(kClickDetectThreshold);
    probe.feed(m_click, kClickLength);
    if (probe.detected()) {
        m_riseOffset = probe.position();
    }
}

// ---------- 処理時間の統計 ----------

uint64_t Measure::beginBlock()
{
    return m_clock.nowUs();
}

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

// ---------- クリック往復の測定 ----------

void Measure::start()
{
    m_result        = LatencyResult();   // 前回の結果を消す
    m_trial         = 0;
    m_waitedSamples = 0;
    m_minSamples    = 0;
    m_maxSamples    = 0;
    m_sumSamples    = 0;
    m_gapLeft       = kMeasureGapBlocks; // 最初も、少し無音を置いてから始める
    m_state         = kGap;
}

void Measure::stop()
{
    m_state = kIdle;
}

bool Measure::done() const
{
    return m_state == kDone;
}

void Measure::process(const float* in, float* out, size_t frames)
{
    if (out == nullptr) {
        return;
    }

    // 基本は無音を出す(測定中は、入力をそのまま出力に流さない)
    for (size_t i = 0; i < frames; i++) {
        out[i] = 0.0f;
    }

    if (m_state == kIdle || m_state == kDone) {
        return;
    }

    // 入力がない、またはブロックがクリックより短いときは、測定できないので終了する
    if (in == nullptr || frames < kClickLength) {
        m_state = kDone;
        return;
    }

    if (m_state == kGap) {
        if (m_gapLeft > 0) {
            m_gapLeft--;
            return;
        }

        // 無音の時間が終わった: このブロックの先頭からクリックを出す
        for (size_t i = 0; i < kClickLength; i++) {
            out[i] = m_click[i];
        }
        m_detector.reset();       // 「クリックを出したブロックの先頭」が、数え始めの0番になる
        m_waitedSamples = 0;
        m_state = kWaiting;
    }

    // 戻りを待つ(クリックを出したブロックを含めて、入力を調べる)
    m_detector.feed(in, frames);
    m_waitedSamples += static_cast<uint32_t>(frames);

    if (m_detector.detected()) {
        recordLatency(m_detector.position());
        finishTrial();
    } else if (m_waitedSamples > kMeasureTimeoutSamples) {
        m_result.timeoutCount++;
        finishTrial();
    }
}

void Measure::recordLatency(uint64_t position)
{
    // 検出位置から「立ち上がりオフセット」を引くと、本当の遅れ(サンプル数)になる
    uint64_t samples = (position > m_riseOffset) ? (position - m_riseOffset) : 0;

    if (m_result.count == 0 || samples < m_minSamples) {
        m_minSamples = samples;
    }
    if (m_result.count == 0 || samples > m_maxSamples) {
        m_maxSamples = samples;
    }
    m_sumSamples += samples;
    m_result.count++;

    // サンプル数を、マイクロ秒に換算する(48kHzなら1サンプル約20.8マイクロ秒)
    const float usPerSample = 1000000.0f / kClickSampleRate;

    m_result.minUs    = static_cast<float>(m_minSamples) * usPerSample;
    m_result.maxUs    = static_cast<float>(m_maxSamples) * usPerSample;
    m_result.avgUs    = static_cast<float>(m_sumSamples) / static_cast<float>(m_result.count) * usPerSample;
    m_result.jitterUs = m_result.maxUs - m_result.minUs;
}

void Measure::finishTrial()
{
    m_trial++;

    if (m_trial >= kMeasureTrials) {
        m_state = kDone;
    } else {
        m_gapLeft = kMeasureGapBlocks;
        m_state   = kGap;
    }
}

// ---------- 結果の取り出し ----------

const ProcessStats& Measure::stats() const { return m_stats; }
const LatencyResult& Measure::result() const { return m_result; }