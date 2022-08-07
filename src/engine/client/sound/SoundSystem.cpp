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

#include <limits>

#include <AL/alext.h>

#include "CDAudio.h"
#include "sound_internal.h"
#include "SoundSystem.h"
#include "WaveSoundLoader.h"

void CheckALErrors()
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

std::optional<SoundSystem> SoundSystem::Create()
{
	if (SoundSystem audio; audio.CreateCore())
	{
		return audio;
	}

	return {};
}

bool SoundSystem::CreateCore()
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

	if (alIsExtensionPresent("AL_SOFT_loop_points") == AL_FALSE)
	{
		Con_SafePrintf("OpenAL does not provide extension \"AL_SOFT_loop_points\", required for audio playback\n");
		return false;
	}

	CheckALErrors();

	alDistanceModel(AL_LINEAR_DISTANCE);

	//So volume is correct.
	Unblock();

	m_KnownSFX.reserve(MAX_SFX);

	m_AmbientSFX[AMBIENT_WATER] = PrecacheSound("ambience/water1.wav");
	m_AmbientSFX[AMBIENT_SKY] = PrecacheSound("ambience/wind2.wav");

	m_Channels.resize(MAX_CHANNELS);

	for (auto& channel : m_Channels)
	{
		channel.source = OpenALSource::Create();
	}

	for (int ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++)
	{
		auto chan = &m_Channels[ambient_channel];

		if (auto sfx = GetSFX(m_AmbientSFX[ambient_channel]); sfx)
		{
			//Set up the channel but don't assign sfx to it yet, that's done in UpdateAmbientSounds.
			//Volume starts at zero so ambient sounds wind up based on user settings.
			SetupChannel(*chan, sfx, vec3_origin, 0, 0, true);
		}
	}

	StopAllSounds();

	return true;
}

void SoundSystem::Block()
{
	m_Blocked = true;
	alListenerf(AL_GAIN, 0);
}

void SoundSystem::Unblock()
{
	m_Blocked = false;
	//Unblock to user-set volume immediately.
	alListenerf(AL_GAIN, volume.value);
}

SoundIndex SoundSystem::PrecacheSound(const char* name)
{
	if (nosound.value)
		return {};

	auto sfx = FindName(name);

	// cache it in
	if (precache.value)
		LoadSound(sfx);

	//Sound indices are 1-based.
	static_assert(MAX_SFX <= (std::numeric_limits<int>::max)(), "SoundIndex needs to be a 64 bit datatype to reference all sound effects");
	return SoundIndex{static_cast<int>(sfx - m_KnownSFX.data()) + 1};
}

void SoundSystem::StartSound(int entnum, int entchannel, SoundIndex index, vec3_t origin, float fvol, float attenuation)
{
	auto sfx = GetSFX(index);

	if (!sfx)
		return;

	if (nosound.value)
		return;

	// pick a channel to play on
	auto target_chan = PickChannel(entnum, entchannel);
	if (!target_chan)
		return;

	target_chan->entnum = entnum;
	target_chan->entchannel = entchannel;

	// new channel
	if (!LoadSound(sfx))
	{
		return;		// couldn't load the sound's data
	}

	target_chan->sfx = sfx;

	// spatialize
	// anything coming from the view entity will allways be full volume
	const bool isRelative = entnum == cl.viewentity;

	SetupChannel(*target_chan, sfx, isRelative ? vec3_origin : origin, fvol, attenuation, isRelative);

	// if an identical sound has also been started this frame, offset the pos
	// a bit to keep it from just making the first one louder
	auto check = &m_Channels[NUM_AMBIENTS];
	for (int ch_idx = NUM_AMBIENTS; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; ch_idx++, check++)
	{
		if (check == target_chan)
			continue;
		if (check->sfx == sfx)
		{
			ALint offset;

			alGetSourcei(check->source.Id, AL_BYTE_OFFSET, &offset);

			if (offset == 0)
			{
				ALint frequency;
				alGetBufferi(sfx->buffer.Id, AL_FREQUENCY, &frequency);

				ALint size;
				alGetBufferi(sfx->buffer.Id, AL_SIZE, &size);

				int skip = rand() % (int)(0.1 * frequency);
				if (skip >= size)
					skip = size - 1;
				alSourcei(target_chan->source.Id, AL_BYTE_OFFSET, skip);
				break;
			}
		}

	}

	alSourcePlay(target_chan->source.Id);
}

