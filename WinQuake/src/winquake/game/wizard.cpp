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

WIZARD

==============================================================================
*/
void Wiz_idlesound(edict_t* self);
void Wiz_StartFast(edict_t* self);
void wiz_run1(edict_t* self);
void wiz_side1(edict_t* self);
void WizardAttackFinished(edict_t* self);

enum class WizardHoverMode
{
	Stand = 0,
	Walk,
	Side
};

void wizard_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void wizard_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_walk(self, 8);

	if (frame == 0)
	{
		Wiz_idlesound(self);
	}
}

void wizard_side_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_run(self, 8);

	if (frame == 0)
	{
		Wiz_idlesound(self);
	}
}

void wizard_hover_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (static_cast<WizardHoverMode>(self->v.animation_mode))
	{
	default:
	case WizardHoverMode::Stand:
		wizard_stand_frame(self, animation, frame);
		break;

	case WizardHoverMode::Walk:
		wizard_walk_frame(self, animation, frame);
		break;

	case WizardHoverMode::Side:
		wizard_side_frame(self, animation, frame);
		break;
	}
}

void wizard_run_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_run(self, 16);

	if (frame == 0)
	{
		Wiz_idlesound(self);
	}
}

void wizard_magatt_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_face(self);

	switch (frame)
	{
	case 0:
		Wiz_StartFast(self);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &wiz_run1;
		SUB_AttackFinished(self, 2);
		WizardAttackFinished(self);
	}
}

void wizard_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = &wiz_run1;
	}
}

void wizard_death_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		self->v.velocity[0] = -200 + 400 * random();
		self->v.velocity[1] = -200 + 400 * random();
		self->v.velocity[2] = 100 + 100 * random();
		self->v.flags &= ~FL_ONGROUND;
		PF_sound(self, CHAN_VOICE, "wizard/wdeath.wav", 1, ATTN_NORM);
		break;

	case 2:
		self->v.solid = SOLID_NOT;
		break;
	}
}

const Animations WizardAnimations = MakeAnimations(
	{
		{"hover", 15, &wizard_hover_frame, AnimationFlag::Looping},
		{"fly", 14, &wizard_run_frame, AnimationFlag::Looping},
		{"magatt", 13, &wizard_magatt_frame},
		{"pain", 4, &wizard_pain_frame},
		{"death", 8, &wizard_death_frame}
	});

/*
==============================================================================

WIZARD

If the player moves behind cover before the missile is launched, launch it
at the last visible spot with no velocity leading, in hopes that the player
will duck back out and catch it.
==============================================================================
*/

/*
=============
LaunchMissile

Sets the given entities velocity and angles so that it will hit self->v.enemy
if self->v.enemy maintains it's current velocity
0.1 is moderately accurate, 0.0 is totally accurate
=============
*/
void LaunchMissile(edict_t* self, edict_t* missile, float mspeed, float accuracy)
{
	PF_makevectors (self->v.angles);
		
// set missile speed
	auto vec = AsVector(self->v.enemy->v.origin) + AsVector(self->v.enemy->v.mins) + AsVector(self->v.enemy->v.size) * 0.7f - AsVector(missile->v.origin);

// calc aproximate time for missile to reach vec
	const float fly = PF_vlen (vec) / mspeed;
	
// get the entities xy velocity
	Vector3D move = AsVector(self->v.enemy->v.velocity);
	move[2] = 0;

// project the target forward in time
	vec = vec + move * fly;
	
	PF_normalize(vec, vec);
	vec = vec + accuracy* AsVector(pr_global_struct->v_up)*(random()- 0.5) + accuracy* AsVector(pr_global_struct->v_right)*(random()- 0.5);
	
	AsVector(missile->v.velocity) = vec * mspeed;

	AsVector(missile->v.angles) = AsVector(vec3_origin);
	missile->v.angles[1] = PF_vectoyaw(missile->v.velocity);

// set missile duration
	missile->v.nextthink = pr_global_struct->time + 5;
	missile->v.think = SUB_Remove;	
}

