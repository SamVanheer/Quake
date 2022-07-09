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

#include "quakedef.h"
#include "ai.h"
#include "animation.h"
#include "combat.h"
#include "fight.h"
#include "Game.h"
#include "monsters.h"
#include "player.h"
#include "subs.h"
#include "weapons.h"

/*
==============================================================================

SHAL-RATH

==============================================================================
*/

void ShalMissile(edict_t* self);
void shal_run1(edict_t* self);

enum class ShalrathWalkMode
{
	Stand = 0,
	Walk,
	Run
};

void shalrath_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void shalrath_walk_core_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (PF_random() < 0.2)
			PF_sound(self, CHAN_VOICE, "shalrath/idle.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	case 0:
	case 7:
		ai_walk(self, 6);
		break;

	case 1:
	case 10:
		ai_walk(self, 4);
		break;

	case 2:
	case 3:
	case 4:
	case 5:
	case 9:
		ai_walk(self, 0);
		break;

	case 6:
	case 8:
	case 11:
		ai_walk(self, 5);
		break;
	}
}

void shalrath_run_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (PF_random() < 0.2)
			PF_sound(self, CHAN_VOICE, "shalrath/idle.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	case 0:
	case 7:
		ai_run(self, 6);
		break;

	case 1:
	case 10:
		ai_run(self, 4);
		break;

	case 2:
	case 3:
	case 4:
	case 5:
	case 9:
		ai_run(self, 0);
		break;

	case 6:
	case 8:
	case 11:
		ai_run(self, 5);
		break;
	}
}

void shalrath_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (static_cast<ShalrathWalkMode>(self->v.animation_mode))
	{
	default:
	case ShalrathWalkMode::Stand:
		shalrath_stand_frame(self, animation, frame);
		break;

	case ShalrathWalkMode::Walk:
		shalrath_walk_core_frame(self, animation, frame);
		break;

	case ShalrathWalkMode::Run:
		shalrath_run_frame(self, animation, frame);
		break;
	}
}

void shalrath_attack_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		PF_sound(self, CHAN_VOICE, "shalrath/attack.wav", 1, ATTN_NORM);
	}

	switch (frame)
	{
	default:
		ai_face(self);
		break;

	case 8:
		ShalMissile(self);
		break;

	case 10: break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &shal_run1;
	}
}

void shalrath_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = &shal_run1;
	}
}

void shalrath_death_frame(edict_t* self, const Animation* animation, int frame)
{
}

const Animations ShalrathAnimations = MakeAnimations(
	{
		{"attack", 11, &shalrath_attack_frame},
		{"pain", 5, &shalrath_pain_frame},
		{"death", 7, &shalrath_death_frame},
		{"walk", 12, &shalrath_walk_frame, AnimationFlag::Looping}
	});

const Animations* shal_animations_get(edict_t* self)
{
	return &ShalrathAnimations;
}

LINK_FUNCTION_TO_NAME(shal_animations_get);

void shal_stand(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(ShalrathWalkMode::Stand);
	animation_set(self, "walk");
}

LINK_FUNCTION_TO_NAME(shal_stand);

void shal_walk1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(ShalrathWalkMode::Walk);
	animation_set(self, "walk");
}

LINK_FUNCTION_TO_NAME(shal_walk1);

void shal_run1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(ShalrathWalkMode::Run);
	animation_set(self, "walk");
}

LINK_FUNCTION_TO_NAME(shal_run1);

void shal_attack1(edict_t* self)
{
	animation_set(self, "attack");
}

LINK_FUNCTION_TO_NAME(shal_attack1);

void shal_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

void shal_death1(edict_t* self)
{
	animation_set(self, "death");
}

void shalrath_pain(edict_t* self, edict_t* attacker, float damage)
{
	if (self->v.pain_finished > pr_global_struct->time)
		return;

	PF_sound(self, CHAN_VOICE, "shalrath/pain.wav", 1, ATTN_NORM);
	shal_pain1(self);
	self->v.pain_finished = pr_global_struct->time + 3;
}

LINK_FUNCTION_TO_NAME(shalrath_pain);

void shalrath_die(edict_t* self)
{
	// check for gib
	if (self->v.health < -90)
	{
		PF_sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead(self, "progs/h_shal.mdl", self->v.health);
		ThrowGib(self, "progs/gib1.mdl", self->v.health);
		ThrowGib(self, "progs/gib2.mdl", self->v.health);
		ThrowGib(self, "progs/gib3.mdl", self->v.health);
		return;
	}

	PF_sound(self, CHAN_VOICE, "shalrath/death.wav", 1, ATTN_NORM);
	shal_death1(self);
	self->v.solid = SOLID_NOT;
	// insert death sounds here
}

