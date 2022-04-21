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
#include "weapons.h"
#include "subs.h"

/*
==============================================================================

DEMON

==============================================================================
*/
void demon1_jump1(edict_t* self);
void demon1_run1(edict_t* self);
void Demon_JumpTouch(edict_t* self, edict_t* other);
void Demon_Melee(edict_t* self, float side);

void demon1_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void demon1_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "demon/idle1.wav", 1, ATTN_IDLE);
	}

	switch (frame)
	{
	case 0:
		ai_walk(self, 8);
		break;

	case 1:
	case 2:
		ai_walk(self, 6);
		break;

	case 3:
		ai_walk(self, 7);
		break;

	case 4:
		ai_walk(self, 4);
		break;

	case 5:
		ai_walk(self, 6);
		break;

	case 7:
	case 8:
		ai_walk(self, 10);
		break;
	}
}

void demon1_run_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "demon/idle1.wav", 1, ATTN_IDLE);

		[[fallthrough]];

	case 3:
		ai_run(self, 20);
		break;

	case 1:
	case 4:
		ai_run(self, 15);
		break;

	case 2:
	case 5:
		ai_run(self, 36);
		break;
	}
}

void demon1_jump_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		//Reset frame if we're in a loop.
		self->v.frame = animation->StartIndex;

		[[fallthrough]];

	case 1:
	case 2:
		ai_face(self);
		break;

	case 3:
		ai_face(self);
		self->v.touch = Demon_JumpTouch;
		PF_makevectors(self->v.angles);
		self->v.origin[2] = self->v.origin[2] + 1;
		AsVector(self->v.velocity) = AsVector(pr_global_struct->v_forward) * 600 + Vector3D{0, 0, 250};
		if (self->v.flags & FL_ONGROUND)
			self->v.flags &= ~FL_ONGROUND;
		break;

	case 9:
		self->v.think = demon1_jump1;
		self->v.nextthink = pr_global_struct->time + 3;
		// if three seconds pass, assume demon is stuck and jump again
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = demon1_run1;
	}
}

void demon1_attack_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
	case 8:
	case 13:
	case 14:
		ai_charge(self, 4);
		break;

	case 1:
	case 2:
		ai_charge(self, 0);
		break;

	case 3:
	case 5:
		ai_charge(self, 1);
		break;

	case 4:
		ai_charge(self, 2);
		Demon_Melee(self, 200);
		break;

	case 6:
		ai_charge(self, 6);
		break;

	case 7:
	case 12:
		ai_charge(self, 8);
		break;

	case 9:
		ai_charge(self, 2);
		break;

	case 10:
		Demon_Melee(self, -200);
		break;

	case 11:
		ai_charge(self, 5);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = demon1_run1;
	}
}

void demon1_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = demon1_run1;
	}
}

void demon1_die_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		PF_sound(self, CHAN_VOICE, "demon/ddeath.wav", 1, ATTN_NORM);
		break;

	case 5:
		self->v.solid = SOLID_NOT;
		break;
	}
}

const Animations DemonAnimations = MakeAnimations(
	{
		{"stand", 13, &demon1_stand_frame, AnimationFlag::Looping},
		{"walk", 8, &demon1_walk_frame, AnimationFlag::Looping},
		{"run", 6, &demon1_run_frame, AnimationFlag::Looping},
		{"leap", 12, &demon1_jump_frame},
		{"pain", 6, &demon1_pain_frame},
		{"death", 9, &demon1_die_frame},
		{"attacka", 15, &demon1_attack_frame}
	});

const Animations* demon_animations_get(edict_t* self)
{
	return &DemonAnimations;
}

LINK_FUNCTION_TO_NAME(demon_animations_get);

//============================================================================
void demon1_stand1(edict_t* self)
{
	animation_set(self, "stand");
}

LINK_FUNCTION_TO_NAME(demon1_stand1);

void demon1_walk1(edict_t* self)
{
	animation_set(self, "walk");
}

LINK_FUNCTION_TO_NAME(demon1_walk1);

void demon1_run1(edict_t* self)
{
	animation_set(self, "run");
}

LINK_FUNCTION_TO_NAME(demon1_run1);

void demon1_jump1(edict_t* self)
{
	animation_set(self, "leap");
}

LINK_FUNCTION_TO_NAME(demon1_jump1);

void demon1_jump11(edict_t* self)
{
	animation_set(self, "leap");
	//TODO: pass frame index to set
	self->v.frame = DemonAnimations.FindAnimationStart("leap") + 10;
}

LINK_FUNCTION_TO_NAME(demon1_jump11);

void demon1_atta1(edict_t* self)
{
	animation_set(self, "attacka");
}

void demon1_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

void demon1_die1(edict_t* self)
{
	animation_set(self, "death");
}

void demon1_pain(edict_t* self, edict_t* attacker, float damage)
{
	if (self->v.touch == Demon_JumpTouch)
		return;

	if (self->v.pain_finished > pr_global_struct->time)
		return;

	self->v.pain_finished = pr_global_struct->time + 1;
	PF_sound (self, CHAN_VOICE, "demon/dpain1.wav", 1, ATTN_NORM);

	if (random()*200 > damage)
		return;		// didn't flinch
		
	demon1_pain1 (self);
}

