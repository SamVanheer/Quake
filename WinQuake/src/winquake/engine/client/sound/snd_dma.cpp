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
void S_Update_();
void S_StopAllSounds(bool clear);
void S_StopAllSoundsC();

struct DummySoundSystem final : public ISoundSystem
{
	bool IsBlocked() const override { return false; }

	void Block() override {}
	void Unblock() override {}

	int GetDMAPosition() const override { return 0; }

	void Submit() override {}
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

// pointer should go away
volatile dma_t* shm = 0;
volatile dma_t sn;

vec3_t listener_origin;
vec3_t listener_forward;
vec3_t listener_right;
vec3_t listener_up;
vec_t sound_nominal_clip_dist = 1000.0;

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
cvar_t _snd_mixahead = {"_snd_mixahead", "0.1", true};

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

	Con_Printf("%5d stereo\n", shm->channels - 1);
	Con_Printf("%5d samples\n", shm->samples);
	Con_Printf("%5d samplepos\n", shm->samplepos);
	Con_Printf("%5d samplebits\n", shm->samplebits);
	Con_Printf("%5d submission_chunk\n", shm->submission_chunk);
	Con_Printf("%5d speed\n", shm->speed);
	Con_Printf("0x%x dma buffer\n", shm->buffer);
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
	Cmd_AddCommand("stopsound", S_StopAllSoundsC);
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
	Cvar_RegisterVariable(&_snd_mixahead);

	snd_initialized = true;

	S_Startup();

	if (sound_started && (!shm || !shm->buffer))
	{
		//In case the dma buffer was incorrecly initialized, treat this as though the sound system wasn't initialized at all.
		Con_Printf("Error starting sound system\n");
		S_Shutdown();
		sound_started = false;
	}

	if (!sound_started)
	{
		//Can't play sounds, so just stop here.
		return;
	}

	known_sfx = reinterpret_cast<sfx_t*>(Hunk_AllocName(MAX_SFX * sizeof(sfx_t), "sfx_t"));
	num_sfx = 0;

	Con_Printf("Sound sampling rate: %i\n", shm->speed);

	// provides a tick sound until washed clean

//	if (shm->buffer)
//		shm->buffer[4] = shm->buffer[5] = 0x7f;	// force a pop for debugging

	ambient_sfx[AMBIENT_WATER] = S_PrecacheSound("ambience/water1.wav");
	ambient_sfx[AMBIENT_SKY] = S_PrecacheSound("ambience/wind2.wav");

	S_StopAllSounds(true);
}

// =======================================================================
// Shutdown sound engine
// =======================================================================
void S_Shutdown()
{
	if (!sound_started)
		return;

	if (shm)
		shm->gamealive = false;

	shm = 0;
	sound_started = false;

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
S_TouchSound

==================
*/
void S_TouchSound(const char* name)
{
	if (!sound_started)
		return;

	auto sfx = S_FindName(name);
	Cache_Check(&sfx->cache);
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

		if (channels[ch_idx].end - paintedtime < life_left)
		{
			life_left = channels[ch_idx].end - paintedtime;
			first_to_die = ch_idx;
		}
	}

	if (first_to_die == -1)
		return NULL;

	if (channels[first_to_die].sfx)
		channels[first_to_die].sfx = NULL;

	return &channels[first_to_die];
}

