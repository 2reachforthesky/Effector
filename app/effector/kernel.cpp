//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include <circle/util.h>
#include <assert.h>
#include "config.hpp"

static const char FromKernel[] = "kernel";

CKernel::CKernel (void) :
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, FALSE),
	m_AudioIO ()
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Serial;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

	if (bOK)
	{
		bOK = m_AudioIO.Initialize ();
	}else
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot initialize audio I/O");
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	// copy sound data
	for (unsigned nCount = 0; 1; nCount++)
	{
	float buffer[BLOCK_SIZE];
		if (m_AudioIO.ReadAudioData (buffer))
		{
			if (m_AudioIO.WriteAudioData (buffer))
			{
				// success
			}
			else
			{
				m_Logger.Write (FromKernel, LogWarning, "Sound output queue full");
			}
		}
		else
		{
			m_Logger.Write (FromKernel, LogWarning, "Sound data dropped");
		}

		m_Scheduler.Yield ();
	}

	return ShutdownHalt;
}
