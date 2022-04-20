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
#include "combat.h"
#include "Game.h"
#include "subs.h"

void ClientObituary(edict_t* self, edict_t* targ, edict_t* attacker);

void monster_death_use(edict_t* self);

//============================================================================

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
bool CanDamage(edict_t* self, edict_t* targ, edict_t* inflictor)
{
	// bmodels need special checking because their origin is 0,0,0
	if (targ->v.movetype == MOVETYPE_PUSH)
	{
		PF_traceline(inflictor->v.origin, 0.5 * (AsVector(targ->v.absmin) + AsVector(targ->v.absmax)), TRUE, self);
		if (pr_global_struct->trace_fraction == 1)
			return true;
		if (pr_global_struct->trace_ent == targ)
			return true;
		return false;
	}

	PF_traceline(inflictor->v.origin, targ->v.origin, TRUE, self);
	if (pr_global_struct->trace_fraction == 1)
		return true;
	PF_traceline(inflictor->v.origin, AsVector(targ->v.origin) + Vector3D{15, 15, 0}, TRUE, self);
	if (pr_global_struct->trace_fraction == 1)
		return true;
	PF_traceline(inflictor->v.origin, AsVector(targ->v.origin) + Vector3D{-15, -15, 0}, TRUE, self);
	if (pr_global_struct->trace_fraction == 1)
		return true;
	PF_traceline(inflictor->v.origin, AsVector(targ->v.origin) + Vector3D{-15, 15, 0}, TRUE, self);
	if (pr_global_struct->trace_fraction == 1)
		return true;
	PF_traceline(inflictor->v.origin, AsVector(targ->v.origin) + Vector3D{15, -15, 0}, TRUE, self);
	if (pr_global_struct->trace_fraction == 1)
		return true;

	return false;
}


/*
============
Killed
============
*/
void Killed(edict_t* self, edict_t* targ, edict_t* attacker)
{
	auto oself = self;
	self = targ;

	if (self->v.health < -99)
		self->v.health = -99;		// don't let sbar look bad if a player

	if (self->v.movetype == MOVETYPE_PUSH || self->v.movetype == MOVETYPE_NONE)
	{	// doors, triggers, etc
		self->v.th_die(self);
		self = oself;
		return;
	}

	self->v.enemy = attacker;

	// bump the monster counter
	if ((int)self->v.flags & FL_MONSTER)
	{
		pr_global_struct->killed_monsters = pr_global_struct->killed_monsters + 1;
		PF_WriteByte(MSG_ALL, SVC_KILLEDMONSTER);
	}

	ClientObituary(self, self, attacker);

	self->v.takedamage = DAMAGE_NO;
	self->v.touch = &SUB_NullTouch;

	monster_death_use(self);
	self->v.th_die(self);

	self = oself;
}


