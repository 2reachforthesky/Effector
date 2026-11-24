#pragma once
#include "../effects/effect.hpp"
//役割：エフェクトを登録順に束ねて、オンのものだけを順にかける。
//使う順番：Add を全部済ませる → Prepare を1回呼ぶ → Process を繰り返し呼ぶ。 
//番号の決まり：Add した順が、処理する順番で、SetEnabled の id（0から始まる）にもなる。
//上限：登録できるのは最大8個。
//起動時の状態：全部オフで、素通し。
//注意：Process の中では、new、delete、printf を使わない。
//Add は、上限（8個）を超えるか、nullptr を渡すと、false を返す（登録されない）。
//Prepare は、オフのエフェクトも含めて、登録されている全部に対して呼ばれる。

class EffectChain{
  
    public:
        void Prepare(float sample_rate);
        void Process(float* samples, int count);
        EffectChain ();
            bool Add(Effect* effect);
        void SetEnabled(int id,bool on);    
         

    private:
        
        static const int kMaxEffects = 8;
            Effect* effects_[kMaxEffects];
            bool enabled_[kMaxEffects];
            int count_;
};