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
#include "combat.h"
#include "fight.h"
#include "Game.h"
#include "subs.h"

/*

A monster is in fight mode if it thinks it can effectively attack its
enemy.

When it decides it can't attack, it goes into hunt mode.

*/

float game_anglemod(float v);

void knight_atk1(edict_t* self);
void knight_runatk1(edict_t* self);
void ogre_smash1(edict_t* self);
void ogre_swing1(edict_t* self);

void sham_smash1(edict_t* self);
void sham_swingr1(edict_t* self);
void sham_swingl1(edict_t* self);

float DemonCheckAttack(edict_t* self);
void Demon_Melee(edict_t* self, float side);

void ChooseTurn(edict_t* self, vec3_t dest);

void ai_face(edict_t* self);

void knight_attack(edict_t* self)
{
	// decide if now is a good swing time
	float len = PF_vlen(AsVector(self->v.enemy->v.origin) + AsVector(self->v.enemy->v.view_ofs) - (AsVector(self->v.origin) + AsVector(self->v.view_ofs)));

	if (len < 80)
		knight_atk1(self);
	else
		knight_runatk1(self);
}

//=============================================================================

/*
===========
CheckAttack

The player is in view, so decide to move or launch an attack
Returns FALSE if movement should continue
============
*/
bool CheckAttack(edict_t* self)
{
	auto targ = self->v.enemy;

	// see if any entities are in the way of the shot
	auto spot1 = AsVector(self->v.origin) + AsVector(self->v.view_ofs);
	auto spot2 = AsVector(targ->v.origin) + AsVector(targ->v.view_ofs);

	PF_traceline(spot1, spot2, FALSE, self);

	if (pr_global_struct->trace_ent != targ)
		return false;		// don't have a clear shot

	if (pr_global_struct->trace_inopen && pr_global_struct->trace_inwater)
		return false;			// sight line crossed contents

	if (enemy_range == RANGE_MELEE)
	{	// melee attack
		if (self->v.th_melee)
		{
			if (!strcmp(self->v.classname, "monster_knight"))
				knight_attack(self);
			else
				self->v.th_melee(self);
			return true;
		}
	}

	// missile attack
	if (!self->v.th_missile)
		return false;

	if (pr_global_struct->time < self->v.attack_finished)
		return false;

	if (enemy_range == RANGE_FAR)
		return false;

	float chance;

	if (enemy_range == RANGE_MELEE)
	{
		chance = 0.9f;
		self->v.attack_finished = 0;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		if (self->v.th_melee)
			chance = 0.2f;
		else
			chance = 0.4f;
	}
	else if (enemy_range == RANGE_MID)
	{
		if (self->v.th_melee)
			chance = 0.05f;
		else
			chance = 0.1f;
	}
	else
		chance = 0;

	if (random() < chance)
	{
		self->v.th_missile(self);
		SUB_AttackFinished(self, 2 * random());
		return true;
	}

	return false;
}


/*
=============
ai_face

Stay facing the enemy
=============
*/
void ai_face(edict_t* self)
{
	self->v.ideal_yaw = PF_vectoyaw(AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin));
	PF_changeyaw(self);
}

/*
=============
ai_charge

The monster is in a melee attack, so get as close as possible to .enemy
=============
*/

void ai_charge(edict_t* self, float d)
{
	ai_face(self);
	SV_MoveToGoal(self, d);		// done in C code...
}

void ai_charge_side(edict_t* self)
{
	// aim to the left of the enemy for a flyby

	self->v.ideal_yaw = PF_vectoyaw(AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin));
	PF_changeyaw(self);

	PF_makevectors(self->v.angles);
	auto dtemp = AsVector(self->v.enemy->v.origin) - 30 * AsVector(pr_global_struct->v_right);
	const float heading = PF_vectoyaw(dtemp - AsVector(self->v.origin));

	PF_walkmove(self, heading, 20);
}


/*
=============
ai_melee

=============
*/
void ai_melee(edict_t* self)
{
	if (!self->v.enemy)
		return;		// removed before stroke

	auto delta = AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin);

	if (PF_vlen(delta) > 60)
		return;

	const float ldmg = (random() + random() + random()) * 3;
	T_Damage(self, self->v.enemy, self, self, ldmg);
}


void ai_melee_side(edict_t* self)
{
	if (!self->v.enemy)
		return;		// removed before stroke

	ai_charge_side(self);

	auto delta = AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin);

	if (PF_vlen(delta) > 60)
		return;
	if (!CanDamage(self, self->v.enemy, self))
		return;
	const float ldmg = (random() + random() + random()) * 3;
	T_Damage(self, self->v.enemy, self, self, ldmg);
}


