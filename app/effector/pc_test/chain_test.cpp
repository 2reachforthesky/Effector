#include <cstdio>
#include "../chain/effect_chain.hpp"

class Times2 : public Effect
{
public:
    const char* Name() const override { return "Times2"; }

    void Prepare(float) override {}

    void Process(float* samples, int count) override
    {
        for (int i = 0; i < count; ++i)
        {
            samples[i] = samples[i] * 2.0f; // 音量を 2 倍にする
        }
    }
};
class Plus1 : public Effect
{
public:
    const char* Name() const override { return "Plus1"; }

    void Prepare(float) override {}

    void Process(float* samples, int count) override
    {
        for (int i = 0; i < count; ++i)
        {
            samples[i] = samples[i] + 1.0f; // 音量を 2 倍にする
        }
    }
};

int main (){

    Times2 t2;
    Plus1 p1;
    EffectChain chain;

    chain.Add(&t2);
    chain.Add(&p1);
    chain.Prepare(48000.0f);
    float buf[1] = {};

    buf[0] = 0.5f;
    chain.Process(buf,1);
    printf("A all off: %f\n", buf[0]);

    chain.SetEnabled(0, true);
    buf[0] = 0.5f;
    chain.Process(buf,1);
    printf("B all off: %f\n", buf[0]);

    chain.SetEnabled(0, false); 
    chain.SetEnabled(1, true);
    buf[0] = 0.5f;
    chain.Process(buf,1);
    printf("C all off: %f\n", buf[0]);

    chain.SetEnabled(0, true);
    buf[0] = 0.5f;
    chain.Process(buf,1);
    printf("D both on: %f\n", buf[0]);

    chain.SetEnabled(5, true);
    chain.SetEnabled(-1, true);
    buf[0] = 0.5f;
    chain.Process(buf,1);
    printf("E of out range: %f\n", buf[0]);

    printf("F %d\n", chain.Add(nullptr));
   
    return 0;
}