/*
=================
WizardCheckAttack
=================
*/
bool WizardCheckAttack(edict_t* self)
{
	if (pr_global_struct->time < self->v.attack_finished)
		return false;
	if (!enemy_vis)
		return false;

	if (enemy_range == RANGE_FAR)
	{
		if (self->v.attack_state != AS_STRAIGHT)
		{
			self->v.attack_state = AS_STRAIGHT;
			wiz_run1 (self);
		}
		return false;
	}
		
	auto targ = self->v.enemy;
	
// see if any entities are in the way of the shot
	auto spot1 = AsVector(self->v.origin) + AsVector(self->v.view_ofs);
	auto spot2 = AsVector(targ->v.origin) + AsVector(targ->v.view_ofs);

	PF_traceline (spot1, spot2, FALSE, self);

	if (pr_global_struct->trace_ent != targ)
	{	// don't have a clear shot, so move to a side
		if (self->v.attack_state != AS_STRAIGHT)
		{
			self->v.attack_state = AS_STRAIGHT;
			wiz_run1 (self);
		}
		return false;
	}

	float chance;
			
	if (enemy_range == RANGE_MELEE)
		chance = 0.9f;
	else if (enemy_range == RANGE_NEAR)
		chance = 0.6f;
	else if (enemy_range == RANGE_MID)
		chance = 0.2f;
	else
		chance = 0;

	if (random () < chance)
	{
		self->v.attack_state = AS_MISSILE;
		return true;
	}

	if (enemy_range == RANGE_MID)
	{
		if (self->v.attack_state != AS_STRAIGHT)
		{
			self->v.attack_state = AS_STRAIGHT;
			wiz_run1 (self);
		}
	}
	else
	{
		if (self->v.attack_state != AS_SLIDING)
		{
			self->v.attack_state = AS_SLIDING;
			wiz_side1 (self);
		}
	}
	
	return false;
}

/*
=================
WizardAttackFinished
=================
*/
void WizardAttackFinished(edict_t* self)
{
	if (enemy_range >= RANGE_MID || !enemy_vis)
	{
		self->v.attack_state = AS_STRAIGHT;
		self->v.think = wiz_run1;
	}
	else
	{
		self->v.attack_state = AS_SLIDING;
		self->v.think = wiz_side1;
	}
}

/*
==============================================================================

FAST ATTACKS

==============================================================================
*/

void Wiz_FastFire(edict_t* self)
{
	if (self->v.owner->v.health > 0)
	{
		self->v.owner->v.effects |= EF_MUZZLEFLASH;

		PF_makevectors (self->v.enemy->v.angles);	
		auto dst = AsVector(self->v.enemy->v.origin) - 13* AsVector(self->v.movedir);
	
		Vector3D vec;
		PF_normalize(dst - AsVector(self->v.origin), vec);
		PF_sound (self, CHAN_WEAPON, "wizard/wattack.wav", 1, ATTN_NORM);
		launch_spike (self, self->v.origin, vec);
		AsVector(newmis->v.velocity) = vec*600;
		newmis->v.owner = self->v.owner;
		newmis->v.classname = "wizspike";
		PF_setmodel (newmis, "progs/w_spike.mdl");
		PF_setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);		
	}

	PF_Remove (self);
}