//=============================================================================

/*
===========
SoldierCheckAttack

The player is in view, so decide to move or launch an attack
Returns FALSE if movement should continue
============
*/
float SoldierCheckAttack(edict_t* self)
{
	auto targ = self->v.enemy;

	// see if any entities are in the way of the shot
	auto spot1 = AsVector(self->v.origin) + AsVector(self->v.view_ofs);
	auto spot2 = AsVector(targ->v.origin) + AsVector(targ->v.view_ofs);

	PF_traceline(spot1, spot2, FALSE, self);

	if (pr_global_struct->trace_inopen && pr_global_struct->trace_inwater)
		return FALSE;			// sight line crossed contents

	if (pr_global_struct->trace_ent != targ)
		return FALSE;	// don't have a clear shot


// missile attack
	if (pr_global_struct->time < self->v.attack_finished)
		return FALSE;

	if (enemy_range == RANGE_FAR)
		return FALSE;

	float chance;

	if (enemy_range == RANGE_MELEE)
		chance = 0.9f;
	else if (enemy_range == RANGE_NEAR)
		chance = 0.4f;
	else if (enemy_range == RANGE_MID)
		chance = 0.05f;
	else
		chance = 0;

	if (random() < chance)
	{
		self->v.th_missile(self);
		SUB_AttackFinished(self, 1 + random());
		if (random() < 0.3)
			self->v.lefty = !self->v.lefty;

		return TRUE;
	}

	return FALSE;
}
//=============================================================================

/*
===========
ShamCheckAttack

The player is in view, so decide to move or launch an attack
Returns FALSE if movement should continue
============
*/
float ShamCheckAttack(edict_t* self)
{
	if (enemy_range == RANGE_MELEE)
	{
		if (CanDamage(self, self->v.enemy, self))
		{
			self->v.attack_state = AS_MELEE;
			return TRUE;
		}
	}

	if (pr_global_struct->time < self->v.attack_finished)
		return FALSE;

	if (!enemy_vis)
		return FALSE;

	auto targ = self->v.enemy;

	// see if any entities are in the way of the shot
	auto spot1 = AsVector(self->v.origin) + AsVector(self->v.view_ofs);
	auto spot2 = AsVector(targ->v.origin) + AsVector(targ->v.view_ofs);

	if (PF_vlen(spot1 - spot2) > 600)
		return FALSE;

	PF_traceline(spot1, spot2, FALSE, self);

	if (pr_global_struct->trace_inopen && pr_global_struct->trace_inwater)
		return FALSE;			// sight line crossed contents

	if (pr_global_struct->trace_ent != targ)
	{
		return FALSE;	// don't have a clear shot
	}

	// missile attack
	if (enemy_range == RANGE_FAR)
		return FALSE;

	self->v.attack_state = AS_MISSILE;
	SUB_AttackFinished(self, 2 + 2 * random());
	return TRUE;
}

//============================================================================

/*
===========
OgreCheckAttack

The player is in view, so decide to move or launch an attack
Returns FALSE if movement should continue
============
*/
float OgreCheckAttack(edict_t* self)
{
	if (enemy_range == RANGE_MELEE)
	{
		if (CanDamage(self, self->v.enemy, self))
		{
			self->v.attack_state = AS_MELEE;
			return TRUE;
		}
	}

	if (pr_global_struct->time < self->v.attack_finished)
		return FALSE;

	if (!enemy_vis)
		return FALSE;

	auto targ = self->v.enemy;

	// see if any entities are in the way of the shot
	auto spot1 = AsVector(self->v.origin) + AsVector(self->v.view_ofs);
	auto spot2 = AsVector(targ->v.origin) + AsVector(targ->v.view_ofs);

	PF_traceline(spot1, spot2, FALSE, self);

	if (pr_global_struct->trace_inopen && pr_global_struct->trace_inwater)
		return FALSE;			// sight line crossed contents

	if (pr_global_struct->trace_ent != targ)
	{
		return FALSE;	// don't have a clear shot
	}

	// missile attack
	if (pr_global_struct->time < self->v.attack_finished)
		return FALSE;

	float chance;

	if (enemy_range == RANGE_FAR)
		return FALSE;

	else if (enemy_range == RANGE_NEAR)
		chance = 0.10f;
	else if (enemy_range == RANGE_MID)
		chance = 0.05f;
	else
		chance = 0;

	self->v.attack_state = AS_MISSILE;
	SUB_AttackFinished(self, 1 + 2 * random());
	return TRUE;
}
