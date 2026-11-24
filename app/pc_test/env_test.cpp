#include "autowah.hpp"
#include <cstdio>

int main()
{
    AutoWah wah;
    wah.Prepare(48000.0f);

    // 一定の大きさ 0.5 の音を 480 サンプル(10ms)流す
    float buf[480];
    for (int i = 0; i < 480; ++i) buf[i] = 0.5f;

    // 240 サンプル(= アタック時定数 5ms)ごとに見る
    wah.Process(buf, 240);
    std::printf("5ms  後 : %.4f\n", wah.Envelope());
    wah.Process(buf + 240, 240);
    std::printf("10ms 後 : %.4f\n", wah.Envelope());

    // 無音を 4800 サンプル(100ms = リリース時定数)流す
    float silence[4800] = {};
    wah.Process(silence, 4800);
    std::printf("無音100ms後: %.4f\n", wah.Envelope());

    // 素通しか確認
    float t[2] = { 0.3f, -0.3f };
    wah.Process(t, 2);
    std::printf("素通し: %.1f %.1f\n", t[0], t[1]);
    return 0;
}