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

struct DummySoundSystem final : public ISoundSystem
{
	bool IsActive() const override { return false; }

	bool IsBlocked() const override { return false; }

	void Block() override {}
	void Unblock() override {}

	bool IsAmbientEnabled() const { return false; }
	void SetAmbientEnabled(bool enable) override {}

	int GetTotalChannelCount() const override { return 0; }

	SoundIndex PrecacheSound(const char* name) override { return {}; }

	void StartSound(int entnum, int entchannel, SoundIndex index, vec3_t origin, float fvol, float attenuation) override {}
	void StaticSound(SoundIndex index, vec3_t origin, float vol, float attenuation) override {}
	void LocalSound(const char* sound, float vol = 1.f) override {}

	void StopSound(int entnum, int entchannel) override {}
	void StopAllSounds() override {}

	void Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up) override {}

	void PrintSoundList() override
	{
		Con_Printf("Total resident: 0\n");
	}
};

/*
*	Internal sound data & structures
*/

static std::optional<SoundSystem> openal_audio;

static DummySoundSystem g_DummySoundSystem;

ISoundSystem* g_SoundSystem = &g_DummySoundSystem;

cvar_t bgmvolume = {"bgmvolume", "1", true};
cvar_t volume = {"volume", "0.7", true};

cvar_t nosound = {"nosound", "0"};
cvar_t precache = {"precache", "1"};
cvar_t bgmbuffer = {"bgmbuffer", "4096"};
cvar_t ambient_level = {"ambient_level", "0.3"};
cvar_t ambient_fade = {"ambient_fade", "100"};
cvar_t snd_show = {"snd_show", "0"};

/*
*	console functions
*/

void S_Play()
{
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
		g_SoundSystem->LocalSound(name);
	}
}

void S_PlayVol()
{
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
		auto vol = Q_atof(Cmd_Argv(i + 1));
		g_SoundSystem->LocalSound(name, vol);
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

void S_SoundInfo_f()
{
	if (!g_SoundSystem->IsActive())
	{
		Con_Printf("sound system not started\n");
		return;
	}

	Con_Printf("%5d total_channels\n", g_SoundSystem->GetTotalChannelCount());
}

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

	if (openal_audio = SoundSystem::Create(); openal_audio.has_value())
	{
		g_SoundSystem = &openal_audio.value();
		Con_SafePrintf("OpenAL sound initialized\n");
	}
	else
	{
		Con_SafePrintf("No sound device initialized\n");
	}
}

void S_Shutdown()
{
	g_SoundSystem = &g_DummySoundSystem;
	openal_audio.reset();
}
