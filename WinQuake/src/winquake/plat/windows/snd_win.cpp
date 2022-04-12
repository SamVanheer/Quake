/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"
#include "winquake.h"

// 64K is > 1 second at 16-bit, 22050 Hz
#define	WAV_BUFFERS				64
#define	WAV_MASK				0x3F
#define	WAV_BUFFER_SIZE			0x0400
#define SECONDARY_BUFFER_SIZE	0x10000

static qboolean	wavonly;
static qboolean	wav_init;
static qboolean	snd_firsttime = true, snd_iswave;
static qboolean	primary_format_set;

static int	sample16;
static int	snd_sent, snd_completed;


/* 
 * Global variables. Must be visible to window-procedure function 
 *  so it can unlock and free the data block after it has been played. 
 */ 

HANDLE		hData;
LPVOID		lpData;
HPSTR		lpData2;

HGLOBAL		hWaveHdr;
LPWAVEHDR	lpWaveHdr;

HWAVEOUT    hWaveOut; 

WAVEOUTCAPS	wavecaps;

MMTIME		mmstarttime;

qboolean SNDDMA_InitWav (void);


/*
==================
S_BlockSound
==================
*/
void S_BlockSound (void)
{

// DirectSound takes care of blocking itself
	if (snd_iswave)
	{
		snd_blocked++;

		if (snd_blocked == 1)
		{
			waveOutReset (hWaveOut);
		}
	}
}


/*
==================
S_UnblockSound
==================
*/
void S_UnblockSound (void)
{

// DirectSound takes care of blocking itself
	if (snd_iswave)
	{
		snd_blocked--;
	}
}


/*
==================
FreeSound
==================
*/
void FreeSound (void)
{
	int		i;

	if (hWaveOut)
	{
		waveOutReset (hWaveOut);

		if (lpWaveHdr)
		{
			for (i=0 ; i< WAV_BUFFERS ; i++)
				waveOutUnprepareHeader (hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR));
		}

		waveOutClose (hWaveOut);

		if (hWaveHdr)
		{
			GlobalUnlock(hWaveHdr); 
			GlobalFree(hWaveHdr);
		}

		if (hData)
		{
			GlobalUnlock(hData);
			GlobalFree(hData);
		}

	}

	hWaveOut = 0;
	hData = 0;
	hWaveHdr = 0;
	lpData = NULL;
	lpWaveHdr = NULL;
	wav_init = false;
}

