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
// snd_dma.c -- main control for any streaming sound output device

#include "quakedef.h"
#include "SoundSystem.h"

void S_Play();
void S_PlayVol();
void S_SoundList();
void S_StopAllSounds();
static void S_SetupChannel(channel_t& chan, sfx_t* sfx, vec3_t origin, float vol, float attenuation, bool isRelative);

struct DummySoundSystem final : public ISoundSystem
{
	bool IsBlocked() const override { return false; }

	void Block() override {}
	void Unblock() override {}
};

// =======================================================================
// Internal sound data & structures
// =======================================================================

channel_t channels[MAX_CHANNELS];
int total_channels;

static bool	snd_ambient = true;
bool snd_initialized = false;
static bool snd_firsttime = true;

static std::optional<OpenALAudio> openal_audio;

static DummySoundSystem g_DummySoundSystem;

ISoundSystem* g_SoundSystem = &g_DummySoundSystem;

vec3_t listener_origin;
vec3_t listener_forward;
vec3_t listener_right;
vec3_t listener_up;
constexpr vec_t sound_nominal_clip_dist = 1000.0;

int soundtime;		// sample PAIRS
int paintedtime; 	// sample PAIRS


constexpr int MAX_SFX = 512;
sfx_t* known_sfx;		// hunk allocated [MAX_SFX]
int num_sfx;

sfx_t* ambient_sfx[NUM_AMBIENTS];

bool sound_started = false;

cvar_t bgmvolume = {"bgmvolume", "1", true};
cvar_t volume = {"volume", "0.7", true};

cvar_t nosound = {"nosound", "0"};
cvar_t precache = {"precache", "1"};
cvar_t bgmbuffer = {"bgmbuffer", "4096"};
cvar_t ambient_level = {"ambient_level", "0.3"};
cvar_t ambient_fade = {"ambient_fade", "100"};
cvar_t snd_noextraupdate = {"snd_noextraupdate", "0"};
cvar_t snd_show = {"snd_show", "0"};

// ====================================================================
// User-setable variables
// ====================================================================

void S_AmbientOff()
{
	snd_ambient = false;
}

void S_AmbientOn()
{
	snd_ambient = true;
}

void S_SoundInfo_f()
{
	if (!sound_started)
	{
		Con_Printf("sound system not started\n");
		return;
	}

	Con_Printf("%5d total_channels\n", total_channels);
}

/*
================
S_Startup
================
*/
void S_Startup()
{
	if (!snd_initialized)
		return;

	const bool wasFirstTime = snd_firsttime;

	if (snd_firsttime)
	{
		snd_firsttime = false;

		openal_audio = OpenALAudio::Create();

		if (openal_audio.has_value())
		{
			g_SoundSystem = &openal_audio.value();

			Con_SafePrintf("OpenAL sound initialized\n");
		}
		else
		{
			Con_SafePrintf("OpenAL sound failed to init\n");
		}

		sound_started = openal_audio.has_value();
	}

	if (!sound_started && wasFirstTime)
	{
		Con_SafePrintf("No sound device initialized\n");
	}
}

/*
================
S_Init
================
*/
void S_Init()
{

	Con_Printf("\nSound Initialization\n");

	if (COM_CheckParm("-nosound"))
		return;

	Cmd_AddCommand("play", S_Play);
	Cmd_AddCommand("playvol", S_PlayVol);
	Cmd_AddCommand("stopsound", S_StopAllSounds);
	Cmd_AddCommand("soundlist", S_SoundList);
	Cmd_AddCommand("soundinfo", S_SoundInfo_f);

	Cvar_RegisterVariable(&nosound);
	Cvar_RegisterVariable(&volume);
	Cvar_RegisterVariable(&precache);
	Cvar_RegisterVariable(&bgmvolume);
	Cvar_RegisterVariable(&bgmbuffer);
	Cvar_RegisterVariable(&ambient_level);
	Cvar_RegisterVariable(&ambient_fade);
	Cvar_RegisterVariable(&snd_noextraupdate);
	Cvar_RegisterVariable(&snd_show);

	snd_initialized = true;

	S_Startup();

	if (!sound_started)
	{
		//Can't play sounds, so just stop here.
		return;
	}

	known_sfx = reinterpret_cast<sfx_t*>(Hunk_AllocName(MAX_SFX * sizeof(sfx_t), "sfx_t"));
	num_sfx = 0;

	ambient_sfx[AMBIENT_WATER] = S_PrecacheSound("ambience/water1.wav");
	ambient_sfx[AMBIENT_SKY] = S_PrecacheSound("ambience/wind2.wav");

	for (auto& channel : channels)
	{
		channel.source = OpenALSource::Create();
	}

	for (int ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++)
	{
		auto chan = &channels[ambient_channel];

		if (auto sfx = ambient_sfx[ambient_channel]; sfx)
		{
			S_SetupChannel(*chan, sfx, vec3_origin, 1, 0, true);
		}
	}

	S_StopAllSounds();
}

