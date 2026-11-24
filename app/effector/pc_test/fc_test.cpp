#include "autowah.hpp"
#include <cstdio>

// 大きさ amp の音を ms ミリ秒流して、fc を表示する
static void Run(AutoWah& wah, float amp, int ms, const char* label)
{
    int n = 48 * ms;           // 48kHz なので 1ms = 48 サンプル
    float buf[128];
    int done = 0;
    while (done < n)
    {
        int c = (n - done > 128) ? 128 : (n - done);
        for (int i = 0; i < c; ++i) buf[i] = amp;
        wah.Process(buf, c);
        done += c;
    }
    std::printf("%-22s env=%.3f  fc=%.1f Hz\n",
                label, wah.Envelope(), wah.CenterFreq());
}

int main()
{
    AutoWah wah;
    wah.Prepare(48000.0f);
    wah.SetSens(3);   // 感度60%

    std::printf("start                  fc=%.1f Hz\n", wah.CenterFreq());
    Run(wah, 0.0f, 100, "silence 100ms");
    Run(wah, 0.3f, 200, "amp 0.3  200ms");
    Run(wah, 0.0f, 500, "silence 500ms");
    return 0;
}