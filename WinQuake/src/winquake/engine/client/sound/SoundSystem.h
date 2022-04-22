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

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

#include "ISoundSystem.h"

template<auto Function>
struct DeleterWrapper final
{
	template<typename T>
	constexpr void operator()(T* object) const
	{
		Function(object);
	}
};

constexpr ALuint NullBuffer = 0;
constexpr ALuint NullSource = 0;

struct OpenALBuffer final
{
	constexpr OpenALBuffer() noexcept = default;

	OpenALBuffer(const OpenALBuffer&) = delete;
	OpenALBuffer& operator=(const OpenALBuffer&) = delete;

	constexpr OpenALBuffer(OpenALBuffer&& other) noexcept
		: Id(other.Id)
	{
		other.Id = NullBuffer;
	}

	constexpr OpenALBuffer& operator=(OpenALBuffer&& other) noexcept
	{
		if (this != &other)
		{
			Id = other.Id;
			other.Id = NullBuffer;
		}

		return *this;
	}

	~OpenALBuffer()
	{
		Delete();
	}

	static OpenALBuffer Create()
	{
		OpenALBuffer buffer;
		alGenBuffers(1, &buffer.Id);
		return buffer;
	}

	constexpr operator bool() const { return Id != NullBuffer; }

	void Delete()
	{
		if (Id != NullBuffer)
		{
			alDeleteBuffers(1, &Id);
		}
	}

	ALuint Id = NullBuffer;
};

struct OpenALSource final
{
	constexpr OpenALSource() noexcept = default;

	OpenALSource(const OpenALSource&) = delete;
	OpenALSource& operator=(const OpenALSource&) = delete;

	constexpr OpenALSource(OpenALSource&& other) noexcept
		: Id(other.Id)
	{
		other.Id = NullSource;
	}

	constexpr OpenALSource& operator=(OpenALSource&& other) noexcept
	{
		if (this != &other)
		{
			Id = other.Id;
			other.Id = NullSource;
		}

		return *this;
	}

	~OpenALSource()
	{
		Delete();
	}

	static OpenALSource Create()
	{
		OpenALSource source;
		alGenSources(1, &source.Id);
		return source;
	}

	constexpr operator bool() const { return Id != NullSource; }

	void Delete()
	{
		if (Id != NullSource)
		{
			alDeleteSources(1, &Id);
		}
	}

	ALuint Id = NullSource;
};

/**
*	@brief OpenAL-based sound system.
*/
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

	static std::optional<OpenALAudio> Create();

	bool IsBlocked() const override { return m_Blocked; }

	void Block() override;
	void Unblock() override;

private:
	bool CreateCore();

private:
	std::unique_ptr<ALCdevice, DeleterWrapper<alcCloseDevice>> m_Device;
	std::unique_ptr<ALCcontext, DeleterWrapper<alcDestroyContext>> m_Context;

	bool m_Blocked = false;
};