void SoundSystem::StaticSound(SoundIndex index, vec3_t origin, float vol, float attenuation)
{
	auto sfx = GetSFX(index);
	if (!sfx)
		return;

	if (m_TotalChannels == MAX_CHANNELS)
	{
		Con_Printf("total_channels == MAX_CHANNELS\n");
		return;
	}

	auto ss = &m_Channels[m_TotalChannels];
	m_TotalChannels++;

	if (!LoadSound(sfx))
		return;

	if (!sfx->IsLooping)
	{
		Con_Printf("Sound %s not looped\n", sfx->name);
		return;
	}

	ss->sfx = sfx;

	SetupChannel(*ss, sfx, origin, vol / 255.f, (attenuation / 64), false);
	alSourcePlay(ss->source.Id);
}

void SoundSystem::LocalSound(const char* sound, float vol)
{
	if (nosound.value)
		return;

	auto sfx = PrecacheSound(sound);
	if (!sfx)
	{
		Con_Printf("LocalSound: can't cache %s\n", sound);
		return;
	}
	StartSound(cl.viewentity, -1, sfx, vec3_origin, vol, 1);
}

void SoundSystem::StopSound(int entnum, int entchannel)
{
	for (int i = 0; i < MAX_DYNAMIC_CHANNELS; i++)
	{
		if (m_Channels[i].entnum == entnum
			&& m_Channels[i].entchannel == entchannel)
		{
			alSourceStop(m_Channels[i].source.Id);
			m_Channels[i].sfx = NULL;
			return;
		}
	}
}

void SoundSystem::StopAllSounds()
{
	m_TotalChannels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;	// no statics

	for (auto& channel : m_Channels)
	{
		if (channel.sfx)
		{
			alSourceStop(channel.source.Id);
			channel.sfx = NULL;
		}

		channel.entnum = 0;
		channel.entchannel = 0;
	}
}

void SoundSystem::Update(const vec3_t origin, const vec3_t forward, const vec3_t right, const vec3_t up)
{
	if (ALC_FALSE == alcMakeContextCurrent(m_Context.get()))
	{
		Con_SafePrintf("Couldn't make OpenAL context current\n");
		return;
	}

	VectorCopy(origin, m_ListenerOrigin);

	const ALfloat orientation[6] =
	{
		forward[0], forward[1], forward[2],
		up[0], up[1], up[2]
	};

	alListenerfv(AL_POSITION, origin);
	alListenerfv(AL_ORIENTATION, orientation);

	//Update volume if changed. If we're currently blocked then volume is 0, so don't update it.
	if (!m_Blocked && m_LastKnownVolume != volume.value)
	{
		m_LastKnownVolume = volume.value;
		alListenerf(AL_GAIN, volume.value);
	}

	// update general area ambient sound sources
	UpdateAmbientSounds();
	UpdateSounds();

	//
	// debugging output
	//
	if (snd_show.value)
	{
		int total = 0;
		Channel* ch = m_Channels.data();
		for (int i = 0; i < m_TotalChannels; i++, ch++)
		{
			if (ch->sfx)
			{
				//Con_Printf ("%3i %3i %s\n", ch->sfx->name);
				total++;
			}
		}

		Con_Printf("----(%i)----\n", total);
	}
}

void SoundSystem::PrintSoundList()
{
	unsigned int total = 0;

	for (const auto& sfx : m_KnownSFX)
	{
		if (!sfx.buffer)
			continue;

		ALint size;
		alGetBufferi(sfx.buffer.Id, AL_SIZE, &size);
		total += size;

		if (sfx.IsLooping)
			Con_Printf("L");
		else
			Con_Printf(" ");

		ALint bits;
		alGetBufferi(sfx.buffer.Id, AL_BITS, &bits);

		Con_Printf("(%2db) %6d : %s\n", static_cast<int>(bits), static_cast<int>(size), sfx.name);
	}
	Con_Printf("Total resident: %u\n", total);
}

