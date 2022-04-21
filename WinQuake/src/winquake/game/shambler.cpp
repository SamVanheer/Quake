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

SHAMBLER

==============================================================================
*/
void sham_run1(edict_t* self);
void sham_swingl1(edict_t* self);
void sham_swingr1(edict_t* self);
void ShamClaw(edict_t* self, float side);
void CastLightning(edict_t* self);

void shambler_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void shambler_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		ai_walk(self, 10);
		break;

	case 3:
		ai_walk(self, 5);
		break;

	case 4:
		ai_walk(self, 6);
		break;

	case 5:
		ai_walk(self, 12);
		break;

	case 6:
		ai_walk(self, 8);
		break;

	case 7:
		ai_walk(self, 3);
		break;

	case 8:
		ai_walk(self, 13);
		break;

	case 10:
	case 11:
		ai_walk(self, 7);
		break;

	default:
		ai_walk(self, 9);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		if (PF_random() > 0.8)
			PF_sound(self, CHAN_VOICE, "shambler/sidle.wav", 1, ATTN_IDLE);
	}
}

void shambler_run_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	default:
		ai_run(self, 20);
		break;

	case 1:
	case 4:
		ai_run(self, 24);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		if (PF_random() > 0.8)
			PF_sound(self, CHAN_VOICE, "shambler/sidle.wav", 1, ATTN_IDLE);
	}
}

void shambler_smash_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		PF_sound(self, CHAN_VOICE, "shambler/melee1.wav", 1, ATTN_NORM);
	}

	switch (frame)
	{
	case 0:
		ai_charge(self, 2);
		break;

	case 1:
	case 2:
		ai_charge(self, 6);
		break;

	case 3:
	case 10:
		ai_charge(self, 5);
		break;

	case 4:
	case 11:
		ai_charge(self, 4);
		break;

	case 5:
		ai_charge(self, 1);
		break;

	default:
		ai_charge(self, 0);
		break;

	case 9:
	{
		if (!self->v.enemy)
			return;
		ai_charge(self, 0);

		auto delta = AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin);

		if (PF_vlen(delta) > 100)
			return;
		if (!CanDamage(self, self->v.enemy, self))
			return;

		const float ldmg = (PF_random() + PF_random() + PF_random()) * 40;
		T_Damage(self, self->v.enemy, self, self, ldmg);
		PF_sound(self, CHAN_VOICE, "shambler/smack.wav", 1, ATTN_NORM);

		SpawnMeatSpray(self, AsVector(self->v.origin) + AsVector(pr_global_struct->v_forward) * 16, crandom() * 100 * AsVector(pr_global_struct->v_right));
		SpawnMeatSpray(self, AsVector(self->v.origin) + AsVector(pr_global_struct->v_forward) * 16, crandom() * 100 * AsVector(pr_global_struct->v_right));
		break;
	}
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &sham_run1;
	}
}

void shambler_swingl_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		PF_sound(self, CHAN_VOICE, "shambler/melee2.wav", 1, ATTN_NORM);
	}

	switch (frame)
	{
	case 0:
	case 6:
		ai_charge(self, 5);
		break;

	case 1:
	case 3:
		ai_charge(self, 3);
		break;

	case 2:
	case 4:
		ai_charge(self, 7);
		break;

	case 5:
		ai_charge(self, 9);
		break;

	case 7:
		ai_charge(self, 4);
		break;

	case 8:
		ai_charge(self, 8);
		break;
	}

	if (frame == 6)
	{
		ShamClaw(self, 250);
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &sham_run1;

		if (PF_random() < 0.5)
			self->v.think = sham_swingr1;
	}
}

void shambler_swingr_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		PF_sound(self, CHAN_VOICE, "shambler/melee2.wav", 1, ATTN_NORM);
	}

	switch (frame)
	{
	case 0:
		ai_charge(self, 1);
		break;

	case 1:
		ai_charge(self, 8);
		break;

	case 2:
		ai_charge(self, 14);
		break;

	case 3:
		ai_charge(self, 7);
		break;

	case 4:
	case 7:
		ai_charge(self, 3);
		break;

	case 5:
	case 6:
		ai_charge(self, 6);
		break;

	case 8:
		ai_charge(self, 1);
		ai_charge(self, 10);
		break;
	}

	if (frame == 6)
	{
		ShamClaw(self, -250);
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &sham_run1;

		if (PF_random() < 0.5)
			self->v.think = sham_swingl1;
	}
}