// =======================================================================
// Shutdown sound engine
// =======================================================================
void S_Shutdown()
{
	if (!sound_started)
		return;

	sound_started = false;

	for (auto& channel : channels)
	{
		alDeleteSources(1, &channel.source.Id);
		channel.source.Id = NullSource;
	}

	for (int i = 0; i < num_sfx; ++i)
	{
		if (known_sfx[i].loopingBuffer)
		{
			alDeleteBuffers(1, &known_sfx[i].loopingBuffer.Id);
			known_sfx[i].loopingBuffer.Id = NullBuffer;
		}

		if (known_sfx[i].buffer)
		{
			alDeleteBuffers(1, &known_sfx[i].buffer.Id);
			known_sfx[i].buffer.Id = NullBuffer;
		}
	}

	g_SoundSystem = &g_DummySoundSystem;
	openal_audio.reset();
}

// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_FindName

==================
*/
sfx_t* S_FindName(const char* name)
{
	if (!name)
		Sys_Error("S_FindName: NULL\n");

	if (Q_strlen(name) >= MAX_QPATH)
		Sys_Error("Sound name too long: %s", name);

	// see if already loaded
	for (int i = 0; i < num_sfx; i++)
	{
		if (!Q_strcmp(known_sfx[i].name, name))
		{
			return &known_sfx[i];
		}
	}

	if (num_sfx == MAX_SFX)
		Sys_Error("S_FindName: out of sfx_t");

	auto sfx = &known_sfx[num_sfx++];
	strcpy(sfx->name, name);

	return sfx;
}

/*
==================
S_PrecacheSound

==================
*/
sfx_t* S_PrecacheSound(const char* name)
{
	if (!sound_started || nosound.value)
		return NULL;

	auto sfx = S_FindName(name);

	// cache it in
	if (precache.value)
		S_LoadSound(sfx);

	return sfx;
}

//=============================================================================