void Wiz_StartFast(edict_t* self)
{
	PF_sound (self, CHAN_WEAPON, "wizard/wattack.wav", 1, ATTN_NORM);
	AsVector(self->v.v_angle) = AsVector(self->v.angles);
	PF_makevectors (self->v.angles);

	auto missile = PF_Spawn ();
	missile->v.owner = self;
	missile->v.nextthink = pr_global_struct->time + 0.6;
	PF_setsize (missile, Vector3D{0, 0, 0}, Vector3D{0, 0, 0});		
	PF_setorigin (missile, AsVector(self->v.origin) + Vector3D{0, 0, 30} + AsVector(pr_global_struct->v_forward)*14 + AsVector(pr_global_struct->v_right)*14);
	missile->v.enemy = self->v.enemy;
	missile->v.nextthink = pr_global_struct->time + 0.8;
	missile->v.think = Wiz_FastFire;
	AsVector(missile->v.movedir) = AsVector(pr_global_struct->v_right);

	missile = PF_Spawn();
	missile->v.owner = self;
	missile->v.nextthink = pr_global_struct->time + 1;
	PF_setsize (missile, Vector3D{0, 0, 0}, Vector3D{0, 0, 0});		
	PF_setorigin (missile, AsVector(self->v.origin) + Vector3D{0, 0, 30} + AsVector(pr_global_struct->v_forward)*14 + AsVector(pr_global_struct->v_right)* -14);
	missile->v.enemy = self->v.enemy;
	missile->v.nextthink = pr_global_struct->time + 0.3;
	missile->v.think = Wiz_FastFire;
	AsVector(missile->v.movedir) = AsVector(vec3_origin) -AsVector(pr_global_struct->v_right);
}

void Wiz_idlesound(edict_t* self)
{
	const float wr = random() * 5;

	if (self->v.waitmin < pr_global_struct->time)
	{
		self->v.waitmin = pr_global_struct->time + 2;
		if (wr > 4.5) 
			PF_sound (self, CHAN_VOICE, "wizard/widle1.wav", 1,  ATTN_IDLE);
		if (wr < 1.5)
			PF_sound (self, CHAN_VOICE, "wizard/widle2.wav", 1, ATTN_IDLE);
	}
	return;
}

void wiz_stand1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(WizardHoverMode::Stand);
	animation_set(self, "hover");
}

void wiz_walk1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(WizardHoverMode::Walk);
	animation_set(self, "hover");
}

void wiz_side1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(WizardHoverMode::Side);
	animation_set(self, "hover");
}

void wiz_run1(edict_t* self)
{
	animation_set(self, "fly");
}

void wiz_fast1(edict_t* self)
{
	animation_set(self, "magatt");
}

void wiz_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

void wiz_death1(edict_t* self)
{
	animation_set(self, "death");
}

void wiz_die(edict_t* self)
{
// check for gib
	if (self->v.health < -40)
	{
		PF_sound (self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead (self, "progs/h_wizard.mdl", self->v.health);
		ThrowGib (self, "progs/gib2.mdl", self->v.health);
		ThrowGib (self, "progs/gib2.mdl", self->v.health);
		ThrowGib (self, "progs/gib2.mdl", self->v.health);
		return;
	}

	wiz_death1 (self);
}

void Wiz_Pain(edict_t* self, edict_t* attacker, float damage)
{
	PF_sound (self, CHAN_VOICE, "wizard/wpain.wav", 1, ATTN_NORM);
	if (random()*70 > damage)
		return;		// didn't flinch

	wiz_pain1 (self);
}


void Wiz_Missile(edict_t* self)
{
	wiz_fast1(self);
}

/*QUAKED monster_wizard (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void monster_wizard(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model ("progs/wizard.mdl");
	PF_precache_model ("progs/h_wizard.mdl");
	PF_precache_model ("progs/w_spike.mdl");

	PF_precache_sound ("wizard/hit.wav");		// used by c code
	PF_precache_sound ("wizard/wattack.wav");
	PF_precache_sound ("wizard/wdeath.wav");
	PF_precache_sound ("wizard/widle1.wav");
	PF_precache_sound ("wizard/widle2.wav");
	PF_precache_sound ("wizard/wpain.wav");
	PF_precache_sound ("wizard/wsight.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel (self, "progs/wizard.mdl");

	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 40});
	self->v.health = 80;

	self->v.th_stand = wiz_stand1;
	self->v.th_walk = wiz_walk1;
	self->v.th_run = wiz_run1;
	self->v.th_missile = Wiz_Missile;
	self->v.th_pain = Wiz_Pain;
	self->v.th_die = wiz_die;
	self->v.animations_get = [](edict_t* self) { return &WizardAnimations; };
		
	flymonster_start (self);
}

LINK_ENTITY_TO_CLASS(monster_wizard);
