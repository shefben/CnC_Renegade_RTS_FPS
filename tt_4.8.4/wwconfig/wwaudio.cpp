#include "General.h"
#include "mss32.h"
#include "engine_vector.h"
#include "engine_string.h"
char *Driver2D;
long Driver3D;
SimpleDynVecClass<char *> SoundDriverNames;

void Close_3D_Device()
{
	AIL_lock();
	if (Driver3D)
	{
		AIL_close_3D_provider(Driver3D);
		Driver3D = 0;
	}
	AIL_unlock();
}

bool Is_Disabled()
{
	return false;
}

void Close_2D_Device()
{
	AIL_lock();
	Close_3D_Device();
	if (Driver2D)
	{
		AIL_waveOutClose(Driver2D);
		Driver2D = 0;
	}
	AIL_unlock();
}

int Open_2D_Device(LPPCMWAVEFORMAT format)
{
	int retval = 1;
	int retval2;
	AIL_lock();
	Close_2D_Device();
	AIL_set_preference(18, 0);
	AIL_set_preference(15, 0);
	char **D2D = &Driver2D;
	if (!AIL_waveOutOpen(&Driver2D, 0, 0, (LPWAVEFORMAT)format))
	{
		if (!*D2D || *((unsigned int *)*D2D + 42) != 1 )
		{
			AIL_unlock();
			return retval;
		}
		AIL_waveOutClose(*D2D);
		//WWAudio: Detected 2D DirectSound emulation, switching to WaveOut.
	}
	char *error = AIL_last_error();
	error;
	AIL_set_preference(15, 1);
	retval2 = AIL_waveOutOpen(&Driver2D, 0, 0, (LPWAVEFORMAT)format);
	retval = (-(retval2 != 0) & 0xFFFFFFFE) + 2;
	if (!retval2)
	{
		AIL_unlock();
		return retval;
	}
	Close_2D_Device();
	//WWAudio: Error initializing 2D device.
	AIL_unlock();
	return retval;
}

void Open_2D_Device(bool stereo,short quality,unsigned short samplerate)
{
	DWORD val;
	PCMWAVEFORMAT waveformat;
	short i;
	*(unsigned int *)&waveformat.wf.nChannels = (unsigned short)((stereo != 0) + 1);
	waveformat.wf.nSamplesPerSec = samplerate;
	waveformat.wf.wFormatTag = 1;
	waveformat.wf.nBlockAlign = quality * (unsigned short)((stereo != 0) + 1) >> 3;
	waveformat.wf.nAvgBytesPerSec = samplerate * quality * (unsigned int)(unsigned short)((stereo != 0) + 1) >> 3;
	waveformat.wBitsPerSample = quality;
	for (i = quality;!Open_2D_Device(&waveformat);waveformat.wf.nAvgBytesPerSec = quality * val * waveformat.wf.nChannels >> 3 )
	{
		if (waveformat.wf.nSamplesPerSec < 11025)
		{
			break;
		}
		val = waveformat.wf.nSamplesPerSec >> 1;
		waveformat.wf.nSamplesPerSec >>= 1;
		waveformat.wf.nBlockAlign = quality * waveformat.wf.nChannels >> 3;
	}
}

void Build_3D_Driver_List()
{
	AIL_lock();
	long m_Next = 0;
	long m_Dest = 0;
	char *m_pName = 0;
	while (AIL_enumerate_3D_providers(&m_Next, &m_Dest, (char **)&m_pName))
	{
		if (AIL_open_3D_provider(m_Dest))
		{
			//WWAudio: Unable to open driver
			char *error = AIL_last_error();
			error;
		}
		else
		{
			SoundDriverNames.Add(newstr(m_pName));
			AIL_close_3D_provider(m_Dest);
		}
	}
	AIL_unlock();
}

void Free_3D_Driver_List()
{
	AIL_lock();
	for (int i = 0;i < SoundDriverNames.Count();i++)
	{
		delete[] SoundDriverNames[i];
	}
	if (Driver3D)
	{
		AIL_close_3D_provider(Driver3D);
		Driver3D = 0;
	}
	AIL_unlock();
}

void Initialize_Audio()
{
	AIL_startup();
	if (!Is_Disabled())
	{
		Open_2D_Device(1,16,44100);
		Build_3D_Driver_List();
	}
}

void Shutdown_Audio()
{
	Free_3D_Driver_List();
	Close_2D_Device();
	AIL_shutdown();
}
