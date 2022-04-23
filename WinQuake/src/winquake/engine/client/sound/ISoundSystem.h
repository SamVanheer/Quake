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

struct ICDAudio;

/**
*	@brief Strongly typed sound index, for disambiguating overloads between <tt>const char*</tt> and <tt>int</tt>.
*/
struct SoundIndex final
{
	static constexpr int InvalidIndex = 0;

	constexpr SoundIndex() noexcept = default;

	explicit constexpr SoundIndex(int index) noexcept
		: Index(index)
	{
	}

	constexpr operator bool() const
	{
		return Index != InvalidIndex;
	}

	int Index = InvalidIndex;
};

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
	virtual SoundIndex PrecacheSound(const char* name) = 0;

	/**
	*	@brief Start a sound effect
	*/
	virtual void StartSound(int entnum, int entchannel, SoundIndex index, vec3_t origin, float fvol, float attenuation) = 0;
	virtual void StaticSound(SoundIndex index, vec3_t origin, float vol, float attenuation) = 0;
	virtual void LocalSound(const char* sound, float vol = 1.f) = 0;

	virtual void StopSound(int entnum, int entchannel) = 0;
	virtual void StopAllSounds() = 0;

	/**
	*	@brief Called once each time through the main loop
	*/
	virtual void Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up) = 0;

	virtual void PrintSoundList() = 0;

	virtual std::unique_ptr<ICDAudio> CreateCDAudio() = 0;
};
