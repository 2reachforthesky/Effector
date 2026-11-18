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
    unsigned GetDroppedFrames(void);
    unsigned GetOutputQueuedMin(void);
    unsigned GetInsertedFrames(void);

    private:
    CUSBSoundBaseDevice *m_pSound;
    float m_OutBuffer[BLOCK_SIZE + 1];   // ← +1 する（足す1サンプル分の場所）
    unsigned m_nDroppedFrames;
    unsigned m_nInsertedFrames;          // ← 新しく足す
    unsigned m_nOutQueuedMin;
};