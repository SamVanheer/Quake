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
void enforcer_fire(edict_t* self);
void enf_run1(edict_t* self);
void enf_atk1(edict_t* self);

void enforcer_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void enforcer_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "enforcer/idle1.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	default:
		ai_walk(self, 4);
		break;

	case 0:
	case 5:
	case 6:
	case 8:
	case 12:
	case 15:
		ai_walk(self, 2);
		break;

	case 3:
	case 13:
		ai_walk(self, 3);
		break;

	case 4:
	case 7:
	case 11:
		ai_walk(self, 1);
		break;
	}
}

void enforcer_run_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "enforcer/idle1.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	case 0:
		ai_run(self, 18);
		break;

	case 1:
	case 4:
	case 5:
		ai_run(self, 14);
		break;

	case 2:
	case 6:
		ai_run(self, 7);
		break;

	case 3:
		ai_run(self, 12);
		break;

	case 7:
		ai_run(self, 11);
		break;
	}
}

void enforcer_attack_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	default:
		ai_face(self);
		break;

	case 5:
	case 9:
		enforcer_fire(self);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = enf_run1;
		SUB_CheckRefire(self, enf_atk1);
	}
}

void enforcer_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = enf_run1;
	}
}

void enforcer_paind_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 3:
		ai_painforward(self, 2);
		break;

	case 4:
	case 10:
	case 11:
	case 12:
		ai_painforward(self, 1);
		break;

	case 15:
	case 16:
		ai_pain(self, 1);
		break;

	default: break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = enf_run1;
	}
}

void enforcer_die_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 2:
		self->v.solid = SOLID_NOT;
		self->v.ammo_cells = 5;
		DropBackpack(self);
		break;

	case 3:
		ai_forward(self, 14);
		break;

	case 4:
		ai_forward(self, 2);
		break;

	case 8:
		ai_forward(self, 3);
		break;

	case 9:
	case 10:
	case 11:
		ai_forward(self, 5);
		break;

	default: break;
	}
}

void enforcer_fdie_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 2)
	{
		self->v.solid = SOLID_NOT;
		self->v.ammo_cells = 5;
		DropBackpack(self);
	}
}

const Animations EnforcerAnimations = MakeAnimations(
	{
		{"stand", 7, &enforcer_stand_frame, AnimationFlag::Looping},
		{"walk", 16, &enforcer_walk_frame, AnimationFlag::Looping},
		{"run", 8, &enforcer_run_frame, AnimationFlag::Looping},
		{"attack", 10, &enforcer_attack_frame},
		{"death", 14, &enforcer_die_frame},
		{"fdeath", 11, &enforcer_fdie_frame},
		{"paina", 4, &enforcer_pain_frame},
		{"painb", 5, &enforcer_pain_frame},
		{"painc", 8, &enforcer_pain_frame},
		{"paind", 19, &enforcer_paind_frame}
	});

const Animations* enf_animations_get(edict_t* self)
{
	return &EnforcerAnimations;
}

LINK_FUNCTION_TO_NAME(enf_animations_get);

void enf_stand1(edict_t* self)
{
	animation_set(self, "stand");
}

LINK_FUNCTION_TO_NAME(enf_stand1);

void enf_walk1(edict_t* self)
{
	animation_set(self, "walk");
}

LINK_FUNCTION_TO_NAME(enf_walk1);

void enf_run1(edict_t* self)
{
	animation_set(self, "run");
}

LINK_FUNCTION_TO_NAME(enf_run1);

void enf_atk1(edict_t* self)
{
	animation_set(self, "attack");
}

LINK_FUNCTION_TO_NAME(enf_atk1);

void enf_paina1(edict_t* self)
{
	animation_set(self, "paina");
}

void enf_painb1(edict_t* self)
{
	animation_set(self, "painb");
}

void enf_painc1(edict_t* self)
{
	animation_set(self, "painb");
}

void enf_paind1(edict_t* self)
{
	animation_set(self, "painb");
}

void enf_die1(edict_t* self)
{
	animation_set(self, "death");
}

void enf_fdie1(edict_t* self)
{
	animation_set(self, "fdeath");
}

void Laser_Touch(edict_t* self, edict_t* other)
{
	if (other == self->v.owner)
		return;		// don't explode on owner

	if (PF_pointcontents(self->v.origin) == CONTENT_SKY)
	{
		PF_Remove(self);
		return;
	}

	PF_sound(self, CHAN_WEAPON, "enforcer/enfstop.wav", 1, ATTN_STATIC);
	vec3_t org;
	PF_normalize(self->v.velocity, org);
	VectorMA(self->v.origin, -8, org, org);

	if (other->v.health)
	{
		SpawnBlood(org, AsVector(self->v.velocity) * 0.2f, 15);
		T_Damage(self, other, self, self->v.owner, 15);
	}
	else
	{
		PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
		PF_WriteByte(MSG_BROADCAST, TE_GUNSHOT);
		PF_WriteCoord(MSG_BROADCAST, org[0]);
		PF_WriteCoord(MSG_BROADCAST, org[1]);
		PF_WriteCoord(MSG_BROADCAST, org[2]);
	}

	PF_Remove(self);
}

