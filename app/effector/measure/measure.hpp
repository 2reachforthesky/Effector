//measure.hpp ---measure(測定)モジュールの「目次」
#pragma once
#include <cstddef> // size_tを使うため  
#include <cstdint> // uint32_tを使うため
#include "click.hpp" //クリック音の作成
#include "click_detector.hpp" //クリック音の検出

//今の時刻をマイクロ秒(1/1,000,000秒)で返す
class IClock{
    public:
        virtual ~IClock() {}
        virtual uint64_t nowUs() = 0;
};

//1ブロックの処理時間の統計
struct ProcessStats{
    uint32_t lastUs; //直近の処理時間
    uint32_t maxUs; //最大処理時間
    uint32_t avgUs; //平均処理時間
    uint32_t overrunCount; //制限時間を超えた回数
};

//往復遅延の測定結果
struct LatencyResult{
    uint32_t count; //測定回数
    uint32_t timeoutCount; //戻ってこれなかった回数
    float minUs; //最小往復遅延
    float maxUs; //最大往復遅延
    float avgUs; //平均往復遅延
    float jitterUs; //ジッタ(最大-最小)
};

    //往復遅延の測定設定(後々変更可能)
    const uint32_t kMeasureTrials = 20; //測定回数
    const uint32_t kMeasureTimeoutSamples = 24000; //戻りを待つ最大の長さ(0.5秒)
    const uint32_t kMeasureGapBlocks = 16; //測定と測定の間の無音(16ブロック = 約43ミリ秒)

class Measure {
    //budgetUs: 1ブロックの処理時間の制限(マイクロ秒)
    public:
        explicit Measure(IClock& clock,uint32_t budgetUs = 2666);

        //常時：1ブロックの処理時間を測る
        uint64_t beginBlock();
        void endBlock(uint64_t startUs);
        
        //測定状態：クリック音の往復を測る
        void start(); //測定開始
        void stop(); //測定終了
        void process(const float*in, float*out, size_t frames); //1ブロック分進める
        bool done() const; //測定が終わったかどうか

        const ProcessStats& stats() const;
        const LatencyResult& result() const;

    private:
        //測定の状態
        enum State{
            kIdle, //測定していない
            kGap, //測定の間の無音
            kWaiting, //クリック音の戻りを待っている
            kDone, //測定が終わった
        };

        void recordLatency(uint64_t position); //検出できた1回を記録
        void finishTrial(); //1回の測定を終える

        IClock& m_clock;
        uint32_t m_budgetUs; //制限時間
        uint64_t m_sumUs; //処理時間の合計
        uint32_t m_blockCount; //測った回数
        ProcessStats m_stats; //処理時間の統計
        LatencyResult m_result; //往復遅延の結果

        // ----クリック往復の測定に使うもの----
        State m_state; //測定の状態
        float m_click[kClickLength]; //クリック音の作成
        uint64_t      m_riseOffset;           // クリックがしきい値を超えるまでのサンプル数
        ClickDetector m_detector;
        uint32_t      m_trial;                // 何回目まで終わったか
        uint32_t      m_gapLeft;              // 残りの無音ブロック数
        uint32_t      m_waitedSamples;        // 戻りを待った長さ(サンプル)
        uint64_t      m_minSamples;           // 遅れ(サンプル)の最小
        uint64_t      m_maxSamples;           // 遅れ(サンプル)の最大
        uint64_t      m_sumSamples;           // 遅れ(サンプル)の合計

};