/*
============
T_Damage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
============
*/
void T_Damage(edict_t* self, edict_t* targ, edict_t* inflictor, edict_t* attacker, float damage)
{
	if (!targ->v.takedamage)
		return;

	// used by buttons and triggers to set activator for target firing
	pr_global_struct->damage_attacker = attacker;

	// check for quad damage powerup on the attacker
	if (attacker->v.super_damage_finished > pr_global_struct->time)
		damage = damage * 4;

	// save damage based on the target's armor level

	auto save = ceil(targ->v.armortype * damage);
	if (save >= targ->v.armorvalue)
	{
		save = targ->v.armorvalue;
		targ->v.armortype = 0;	// lost all armor
		targ->v.items &= ~(IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3);
	}

	targ->v.armorvalue = targ->v.armorvalue - save;
	auto take = ceil(damage - save);

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// FIXME: remove after combining shotgun blasts?
	if ((int)targ->v.flags & FL_CLIENT)
	{
		targ->v.dmg_take = targ->v.dmg_take + take;
		targ->v.dmg_save = targ->v.dmg_save + save;
		targ->v.dmg_inflictor = inflictor;
	}

	// figure momentum add
	if ((inflictor != pr_global_struct->world) && (targ->v.movetype == MOVETYPE_WALK))
	{
		vec3_t center;
		VectorAdd(inflictor->v.absmin, inflictor->v.absmax, center);

		vec3_t dir;
		VectorMA(targ->v.origin, -0.5, center, dir);
		PF_normalize(dir, dir);
		VectorMA(targ->v.velocity, damage * 8, dir, targ->v.velocity);
	}

	// check for godmode or invincibility
	if ((int)targ->v.flags & FL_GODMODE)
		return;
	if (targ->v.invincible_finished >= pr_global_struct->time)
	{
		if (self->v.invincible_sound < pr_global_struct->time)
		{
			PF_sound(targ, CHAN_ITEM, "items/protect3.wav", 1, ATTN_NORM);
			self->v.invincible_sound = pr_global_struct->time + 2;
		}
		return;
	}

	// team play damage avoidance
	if ((pr_global_struct->teamplay == 1) && (targ->v.team > 0) && (targ->v.team == attacker->v.team))
		return;

	// do the damage
	targ->v.health = targ->v.health - take;

	if (targ->v.health <= 0)
	{
		Killed(self, targ, attacker);
		return;
	}

	// react to the damage
	auto oldself = self;
	self = targ;

	if (((int)self->v.flags & FL_MONSTER) && attacker != pr_global_struct->world)
	{
		// get mad unless of the same class (except for soldiers)
		if (self != attacker && attacker != self->v.enemy)
		{
			if ((self->v.classname != attacker->v.classname)
			|| (!strcmp(self->v.classname, "monster_army")))
			{
				if (self->v.enemy && !strcmp(self->v.enemy->v.classname, "player"))
					self->v.oldenemy = self->v.enemy;
				self->v.enemy = attacker;
				FoundTarget(self);
			}
		}
	}

	if (self->v.th_pain)
	{
		self->v.th_pain(self, attacker, take);
		// nightmare mode monsters don't go into pain frames often
		if (pr_global_struct->game_skill == 3)
			self->v.pain_finished = pr_global_struct->time + 5;
	}

	self = oldself;
}

/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage(edict_t* self, edict_t* inflictor, edict_t* attacker, float damage, edict_t* ignore)
{
	auto head = PF_findradius(inflictor->v.origin, damage + 40);

	while (head)
	{
		if (head != ignore)
		{
			if (head->v.takedamage)
			{
				Vector3D org = AsVector(head->v.origin) + (AsVector(head->v.mins) + AsVector(head->v.maxs)) * 0.5;
				float points = 0.5 * PF_vlen(AsVector(inflictor->v.origin) - org);
				if (points < 0)
					points = 0;
				points = damage - points;
				if (head == attacker)
					points = points * 0.5;
				if (points > 0)
				{
					if (CanDamage(self, head, inflictor))
					{	// shambler takes half damage from all explosions
						if (!strcmp(head->v.classname, "monster_shambler"))
							T_Damage(self, head, inflictor, attacker, points * 0.5);
						else
							T_Damage(self, head, inflictor, attacker, points);
					}
				}
			}
		}
		head = head->v.chain;
	}
}

/*
============
T_BeamDamage
============
*/
void T_BeamDamage(edict_t* self, edict_t* attacker, float damage)
{
	auto head = PF_findradius(attacker->v.origin, damage + 40);

	while (head)
	{
		if (head->v.takedamage)
		{
			float points = 0.5 * PF_vlen(AsVector(attacker->v.origin) - AsVector(head->v.origin));
			if (points < 0)
				points = 0;
			points = damage - points;
			if (head == attacker)
				points = points * 0.5;
			if (points > 0)
			{
				if (CanDamage(self, head, attacker))
				{
					if (!strcmp(head->v.classname, "monster_shambler"))
						T_Damage(self, head, attacker, attacker, points * 0.5);
					else
						T_Damage(self, head, attacker, attacker, points);
				}
			}
		}
		head = head->v.chain;
	}
}