void shambler_magic_frame(edict_t* self, const Animation* animation, int frame)
{
	//Skip frames 6-7
	if (frame == 6)
	{
		frame = 8;
		self->v.frame = animation->StartIndex + frame;
	}

	switch (frame)
	{
	case 0:
		ai_face(self);
		PF_sound(self, CHAN_WEAPON, "shambler/sattck1.wav", 1, ATTN_NORM);
		break;

	case 1:
		ai_face(self);
		break;

	case 2:
	{
		ai_face(self);
		self->v.nextthink = self->v.nextthink + 0.2;

		self->v.effects |= EF_MUZZLEFLASH;
		ai_face(self);
		auto o = self->v.owner = PF_Spawn();
		PF_setmodel(o, "progs/s_light.mdl");
		PF_setorigin(o, self->v.origin);
		AsVector(o->v.angles) = AsVector(self->v.angles);
		o->v.nextthink = pr_global_struct->time + 0.7;
		o->v.think = SUB_Remove;
		break;
	}

	case 3:
		self->v.effects |= EF_MUZZLEFLASH;
		self->v.owner->v.frame = 1;
		break;

	case 4:
		self->v.effects |= EF_MUZZLEFLASH;
		self->v.owner->v.frame = 2;
		break;

	case 5:
		PF_Remove(self->v.owner);
		CastLightning(self);
		PF_sound(self, CHAN_WEAPON, "shambler/sboom.wav", 1, ATTN_NORM);
		break;

	case 8:
	case 9:
		CastLightning(self);
		break;

	case 10:
		if (pr_global_struct->skill == 3)
			CastLightning(self);
		break;

	default: break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &sham_run1;
	}
}

void shambler_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = &sham_run1;
	}
}

void shambler_death_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 2)
	{
		self->v.solid = SOLID_NOT;
	}
}

const Animations ShamblerAnimations = MakeAnimations(
	{
		{"stand", 17, &shambler_stand_frame, AnimationFlag::Looping},
		{"walk", 12, &shambler_walk_frame, AnimationFlag::Looping},
		{"run", 6, &shambler_run_frame, AnimationFlag::Looping},
		{"smash", 12, &shambler_smash_frame},
		{"swingr", 9, &shambler_swingr_frame},
		{"swingl", 9, &shambler_swingl_frame},
		{"magic", 12, &shambler_magic_frame},
		{"pain", 6, &shambler_pain_frame},
		{"death", 11, &shambler_death_frame}
	});

const Animations* shambler_animations_get(edict_t* self)
{
	return &ShamblerAnimations;
}

LINK_FUNCTION_TO_NAME(shambler_animations_get);

void sham_stand1(edict_t* self)
{
	animation_set(self, "stand");
}

LINK_FUNCTION_TO_NAME(sham_stand1);

void sham_walk1(edict_t* self)
{
	animation_set(self, "walk");
}

LINK_FUNCTION_TO_NAME(sham_walk1);

void sham_run1(edict_t* self)
{
	animation_set(self, "run");
}

LINK_FUNCTION_TO_NAME(sham_run1);

void sham_smash1(edict_t* self)
{
	animation_set(self, "smash");
}

void ShamClaw(edict_t* self, float side)
{
	if (!self->v.enemy)
		return;
	ai_charge(self, 10);

	auto delta = AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin);

	if (PF_vlen(delta) > 100)
		return;
		
	const float ldmg = (PF_random() + PF_random() + PF_random()) * 20;
	T_Damage (self, self->v.enemy, self, self, ldmg);
	PF_sound (self, CHAN_VOICE, "shambler/smack.wav", 1, ATTN_NORM);

	if (side)
	{
		PF_makevectors (self->v.angles);
		SpawnMeatSpray (self, AsVector(self->v.origin) + AsVector(pr_global_struct->v_forward)*16, side * AsVector(pr_global_struct->v_right));
	}
}

void sham_swingl1(edict_t* self)
{
	animation_set(self, "swingl");
}

LINK_FUNCTION_TO_NAME(sham_swingl1);

void sham_swingr1(edict_t* self)
{
	animation_set(self, "swingr");
}

LINK_FUNCTION_TO_NAME(sham_swingr1);

void sham_melee(edict_t* self)
{
	const float chance = PF_random();
	if (chance > 0.6 || self->v.health == 600)
		sham_smash1 (self);
	else if (chance > 0.3)
		sham_swingr1 (self);
	else
		sham_swingl1 (self);
}

