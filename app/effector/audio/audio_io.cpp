#include "audio_io.hpp"

CAudioIO::CAudioIO(void)
    : m_pSound(0),
      m_nDroppedFrames(0),
      m_nInsertedFrames(0),
      m_nOutQueuedMin((unsigned) -1)
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

    if (nQueued < m_nOutQueuedMin)
    {
        m_nOutQueuedMin = nQueued;
    }

        // 1サンプル足すこともあるので、BLOCK_SIZE + 1 分の空きが必要
    if (m_pSound->GetQueueSizeFrames () - nQueued < BLOCK_SIZE + 1)
    {
        return false;
    }

    unsigned nFrames = BLOCK_SIZE;
    if (nQueued > OUTPUT_MAX_QUEUED_FRAMES)
    {
        // 溜まりすぎ：最後の1サンプルを捨てる
        nFrames = BLOCK_SIZE - 1;
        m_nDroppedFrames++;
    }
    else if (nQueued < OUTPUT_MIN_QUEUED_FRAMES)
    {
        // 少なすぎ：最後のサンプルをもう一度書いて1サンプル足す
        nFrames = BLOCK_SIZE + 1;
        m_nInsertedFrames++;
    }

    for (unsigned i = 0; i < BLOCK_SIZE; ++i)
    {
        m_OutBuffer[i] = buffer[i];
        if (m_OutBuffer[i] > 1.0f) m_OutBuffer[i] = 1.0f;
        else if (m_OutBuffer[i] < -1.0f) m_OutBuffer[i] = -1.0f;
    }
    m_OutBuffer[BLOCK_SIZE] = m_OutBuffer[BLOCK_SIZE - 1];

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

unsigned CAudioIO::GetOutputQueuedMin(void)
{
    unsigned nMin = m_nOutQueuedMin;
    m_nOutQueuedMin = (unsigned) -1;
    return nMin;
}

unsigned CAudioIO::GetInsertedFrames(void)
{
    return m_nInsertedFrames;
}

void CAudioIO::GetTXStats (unsigned *pPackets,
                           unsigned *pFrames,
                           unsigned *pErroes,
                           unsigned *pMinFrames,
                           unsigned *pMaxFrames)
{
    *pPackets = *pFrames = *pErroes = *pMinFrames = *pMaxFrames = 0;
    if (m_pSound)
    {
        m_pSound->GetTXStats (pPackets, pFrames, pErroes, pMinFrames, pMaxFrames);
    }
}