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

//TODO: move openal stuff out of this header
#include <AL/al.h>
#include <AL/alc.h>

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

typedef struct sfx_s
{
	char name[MAX_QPATH];
	OpenALBuffer buffer;
	OpenALBuffer loopingBuffer;
} sfx_t;

typedef struct
{
	sfx_t* sfx;			// sfx number
	int entnum;			// to allow overriding a specific sound
	int entchannel;		//
	OpenALSource source;
} channel_t;

struct ISoundSystem
{
	virtual ~ISoundSystem() {}

	virtual bool IsActive() const = 0;

	virtual bool IsBlocked() const = 0;

	virtual void Block() = 0;
	virtual void Unblock() = 0;

	virtual bool IsAmbientEnabled() const = 0;
	virtual void SetAmbientEnabled(bool enable) = 0;

	virtual int GetTotalChannelCount() const = 0;

	/**
	*	@brief Load a sound
	*/
	virtual sfx_t* PrecacheSound(const char* name) = 0;

	/**
	*	@brief Start a sound effect
	*/
	virtual void StartSound(int entnum, int entchannel, sfx_t* sfx, vec3_t origin, float fvol, float attenuation) = 0;
	virtual void StaticSound(sfx_t* sfx, vec3_t origin, float vol, float attenuation) = 0;
	virtual void LocalSound(const char* sound, float vol = 1.f) = 0;

	virtual void StopSound(int entnum, int entchannel) = 0;
	virtual void StopAllSounds() = 0;

	/**
	*	@brief Called once each time through the main loop
	*/
	virtual void Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up) = 0;

	virtual void PrintSoundList() = 0;
};
