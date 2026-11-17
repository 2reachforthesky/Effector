#pragma once

#include "../config.hpp"
#include <circle/sound/usbsoundbasedevice.h>

class CAudioIO
{
public:
    CAudioIO(void);
    ~CAudioIO();
    bool Initialize(void);
    bool ReadAudioData(float* buffer);
    bool WriteAudioData(const float* buffer);
    bool IsConnected(void);
    unsigned GetInputQueued(void);
    unsigned GetOutputQueued(void);
    
private:
    CUSBSoundBaseDevice *m_pSound;
    float m_OutBuffer[BLOCK_SIZE];
};