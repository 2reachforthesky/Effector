#include "audio_io.hpp"

CAudioIO::CAudioIO(void)
    : m_pSound(0),
      m_nDroppedFrames(0)
{

}

CAudioIO::~CAudioIO()
{
    delete m_pSound;
    m_pSound = 0;
}

bool CAudioIO::Initialize(void)
{
    m_pSound = new CUSBSoundBaseDevice(SAMPLE_RATE, CUSBSoundBaseDevice::DeviceModeTXRX);
    if (!m_pSound)
    {
        return false;
    }

    if (!m_pSound->AllocateReadQueue(QUEUE_SIZE_MSECS))
    {
        return false;
    }

    m_pSound->SetReadFormat(SoundFormatFloat32, 1);

    if (!m_pSound->AllocateQueue(QUEUE_SIZE_MSECS))
    {
        return false;
    }

    m_pSound->SetWriteFormat(SoundFormatFloat32, 1);

    float tempBuffer[BLOCK_SIZE];
    for (unsigned j = 0; j < BLOCK_SIZE; ++j)
    {
        tempBuffer[j] = 0.0f;
    }
    
    for (unsigned i = 0; i < OUTPUT_PREFILL_BLOCKS; ++i)
    {
        if (!WriteAudioData(tempBuffer))
        {
            return false;
        }
    }

    if (!m_pSound->Start())
    {
        return false;
    }

    return true;
}

bool CAudioIO::ReadAudioData (float *buffer)
{
    const int nBytes = (int) (BLOCK_SIZE * sizeof (float));

    if (!m_pSound)
    {
        return false;
    }

    if (m_pSound->GetReadQueueFramesAvail () < BLOCK_SIZE)
    {
        return false;
    }

    return m_pSound->Read (buffer, nBytes) == nBytes;
}

bool CAudioIO::WriteAudioData(const float* buffer)
{
    if (!m_pSound)
    {
        return false;
    }

    unsigned nQueued = m_pSound->GetQueueFramesAvail ();

    if (m_pSound->GetQueueSizeFrames () - nQueued < BLOCK_SIZE)
    {
        return false;
    }

    // 出力キューに溜まりすぎていたら、このブロックの最後の1サンプルを捨てる
    unsigned nFrames = BLOCK_SIZE;
    if (nQueued > OUTPUT_MAX_QUEUED_FRAMES)
    {
        nFrames = BLOCK_SIZE - 1;
        m_nDroppedFrames++;
    }

    for (unsigned i = 0; i < BLOCK_SIZE; ++i)
    {
        m_OutBuffer[i] = buffer[i];
        if (m_OutBuffer[i] > 1.0f) m_OutBuffer[i] = 1.0f;
        else if (m_OutBuffer[i] < -1.0f) m_OutBuffer[i] = -1.0f;
    }

    const int nBytes = (int) (nFrames * sizeof (float));
    return m_pSound->Write (m_OutBuffer, nBytes) == nBytes;
}

unsigned CAudioIO::GetInputQueued(void)
{
    if (!m_pSound)
    {
        return 0;
    }

    return m_pSound->GetReadQueueFramesAvail();
}

unsigned CAudioIO::GetOutputQueued(void)
{
    if (!m_pSound)
    {
        return 0;
    }

    return m_pSound->GetQueueFramesAvail();
}

unsigned CAudioIO::GetDroppedFrames(void)
{
    return m_nDroppedFrames;
}