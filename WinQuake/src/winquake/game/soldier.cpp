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
#include "fight.h"
#include "Game.h"
#include "items.h"
#include "monsters.h"
#include "player.h"
#include "subs.h"
#include "weapons.h"

/*
==============================================================================

SOLDIER / PLAYER

==============================================================================
*/
void army_fire(edict_t* self);
void army_atk1(edict_t* self);
void army_run1(edict_t* self);

void army_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void army_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (PF_random() < 0.2)
			PF_sound(self, CHAN_VOICE, "soldier/idle.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	default:
		ai_walk(self, 1);
		break;

	case 4:
	case 8:
	case 9:
	case 10:
	case 20:
		ai_walk(self, 2);
		break;

	case 5:
	case 16:
	case 17:
	case 18:
	case 19:
		ai_walk(self, 3);
		break;

	case 6:
	case 7:
		ai_walk(self, 4);
		break;

	case 12:
		ai_walk(self, 0);
		break;
	}
}

void army_run_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (PF_random() < 0.2)
			PF_sound(self, CHAN_VOICE, "soldier/idle.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	default:
		ai_run(self, 11);
		break;

	case 1:
	case 5:
		ai_run(self, 15);
		break;

	case 2:
	case 3:
	case 6:
		ai_run(self, 10);
		break;

	case 4:
	case 7:
		ai_run(self, 8);
		break;
	}
}

void army_shoot_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	default:
		ai_face(self);
		break;

	case 4:
		ai_face(self);
		army_fire(self);
		self->v.effects |= EF_MUZZLEFLASH;
		break;

	case 6:
		ai_face(self);
		SUB_CheckRefire(self, army_atk1);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = army_run1;
	}
}

void army_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = army_run1;
		ai_pain(self, 1);
	}
}

void army_painb_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 1:
		ai_painforward(self, 13);
		break;

	case 2:
		ai_painforward(self, 9);
		break;

	case 11:
		ai_pain(self, 2);
		break;

	default: break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = army_run1;
	}
}

void army_painc_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 1:
	case 7:
		ai_pain(self, 1);
		break;

	case 4:
	case 5:
		ai_painforward(self, 1);
		break;

	case 8:
		ai_painforward(self, 4);
		break;

	case 9:
		ai_painforward(self, 3);
		break;

	case 10:
		ai_painforward(self, 6);
		break;

	case 11:
		ai_painforward(self, 8);
		break;

	default: break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = army_run1;
	}
}

void army_die_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 2)
	{
		self->v.solid = SOLID_NOT;
		self->v.ammo_shells = 5;
		DropBackpack(self);
	}
}

void army_diec_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 1:
		ai_back(self, 5);
		break;

	case 2:
		self->v.solid = SOLID_NOT;
		self->v.ammo_shells = 5;
		DropBackpack(self);
		ai_back(self, 4);
		break;

	case 3:
		ai_back(self, 13);
		break;
	case 4:
		ai_back(self, 3);
		break;
	case 5:
		ai_back(self, 4);
		break;

	default: break;
	}
}

const Animations SoldierAnimations = MakeAnimations(
	{
		{"stand", 8, &army_stand_frame, AnimationFlag::Looping},
		{"death", 10, &army_die_frame},
		{"deathc", 11, &army_diec_frame},
		{"load", 11}, //Never used
		{"pain", 6, &army_pain_frame},
		{"painb", 14, &army_painb_frame},
		{"painc", 13, &army_painc_frame},
		{"run", 8, &army_run_frame, AnimationFlag::Looping},
		{"shoot", 9, &army_shoot_frame},
		{"prowl_", 24, &army_walk_frame, AnimationFlag::Looping}
	});

const Animations* army_animations_get(edict_t* self)
{
	return &SoldierAnimations;
}

LINK_FUNCTION_TO_NAME(army_animations_get);

/*
==============================================================================
SOLDIER CODE
==============================================================================
*/

void army_stand1(edict_t* self)
{
	animation_set(self, "stand");
}

LINK_FUNCTION_TO_NAME(army_stand1);

void army_walk1(edict_t* self)
{
	animation_set(self, "prowl_");
}

