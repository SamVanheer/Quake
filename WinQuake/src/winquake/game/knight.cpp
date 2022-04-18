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
#include "monsters.h"
#include "player.h"
/*
==============================================================================

KNIGHT

==============================================================================
*/

void knight_run1(edict_t* self);
void knight_walk1(edict_t* self);

void knight_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void knight_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "knight/idle.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	default:
		ai_walk(self, 3);
		break;

	case 1:
	case 10:
		ai_walk(self, 2);
		break;

	case 3:
	case 7:
	case 12:
		ai_walk(self, 4);
		break;
	}
}

void knight_run_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "knight/idle.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	default:
		ai_run(self, 16);
		break;

	case 1:
	case 5:
		ai_run(self, 20);
		break;

	case 3:
		ai_run(self, 7);
		break;

	case 2:
		ai_run(self, 13);
		break;

	case 6:
		ai_run(self, 14);
		break;

	case 7:
		ai_run(self, 6);
		break;
	}
}

void knight_runattack_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (random() > 0.5)
			PF_sound(self, CHAN_WEAPON, "knight/sword2.wav", 1, ATTN_NORM);
		else
			PF_sound(self, CHAN_WEAPON, "knight/sword1.wav", 1, ATTN_NORM);
	}

	switch (frame)
	{
	case 0:
		ai_charge(self, 20);
		break;

	case 1:
	case 2:
	case 3:
	case 9:
		ai_charge_side(self);
		break;

	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		ai_melee_side(self);
		break;

	case 10:
		ai_charge(self, 10);
		break;

	default: break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &knight_run1;
	}
}

void knight_attack_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		PF_sound(self, CHAN_WEAPON, "knight/sword1.wav", 1, ATTN_NORM);
	}

	switch (frame)
	{
	case 0:
	case 3:
		ai_charge(self, 0);
		break;

	case 1:
		ai_charge(self, 7);
		break;

	case 2:
		ai_charge(self, 4);
		break;

	case 4:
		ai_charge(self, 3);
		break;

	case 5:
		ai_charge(self, 4);
		ai_melee(self);
		break;

	case 6:
		ai_charge(self, 1);
		ai_melee(self);
		break;

	case 7:
		ai_charge(self, 3);
		ai_melee(self);
		break;

	case 8:
		ai_charge(self, 1);
		break;

	case 9:
		ai_charge(self, 5);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &knight_run1;
	}
}

void knight_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = &knight_run1;
	}
}

void knight_painb_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
	case 9:
		ai_painforward(self, 0);
		break;

	case 1:
		ai_painforward(self, 3);
		break;

	case 4:
	case 6:
		ai_painforward(self, 2);
		break;

	case 5:
		ai_painforward(self, 4);
		break;

	case 7:
	case 8:
		ai_painforward(self, 5);
		break;
	default: break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &knight_run1;
	}
}

void knight_bow_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_turn(self);

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &knight_walk1;
	}
}

void knight_die_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 2)
	{
		self->v.solid = SOLID_NOT;
	}
}

const Animations KnightAnimations = MakeAnimations(
	{
		{"stand", 9, &knight_stand_frame, AnimationFlag::Looping},
		{"runb", 8, &knight_run_frame, AnimationFlag::Looping},
		{"runattack", 11, &knight_runattack_frame},
		{"pain", 3, &knight_pain_frame},
		{"painb", 11, &knight_painb_frame},
		{"attackb", 10, &knight_attack_frame},
		{"walk", 14, &knight_walk_frame, AnimationFlag::Looping},
		{"kneel", 5, &knight_bow_frame},
		{"standing", 5}, //Never used, but still defined.
		{"death", 10, &knight_die_frame},
		{"deathb", 11, &knight_die_frame}
	});

void knight_stand1(edict_t* self)
{
	animation_set(self, "stand");
}

void knight_walk1(edict_t* self)
{
	animation_set(self, "walk");
}

void knight_run1(edict_t* self)
{
	animation_set(self, "runb");
}

void knight_runatk1(edict_t* self)
{
	animation_set(self, "runattack");
}

void knight_atk1(edict_t* self)
{
	animation_set(self, "attackb");
}

//===========================================================================
void knight_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

void knight_painb1(edict_t* self)
{
	animation_set(self, "painb");
}

void knight_pain(edict_t* self, edict_t* attacker, float damage)
{
	if (self->v.pain_finished > pr_global_struct->time)
		return;

	const float r = random();
	
	PF_sound (self, CHAN_VOICE, "knight/khurt.wav", 1, ATTN_NORM);
	if (r < 0.85)
	{
		knight_pain1 (self);
		self->v.pain_finished = pr_global_struct->time + 1;
	}
	else
	{
		knight_painb1 (self);
		self->v.pain_finished = pr_global_struct->time + 1;
	}
}

//===========================================================================
void knight_bow1(edict_t* self)
{
	animation_set(self, "kneel");
}

void knight_die1(edict_t* self)
{
	animation_set(self, "death");
}

void knight_dieb1(edict_t* self)
{
	animation_set(self, "deathb");
}

void knight_die(edict_t* self)
{
// check for gib
	if (self->v.health < -40)
	{
		PF_sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead (self, "progs/h_knight.mdl", self->v.health);
		ThrowGib (self, "progs/gib1.mdl", self->v.health);
		ThrowGib (self, "progs/gib2.mdl", self->v.health);
		ThrowGib (self, "progs/gib3.mdl", self->v.health);
		return;
	}

// regular death
	PF_sound(self, CHAN_VOICE, "knight/kdeath.wav", 1, ATTN_NORM);
	if (random() < 0.5)
		knight_die1 (self);
	else
		knight_dieb1 (self);
}

/*QUAKED monster_knight (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void monster_knight(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model("progs/knight.mdl");
	PF_precache_model("progs/h_knight.mdl");

	PF_precache_sound("knight/kdeath.wav");
	PF_precache_sound("knight/khurt.wav");
	PF_precache_sound("knight/ksight.wav");
	PF_precache_sound("knight/sword1.wav");
	PF_precache_sound("knight/sword2.wav");
	PF_precache_sound("knight/idle.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel (self, "progs/knight.mdl");

	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 40});
	self->v.health = 75;

	self->v.th_stand = knight_stand1;
	self->v.th_walk = knight_walk1;
	self->v.th_run = knight_run1;
	self->v.th_melee = knight_atk1;
	self->v.th_pain = knight_pain;
	self->v.th_die = knight_die;
	self->v.animations_get = [](edict_t*) { return &KnightAnimations; };
	
	walkmonster_start (self);
}

LINK_ENTITY_TO_CLASS(monster_knight);
