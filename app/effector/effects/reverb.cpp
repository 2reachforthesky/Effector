#include "reverb.hpp"
static const float kWetGain[5] = {0.5f,1.0f,1.5f,2.0f,3.0f};

Reverb::Reverb() : strength_(2)
{
}
const char* Reverb::Name()const{


    return "Reverb";
}
void Reverb::Prepare(float sample_rate)
{
    (void)sample_rate;
    comb0_.Clear();
    comb1_.Clear();
    comb2_.Clear();
    comb3_.Clear();
    ap0_.Clear();
    ap1_.Clear();
}

void Reverb::Process(float* samples, int count)
{
    for (int i = 0; i < count; i++)
    {
        float dry = samples[i];
        float in = samples[i] * 0.03f;
        float sum = comb0_.Process(in) + comb1_.Process(in) + comb2_.Process(in) + comb3_.Process(in);
        float wet = ap0_.Process(sum);
                 wet = ap1_.Process(wet); 
        samples[i] = dry + wet * kWetGain[strength_];           
    }

}

void Reverb::SetStrength(int level)
{

    if(level  < 0 )
    {
       level = 0;
    }
    if(level  > 4 )
    {
        
       level = 4;
    }
       strength_ = level;
}