/*
==================
SNDDM_InitWav

Crappy windows multimedia base
==================
*/
qboolean SNDDMA_InitWav (void)
{
	WAVEFORMATEX  format; 
	int				i;
	HRESULT			hr;
	
	snd_sent = 0;
	snd_completed = 0;

	shm = &sn;

	shm->channels = 2;
	shm->samplebits = 16;
	shm->speed = 11025;

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = shm->channels;
	format.wBitsPerSample = shm->samplebits;
	format.nSamplesPerSec = shm->speed;
	format.nBlockAlign = format.nChannels
		*format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec
		*format.nBlockAlign; 
	
	/* Open a waveform device for output using window callback. */ 
	while ((hr = waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER, 
					&format, 
					0, 0L, CALLBACK_NULL)) != MMSYSERR_NOERROR)
	{
		if (hr != MMSYSERR_ALLOCATED)
		{
			Con_SafePrintf ("waveOutOpen failed\n");
			return false;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another app.\n\n"
					    "Select Retry to try to start sound again or Cancel to run Quake with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			Con_SafePrintf ("waveOutOpen failure;\n"
							"  hardware already in use\n");
			return false;
		}
	} 

	/* 
	 * Allocate and lock memory for the waveform data. The memory 
	 * for waveform data must be globally allocated with 
	 * GMEM_MOVEABLE and GMEM_SHARE flags. 

	*/ 
	unsigned int gSndBufSize = WAV_BUFFERS*WAV_BUFFER_SIZE;
	hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, gSndBufSize); 
	if (!hData) 
	{ 
		Con_SafePrintf ("Sound: Out of memory.\n");
		FreeSound ();
		return false; 
	}
	lpData = reinterpret_cast<HPSTR>( GlobalLock(hData) );
	if (!lpData)
	{ 
		Con_SafePrintf ("Sound: Failed to lock.\n");
		FreeSound ();
		return false; 
	} 
	memset (lpData, 0, gSndBufSize);

	/* 
	 * Allocate and lock memory for the header. This memory must 
	 * also be globally allocated with GMEM_MOVEABLE and 
	 * GMEM_SHARE flags. 
	 */ 
	hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, 
		(DWORD) sizeof(WAVEHDR) * WAV_BUFFERS); 

	if (hWaveHdr == NULL)
	{ 
		Con_SafePrintf ("Sound: Failed to Alloc header.\n");
		FreeSound ();
		return false; 
	} 

	lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr); 

	if (lpWaveHdr == NULL)
	{ 
		Con_SafePrintf ("Sound: Failed to lock header.\n");
		FreeSound ();
		return false; 
	}

	memset (lpWaveHdr, 0, sizeof(WAVEHDR) * WAV_BUFFERS);

	/* After allocation, set up and prepare headers. */ 
	for (i=0 ; i<WAV_BUFFERS ; i++)
	{
		lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE; 
		lpWaveHdr[i].lpData = reinterpret_cast<char*>( lpData ) + i*WAV_BUFFER_SIZE;

		if (waveOutPrepareHeader(hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR)) !=
				MMSYSERR_NOERROR)
		{
			Con_SafePrintf ("Sound: failed to prepare wave headers\n");
			FreeSound ();
			return false;
		}
	}

	shm->soundalive = true;
	shm->splitbuffer = false;
	shm->samples = gSndBufSize/(shm->samplebits/8);
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = (unsigned char *) lpData;
	sample16 = (shm->samplebits/8) - 1;

	wav_init = true;

	return true;
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/

int SNDDMA_Init(void)
{
	if (COM_CheckParm ("-wavonly"))
		wavonly = true;

	wav_init = 0;

	if (snd_firsttime || snd_iswave)
	{
		snd_iswave = SNDDMA_InitWav ();

		if (snd_iswave)
		{
			if (snd_firsttime)
				Con_SafePrintf ("Wave sound initialized\n");
		}
		else
		{
			Con_SafePrintf ("Wave sound failed to init\n");
		}
	}

	snd_firsttime = false;

	if (!wav_init)
	{
		if (snd_firsttime)
			Con_SafePrintf ("No sound device initialized\n");

		return 0;
	}

	return 1;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos(void)
{
	int		s;

	if (wav_init)
	{
		s = snd_sent * WAV_BUFFER_SIZE;
	}
	else
	{
		s = 0;
	}


	s >>= sample16;

	s &= (shm->samples-1);

	return s;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
	LPWAVEHDR	h;
	int			wResult;

	if (!wav_init)
		return;

	//
	// find which sound blocks have completed
	//
	while (1)
	{
		if ( snd_completed == snd_sent )
		{
			Con_DPrintf ("Sound overrun\n");
			break;
		}

		if ( ! (lpWaveHdr[ snd_completed & WAV_MASK].dwFlags & WHDR_DONE) )
		{
			break;
		}

		snd_completed++;	// this buffer has been played
	}

	//
	// submit two new sound blocks
	//
	while (((snd_sent - snd_completed) >> sample16) < 4)
	{
		h = lpWaveHdr + ( snd_sent&WAV_MASK );

		snd_sent++;
		/* 
		 * Now the data block can be sent to the output device. The 
		 * waveOutWrite function returns immediately and waveform 
		 * data is sent to the output device in the background. 
		 */ 
		wResult = waveOutWrite(hWaveOut, h, sizeof(WAVEHDR)); 

		if (wResult != MMSYSERR_NOERROR)
		{ 
			Con_SafePrintf ("Failed to write block to device\n");
			FreeSound ();
			return; 
		} 
	}
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown(void)
{
	FreeSound ();
}

