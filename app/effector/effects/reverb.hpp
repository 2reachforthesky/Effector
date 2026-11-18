#pragma once
#include "effect.hpp"

template <int N>
class Comb{
    public:
    void Clear()
    {
        for (int i = 0; i < N; i++)
        {
          buf_[i] = 0.0f;
        }
        idx_ = 0 ;
    }
    Comb()
    {
        idx_ = 0;
        feedback_ = 0.84f;
        Clear(); 
    }
    float Process(float x)
    {
        float y = buf_[idx_ ];
        buf_[idx_ ] = x + y * feedback_;
        idx_++;
        
        if( idx_ >= N)
        {
            idx_ = 0;
        }
        return y;

    }
    private:
       int idx_;
       float feedback_;
       float buf_[N];
        
};
template <int N>
class Allpass{
    public:
    void Clear()
    {
        for (int i = 0; i < N; i++)
        {
          buf_[i] = 0.0f;
        }
        idx_ = 0 ;
    }
    Allpass()
    {
        idx_ = 0;
        feedback_ = 0.5f;
        Clear(); 
    }
    float Process(float x)
    {
        float old = buf_[idx_ ];
        float out = old - x;
        buf_[idx_ ] = x + old * feedback_;
        idx_++;
        
        if( idx_ >= N)
        {
            idx_ = 0;
        }
        return out;

    }
    private:
       int idx_;
       float feedback_;
       float buf_[N];
        
};
class Reverb : public Effect
{
public:
    Reverb();
    const char* Name() const override;
    void Prepare(float sample_rate) override;                     
    void Process(float *samples, int count) override;  
    void SetStrength(int level);     

private:
    int strength_;
    Comb<1215> comb0_;
    Comb<1293> comb1_;
    Comb<1390> comb2_;
    Comb<1476> comb3_;
    Allpass<605> ap0_;
    Allpass<480> ap1_;
    
};