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

/**
*	@file
*
*	OpenAL-based sound system.
*/

#include <cassert>
#include <memory>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

#include "quakedef.h"

template<auto Function>
struct DeleterWrapper final
{
	template<typename T>
	constexpr void operator()(T* object) const
	{
		Function(object);
	}
};

struct OpenALBuffer final
{
	constexpr OpenALBuffer() noexcept = default;

	OpenALBuffer(const OpenALBuffer&) = delete;
	OpenALBuffer& operator=(const OpenALBuffer&) = delete;

	constexpr OpenALBuffer(OpenALBuffer&& other) noexcept
		: Id(other.Id)
	{
		other.Id = 0;
	}

	constexpr OpenALBuffer& operator=(OpenALBuffer&& other) noexcept
	{
		if (this != &other)
		{
			Id = other.Id;
			other.Id = 0;
		}

		return *this;
	}

	~OpenALBuffer()
	{
		alDeleteBuffers(1, &Id);
	}

	static OpenALBuffer Create()
	{
		OpenALBuffer buffer;
		alGenBuffers(1, &buffer.Id);
		return buffer;
	}

	ALuint Id = 0;
};

struct OpenALSource final
{
	constexpr OpenALSource() noexcept = default;

	OpenALSource(const OpenALSource&) = delete;
	OpenALSource& operator=(const OpenALSource&) = delete;

	constexpr OpenALSource(OpenALSource&& other) noexcept
		: Id(other.Id)
	{
		other.Id = 0;
	}

	constexpr OpenALSource& operator=(OpenALSource&& other) noexcept
	{
		if (this != &other)
		{
			Id = other.Id;
			other.Id = 0;
		}

		return *this;
	}

	~OpenALSource()
	{
		alGenSources(1, &Id);
	}

	static OpenALSource Create()
	{
		OpenALSource source;
		alGenSources(1, &source.Id);
		return source;
	}

	ALuint Id = 0;
};

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
	}
	while (error != AL_NO_ERROR);
}

class OpenALAudio final : public ISoundSystem
{
private:
	// 64K is > 1 second at 16-bit, 22050 Hz
	static constexpr int BUFFER_MASK = 0x3F;
	static constexpr int NUM_BUFFERS = BUFFER_MASK + 1;
	static constexpr int BUFFER_SIZE = 0x0400;

	//The higher this is the more latency there is. Originally 4 in the wave implementation.
	//There is likely a problem with this implementation
	//because the sound painting code can paint ahead further than the amount of buffers we're queueing.
	//
	//The DirectSound version wrote directly to the buffer which allowed it to stay in sync.
	//
	//There is no perfect solution for this as-is; the sound playback code needs to use OpenAL more directly
	//so sounds are played in real-time instead of writing to a single buffer that gets queueud in chunks.
	//
	//In effect: S_StartSound => alSourcePlay
	static constexpr int MAX_BUFFERS_AHEAD = 2;

public:
	static OpenALAudio Create();

	bool IsInitialized() const { return m_Initialized; }

	bool IsBlocked() const override { return m_Blocked; }

	void Block() override;
	void Unblock() override;

	int GetDMAPosition() const override;

	void Submit();

private:
	bool CreateCore();

private:
	bool m_Initialized = false;

	std::unique_ptr<ALCdevice, DeleterWrapper<alcCloseDevice>> m_Device;
	std::unique_ptr<ALCcontext, DeleterWrapper<alcDestroyContext>> m_Context;

	int m_Sent = 0;
	int m_Completed = 0;

	int m_Sample16 = 0;

	bool m_Blocked = false;

	std::vector<byte> m_Data;
	std::vector<OpenALBuffer> m_Buffers;
	OpenALSource m_Source;

	ALenum m_Format = 0;
};

static bool snd_firsttime = true;

static OpenALAudio openal_audio;

ISoundSystem* g_SoundSystem = &openal_audio;

OpenALAudio OpenALAudio::Create()
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

	m_Initialized = true;

	return m_Initialized;
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
	int s = m_Initialized ? m_Sent * BUFFER_SIZE : 0;

	s >>= m_Sample16;

	s &= (shm->samples - 1);

	return s;
}

void OpenALAudio::Submit()
{
	if (!m_Initialized)
		return;

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

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/
int SNDDMA_Init()
{
	if (snd_firsttime)
	{
		openal_audio = OpenALAudio::Create();

		if (openal_audio.IsInitialized())
		{
			if (snd_firsttime)
				Con_SafePrintf("OpenAL sound initialized\n");
		}
		else
		{
			Con_SafePrintf("OpenAL sound failed to init\n");
		}
	}

	const bool wasFirstTime = snd_firsttime;

	snd_firsttime = false;

	if (!openal_audio.IsInitialized())
	{
		if (wasFirstTime)
			Con_SafePrintf("No sound device initialized\n");

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
int SNDDMA_GetDMAPos()
{
	return openal_audio.GetDMAPosition();
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown()
{
	openal_audio = OpenALAudio{};
}
