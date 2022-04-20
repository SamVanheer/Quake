/*  Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	See file, 'COPYING', for details.
*/

#pragma once

#include "IGame.h"
#include "entitymap.h"
#include "vector.h"

class Game : public IGame
{
public:
	void Initialize() override;
	void Shutdown() override;

	void NewMapStarted() override;

	bool SpawnEntity(edict_t* entity, const char* classname) override;
	void EntityThink(edict_t* entity, edict_t* other) override;
	void EntityTouch(edict_t* entity, edict_t* other) override;
	void EntityBlocked(edict_t* entity, edict_t* other) override;

	void StartFrame(edict_t* entities) override;

	void SetChangeParms(edict_t* self, float* parms) override;
	void SetNewParms(float* parms) override;
	void ClientKill(edict_t* self) override;
	void PutClientInServer(edict_t* self) override;
	void PlayerPreThink(edict_t* self) override;
	void PlayerPostThink(edict_t* self) override;
	void ClientConnect(edict_t* self) override;
	void ClientDisconnect(edict_t* self) override;
};

//
// globals
//
//TODO: need to reset these on map load
inline float	gameover;		// set when a rule exits

inline edict_t* activator;		// the entity that activated a trigger or brush

inline edict_t* damage_attacker;	// set by T_Damage
inline float	framecount;

inline float	game_skill;

inline float	intermission_running;
inline float	intermission_exittime;

//
// AI globals
//
inline float	current_yaw;

//
// when a monster becomes angry at a player, that monster will be used
// as the sight target the next frame so that monsters near that one
// will wake up even if they wouldn't have noticed the player
//
inline edict_t* sight_entity;
inline float	sight_entity_time;

//
// constants
//

// edict.movetype values
constexpr float	MOVETYPE_BOUNCEMISSILE = 11;	// bounce with extra size

// range values
constexpr float	RANGE_MELEE = 0;
constexpr float	RANGE_NEAR = 1;
constexpr float	RANGE_MID = 2;
constexpr float	RANGE_FAR = 3;

// deadflag values

constexpr float	DEAD_RESPAWNABLE = 3;

// point content values

constexpr float	CONTENT_EMPTY = -1;
constexpr float	CONTENT_SOLID = -2;
constexpr float	CONTENT_WATER = -3;
constexpr float	CONTENT_SLIME = -4;
constexpr float	CONTENT_LAVA = -5;
constexpr float	CONTENT_SKY = -6;

constexpr float	STATE_TOP = 0;
constexpr float	STATE_BOTTOM = 1;
constexpr float	STATE_UP = 2;
constexpr float	STATE_DOWN = 3;

constexpr vec3_t VEC_ORIGIN = {0, 0, 0};
constexpr vec3_t VEC_HULL_MIN = {-16, -16, -24};
constexpr vec3_t VEC_HULL_MAX = {16, 16, 32};

constexpr vec3_t VEC_HULL2_MIN = {-32, -32, -24};
constexpr vec3_t VEC_HULL2_MAX = {32, 32, 64};

// protocol bytes
constexpr float	SVC_TEMPENTITY = 23;
constexpr float	SVC_KILLEDMONSTER = 27;
constexpr float	SVC_FOUNDSECRET = 28;
constexpr float	SVC_INTERMISSION = 30;
constexpr float	SVC_FINALE = 31;
constexpr float	SVC_CDTRACK = 32;
constexpr float	SVC_SELLSCREEN = 33;

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
constexpr float	CHAN_AUTO = 0;
constexpr float	CHAN_WEAPON = 1;
constexpr float	CHAN_VOICE = 2;
constexpr float	CHAN_ITEM = 3;
constexpr float	CHAN_BODY = 4;

constexpr float	ATTN_NONE = 0;
constexpr float	ATTN_NORM = 1;
constexpr float	ATTN_IDLE = 2;
constexpr float	ATTN_STATIC = 3;

// update types

constexpr float	UPDATE_GENERAL = 0;
constexpr float	UPDATE_STATIC = 1;
constexpr float	UPDATE_BINARY = 2;
constexpr float	UPDATE_TEMP = 3;

// messages
constexpr float	MSG_BROADCAST = 0;		// unreliable to all
constexpr float	MSG_ONE = 1;		// reliable to one (msg_entity)
constexpr float	MSG_ALL = 2;		// reliable to all
constexpr float	MSG_INIT = 3;		// write to the init string

constexpr int AS_STRAIGHT = 1;
constexpr int AS_SLIDING = 2;
constexpr int AS_MELEE = 3;
constexpr int AS_MISSILE = 4;