/*
=================
SND_Spatialize
spatializes a channel
=================
*/
void SND_Spatialize(channel_t* ch)
{
	// anything coming from the view entity will allways be full volume
	if (ch->entnum == cl.viewentity)
	{
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
		return;
	}

	// calculate stereo seperation and distance attenuation

	vec3_t source_vec;
	VectorSubtract(ch->origin, listener_origin, source_vec);

	const vec_t dist = VectorNormalize(source_vec) * ch->dist_mult;

	const vec_t dot = DotProduct(listener_right, source_vec);

	vec_t rscale = 1.0;
	vec_t lscale = 1.0;

	if (shm->channels != 1)
	{
		rscale += dot;
		lscale -= dot;
	}

	// add in distance effect
	vec_t scale = (1.0 - dist) * rscale;
	ch->rightvol = (int)(ch->master_vol * scale);
	if (ch->rightvol < 0)
		ch->rightvol = 0;

	scale = (1.0 - dist) * lscale;
	ch->leftvol = (int)(ch->master_vol * scale);
	if (ch->leftvol < 0)
		ch->leftvol = 0;
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

	const int vol = fvol * 255;

	// pick a channel to play on
	auto target_chan = SND_PickChannel(entnum, entchannel);
	if (!target_chan)
		return;

	// spatialize
	memset(target_chan, 0, sizeof(*target_chan));
	VectorCopy(origin, target_chan->origin);
	target_chan->dist_mult = attenuation / sound_nominal_clip_dist;
	target_chan->master_vol = vol;
	target_chan->entnum = entnum;
	target_chan->entchannel = entchannel;
	SND_Spatialize(target_chan);

	if (!target_chan->leftvol && !target_chan->rightvol)
		return;		// not audible at all

// new channel
	auto sc = S_LoadSound(sfx);
	if (!sc)
	{
		target_chan->sfx = NULL;
		return;		// couldn't load the sound's data
	}

	target_chan->sfx = sfx;
	target_chan->pos = 0.0;
	target_chan->end = paintedtime + sc->length;

	// if an identical sound has also been started this frame, offset the pos
	// a bit to keep it from just making the first one louder
	auto check = &channels[NUM_AMBIENTS];
	for (int ch_idx = NUM_AMBIENTS; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; ch_idx++, check++)
	{
		if (check == target_chan)
			continue;
		if (check->sfx == sfx && !check->pos)
		{
			int skip = rand() % (int)(0.1 * shm->speed);
			if (skip >= target_chan->end)
				skip = target_chan->end - 1;
			target_chan->pos += skip;
			target_chan->end -= skip;
			break;
		}

	}
}

void S_StopSound(int entnum, int entchannel)
{
	for (int i = 0; i < MAX_DYNAMIC_CHANNELS; i++)
	{
		if (channels[i].entnum == entnum
			&& channels[i].entchannel == entchannel)
		{
			channels[i].end = 0;
			channels[i].sfx = NULL;
			return;
		}
	}
}

void S_StopAllSounds(bool clear)
{
	if (!sound_started)
		return;

	total_channels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;	// no statics

	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (channels[i].sfx)
			channels[i].sfx = NULL;
	}

	Q_memset(channels, 0, MAX_CHANNELS * sizeof(channel_t));

	if (clear)
		S_ClearBuffer();
}

void S_StopAllSoundsC()
{
	S_StopAllSounds(true);
}

