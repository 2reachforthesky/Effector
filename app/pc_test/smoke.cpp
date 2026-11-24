#include "autowah.hpp"
#include <cstdio>

int main()
{
    AutoWah wah;
    wah.Prepare(48000.0f);
    wah.SetSens(3);
    wah.SetQ(0);      // 範囲外 → 1 に寄せられる
    wah.SetLevel(99); // 範囲外 → 5 に寄せられる

    float buf[4] = { 0.1f, -0.2f, 0.3f, -0.4f };
    wah.Process(buf, 4);

    std::printf("%s: %.1f %.1f %.1f %.1f\n",
                wah.Name(), buf[0], buf[1], buf[2], buf[3]);
    return 0;
}