std::unique_ptr<ICDAudio> SoundSystem::CreateCDAudio()
{
	if (auto cdaudio = std::make_unique<CDAudio>(); cdaudio->Create(*this))
	{
		return cdaudio;
	}

	return {};
}

SoundSystem::SoundEffect* SoundSystem::FindName(const char* name)
{
	if (!name)
		Sys_Error("FindName: NULL\n");

	if (Q_strlen(name) >= MAX_QPATH)
		Sys_Error("Sound name too long: %s", name);

	// see if already loaded
	for (auto& sfx : m_KnownSFX)
	{
		if (!Q_strcmp(sfx.name, name))
		{
			return &sfx;
		}
	}

	if (m_KnownSFX.size() == MAX_SFX)
		Sys_Error("FindName: out of sfx_t");

	auto& sfx = m_KnownSFX.emplace_back();
	strcpy(sfx.name, name);

	return &sfx;
}

SoundSystem::SoundEffect* SoundSystem::GetSFX(SoundIndex index)
{
	const int i = index.Index - 1;

	if (i < 0 || static_cast<std::size_t>(i) >= m_KnownSFX.size())
	{
		return nullptr;
	}

	return &m_KnownSFX[i];
}

bool SoundSystem::LoadSound(SoundEffect* s)
{
	// see if still in memory
	if (s->buffer)
		return true;

	//Con_Printf ("S_LoadSound: %x\n", (int)stackbuf);
	// load it in
	char namebuffer[256];
	Q_strcpy(namebuffer, "sound/");
	Q_strcat(namebuffer, s->name);

	//	Con_Printf ("loading %s\n",namebuffer);

	byte stackbuf[1 * 1024]; // avoid dirtying the cache heap
	auto data = COM_LoadStackFile(namebuffer, stackbuf, sizeof(stackbuf));

	if (!data)
	{
		Con_Printf("Couldn't load %s\n", namebuffer);
		return false;
	}

	const wavinfo_t info = GetWavinfo(s->name, data, com_filesize);

	if (info.channels != 1)
	{
		Con_Printf("%s is a stereo sample\n", s->name);
		return false;
	}

	const ALenum format = [&]()
	{
		switch (info.channels)
		{
		case 1:
			switch (info.width)
			{
			case 1: return AL_FORMAT_MONO8;
			case 2: return AL_FORMAT_MONO16;
			}
			break;
		case 2:
			switch (info.width)
			{
			case 1: return AL_FORMAT_STEREO8;
			case 2: return AL_FORMAT_STEREO16;
			}
			break;
		}

		Con_Printf("%s has invalid format (channels: %d, sample bits: %d)\n", s->name, info.channels, info.width * 8);
		return 0;
	}();

	if (format == 0)
	{
		return false;
	}

	s->buffer = OpenALBuffer::Create();

	if (!s->buffer)
		return false;

	byte* samples = data + info.dataofs;

	if (info.width == 2)
	{
		for (int i = 0; i < info.samples; i++)
		{
			((short*)samples)[i] = LittleShort(((short*)samples)[i]);
		}
	}

	s->IsLooping = info.loopstart >= 0;

	// TODO: this code has not been tested with stereo sounds, these calculations may not be correct.
	alBufferData(s->buffer.Id, format, samples, info.samples * info.width * info.channels, info.rate);

	const int loopstart = info.loopstart >= 0 ? info.loopstart : 0;

	const ALint loopPoints[2] = {loopstart * info.width * info.channels, info.samples * info.width * info.channels};
	alBufferiv(s->buffer.Id, AL_LOOP_POINTS_SOFT, loopPoints);

	return true;
}