void S_ClearBuffer()
{
	if (!sound_started)
		return;

	const int clear = shm->samplebits == 8 ? 0x80: 0;

	Q_memset(shm->buffer, clear, shm->samples * shm->samplebits / 8);
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

	if (sc->loopstart == -1)
	{
		Con_Printf("Sound %s not looped\n", sfx->name);
		return;
	}

	ss->sfx = sfx;
	VectorCopy(origin, ss->origin);
	ss->master_vol = vol;
	ss->dist_mult = (attenuation / 64) / sound_nominal_clip_dist;
	ss->end = paintedtime + sc->length;

	SND_Spatialize(ss);
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
			channels[ambient_channel].sfx = NULL;
		return;
	}

	for (int ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++)
	{
		auto chan = &channels[ambient_channel];
		chan->sfx = ambient_sfx[ambient_channel];

		float vol = ambient_level.value * l->ambient_sound_level[ambient_channel];
		if (vol < 8)
			vol = 0;

		// don't adjust volume too fast
		if (chan->master_vol < vol)
		{
			chan->master_vol += host_frametime * ambient_fade.value;
			if (chan->master_vol > vol)
				chan->master_vol = vol;
		}
		else if (chan->master_vol > vol)
		{
			chan->master_vol -= host_frametime * ambient_fade.value;
			if (chan->master_vol < vol)
				chan->master_vol = vol;
		}

		chan->leftvol = chan->rightvol = chan->master_vol;
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

	// update general area ambient sound sources
	S_UpdateAmbientSounds();

	channel_t* combine = NULL;

	// update spatialization for static and dynamic sounds	
	channel_t* ch = channels + NUM_AMBIENTS;
	for (int i = NUM_AMBIENTS; i < total_channels; i++, ch++)
	{
		if (!ch->sfx)
			continue;
		SND_Spatialize(ch);         // respatialize channel
		if (!ch->leftvol && !ch->rightvol)
			continue;

		// try to combine static sounds with a previous channel of the same
		// sound effect so we don't mix five torches every frame

		if (i >= MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS)
		{
			// see if it can just use the last one
			if (combine && combine->sfx == ch->sfx)
			{
				combine->leftvol += ch->leftvol;
				combine->rightvol += ch->rightvol;
				ch->leftvol = ch->rightvol = 0;
				continue;
			}
			// search for one
			combine = channels + MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;
			int j;
			for (j = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS; j < i; j++, combine++)
				if (combine->sfx == ch->sfx)
					break;

			if (j == total_channels)
			{
				combine = NULL;
			}
			else
			{
				if (combine != ch)
				{
					combine->leftvol += ch->leftvol;
					combine->rightvol += ch->rightvol;
					ch->leftvol = ch->rightvol = 0;
				}
				continue;
			}
		}


	}

	//
	// debugging output
	//
	if (snd_show.value)
	{
		int total = 0;
		ch = channels;
		for (int i = 0; i < total_channels; i++, ch++)
		{
			if (ch->sfx && (ch->leftvol || ch->rightvol))
			{
				//Con_Printf ("%3i %3i %s\n", ch->leftvol, ch->rightvol, ch->sfx->name);
				total++;
			}
		}

		Con_Printf("----(%i)----\n", total);
	}

	// mix some sound
	S_Update_();
}

void GetSoundtime()
{
	static int buffers = 0;
	static int oldsamplepos = 0;

	const int fullsamples = shm->samples / shm->channels;

	// it is possible to miscount buffers if it has wrapped twice between
	// calls to S_Update.  Oh well.
	const int samplepos = g_SoundSystem->GetDMAPosition();

	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped

		if (paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			paintedtime = fullsamples;
			S_StopAllSounds(true);
		}
	}
	oldsamplepos = samplepos;

	soundtime = buffers * fullsamples + samplepos / shm->channels;
}

void S_ExtraUpdate()
{
	if (snd_noextraupdate.value)
		return;		// don't pollute timings
	S_Update_();
}

void S_Update_()
{
	if (!sound_started)
		return;

	// Updates DMA time
	GetSoundtime();

	// check to make sure that we haven't overshot
	if (paintedtime < soundtime)
	{
		//Con_Printf ("S_Update_ : overflow\n");
		paintedtime = soundtime;
	}

	// mix ahead of current position
	unsigned int endtime = soundtime + _snd_mixahead.value * shm->speed;
	const int samps = shm->samples >> (shm->channels - 1);
	if (endtime - soundtime > static_cast<unsigned>(samps))
		endtime = soundtime + samps;

	S_PaintChannels(endtime);

	g_SoundSystem->Submit();
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
	int total = 0;

	sfx_t* sfx = known_sfx;

	for (int i = 0; i < num_sfx; i++, sfx++)
	{
		auto sc = reinterpret_cast<sfxcache_t*>(Cache_Check(&sfx->cache));
		if (!sc)
			continue;
		const int size = sc->length * sc->width * (sc->stereo + 1);
		total += size;
		if (sc->loopstart >= 0)
			Con_Printf("L");
		else
			Con_Printf(" ");
		Con_Printf("(%2db) %6i : %s\n", sc->width * 8, size, sfx->name);
	}
	Con_Printf("Total resident: %i\n", total);
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
