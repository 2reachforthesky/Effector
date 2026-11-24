#include "autowah.hpp"
#include <cstdio>
#include <cmath>

static const double PI = 3.14159265358979;

// 周波数 freq・振幅 amp の正弦波を ms ミリ秒流して、出力の rms を返す
static double Feed(AutoWah& w, float freq, float amp, int ms, long& pos)
{
    const int n = 48 * ms;
    double sum = 0.0;
    float buf[1];
    for (int i = 0; i < n; ++i)
    {
        buf[0] = amp * (float)std::sin(2.0 * PI * freq * (pos + i) / 48000.0);
        w.Process(buf, 1);
        sum += buf[0] * buf[0];
    }
    pos += n;
    return std::sqrt(sum / n);
}

int main()
{
    // A) SENS 1～5: 弱い音(振幅0.1)で、中心周波数がどこまで上がるか
    for (int s = 1; s <= 5; ++s)
    {
        AutoWah w;
        w.Prepare(48000.0f);
        w.SetSens(s);
        long pos = 0;
        Feed(w, 1000.0f, 0.1f, 500, pos);
        std::printf("SENS  stage%d : fc=%.0f Hz\n", s, w.CenterFreq());
    }

    // B) LEVEL 1～5: 出力の大きさ(2500Hz, 振幅0.3)
    for (int l = 1; l <= 5; ++l)
    {
        AutoWah w;
        w.Prepare(48000.0f);
        w.SetLevel(l);
        long pos = 0;
        Feed(w, 2500.0f, 0.3f, 500, pos);
        double r = Feed(w, 2500.0f, 0.3f, 500, pos);
        std::printf("LEVEL stage%d : out-rms=%.3f\n", l, r);
    }

    // C) LEVEL を 1 → 5 に切り替えた直後の出力の大きさ(1ms ごと)
    {
        AutoWah w;
        w.Prepare(48000.0f);
        w.SetLevel(1);
        long pos = 0;
        Feed(w, 2500.0f, 0.3f, 500, pos);
        double b  = Feed(w, 2500.0f, 0.3f, 1, pos);
        w.SetLevel(5);
        double a1 = Feed(w, 2500.0f, 0.3f, 1, pos);
        double a2 = Feed(w, 2500.0f, 0.3f, 1, pos);
        double a3 = Feed(w, 2500.0f, 0.3f, 10, pos);
        std::printf("SWITCH 1->5   : before=%.3f  +0-1ms=%.3f  +1-2ms=%.3f  +2-12ms=%.3f\n",
                    b, a1, a2, a3);
    }
    return 0;
}