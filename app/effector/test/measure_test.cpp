// measure_test.cpp --- 偽の時計で、処理時間の統計をテストする
#include "measure.hpp"
#include <cstdio>

// 偽の時計: 私たちが好きなだけ時刻を進められる
class FakeClock : public IClock {
public:
    FakeClock() : m_now(0) {}
    uint64_t nowUs() override { return m_now; }
    void advance(uint64_t us) { m_now += us; }
private:
    uint64_t m_now;
};

static int g_failed = 0;

// 結果が正しいか表示する。間違っていたら失敗の数を増やす
static void check(bool ok, const char* name)
{
    std::printf("%s : %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) {
        g_failed++;
    }
}

// 「us マイクロ秒かかったブロック処理」を1回ぶん再現する
static void runBlock(Measure& m, FakeClock& c, uint64_t us)
{
    uint64_t t0 = m.beginBlock();
    c.advance(us);
    m.endBlock(t0);
}

int main()
{
    FakeClock clock;
    Measure measure(clock, 2666);

    // run1: 1000マイクロ秒(制限内)
    runBlock(measure, clock, 1000);
    check(measure.stats().lastUs == 1000,      "run1 lastUs = 1000");
    check(measure.stats().maxUs == 1000,       "run1 maxUs = 1000");
    check(measure.stats().avgUs == 1000,       "run1 avgUs = 1000");
    check(measure.stats().overrunCount == 0,   "run1 overrunCount = 0");

    // run2: 3000マイクロ秒(制限オーバー)
    runBlock(measure, clock, 3000);
    check(measure.stats().lastUs == 3000,      "run2 lastUs = 3000");
    check(measure.stats().maxUs == 3000,       "run2 maxUs = 3000");
    check(measure.stats().avgUs == 2000,       "run2 avgUs = 2000");
    check(measure.stats().overrunCount == 1,   "run2 overrunCount = 1");

    // run3: 500マイクロ秒(制限内)
    runBlock(measure, clock, 500);
    check(measure.stats().lastUs == 500,       "run3 lastUs = 500");
    check(measure.stats().maxUs == 3000,       "run3 maxUs = 3000");
    check(measure.stats().avgUs == 1500,       "run3 avgUs = 1500");
    check(measure.stats().overrunCount == 1,   "run3 overrunCount = 1");

    return g_failed == 0 ? 0 : 1;
}