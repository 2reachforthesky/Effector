//measure.hpp ---measure(測定)モジュールの「目次」
#pragma once
#include <cstddef> // size_tを使うため  
#include <cstdint> // uint32_tを使うため

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
    float jitterUs; //ジッタ(ばらつき)の平均値
};

class Measure {
    public:
        explicit Measure(IClock& clock,uint32_t budgetUs = 2666);

        //常時：1ブロックの処理時間を測る
        uint64_t beginBlock();
        void endBlock(uint64_t startUs);
        
        //測定状態：クリック音の往復を測る
        void start();
        void stop();
        void process(const float*in, float*out, size_t frames);
        bool done() const;

        const ProcessStats& stats() const;
        const LatencyResult& result() const;

    private:
        IClock& m_clock;
        uint32_t m_budgetUs; //制限時間
        uint64_t m_sumUs; //処理時間の合計
        uint32_t m_blockCount; //測った回数
        ProcessStats m_stats; //処理時間の統計
        LatencyResult m_result; //往復遅延の結果

};


