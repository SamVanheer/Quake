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

#include <cassert>

#include "quakedef.h"
#include "SoundSystem.h"

static void CheckALErrors()
{
	GLenum error = AL_NO_ERROR;

	do
	{
		error = alGetError();

		if (error != AL_NO_ERROR)
		{
			Con_SafePrintf("OpenAL Error: %u\n", error);
		}
	} while (error != AL_NO_ERROR);
}

std::optional<OpenALAudio> OpenALAudio::Create()
{
	if (OpenALAudio audio; audio.CreateCore())
	{
		return audio;
	}

	return {};
}

bool OpenALAudio::CreateCore()
{
	m_Device.reset(alcOpenDevice(nullptr));

	if (!m_Device)
	{
		Con_SafePrintf("Couldn't open OpenAL device\n");
		return false;
	}

	m_Context.reset(alcCreateContext(m_Device.get(), nullptr));

	if (!m_Context)
	{
		Con_SafePrintf("Couldn't create OpenAL context\n");
		return false;
	}

	if (ALC_FALSE == alcMakeContextCurrent(m_Context.get()))
	{
		Con_SafePrintf("Couldn't make OpenAL context current\n");
		return false;
	}

	CheckALErrors();

	shm = &sn;

	shm->channels = 2;
	shm->samplebits = 16;
	shm->speed = 11025;

	/*
	 * Allocate and lock memory for the waveform data.
	*/
	m_Data.resize(NUM_BUFFERS * BUFFER_SIZE, 0);

	m_Buffers.resize(NUM_BUFFERS);

	CheckALErrors();

	for (auto& buffer : m_Buffers)
	{
		buffer = OpenALBuffer::Create();
		CheckALErrors();
	}

	m_Source = OpenALSource::Create();
	CheckALErrors();

	alSourceRewind(m_Source.Id);
	CheckALErrors();

	alSource3i(m_Source.Id, AL_POSITION, 0, 0, -1);
	CheckALErrors();
	alSourcei(m_Source.Id, AL_SOURCE_RELATIVE, AL_TRUE);
	CheckALErrors();
	alSourcei(m_Source.Id, AL_ROLLOFF_FACTOR, 0);
	CheckALErrors();
	//alSourcei(m_Source.Id, AL_LOOPING, AL_TRUE);
	CheckALErrors();

	shm->soundalive = true;
	shm->splitbuffer = false;
	shm->samples = m_Data.size() / (shm->samplebits / 8);
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = m_Data.data();
	m_Sample16 = (shm->samplebits / 8) - 1;

	m_Format = []()
	{
		switch (shm->channels)
		{
		case 1:
			switch (shm->samplebits)
			{
			case 8: return AL_FORMAT_MONO8;
			case 16: return AL_FORMAT_MONO16;
			}
			break;
		case 2:
			switch (shm->samplebits)
			{
			case 8: return AL_FORMAT_STEREO8;
			case 16: return AL_FORMAT_STEREO16;
			}
			break;
		}

		Con_SafePrintf("Invalid sound system state (channels: %d, sample bits: %d)\n", shm->channels, shm->samplebits);
		return 0;
	}();

	if (m_Format == 0)
	{
		shm = nullptr;
		return false;
	}

	return true;
}

void OpenALAudio::Block()
{
	m_Blocked = true;
	alListenerf(AL_GAIN, 0);
}

void OpenALAudio::Unblock()
{
	m_Blocked = false;
	alListenerf(AL_GAIN, 1);
}

int OpenALAudio::GetDMAPosition() const
{
	int s = m_Sent * BUFFER_SIZE;

	s >>= m_Sample16;

	s &= (shm->samples - 1);

	return s;
}

void OpenALAudio::Submit()
{
	ALint state;
	alGetSourcei(m_Source.Id, AL_SOURCE_STATE, &state);
	CheckALErrors();

	//
	// find which sound blocks have completed
	//
	{
		ALint processedBuffers = 0;
		alGetSourcei(m_Source.Id, AL_BUFFERS_PROCESSED, &processedBuffers);
		CheckALErrors();

		//Unqueue all processed buffers.
		for (ALint i = 0; i < processedBuffers; ++i)
		{
			if (m_Completed == m_Sent)
			{
				Con_DPrintf("Sound overrun\n");
				break;
			}

			ALuint bufferId;
			alSourceUnqueueBuffers(m_Source.Id, 1, &bufferId);
			CheckALErrors();

			assert(bufferId == m_Buffers[m_Completed & BUFFER_MASK].Id);

			++m_Completed; // this buffer has been played
		}
	}

	//
	// submit up to MAX_BUFFERS_AHEAD new sound blocks ahead of what's been completed.
	//
	while (((m_Sent - m_Completed) >> m_Sample16) < MAX_BUFFERS_AHEAD)
	{
		const int index = m_Sent & BUFFER_MASK;

		const auto& buffer = m_Buffers[index];

		++m_Sent;

		alBufferData(buffer.Id, m_Format, m_Data.data() + (BUFFER_SIZE * index), BUFFER_SIZE, shm->speed);
		CheckALErrors();

		alSourceQueueBuffers(m_Source.Id, 1, &buffer.Id);
		CheckALErrors();
	}

	//Restart the source if it stopped playing.
	if (state != AL_PLAYING)
	{
		alSourcePlay(m_Source.Id);
		CheckALErrors();
	}
}
