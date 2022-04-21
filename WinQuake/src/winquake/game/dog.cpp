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

/*
==============================================================================

DOG

==============================================================================
*/
void dog_leap1(edict_t* self);
void dog_run1(edict_t* self);
void dog_bite(edict_t* self);
void Dog_JumpTouch(edict_t* self, edict_t* other);

void dog_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void dog_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "dog/idle.wav", 1, ATTN_IDLE);
	}

	ai_walk(self, 8);
}

void dog_run_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "dog/idle.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	case 0:
	case 6:
		ai_run(self, 16);
		break;

	default:
		ai_run(self, 32);
		break;

	case 3:
	case 9:
		ai_run(self, 20);
		break;

	case 4:
	case 10:
		ai_run(self, 64);
		break;
	}
}

void dog_attack_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 3)
	{
		PF_sound(self, CHAN_VOICE, "dog/dattack1.wav", 1, ATTN_NORM);
		dog_bite(self);
	}
	else
	{
		ai_charge(self, 10);
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = dog_run1;
	}
}

void dog_leap_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		ai_face(self);
		break;

	case 1:
		ai_face(self);
		self->v.touch = Dog_JumpTouch;
		PF_makevectors(self->v.angles);
		self->v.origin[2] = self->v.origin[2] + 1;
		AsVector(self->v.velocity) = AsVector(pr_global_struct->v_forward) * 300 + Vector3D{0, 0, 200};
		if (self->v.flags & FL_ONGROUND)
			self->v.flags &= ~FL_ONGROUND;
		break;

	default: break;
	}
}

void dog_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = dog_run1;
	}
}

void dog_painb_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 2:
	case 7:
		ai_pain(self, 4);
		break;

	case 3:
	case 4:
		ai_pain(self, 12);
		break;

	case 5:
		ai_pain(self, 2);
		break;

	case 9:
		ai_pain(self, 10);
		break;

	default: break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = dog_run1;
	}
}

void dog_die_frame(edict_t* self, const Animation* animation, int frame)
{
}

const Animations DogAnimations = MakeAnimations(
	{
		{"attack", 8, &dog_attack_frame},
		{"death", 9, &dog_die_frame},
		{"deathb", 9, &dog_die_frame},
		{"pain", 6, &dog_pain_frame},
		{"painb", 16, &dog_painb_frame},
		{"run", 12, &dog_run_frame, AnimationFlag::Looping},
		{"leap", 9, &dog_leap_frame},
		{"stand", 9, &dog_stand_frame, AnimationFlag::Looping},
		{"walk", 8, &dog_walk_frame, AnimationFlag::Looping}
	});

const Animations* dog_animations_get(edict_t* self)
{
	return &DogAnimations;
}

LINK_FUNCTION_TO_NAME(dog_animations_get);

/*
================
dog_bite

================
*/
void dog_bite(edict_t* self)
{
	if (!self->v.enemy)
		return;

	ai_charge(self, 10);

	if (!CanDamage (self, self->v.enemy, self))
		return;

	auto delta = AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin);

	if (PF_vlen(delta) > 100)
		return;
		
	const float ldmg = (random() + random() + random()) * 8;
	T_Damage (self, self->v.enemy, self, self, ldmg);
}

void Dog_JumpTouch(edict_t* self, edict_t* other)
{
	if (self->v.health <= 0)
		return;
		
	if (other->v.takedamage)
	{
		if ( PF_vlen(self->v.velocity) > 300 )
		{
			const float ldmg = 10 + 10*random();
			T_Damage (self, other, self, self, ldmg);
		}
	}

	if (!PF_checkbottom(self))
	{
		if (self->v.flags & FL_ONGROUND)
		{	// jump randomly to not get hung up
//dprint ("popjump\n");
	self->v.touch = SUB_NullTouch;
	self->v.think = dog_leap1;
	self->v.nextthink = pr_global_struct->time + 0.1;

//			self->v.velocity[0] = (random() - 0.5) * 600;
//			self->v.velocity[1] = (random() - 0.5) * 600;
//			self->v.velocity[2] = 200;
//			self->v.flags &= ~FL_ONGROUND;
		}
		return;	// not on ground yet
	}

	self->v.touch = SUB_NullTouch;
	self->v.think = dog_run1;
	self->v.nextthink = pr_global_struct->time + 0.1;
}

LINK_FUNCTION_TO_NAME(Dog_JumpTouch);

void dog_stand1(edict_t* self)
{
	animation_set(self, "stand");
}

