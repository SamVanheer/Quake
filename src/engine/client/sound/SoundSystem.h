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

#include <array>
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
			Delete();
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
			Id = NullBuffer;
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
			Delete();
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
			Id = NullSource;
		}
	}

	ALuint Id = NullSource;
};

/**
*	@brief OpenAL-based sound system.
*/
class SoundSystem final : public ISoundSystem
{
private:
	struct SoundEffect
	{
		char name[MAX_QPATH];
		OpenALBuffer buffer;
		bool IsLooping = false;
	};

	struct Channel
	{
		SoundEffect* sfx = nullptr;	// sfx number
		int entnum = 0;			// to allow overriding a specific sound
		int entchannel = 0;
		OpenALSource source;
	};

	static constexpr std::size_t MAX_SFX = 512;

	// 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
	// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
	// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds
	static constexpr int MAX_CHANNELS = 128;
	static constexpr int MAX_DYNAMIC_CHANNELS = 8;

	static constexpr vec_t sound_nominal_clip_dist = 1000.0;

public:

	static std::optional<SoundSystem> Create();

	bool IsActive() const override { return true; }

	bool IsBlocked() const override { return m_Blocked; }

	void Block() override;
	void Unblock() override;

	bool IsAmbientEnabled() const override { return m_AmbientEnabled; }
	void SetAmbientEnabled(bool enable) override;

	int GetTotalChannelCount() const override { return m_TotalChannels; }

	SoundIndex PrecacheSound(const char* name) override;

	void StartSound(int entnum, int entchannel, SoundIndex index, vec3_t origin, float fvol, float attenuation) override;
	void StaticSound(SoundIndex index, vec3_t origin, float vol, float attenuation) override;
	void LocalSound(const char* sound, float vol = 1.f) override;

	void StopSound(int entnum, int entchannel) override;
	void StopAllSounds() override;

	void Update(const vec3_t origin, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up) override;

	void PrintSoundList() override;

	std::unique_ptr<ICDAudio> CreateCDAudio() override;

	ALCdevice* GetDevice() { return m_Device.get(); }

private:
	bool CreateCore();

	SoundEffect* FindName(const char* name);

	SoundEffect* GetSFX(SoundIndex index);

	bool LoadSound(SoundEffect* s);

	/**
	*	@brief picks a channel based on priorities, empty slots, number of channels
	*/
	Channel* PickChannel(int entnum, int entchannel);
	void SetupChannel(Channel& chan, SoundEffect* sfx, vec3_t origin, float vol, float attenuation, bool isRelative);

	void UpdateAmbientSounds();
	void UpdateSounds();

private:
	std::unique_ptr<ALCdevice, DeleterWrapper<alcCloseDevice>> m_Device;
	std::unique_ptr<ALCcontext, DeleterWrapper<alcDestroyContext>> m_Context;

	bool m_Blocked = false;
	bool m_AmbientEnabled = true;

	float m_LastKnownVolume = -1;

	vec3_t m_ListenerOrigin{};

	std::vector<SoundEffect> m_KnownSFX; // [MAX_SFX]	

	std::array<SoundIndex, NUM_AMBIENTS> m_AmbientSFX;

	std::vector<Channel> m_Channels;
	int m_TotalChannels = 0;
};

inline void SoundSystem::SetAmbientEnabled(bool enable)
{
	m_AmbientEnabled = enable;
}

void CheckALErrors();
