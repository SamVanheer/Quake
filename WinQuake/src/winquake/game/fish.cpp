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

void fish_melee(edict_t* self);
void f_run1(edict_t* self);

enum class FishSwimMode
{
	Stand = 0,
	Walk,
	Run
};

void fish_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void fish_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_walk(self, 8);
}

void fish_run_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_run(self, 12);

	if (frame == 0)
	{
		if (random() < 0.5)
			PF_sound(self, CHAN_VOICE, "fish/idle.wav", 1, ATTN_NORM);
	}
}

void fish_swim_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (static_cast<FishSwimMode>(self->v.animation_mode))
	{
	default:
	case FishSwimMode::Stand:
		fish_stand_frame(self, animation, frame);
		break;

	case FishSwimMode::Walk:
		fish_walk_frame(self, animation, frame);
		break;

	case FishSwimMode::Run:
		fish_run_frame(self, animation, frame);
		break;
	}
}

void fish_attack_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	default:
		ai_charge(self, 10);
		break;

	case 2:
	case 8:
	case 14:
		fish_melee(self);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = f_run1;
	}
}

void fish_death_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		PF_sound(self, CHAN_VOICE, "fish/death.wav", 1, ATTN_NORM);
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.solid = SOLID_NOT;
	}
}

void fish_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame != 0)
	{
		ai_pain(self, 6);
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = f_run1;
	}
}

const Animations FishAnimations = MakeAnimations(
	{
		{"attack", 18, &fish_attack_frame},
		{"death", 21, &fish_death_frame},
		{"swim", 18, &fish_swim_frame, AnimationFlag::Looping},
		{"pain", 9, &fish_pain_frame}
	});

const Animations* fish_animations_get(edict_t* self)
{
	return &FishAnimations;
}

LINK_FUNCTION_TO_NAME(fish_animations_get);

void f_stand1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(FishSwimMode::Stand);
	animation_set(self, "swim");
}

LINK_FUNCTION_TO_NAME(f_stand1);

void f_walk1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(FishSwimMode::Walk);
	animation_set(self, "swim");
}

LINK_FUNCTION_TO_NAME(f_walk1);

void f_run1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(FishSwimMode::Run);
	animation_set(self, "swim");
}

LINK_FUNCTION_TO_NAME(f_run1);

void fish_melee(edict_t* self)
{
	if (!self->v.enemy)
		return;		// removed before stroke
		
	auto delta = AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin);

	if (PF_vlen(delta) > 60)
		return;
		
	PF_sound (self, CHAN_VOICE, "fish/bite.wav", 1, ATTN_NORM);
	const float ldmg = (random() + random()) * 3;
	T_Damage (self, self->v.enemy, self, self, ldmg);
}

void f_attack1(edict_t* self)
{
	animation_set(self, "attack");
}

LINK_FUNCTION_TO_NAME(f_attack1);

void f_death1(edict_t* self)
{
	animation_set(self, "death");
}

LINK_FUNCTION_TO_NAME(f_death1);

void f_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

void fish_pain(edict_t* self, edict_t* attacker, float damage)
{

// fish allways do pain frames
	f_pain1 (self);
}

LINK_FUNCTION_TO_NAME(fish_pain);

/*QUAKED monster_fish (1 0 0) (-16 -16 -24) (16 16 24) Ambush
*/
void monster_fish(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model ("progs/fish.mdl");

	PF_precache_sound ("fish/death.wav");
	PF_precache_sound ("fish/bite.wav");
	PF_precache_sound ("fish/idle.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel (self, "progs/fish.mdl");

	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 24});
	self->v.health = 25;
	
	self->v.th_stand = f_stand1;
	self->v.th_walk = f_walk1;
	self->v.th_run = f_run1;
	self->v.th_die = f_death1;
	self->v.th_pain = fish_pain;
	self->v.th_melee = f_attack1;
	self->v.animations_get = &fish_animations_get;
	
	swimmonster_start (self);
}

LINK_ENTITY_TO_CLASS(monster_fish);