LINK_FUNCTION_TO_NAME(army_walk1);

void army_run1(edict_t* self)
{
	animation_set(self, "run");
}

LINK_FUNCTION_TO_NAME(army_run1);

void army_atk1(edict_t* self)
{
	animation_set(self, "shoot");
}

LINK_FUNCTION_TO_NAME(army_atk1);

void army_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

void army_painb1(edict_t* self)
{
	animation_set(self, "painb");
}

void army_painc1(edict_t* self)
{
	animation_set(self, "painc");
}

void army_die1(edict_t* self)
{
	animation_set(self, "death");
}

void army_cdie1(edict_t* self)
{
	animation_set(self, "deathc");
}

void army_pain(edict_t* self, edict_t* attacker, float damage)
{
	if (self->v.pain_finished > pr_global_struct->time)
		return;

	const float r = PF_random();

	if (r < 0.2)
	{
		self->v.pain_finished = pr_global_struct->time + 0.6;
		army_pain1(self);
		PF_sound(self, CHAN_VOICE, "soldier/pain1.wav", 1, ATTN_NORM);
	}
	else if (r < 0.6)
	{
		self->v.pain_finished = pr_global_struct->time + 1.1;
		army_painb1(self);
		PF_sound(self, CHAN_VOICE, "soldier/pain2.wav", 1, ATTN_NORM);
	}
	else
	{
		self->v.pain_finished = pr_global_struct->time + 1.1;
		army_painc1(self);
		PF_sound(self, CHAN_VOICE, "soldier/pain2.wav", 1, ATTN_NORM);
	}
}

LINK_FUNCTION_TO_NAME(army_pain);

void army_fire(edict_t* self)
{
	ai_face(self);

	PF_sound(self, CHAN_WEAPON, "soldier/sattck1.wav", 1, ATTN_NORM);

	// fire somewhat behind the player, so a dodging player is harder to hit
	auto en = self->v.enemy;

	auto dir = AsVector(en->v.origin) - AsVector(en->v.velocity) * 0.2f;
	PF_normalize(dir - AsVector(self->v.origin), dir);

	FireBullets(self, 4, dir, Vector3D{0.1f, 0.1f, 0});
}

void army_die(edict_t* self)
{
	// check for gib
	if (self->v.health < -35)
	{
		PF_sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead(self, "progs/h_guard.mdl", self->v.health);
		ThrowGib(self, "progs/gib1.mdl", self->v.health);
		ThrowGib(self, "progs/gib2.mdl", self->v.health);
		ThrowGib(self, "progs/gib3.mdl", self->v.health);
		return;
	}

	// regular death
	PF_sound(self, CHAN_VOICE, "soldier/death1.wav", 1, ATTN_NORM);
	if (PF_random() < 0.5)
		army_die1(self);
	else
		army_cdie1(self);
}

LINK_FUNCTION_TO_NAME(army_die);

/*QUAKED monster_army (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void monster_army(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model("progs/soldier.mdl");
	PF_precache_model("progs/h_guard.mdl");
	PF_precache_model("progs/gib1.mdl");
	PF_precache_model("progs/gib2.mdl");
	PF_precache_model("progs/gib3.mdl");

	PF_precache_sound("soldier/death1.wav");
	PF_precache_sound("soldier/idle.wav");
	PF_precache_sound("soldier/pain1.wav");
	PF_precache_sound("soldier/pain2.wav");
	PF_precache_sound("soldier/sattck1.wav");
	PF_precache_sound("soldier/sight1.wav");

	PF_precache_sound("player/udeath.wav");		// gib death


	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel(self, "progs/soldier.mdl");

	PF_setsize(self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 40});
	self->v.health = 30;

	self->v.th_stand = army_stand1;
	self->v.th_walk = army_walk1;
	self->v.th_run = army_run1;
	self->v.th_missile = army_atk1;
	self->v.th_pain = army_pain;
	self->v.th_die = army_die;
	self->v.animations_get = &army_animations_get;

	walkmonster_start(self);
}

LINK_ENTITY_TO_CLASS(monster_army);
