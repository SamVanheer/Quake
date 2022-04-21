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
#include "subs.h"
#include "weapons.h"

/*
==============================================================================

KNIGHT

==============================================================================
*/
void hknight_char_a1(edict_t* self);
void hknight_run1(edict_t* self);
void hk_idle_sound(edict_t* self);
void CheckForCharge(edict_t* self);
void CheckContinueCharge(edict_t* self);
void hknight_shot(edict_t* self, float offset);

void hknight_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void hknight_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		hk_idle_sound(self);
	}

	switch (frame)
	{
	case 0:
	case 5:
	case 6:
	case 13:
	case 14:
	case 19:
		ai_walk(self, 2);
		break;

	case 1:
	case 2:
		ai_walk(self, 5);
		break;

	case 3:
	case 4:
	case 9:
	case 11:
	case 15:
		ai_walk(self, 4);
		break;

	case 7:
	case 8:
	case 10:
	case 16:
	case 17:
	case 18:
		ai_walk(self, 3);
		break;

	case 12:
		ai_walk(self, 6);
		break;
	}
}

void hknight_run_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		hk_idle_sound(self);
	}

	switch (frame)
	{
	case 0:
		ai_run(self, 20);
		break;

	case 1:
	case 5:
		ai_run(self, 25);
		break;

	case 2:
		ai_run(self, 18);
		break;

	case 3:
		ai_run(self, 16);
		break;

	case 4:
		ai_run(self, 14);
		break;

	case 6:
		ai_run(self, 21);
		break;

	case 7:
		ai_run(self, 13);
		break;
	}

	if (frame == 0)
	{
		CheckForCharge(self);
	}
}

void hknight_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		PF_sound(self, CHAN_VOICE, "hknight/pain1.wav", 1, ATTN_NORM);
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &hknight_run1;
	}
}

void hknight_death_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
	case 7:
		ai_forward(self, 10);
		break;

	case 1:
		ai_forward(self, 8);
		break;

	case 2:
		self->v.solid = SOLID_NOT;
		ai_forward(self, 7);
		break;

	case 8:
		ai_forward(self, 11);
		break;

	default: break;
	}
}

void hknight_deathb_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 2)
	{
		self->v.solid = SOLID_NOT;
	}
}

void hknight_magic_core_frame(edict_t* self, const Animation* animation, int frame, int startFrame)
{
	if (frame >= startFrame && frame <= (startFrame + 5))
	{
		//Increase offset by one every frame it's active.
		hknight_shot(self, -2 + (frame - startFrame));
	}
	else
	{
		ai_face(self);
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &hknight_run1;
	}
}

void hknight_magicab_frame(edict_t* self, const Animation* animation, int frame)
{
	hknight_magic_core_frame(self, animation, frame, 6);
}

void hknight_magicac_frame(edict_t* self, const Animation* animation, int frame)
{
	hknight_magic_core_frame(self, animation, frame, 5);
}

void hknight_char_a_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	default:
		ai_charge(self, 20);
		break;

	case 1:
	case 13:
		ai_charge(self, 25);
		break;

	case 2:
	case 10:
		ai_charge(self, 18);
		break;

	case 3:
	case 11:
		ai_charge(self, 16);
		break;

	case 4:
	case 12:
		ai_charge(self, 14);
		break;

	case 6:
	case 14:
		ai_charge(self, 21);
		break;

	case 7:
	case 15:
		ai_charge(self, 13);
		break;
	}

	if (frame >= 6 && frame <= 11)
	{
		ai_melee(self);
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &hknight_run1;
	}
}

void hknight_char_b_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		CheckContinueCharge(self);
		ai_charge(self, 23);
		break;

	case 1:
		ai_charge(self, 17);
		break;

	case 2:
		ai_charge(self, 12);
		break;

	case 3:
		ai_charge(self, 22);
		break;

	case 4:
		ai_charge(self, 18);
		break;

	case 5:
		ai_charge(self, 8);
		break;
	}

	ai_melee(self);
}

void hknight_slice_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		ai_charge(self, 9);
		break;

	case 1:
		ai_charge(self, 6);
		break;

	case 2:
		ai_charge(self, 13);
		break;

	case 3:
		ai_charge(self, 4);
		break;

	case 4:
		ai_charge(self, 7);
		ai_melee(self);
		break;

	case 5:
		ai_charge(self, 15);
		ai_melee(self);
		break;

	case 6:
		ai_charge(self, 8);
		ai_melee(self);
		break;

	case 7:
		ai_charge(self, 2);
		ai_melee(self);
		break;

	case 8:
		ai_melee(self);
		break;

	case 9:
		ai_charge(self, 3);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &hknight_run1;
	}
}

