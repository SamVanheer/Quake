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
#include "sound_internal.h"
#include "SoundSystem.h"

void S_Play();
void S_PlayVol();
void S_StopAllSounds();
void S_SoundList();

struct DummySoundSystem final : public ISoundSystem
{
	bool IsActive() const override { return false; }

	bool IsBlocked() const override { return false; }

	void Block() override {}
	void Unblock() override {}

	bool IsAmbientEnabled() const { return false; }
	void SetAmbientEnabled(bool enable) override {}

	int GetTotalChannelCount() const override { return 0; }

	sfx_t* PrecacheSound(const char* name) override { return nullptr; }

	void StartSound(int entnum, int entchannel, sfx_t* sfx, vec3_t origin, float fvol, float attenuation) override {}
	void StaticSound(sfx_t* sfx, vec3_t origin, float vol, float attenuation) override {}
	void LocalSound(const char* sound) override {}

	void StopSound(int entnum, int entchannel) override {}
	void StopAllSounds() override {}

	void Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up) override {}

	void PrintSoundList() override
	{
		Con_Printf("Total resident: 0\n");
	}
};

// =======================================================================
// Internal sound data & structures
// =======================================================================

static bool snd_firsttime = true;

static std::optional<SoundSystem> openal_audio;

static DummySoundSystem g_DummySoundSystem;

ISoundSystem* g_SoundSystem = &g_DummySoundSystem;

vec3_t listener_origin;
vec3_t listener_forward;
vec3_t listener_right;
vec3_t listener_up;

cvar_t bgmvolume = {"bgmvolume", "1", true};
cvar_t volume = {"volume", "0.7", true};

cvar_t nosound = {"nosound", "0"};
cvar_t precache = {"precache", "1"};
cvar_t bgmbuffer = {"bgmbuffer", "4096"};
cvar_t ambient_level = {"ambient_level", "0.3"};
cvar_t ambient_fade = {"ambient_fade", "100"};
cvar_t snd_show = {"snd_show", "0"};

// ====================================================================
// User-setable variables
// ====================================================================

void S_SoundInfo_f()
{
	if (!g_SoundSystem->IsActive())
	{
		Con_Printf("sound system not started\n");
		return;
	}

	Con_Printf("%5d total_channels\n", g_SoundSystem->GetTotalChannelCount());
}

/*
================
S_Startup
================
*/
void S_Startup()
{
	const bool wasFirstTime = snd_firsttime;

	if (snd_firsttime)
	{
		snd_firsttime = false;

		openal_audio = SoundSystem::Create();

		if (openal_audio.has_value())
		{
			g_SoundSystem = &openal_audio.value();

			Con_SafePrintf("OpenAL sound initialized\n");
		}
		else
		{
			Con_SafePrintf("OpenAL sound failed to init\n");
		}
	}

	if (!openal_audio.has_value() && wasFirstTime)
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
	Cvar_RegisterVariable(&snd_show);

	S_Startup();
}

// =======================================================================
// Shutdown sound engine
// =======================================================================
void S_Shutdown()
{
	g_SoundSystem = &g_DummySoundSystem;
	openal_audio.reset();
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
		auto sfx = g_SoundSystem->PrecacheSound(name);
		g_SoundSystem->StartSound(hash++, 0, sfx, listener_origin, 1.0, 1.0);
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
		auto sfx = g_SoundSystem->PrecacheSound(name);
		auto vol = Q_atof(Cmd_Argv(i + 1));
		g_SoundSystem->StartSound(hash++, 0, sfx, listener_origin, vol, 1.0);
	}
}

void S_StopAllSounds()
{
	g_SoundSystem->StopAllSounds();
}

void S_SoundList()
{
	g_SoundSystem->PrintSoundList();
}
