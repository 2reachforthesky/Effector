#include "effect_chain.hpp"

void EffectChain::Prepare(float sample_rate){

for(int i = 0; i < count_; i++){

    effects_[i] -> Prepare (sample_rate);

}

}

void EffectChain::Process(float* samples, int count){

    for( int i = 0; i < count_; i++){

        if(enabled_[i]){
    
        effects_[i] ->Process(samples,count);

        }
    }
}

EffectChain::EffectChain() : count_(0){

    for(int i = 0; i < kMaxEffects ;i++){

        effects_[i] = nullptr; 
        enabled_[i] = false;

}
}
bool EffectChain::Add(Effect*effect){

     if(effect == nullptr || count_ >= kMaxEffects ){

         return false;

     }
        effects_[count_] = effect;
        enabled_[count_] = false;
        count_++;
        return true;

}
void EffectChain::SetEnabled(int id, bool on){

    if(id < 0 || id >= count_){
        
        return;   
    
    }

    enabled_[id] = on;    
    
}


        