/*
=================
SND_PickChannel
picks a channel based on priorities, empty slots, number of channels
=================
*/
channel_t* SND_PickChannel(int entnum, int entchannel)
{
	// Check for replacement sound, or find the best one to replace
	int first_to_die = -1;
	int life_left = 0x7fffffff;
	for (int ch_idx = NUM_AMBIENTS; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; ch_idx++)
	{
		if (entchannel != 0		// channel 0 never overrides
			&& channels[ch_idx].entnum == entnum
			&& (channels[ch_idx].entchannel == entchannel || entchannel == -1))
		{	// allways override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (channels[ch_idx].entnum == cl.viewentity && entnum != cl.viewentity && channels[ch_idx].sfx)
			continue;

		if (!channels[ch_idx].sfx)
		{
			first_to_die = ch_idx;
			life_left = -1;
			continue;
		}

		ALint size;
		alGetBufferi(channels[ch_idx].sfx->buffer.Id, AL_SIZE, &size);

		ALint offset;
		alGetSourcei(channels[ch_idx].source.Id, AL_BYTE_OFFSET, &offset);

		if (size - offset < life_left)
		{
			life_left = size - offset;
			first_to_die = ch_idx;
		}
	}

	if (first_to_die == -1)
		return NULL;

	//Stop sound if it's playing.
	alSourceStop(channels[first_to_die].source.Id);

	if (channels[first_to_die].sfx)
		channels[first_to_die].sfx = NULL;

	return &channels[first_to_die];
}

static void S_SetupChannel(channel_t& chan, sfx_t* sfx, vec3_t origin, float vol, float attenuation, bool isRelative)
{
	alSourcefv(chan.source.Id, AL_POSITION, origin);
	alSourcef(chan.source.Id, AL_GAIN, vol);
	alSourcef(chan.source.Id, AL_ROLLOFF_FACTOR, attenuation);
	alSourcef(chan.source.Id, AL_MAX_DISTANCE, sound_nominal_clip_dist);
	alSourcei(chan.source.Id, AL_SOURCE_RELATIVE, isRelative ? AL_TRUE : AL_FALSE);
	alSourcei(chan.source.Id, AL_LOOPING, AL_FALSE);

	alSourcei(chan.source.Id, AL_BUFFER, NullBuffer);

	if (sfx->loopingBuffer)
	{
		const ALuint buffers[2] = {sfx->buffer.Id, sfx->loopingBuffer.Id};
		alSourceQueueBuffers(chan.source.Id, 2, buffers);
	}
	else
	{
		alSourcei(chan.source.Id, AL_BUFFER, sfx->buffer.Id);
	}
}

// =======================================================================
// Start a sound effect
// =======================================================================
void S_StartSound(int entnum, int entchannel, sfx_t* sfx, vec3_t origin, float fvol, float attenuation)
{
	if (!sound_started)
		return;

	if (!sfx)
		return;

	if (nosound.value)
		return;

	// pick a channel to play on
	auto target_chan = SND_PickChannel(entnum, entchannel);
	if (!target_chan)
		return;

	target_chan->entnum = entnum;
	target_chan->entchannel = entchannel;

	// new channel
	auto sc = S_LoadSound(sfx);
	if (!sc)
	{
		return;		// couldn't load the sound's data
	}

	target_chan->sfx = sfx;

	// spatialize
	// anything coming from the view entity will allways be full volume
	const bool isRelative = entnum == cl.viewentity;

	S_SetupChannel(*target_chan, sfx, isRelative ? vec3_origin : origin, fvol, attenuation, isRelative);

	// if an identical sound has also been started this frame, offset the pos
	// a bit to keep it from just making the first one louder
	auto check = &channels[NUM_AMBIENTS];
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

void S_StopSound(int entnum, int entchannel)
{
	for (int i = 0; i < MAX_DYNAMIC_CHANNELS; i++)
	{
		if (channels[i].entnum == entnum
			&& channels[i].entchannel == entchannel)
		{
			alSourceStop(channels[i].source.Id);
			channels[i].sfx = NULL;
			return;
		}
	}
}

void S_StopAllSounds()
{
	if (!sound_started)
		return;

	total_channels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;	// no statics

	for (auto& channel : channels)
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

/*
=================
S_StaticSound
=================
*/
void S_StaticSound(sfx_t* sfx, vec3_t origin, float vol, float attenuation)
{
	if (!sfx)
		return;

	if (total_channels == MAX_CHANNELS)
	{
		Con_Printf("total_channels == MAX_CHANNELS\n");
		return;
	}

	auto ss = &channels[total_channels];
	total_channels++;

	auto sc = S_LoadSound(sfx);
	if (!sc)
		return;

	if (!sc->loopingBuffer)
	{
		Con_Printf("Sound %s not looped\n", sfx->name);
		return;
	}

	ss->sfx = sfx;

	S_SetupChannel(*ss, sfx, origin, vol / 255.f, (attenuation / 64), false);
	alSourcePlay(ss->source.Id);
}

//=============================================================================

/*
===================
S_UpdateAmbientSounds
===================
*/
void S_UpdateAmbientSounds()
{
	if (!snd_ambient)
		return;

	// calc ambient sound levels
	if (!cl.worldmodel)
		return;

	auto l = Mod_PointInLeaf(listener_origin, cl.worldmodel);
	if (!l || !ambient_level.value)
	{
		for (int ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++)
		{
			alSourceStop(channels[ambient_channel].source.Id);
			channels[ambient_channel].sfx = NULL;
		}
		return;
	}

	for (int ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++)
	{
		auto chan = &channels[ambient_channel];
		chan->sfx = ambient_sfx[ambient_channel];
		
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

static void S_UpdateSounds()
{
	//Update all sounds that are looping, clear finished sounds.
	for (int i = 0; i < total_channels; ++i)
	{
		auto& chan = channels[i];

		if (!chan.sfx)
		{
			continue;
		}

		if (chan.sfx->loopingBuffer)
		{
			ALint queued;
			alGetSourcei(chan.source.Id, AL_BUFFERS_QUEUED, &queued);

			if (queued == 2)
			{
				ALint processed;
				alGetSourcei(chan.source.Id, AL_BUFFERS_PROCESSED, &processed);

				if (processed > 0)
				{
					//The intro part has played, convert source to use single looping buffer playing at offset.
					ALint offset = 0;

					if (processed == 1)
					{
						alGetSourcei(chan.source.Id, AL_BYTE_OFFSET, &offset);

						ALint introSize;
						alGetBufferi(chan.sfx->buffer.Id, AL_SIZE, &introSize);

						offset -= introSize;
					}

					alSourceStop(chan.source.Id);
					alSourcei(chan.source.Id, AL_BUFFER, NullBuffer);
					alSourcei(chan.source.Id, AL_BUFFER, chan.sfx->loopingBuffer.Id);
					alSourcei(chan.source.Id, AL_BYTE_OFFSET, offset);
					alSourcei(chan.source.Id, AL_LOOPING, AL_TRUE);
					alSourcePlay(chan.source.Id);
				}
			}
		}
		else
		{
			ALint state;
			alGetSourcei(chan.source.Id, AL_SOURCE_STATE, &state);

			if (state == AL_STOPPED)
			{
				chan.sfx = nullptr;
			}
		}
	}
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
	if (!sound_started)
		return;

	VectorCopy(origin, listener_origin);
	VectorCopy(forward, listener_forward);
	VectorCopy(right, listener_right);
	VectorCopy(up, listener_up);

	const ALfloat orientation[6] =
	{
		forward[0], forward[1], forward[2],
		up[0], up[1], up[2]
	};

	alListenerfv(AL_POSITION, origin);
	alListenerfv(AL_ORIENTATION, orientation);

	// update general area ambient sound sources
	S_UpdateAmbientSounds();
	S_UpdateSounds();

	//
	// debugging output
	//
	if (snd_show.value)
	{
		int total = 0;
		channel_t* ch = channels;
		for (int i = 0; i < total_channels; i++, ch++)
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

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play()
{
	static int hash = 345;
	char name[256];

	for (int i = 1; i < Cmd_Argc(); ++i)
	{
		if (!Q_strrchr(Cmd_Argv(i), '.'))
		{
			Q_strcpy(name, Cmd_Argv(i));
			Q_strcat(name, ".wav");
		}
		else
			Q_strcpy(name, Cmd_Argv(i));
		auto sfx = S_PrecacheSound(name);
		S_StartSound(hash++, 0, sfx, listener_origin, 1.0, 1.0);
	}
}

void S_PlayVol()
{
	static int hash = 543;
	char name[256];

	for (int i = 1; i < Cmd_Argc(); i += 2)
	{
		if (!Q_strrchr(Cmd_Argv(i), '.'))
		{
			Q_strcpy(name, Cmd_Argv(i));
			Q_strcat(name, ".wav");
		}
		else
			Q_strcpy(name, Cmd_Argv(i));
		auto sfx = S_PrecacheSound(name);
		auto vol = Q_atof(Cmd_Argv(i + 1));
		S_StartSound(hash++, 0, sfx, listener_origin, vol, 1.0);
	}
}

void S_SoundList()
{
	unsigned int total = 0;

	sfx_t* sfx = known_sfx;

	for (int i = 0; i < num_sfx; i++, sfx++)
	{
		if (!sfx->buffer)
			continue;

		ALint size;
		alGetBufferi(sfx->buffer.Id, AL_SIZE, &size);
		total += size;

		if (sfx->loopingBuffer)
			Con_Printf("L");
		else
			Con_Printf(" ");

		ALint bits;
		alGetBufferi(sfx->buffer.Id, AL_BITS, &bits);

		Con_Printf("(%2db) %6d : %s\n", static_cast<int>(bits), static_cast<int>(size), sfx->name);
	}
	Con_Printf("Total resident: %u\n", total);
}

void S_LocalSound(const char* sound)
{
	if (nosound.value)
		return;
	if (!sound_started)
		return;

	auto sfx = S_PrecacheSound(sound);
	if (!sfx)
	{
		Con_Printf("S_LocalSound: can't cache %s\n", sound);
		return;
	}
	S_StartSound(cl.viewentity, -1, sfx, vec3_origin, 1, 1);
}
