#include "autowah.hpp"
#include <cstdio>
#include <cmath>

// 周波数 freq・振幅 amp の正弦波を 1 秒流し、後半 0.5 秒の出力の大きさを測る
static void Run(float freq, float amp, int q_stage, const char* label)
{
    AutoWah w;
    w.Prepare(48000.0f);
    w.SetSens(3);
    w.SetQ(q_stage);
    w.SetLevel(5);          // 音量 100% で測る

    const int N = 48000;
    float buf[128];
    double sum = 0.0;
    int cnt = 0;
    float peak = 0.0f;

    for (int n = 0; n < N; n += 128)
    {
        for (int i = 0; i < 128; ++i)
            buf[i] = amp * std::sin(2.0 * 3.14159265 * freq * (n + i) / 48000.0);

        w.Process(buf, 128);

        if (n > 24000)      // 立ち上がりが落ち着いた後半だけ測る
        {
            for (int i = 0; i < 128; ++i)
            {
                sum += buf[i] * buf[i];
                ++cnt;
                if (std::fabs(buf[i]) > peak) peak = std::fabs(buf[i]);
            }
        }
    }
    std::printf("%-8s Qstage=%d  in-rms=%.3f  out-rms=%.3f  out-peak=%.3f  fc=%.0f\n",
                label, q_stage, amp * 0.7071f, std::sqrt(sum / cnt), peak, w.CenterFreq());
}

int main()
{
    for (int q = 1; q <= 5; q += 2)
    {
        Run(100.0f,  0.3f, q, "100Hz");
        Run(500.0f,  0.3f, q, "500Hz");
        Run(2500.0f, 0.3f, q, "2500Hz");
    }
    return 0;
}