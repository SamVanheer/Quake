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

/**
*	@brief OpenAL-based sound system.
*/
class SoundSystem final : public ISoundSystem
{
private:
	static constexpr int MAX_SFX = 512;

	// 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
	// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
	// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds
	static constexpr int MAX_CHANNELS = 128;
	static constexpr int MAX_DYNAMIC_CHANNELS = 8;

	static constexpr vec_t sound_nominal_clip_dist = 1000.0;

public:

	static std::optional<SoundSystem> Create();

	SoundSystem() = default;
	SoundSystem(SoundSystem&&) = default;
	SoundSystem& operator=(SoundSystem&&) = default;
	~SoundSystem() override;

	bool IsActive() const override { return true; }

	bool IsBlocked() const override { return m_Blocked; }

	void Block() override;
	void Unblock() override;

	bool IsAmbientEnabled() const override { return m_AmbientEnabled; }
	void SetAmbientEnabled(bool enable) override;

	int GetTotalChannelCount() const override { return total_channels; }

	sfx_t* PrecacheSound(const char* name) override;

	void StartSound(int entnum, int entchannel, sfx_t* sfx, vec3_t origin, float fvol, float attenuation) override;
	void StaticSound(sfx_t* sfx, vec3_t origin, float vol, float attenuation) override;
	void LocalSound(const char* sound, float vol = 1.f) override;

	void StopSound(int entnum, int entchannel) override;
	void StopAllSounds() override;

	void Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up) override;

	void PrintSoundList() override;

private:
	bool CreateCore();

	sfx_t* FindName(const char* name);

	/**
	*	@brief picks a channel based on priorities, empty slots, number of channels
	*/
	channel_t* PickChannel(int entnum, int entchannel);
	void SetupChannel(channel_t& chan, sfx_t* sfx, vec3_t origin, float vol, float attenuation, bool isRelative);

	void UpdateAmbientSounds();
	void UpdateSounds();

private:
	std::unique_ptr<ALCdevice, DeleterWrapper<alcCloseDevice>> m_Device;
	std::unique_ptr<ALCcontext, DeleterWrapper<alcDestroyContext>> m_Context;

	bool m_Blocked = false;
	bool m_AmbientEnabled = true;

	float m_LastKnownVolume = -1;

	vec3_t m_ListenerOrigin{};

	sfx_t* known_sfx = nullptr;		// hunk allocated [MAX_SFX]
	int num_sfx = 0;

	sfx_t* ambient_sfx[NUM_AMBIENTS]{};

	channel_t channels[MAX_CHANNELS]{};
	int total_channels = 0;
};

inline void SoundSystem::SetAmbientEnabled(bool enable)
{
	m_AmbientEnabled = enable;
}