LINK_FUNCTION_TO_NAME(demon1_pain);

void demon_die(edict_t* self)
{
// check for gib
	if (self->v.health < -80)
	{
		PF_sound (self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead (self, "progs/h_demon.mdl", self->v.health);
		ThrowGib (self, "progs/gib1.mdl", self->v.health);
		ThrowGib (self, "progs/gib1.mdl", self->v.health);
		ThrowGib (self, "progs/gib1.mdl", self->v.health);
		return;
	}

// regular death
	demon1_die1 (self);
}

LINK_FUNCTION_TO_NAME(demon_die);

void Demon_MeleeAttack(edict_t* self)
{
	demon1_atta1 (self);
}

LINK_FUNCTION_TO_NAME(Demon_MeleeAttack);

/*QUAKED monster_demon1 (1 0 0) (-32 -32 -24) (32 32 64) Ambush

*/
void monster_demon1(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model ("progs/demon.mdl");
	PF_precache_model("progs/h_demon.mdl");

	PF_precache_sound ("demon/ddeath.wav");
	PF_precache_sound ("demon/dhit2.wav");
	PF_precache_sound ("demon/djump.wav");
	PF_precache_sound ("demon/dpain1.wav");
	PF_precache_sound ("demon/idle1.wav");
	PF_precache_sound ("demon/sight2.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel (self, "progs/demon.mdl");

	PF_setsize (self, VEC_HULL2_MIN, VEC_HULL2_MAX);
	self->v.health = 300;

	self->v.th_stand = demon1_stand1;
	self->v.th_walk = demon1_walk1;
	self->v.th_run = demon1_run1;
	self->v.th_die = demon_die;
	self->v.th_melee = Demon_MeleeAttack;		// one of two attacks
	self->v.th_missile = demon1_jump1;			// jump attack
	self->v.th_pain = demon1_pain;
	self->v.animations_get = &demon_animations_get;
		
	walkmonster_start(self);
}

LINK_ENTITY_TO_CLASS(monster_demon1);

/*
==============================================================================

DEMON

==============================================================================
*/

/*
==============
CheckDemonMelee

Returns true if a melee attack would hit right now
==============
*/
bool CheckDemonMelee(edict_t* self)
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
CheckDemonJump

==============
*/
bool CheckDemonJump(edict_t* self)
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
	
	if (d < 100)
		return false;
		
	if (d > 200)
	{
		if (random() < 0.9)
			return false;
	}
		
	return true;
}

bool DemonCheckAttack(edict_t* self)
{
// if close enough for slashing, go for it
	if (CheckDemonMelee (self))
	{
		self->v.attack_state = AS_MELEE;
		return true;
	}
	
	if (CheckDemonJump (self))
	{
		self->v.attack_state = AS_MISSILE;
		PF_sound (self, CHAN_VOICE, "demon/djump.wav", 1, ATTN_NORM);
		return true;
	}
	
	return false;
}

//===========================================================================

void Demon_Melee(edict_t* self, float side)
{
	ai_face (self);
	PF_walkmove (self, self->v.ideal_yaw, 12);	// allow a little closing

	auto delta = AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin);

	if (PF_vlen(delta) > 100)
		return;
	if (!CanDamage (self, self->v.enemy, self))
		return;
		
	PF_sound (self, CHAN_WEAPON, "demon/dhit2.wav", 1, ATTN_NORM);
	const float ldmg = 10 + 5*random();
	T_Damage (self, self->v.enemy, self, self, ldmg);	

	PF_makevectors (self->v.angles);
	SpawnMeatSpray (self, AsVector(self->v.origin) + AsVector(pr_global_struct->v_forward)*16, side * AsVector(pr_global_struct->v_right));
}

void Demon_JumpTouch(edict_t* self, edict_t* other)
{
	if (self->v.health <= 0)
		return;
		
	if (other->v.takedamage)
	{
		if ( PF_vlen(self->v.velocity) > 400 )
		{
			const float ldmg = 40 + 10*random();
			T_Damage (self, other, self, self, ldmg);	
		}
	}

	if (!PF_checkbottom(self))
	{
		if (self->v.flags & FL_ONGROUND)
		{	// jump randomly to not get hung up
//dprint ("popjump\n");
	self->v.touch = SUB_NullTouch;
	self->v.think = demon1_jump1;
	self->v.nextthink = pr_global_struct->time + 0.1;

//			self->v.velocity[0] = (random() - 0.5) * 600;
//			self->v.velocity[1] = (random() - 0.5) * 600;
//			self->v.velocity[2] = 200;
//			self->v.flags &= ~FL_ONGROUND;
		}
		return;	// not on ground yet
	}

	self->v.touch = SUB_NullTouch;
	self->v.think = demon1_jump11;
	self->v.nextthink = pr_global_struct->time + 0.1;
}

LINK_FUNCTION_TO_NAME(Demon_JumpTouch);