void hknight_smash_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		ai_charge(self, 1);
		break;

	case 1:
		ai_charge(self, 13);
		break;

	case 2:
		ai_charge(self, 9);
		break;

	case 3:
		ai_charge(self, 11);
		break;

	case 4:
		ai_charge(self, 10);
		ai_melee(self);
		break;

	case 5:
		ai_charge(self, 7);
		ai_melee(self);
		break;

	case 6:
		ai_charge(self, 12);
		ai_melee(self);
		break;

	case 7:
		ai_charge(self, 2);
		ai_melee(self);
		break;

	case 8:
		ai_charge(self, 3);
		ai_melee(self);
		break;

	case 9:
		ai_charge(self, 0);
		break;

	case 10:
		ai_charge(self, 0);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &hknight_run1;
	}
}

void hknight_attack_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		ai_charge(self, 2);
		break;

	default:
		ai_charge(self, 0);
		break;

	case 3:
	case 4:
	case 5:
		ai_melee(self);
		break;

	case 6:
	case 15:
		ai_charge(self, 1);
		break;

	case 7:
		ai_charge(self, 4);
		break;

	case 8:
		ai_charge(self, 5);
		break;

	case 9:
		ai_charge(self, 3);
		ai_melee(self);
		break;

	case 10:
	case 11:
		ai_charge(self, 2);
		ai_melee(self);
		break;

	case 16:
		ai_charge(self, 1);
		ai_melee(self);
		break;

	case 17:
		ai_charge(self, 3);
		ai_melee(self);
		break;

	case 18:
		ai_charge(self, 4);
		ai_melee(self);
		break;

	case 20:
		ai_charge(self, 6);
		break;

	case 21:
		ai_charge(self, 7);
		break;

	case 22:
		ai_charge(self, 3);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &hknight_run1;
	}
}

const Animations HellKnightAnimations = MakeAnimations(
	{
		{"stand", 9, &hknight_stand_frame, AnimationFlag::Looping},
		{"walk", 20, &hknight_walk_frame, AnimationFlag::Looping},
		{"run", 8, &hknight_run_frame, AnimationFlag::Looping},
		{"pain", 5, &hknight_pain_frame},
		{"death", 12, &hknight_death_frame},
		{"deathb", 9, &hknight_deathb_frame},
		{"char_a", 16, &hknight_char_a_frame},
		{"magica", 14, &hknight_magicab_frame},
		{"magicb", 13, &hknight_magicab_frame},
		{"char_b", 6, &hknight_char_b_frame, AnimationFlag::Looping},
		{"slice", 10, &hknight_slice_frame},
		{"smash", 11, &hknight_smash_frame},
		{"w_attack", 22, &hknight_attack_frame},
		{"magicc", 11, &hknight_magicac_frame}
	});

const Animations* hknight_animations_get(edict_t* self)
{
	return &HellKnightAnimations;
}

LINK_FUNCTION_TO_NAME(hknight_animations_get);

void hknight_shot(edict_t* self, float offset)
{
	Vector3D offang;
	Vector3D vec;
	
	PF_vectoangles (AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin), offang);
	offang[1] += offset * 6;
	
	PF_makevectors (offang);

	auto org = AsVector(self->v.origin) + AsVector(self->v.mins) + AsVector(self->v.size)*0.5f + AsVector(pr_global_struct->v_forward) * 20;

// set missile speed
	PF_normalize (pr_global_struct->v_forward, vec);
	vec[2] = 0 - vec[2] + (PF_random() - 0.5) * 0.1;
	
	auto newmis = launch_spike (self, org, vec);
	newmis->v.classname = "knightspike";
	PF_setmodel (newmis, "progs/k_spike.mdl");
	PF_setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);		
	AsVector(newmis->v.velocity) = vec*300;
	PF_sound (self, CHAN_WEAPON, "hknight/attack1.wav", 1, ATTN_NORM);
}

void CheckForCharge(edict_t* self)
{
// check for mad charge
if (!enemy_vis)
	return;
if (pr_global_struct->time < self->v.attack_finished)
	return;	
if ( fabs(self->v.origin[2] - self->v.enemy->v.origin[2]) > 20)
	return;		// too much height change
if ( PF_vlen (AsVector(self->v.origin) - AsVector(self->v.enemy->v.origin)) < 80)
	return;		// use regular attack

// charge		
	SUB_AttackFinished (self, 2);
	hknight_char_a1 (self);

}

void CheckContinueCharge(edict_t* self)
{
	if (pr_global_struct->time > self->v.attack_finished)
	{
		SUB_AttackFinished (self, 3);
		hknight_run1 (self);
		return;		// done charging
	}
	if (PF_random() > 0.5)
		PF_sound (self, CHAN_WEAPON, "knight/sword2.wav", 1, ATTN_NORM);
	else
		PF_sound (self, CHAN_WEAPON, "knight/sword1.wav", 1, ATTN_NORM);
}

//===========================================================================

void hknight_stand1(edict_t* self)
{
	animation_set(self, "stand");
}

LINK_FUNCTION_TO_NAME(hknight_stand1);

//===========================================================================

void hknight_walk1(edict_t* self)
{
	animation_set(self, "walk");
}

LINK_FUNCTION_TO_NAME(hknight_walk1);

//===========================================================================

