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
// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

#include "ISoundSystem.h"
#include "SoundSystem.h"

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

typedef struct
{
	int left;
	int right;
} portable_samplepair_t;

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

typedef struct
{
	int rate;
	int width;
	int channels;
	int loopstart;
	int samples;
	int dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;

extern ISoundSystem* g_SoundSystem;

void S_Init();
void S_Shutdown();
void S_StartSound(int entnum, int entchannel, sfx_t* sfx, vec3_t origin, float fvol, float attenuation);
void S_StaticSound(sfx_t* sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound(int entnum, int entchannel);
void S_StopAllSounds();
void S_Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);

sfx_t* S_PrecacheSound(const char* sample);

// ====================================================================
// User-setable variables
// ====================================================================

constexpr int MAX_CHANNELS = 128;
constexpr int MAX_DYNAMIC_CHANNELS = 8;

extern channel_t channels[MAX_CHANNELS];
// 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds

extern int total_channels;

extern int paintedtime;
extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;

extern cvar_t bgmvolume;
extern cvar_t volume;

extern bool	snd_initialized;

void S_LocalSound(const char* s);
sfx_t* S_LoadSound(sfx_t* s);

void S_AmbientOff();
void S_AmbientOn();

#endif