LINK_FUNCTION_TO_NAME(dog_stand1);

void dog_walk1(edict_t* self)
{
	animation_set(self, "walk");
}

LINK_FUNCTION_TO_NAME(dog_walk1);

void dog_run1(edict_t* self)
{
	animation_set(self, "run");
}

LINK_FUNCTION_TO_NAME(dog_run1);

void dog_atta1(edict_t* self)
{
	animation_set(self, "attack");
}

LINK_FUNCTION_TO_NAME(dog_atta1);

void dog_leap1(edict_t* self)
{
	animation_set(self, "leap");
}

LINK_FUNCTION_TO_NAME(dog_leap1);

void dog_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

void dog_painb1(edict_t* self)
{
	animation_set(self, "painb");
}

void dog_die1(edict_t* self)
{
	animation_set(self, "death");
}

void dog_dieb1(edict_t* self)
{
	animation_set(self, "deathb");
}

void dog_pain(edict_t* self, edict_t* attacker, float damage)
{
	PF_sound (self, CHAN_VOICE, "dog/dpain1.wav", 1, ATTN_NORM);

	if (random() > 0.5)
		dog_pain1 (self);
	else
		dog_painb1 (self);
}

LINK_FUNCTION_TO_NAME(dog_pain);

void dog_die(edict_t* self)
{
// check for gib
	if (self->v.health < -35)
	{
		PF_sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowGib (self, "progs/gib3.mdl", self->v.health);
		ThrowGib (self, "progs/gib3.mdl", self->v.health);
		ThrowGib (self, "progs/gib3.mdl", self->v.health);
		ThrowHead (self, "progs/h_dog.mdl", self->v.health);
		return;
	}

// regular death
	PF_sound(self, CHAN_VOICE, "dog/ddeath.wav", 1, ATTN_NORM);
	self->v.solid = SOLID_NOT;

	if (random() > 0.5)
		dog_die1 (self);
	else
		dog_dieb1 (self);
}

LINK_FUNCTION_TO_NAME(dog_die);

//============================================================================

/*
==============
CheckDogMelee

Returns TRUE if a melee attack would hit right now
==============
*/
bool CheckDogMelee(edict_t* self)
{
	if (enemy_range == RANGE_MELEE)
	{	// FIXME: check canreach
		self->v.attack_state = AS_MELEE;
		return true;
	}
	return false;
}

/*
==============
CheckDogJump

==============
*/
bool CheckDogJump(edict_t* self)
{
	if (self->v.origin[2] + self->v.mins[2] > self->v.enemy->v.origin[2] + self->v.enemy->v.mins[2]
	+ 0.75 * self->v.enemy->v.size[2])
		return false;
		
	if (self->v.origin[2] + self->v.maxs[2] < self->v.enemy->v.origin[2] + self->v.enemy->v.mins[2]
	+ 0.25 * self->v.enemy->v.size[2])
		return false;
		
	auto dist = AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin);
	dist[2] = 0;
	
	const float d = PF_vlen(dist);
	
	if (d < 80)
		return false;
		
	if (d > 150)
		return false;
		
	return true;
}

bool DogCheckAttack(edict_t* self)
{
// if close enough for slashing, go for it
	if (CheckDogMelee (self))
	{
		self->v.attack_state = AS_MELEE;
		return true;
	}
	
	if (CheckDogJump (self))
	{
		self->v.attack_state = AS_MISSILE;
		return true;
	}
	
	return false;
}


//===========================================================================

/*QUAKED monster_dog (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void monster_dog(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model ("progs/h_dog.mdl");
	PF_precache_model("progs/dog.mdl");

	PF_precache_sound ("dog/dattack1.wav");
	PF_precache_sound ("dog/ddeath.wav");
	PF_precache_sound ("dog/dpain1.wav");
	PF_precache_sound ("dog/dsight.wav");
	PF_precache_sound ("dog/idle.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel (self, "progs/dog.mdl");

	PF_setsize (self, Vector3D{-32, -32, -24}, Vector3D{32, 32, 40});
	self->v.health = 25;

	self->v.th_stand = dog_stand1;
	self->v.th_walk = dog_walk1;
	self->v.th_run = dog_run1;
	self->v.th_pain = dog_pain;
	self->v.th_die = dog_die;
	self->v.th_melee = dog_atta1;
	self->v.th_missile = dog_leap1;
	self->v.animations_get = &dog_animations_get;

	walkmonster_start(self);
}

LINK_ENTITY_TO_CLASS(monster_dog);