LINK_FUNCTION_TO_NAME(Laser_Touch);

edict_t* LaunchLaser(edict_t* self, vec3_t org, vec3_t vec)
{
	if (!strcmp(self->v.classname, "monster_enforcer"))
		PF_sound(self, CHAN_WEAPON, "enforcer/enfire.wav", 1, ATTN_NORM);

	PF_normalize(vec, vec);

	auto newmis = PF_Spawn();
	newmis->v.owner = self;
	newmis->v.movetype = MOVETYPE_FLY;
	newmis->v.solid = SOLID_BBOX;
	newmis->v.effects = EF_DIMLIGHT;

	PF_setmodel(newmis, "progs/laser.mdl");
	PF_setsize(newmis, Vector3D{0, 0, 0}, Vector3D{0, 0, 0});

	PF_setorigin(newmis, org);

	AsVector(newmis->v.velocity) = AsVector(vec) * 600;
	PF_vectoangles(newmis->v.velocity, newmis->v.angles);

	newmis->v.nextthink = pr_global_struct->time + 5;
	newmis->v.think = SUB_Remove;
	newmis->v.touch = Laser_Touch;

	return newmis;
}

void enforcer_fire(edict_t* self)
{
	self->v.effects |= EF_MUZZLEFLASH;
	PF_makevectors(self->v.angles);

	auto org = AsVector(self->v.origin) + AsVector(pr_global_struct->v_forward) * 30 + AsVector(pr_global_struct->v_right) * 8.5 + Vector3D{0, 0, 16};

	LaunchLaser(self, org, AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin));
}

//============================================================================

void enf_pain(edict_t* self, edict_t* attacker, float damage)
{
	const float r = random();
	if (self->v.pain_finished > pr_global_struct->time)
		return;


	if (r < 0.5)
		PF_sound(self, CHAN_VOICE, "enforcer/pain1.wav", 1, ATTN_NORM);
	else
		PF_sound(self, CHAN_VOICE, "enforcer/pain2.wav", 1, ATTN_NORM);

	if (r < 0.2)
	{
		self->v.pain_finished = pr_global_struct->time + 1;
		enf_paina1(self);
	}
	else if (r < 0.4)
	{
		self->v.pain_finished = pr_global_struct->time + 1;
		enf_painb1(self);
	}
	else if (r < 0.7)
	{
		self->v.pain_finished = pr_global_struct->time + 1;
		enf_painc1(self);
	}
	else
	{
		self->v.pain_finished = pr_global_struct->time + 2;
		enf_paind1(self);
	}
}

LINK_FUNCTION_TO_NAME(enf_pain);

//============================================================================

void enf_die(edict_t* self)
{
	// check for gib
	if (self->v.health < -35)
	{
		PF_sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead(self, "progs/h_mega.mdl", self->v.health);
		ThrowGib(self, "progs/gib1.mdl", self->v.health);
		ThrowGib(self, "progs/gib2.mdl", self->v.health);
		ThrowGib(self, "progs/gib3.mdl", self->v.health);
		return;
	}

	// regular death
	PF_sound(self, CHAN_VOICE, "enforcer/death1.wav", 1, ATTN_NORM);
	if (random() > 0.5)
		enf_die1(self);
	else
		enf_fdie1(self);
}

LINK_FUNCTION_TO_NAME(enf_die);

/*QUAKED monster_enforcer (1 0 0) (-16 -16 -24) (16 16 40) Ambush

*/
void monster_enforcer(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model("progs/enforcer.mdl");
	PF_precache_model("progs/h_mega.mdl");
	PF_precache_model("progs/laser.mdl");

	PF_precache_sound("enforcer/death1.wav");
	PF_precache_sound("enforcer/enfire.wav");
	PF_precache_sound("enforcer/enfstop.wav");
	PF_precache_sound("enforcer/idle1.wav");
	PF_precache_sound("enforcer/pain1.wav");
	PF_precache_sound("enforcer/pain2.wav");
	PF_precache_sound("enforcer/sight1.wav");
	PF_precache_sound("enforcer/sight2.wav");
	PF_precache_sound("enforcer/sight3.wav");
	PF_precache_sound("enforcer/sight4.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel(self, "progs/enforcer.mdl");

	PF_setsize(self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 40});
	self->v.health = 80;

	self->v.th_stand = enf_stand1;
	self->v.th_walk = enf_walk1;
	self->v.th_run = enf_run1;
	self->v.th_pain = enf_pain;
	self->v.th_die = enf_die;
	self->v.th_missile = enf_atk1;
	self->v.animations_get = &enf_animations_get;

	walkmonster_start(self);
}

LINK_ENTITY_TO_CLASS(monster_enforcer);