LINK_FUNCTION_TO_NAME(sham_melee);

//============================================================================

void CastLightning(edict_t* self)
{
	self->v.effects |= EF_MUZZLEFLASH;

	ai_face (self);

	auto org = AsVector(self->v.origin) + Vector3D{0, 0, 40};

	auto dir = AsVector(self->v.enemy->v.origin) + Vector3D{0, 0, 16} - org;
	PF_normalize (dir, dir);

	PF_traceline (org, AsVector(self->v.origin) + AsVector(dir)*600, MOVE_NOMONSTERS, self);

	PF_WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte (MSG_BROADCAST, TE_LIGHTNING1);
	PF_WriteEntity (MSG_BROADCAST, self);
	PF_WriteCoord (MSG_BROADCAST, org[0]);
	PF_WriteCoord (MSG_BROADCAST, org[1]);
	PF_WriteCoord (MSG_BROADCAST, org[2]);
	PF_WriteCoord (MSG_BROADCAST, pr_global_struct->trace_endpos[0]);
	PF_WriteCoord (MSG_BROADCAST, pr_global_struct->trace_endpos[1]);
	PF_WriteCoord (MSG_BROADCAST, pr_global_struct->trace_endpos[2]);

	LightningDamage (self, org, pr_global_struct->trace_endpos, self, 10);
}

void sham_magic1(edict_t* self)
{
	animation_set(self, "magic");
}

LINK_FUNCTION_TO_NAME(sham_magic1);

void sham_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

void sham_pain(edict_t* self, edict_t* attacker, float damage)
{
	PF_sound (self, CHAN_VOICE, "shambler/shurt2.wav", 1, ATTN_NORM);

	if (self->v.health <= 0)
		return;		// allready dying, don't go into pain frame

	if (PF_random()*400 > damage)
		return;		// didn't flinch

	if (self->v.pain_finished > pr_global_struct->time)
		return;
	self->v.pain_finished = pr_global_struct->time + 2;
		
	sham_pain1 (self);
}

LINK_FUNCTION_TO_NAME(sham_pain);

//============================================================================

void sham_death1(edict_t* self)
{
	animation_set(self, "death");
}

void sham_die(edict_t* self)
{
// check for gib
	if (self->v.health < -60)
	{
		PF_sound (self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead (self, "progs/h_shams.mdl", self->v.health);
		ThrowGib (self, "progs/gib1.mdl", self->v.health);
		ThrowGib (self, "progs/gib2.mdl", self->v.health);
		ThrowGib (self, "progs/gib3.mdl", self->v.health);
		return;
	}

// regular death
	PF_sound (self, CHAN_VOICE, "shambler/sdeath.wav", 1, ATTN_NORM);
	sham_death1 (self);
}

LINK_FUNCTION_TO_NAME(sham_die);

//============================================================================


/*QUAKED monster_shambler (1 0 0) (-32 -32 -24) (32 32 64) Ambush
*/
void monster_shambler(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model ("progs/shambler.mdl");
	PF_precache_model ("progs/s_light.mdl");
	PF_precache_model ("progs/h_shams.mdl");
	PF_precache_model ("progs/bolt.mdl");
	
	PF_precache_sound ("shambler/sattck1.wav");
	PF_precache_sound ("shambler/sboom.wav");
	PF_precache_sound ("shambler/sdeath.wav");
	PF_precache_sound ("shambler/shurt2.wav");
	PF_precache_sound ("shambler/sidle.wav");
	PF_precache_sound ("shambler/ssight.wav");
	PF_precache_sound ("shambler/melee1.wav");
	PF_precache_sound ("shambler/melee2.wav");
	PF_precache_sound ("shambler/smack.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;
	PF_setmodel (self, "progs/shambler.mdl");

	PF_setsize (self, VEC_HULL2_MIN, VEC_HULL2_MAX);
	self->v.health = 600;

	self->v.th_stand = sham_stand1;
	self->v.th_walk = sham_walk1;
	self->v.th_run = sham_run1;
	self->v.th_die = sham_die;
	self->v.th_melee = sham_melee;
	self->v.th_missile = sham_magic1;
	self->v.th_pain = sham_pain;
	self->v.animations_get = &shambler_animations_get;
	
	walkmonster_start(self);
}

LINK_ENTITY_TO_CLASS(monster_shambler);