SoundSystem::Channel* SoundSystem::PickChannel(int entnum, int entchannel)
{
	// Check for replacement sound, or find the best one to replace
	int first_to_die = -1;
	int life_left = 0x7fffffff;
	for (int ch_idx = NUM_AMBIENTS; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; ch_idx++)
	{
		if (entchannel != 0		// channel 0 never overrides
			&& m_Channels[ch_idx].entnum == entnum
			&& (m_Channels[ch_idx].entchannel == entchannel || entchannel == -1))
		{	// allways override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (m_Channels[ch_idx].entnum == cl.viewentity && entnum != cl.viewentity && m_Channels[ch_idx].sfx)
			continue;

		if (!m_Channels[ch_idx].sfx)
		{
			first_to_die = ch_idx;
			life_left = -1;
			continue;
		}

		ALint size;
		alGetBufferi(m_Channels[ch_idx].sfx->buffer.Id, AL_SIZE, &size);

		ALint offset;
		alGetSourcei(m_Channels[ch_idx].source.Id, AL_BYTE_OFFSET, &offset);

		if (size - offset < life_left)
		{
			life_left = size - offset;
			first_to_die = ch_idx;
		}
	}

	if (first_to_die == -1)
		return NULL;

	//Stop sound if it's playing.
	alSourceStop(m_Channels[first_to_die].source.Id);

	if (m_Channels[first_to_die].sfx)
		m_Channels[first_to_die].sfx = NULL;

	return &m_Channels[first_to_die];
}

void SoundSystem::SetupChannel(Channel& chan, SoundEffect* sfx, vec3_t origin, float vol, float attenuation, bool isRelative)
{
	alSourcefv(chan.source.Id, AL_POSITION, origin);
	alSourcef(chan.source.Id, AL_GAIN, vol);
	alSourcef(chan.source.Id, AL_ROLLOFF_FACTOR, attenuation);
	alSourcef(chan.source.Id, AL_MAX_DISTANCE, sound_nominal_clip_dist);
	alSourcei(chan.source.Id, AL_SOURCE_RELATIVE, isRelative ? AL_TRUE : AL_FALSE);
	alSourcei(chan.source.Id, AL_LOOPING, sfx->IsLooping ? AL_TRUE : AL_FALSE);

	alSourcei(chan.source.Id, AL_BUFFER, sfx->buffer.Id);
}

void SoundSystem::UpdateAmbientSounds()
{
	if (!m_AmbientEnabled)
		return;

	// calc ambient sound levels
	if (!cl.worldmodel)
		return;

	auto l = Mod_PointInLeaf(m_ListenerOrigin, cl.worldmodel);
	if (!l || !ambient_level.value)
	{
		for (int ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++)
		{
			alSourceStop(m_Channels[ambient_channel].source.Id);
			m_Channels[ambient_channel].sfx = NULL;
		}
		return;
	}

	for (int ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++)
	{
		auto chan = &m_Channels[ambient_channel];
		chan->sfx = GetSFX(m_AmbientSFX[ambient_channel]);

		if (!chan->sfx)
		{
			continue;
		}

		float vol = ambient_level.value * l->ambient_sound_level[ambient_channel];
		if (vol < 8)
			vol = 0;

		// don't adjust volume too fast
		ALfloat gain;
		alGetSourcef(chan->source.Id, AL_GAIN, &gain);

		gain *= 255;

		if (gain < vol)
		{
			gain += host_frametime * ambient_fade.value;
			if (gain > vol)
				gain = vol;
		}
		else if (gain > vol)
		{
			gain -= host_frametime * ambient_fade.value;
			if (gain < vol)
				gain = vol;
		}

		gain /= 255;

		alSourcef(chan->source.Id, AL_GAIN, gain);

		ALint state;
		alGetSourcei(chan->source.Id, AL_SOURCE_STATE, &state);

		if (state != AL_PLAYING)
		{
			alSourcePlay(chan->source.Id);
		}
	}
}

void SoundSystem::UpdateSounds()
{
	//Update all sounds that are looping, clear finished sounds.
	for (int i = 0; i < m_TotalChannels; ++i)
	{
		auto& chan = m_Channels[i];

		if (!chan.sfx)
		{
			continue;
		}

		ALint state;
		alGetSourcei(chan.source.Id, AL_SOURCE_STATE, &state);

		if (state == AL_STOPPED)
		{
			chan.sfx = nullptr;
		}
	}
}