LINK_FUNCTION_TO_NAME(shalrath_die);

/*
================
ShalMissile
================
*/
void ShalMissileTouch(edict_t* self, edict_t* other);
void ShalHome(edict_t* self);
void ShalMissile(edict_t* self)
{
	Vector3D dir;

	PF_normalize((AsVector(self->v.enemy->v.origin) + Vector3D{0, 0, 10}) - AsVector(self->v.origin), dir);
	const float dist = PF_vlen(AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin));
	float flytime = dist * 0.002;
	if (flytime < 0.1)
		flytime = 0.1f;

	self->v.effects = self->v.effects | EF_MUZZLEFLASH;
	PF_sound(self, CHAN_WEAPON, "shalrath/attack2.wav", 1, ATTN_NORM);

	auto missile = PF_Spawn();
	missile->v.owner = self;

	missile->v.solid = SOLID_BBOX;
	missile->v.movetype = MOVETYPE_FLYMISSILE;
	PF_setmodel(missile, "progs/v_spike.mdl");

	PF_setsize(missile, vec3_origin, vec3_origin);

	AsVector(missile->v.origin) = AsVector(self->v.origin) + Vector3D{0, 0, 10};
	AsVector(missile->v.velocity) = dir * 400;
	AsVector(missile->v.avelocity) = Vector3D{300, 300, 300};
	missile->v.nextthink = flytime + pr_global_struct->time;
	missile->v.think = ShalHome;
	missile->v.enemy = self->v.enemy;
	missile->v.touch = ShalMissileTouch;
}

void ShalHome(edict_t* self)
{
	auto vtemp = AsVector(self->v.enemy->v.origin) + Vector3D{0, 0, 10};
	if (self->v.enemy->v.health < 1)
	{
		PF_Remove(self);
		return;
	}

	Vector3D dir;
	PF_normalize(vtemp - AsVector(self->v.origin), dir);
	if (pr_global_struct->skill == 3)
		AsVector(self->v.velocity) = dir * 350;
	else
		AsVector(self->v.velocity) = dir * 250;
	self->v.nextthink = pr_global_struct->time + 0.2;
	self->v.think = ShalHome;
}

LINK_FUNCTION_TO_NAME(ShalHome);

void ShalMissileTouch(edict_t* self, edict_t* other)
{
	if (other == self->v.owner)
		return;		// don't explode on owner

	if (!strcmp(other->v.classname, "monster_zombie"))
		T_Damage(self, other, self, self, 110);
	T_RadiusDamage(self, self, self->v.owner, 40, pr_global_struct->world);
	PF_sound(self, CHAN_WEAPON, "weapons/r_exp3.wav", 1, ATTN_NORM);

	PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte(MSG_BROADCAST, TE_EXPLOSION);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[0]);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[1]);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[2]);

	AsVector(self->v.velocity) = AsVector(vec3_origin);
	self->v.touch = SUB_NullTouch;
	PF_setmodel(self, "progs/s_explod.spr");
	self->v.solid = SOLID_NOT;
	s_explode1(self);
}

LINK_FUNCTION_TO_NAME(ShalMissileTouch);

//=================================================================

/*QUAKED monster_shalrath (1 0 0) (-32 -32 -24) (32 32 48) Ambush
*/
void monster_shalrath(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model("progs/shalrath.mdl");
	PF_precache_model("progs/h_shal.mdl");
	PF_precache_model("progs/v_spike.mdl");

	PF_precache_sound("shalrath/attack.wav");
	PF_precache_sound("shalrath/attack2.wav");
	PF_precache_sound("shalrath/death.wav");
	PF_precache_sound("shalrath/idle.wav");
	PF_precache_sound("shalrath/pain.wav");
	PF_precache_sound("shalrath/sight.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel(self, "progs/shalrath.mdl");
	PF_setsize(self, VEC_HULL2_MIN, VEC_HULL2_MAX);
	self->v.health = 400;

	self->v.th_stand = shal_stand;
	self->v.th_walk = shal_walk1;
	self->v.th_run = shal_run1;
	self->v.th_die = shalrath_die;
	self->v.th_pain = shalrath_pain;
	self->v.th_missile = shal_attack1;
	self->v.animations_get = &shal_animations_get;

	self->v.think = walkmonster_start;
	self->v.nextthink = pr_global_struct->time + 0.1 + PF_random() * 0.1;

}

LINK_ENTITY_TO_CLASS(monster_shalrath);