void hknight_run1(edict_t* self)
{
	animation_set(self, "run");
}

LINK_FUNCTION_TO_NAME(hknight_run1);

//============================================================================

void hknight_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

//============================================================================

void hknight_die1(edict_t* self)
{
	animation_set(self, "death");
}

void hknight_dieb1(edict_t* self)
{
	animation_set(self, "deathb");
}

void hknight_die(edict_t* self)
{
// check for gib
	if (self->v.health < -40)
	{
		PF_sound (self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead (self, "progs/h_hellkn.mdl", self->v.health);
		ThrowGib (self, "progs/gib1.mdl", self->v.health);
		ThrowGib (self, "progs/gib2.mdl", self->v.health);
		ThrowGib (self, "progs/gib3.mdl", self->v.health);
		return;
	}

// regular death
	PF_sound (self, CHAN_VOICE, "hknight/death1.wav", 1, ATTN_NORM);
	if (PF_random() > 0.5)
		hknight_die1 (self);
	else
		hknight_dieb1 (self);
}

LINK_FUNCTION_TO_NAME(hknight_die);

//============================================================================

void hknight_magica1(edict_t* self)
{
	animation_set(self, "magica");
}

//============================================================================

void hknight_magicb1(edict_t* self)
{
	animation_set(self, "magicb");
}

//============================================================================

void hknight_magicc1(edict_t* self)
{
	animation_set(self, "magicc");
}

LINK_FUNCTION_TO_NAME(hknight_magicc1);

//===========================================================================

void hknight_char_a1(edict_t* self)
{
	animation_set(self, "char_a");
}

//===========================================================================

void hknight_char_b1(edict_t* self)
{
	animation_set(self, "char_b");
}

//===========================================================================

void hknight_slice1(edict_t* self)
{
	animation_set(self, "slice");
}

//===========================================================================

void hknight_smash1(edict_t* self)
{
	animation_set(self, "smash");
}

//============================================================================

void hknight_watk1(edict_t* self)
{
	animation_set(self, "w_attack");
}

//============================================================================

void hk_idle_sound(edict_t* self)
{
	if (PF_random() < 0.2)
		PF_sound (self, CHAN_VOICE, "hknight/idle.wav", 1, ATTN_NORM);
}

void hknight_pain(edict_t* self, edict_t* attacker, float damage)
{
	if (self->v.pain_finished > pr_global_struct->time)
		return;

	PF_sound (self, CHAN_VOICE, "hknight/pain1.wav", 1, ATTN_NORM);

	if (pr_global_struct->time - self->v.pain_finished > 5)
	{	// allways go into pain frame if it has been a while
		hknight_pain1 (self);
		self->v.pain_finished = pr_global_struct->time + 1;
		return;
	}
	
	if ((PF_random()*30 > damage) )
		return;		// didn't flinch

	self->v.pain_finished = pr_global_struct->time + 1;
	hknight_pain1 (self);
}

LINK_FUNCTION_TO_NAME(hknight_pain);

void hknight_melee(edict_t* self)
{
	++pr_global_struct->hknight_type;

	PF_sound (self, CHAN_WEAPON, "hknight/slash1.wav", 1, ATTN_NORM);
	if (pr_global_struct->hknight_type == 1)
		hknight_slice1 (self);
	else if (pr_global_struct->hknight_type == 2)
		hknight_smash1 (self);
	else if (pr_global_struct->hknight_type == 3)
	{
		hknight_watk1 (self);
		pr_global_struct->hknight_type = 0;
	}
}

LINK_FUNCTION_TO_NAME(hknight_melee);

/*QUAKED monster_hell_knight (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void monster_hell_knight(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model ("progs/hknight.mdl");
	PF_precache_model ("progs/k_spike.mdl");
	PF_precache_model ("progs/h_hellkn.mdl");

	
	PF_precache_sound ("hknight/attack1.wav");
	PF_precache_sound ("hknight/death1.wav");
	PF_precache_sound ("hknight/pain1.wav");
	PF_precache_sound ("hknight/sight1.wav");
	PF_precache_sound("hknight/hit.wav");		// used by C code, so don't sound2
	PF_precache_sound ("hknight/slash1.wav");
	PF_precache_sound ("hknight/idle.wav");
	PF_precache_sound ("hknight/grunt.wav");

	PF_precache_sound ("knight/sword1.wav");
	PF_precache_sound ("knight/sword2.wav");
	
	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel (self, "progs/hknight.mdl");

	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 40});
	self->v.health = 250;

	self->v.th_stand = hknight_stand1;
	self->v.th_walk = hknight_walk1;
	self->v.th_run = hknight_run1;
	self->v.th_melee = hknight_melee;
	self->v.th_missile = hknight_magicc1;
	self->v.th_pain = hknight_pain;
	self->v.th_die = hknight_die;
	self->v.animations_get = &hknight_animations_get;
	
	walkmonster_start (self);
}

LINK_ENTITY_TO_CLASS(monster_hell_knight);
