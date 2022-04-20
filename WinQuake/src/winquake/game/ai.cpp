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
#include "fight.h"
#include "Game.h"
#include "subs.h"

void t_movetarget(edict_t* self, edict_t* other);

/*

.enemy
Will be world if not currently angry at anyone.

.movetarget
The next path spot to walk toward.  If .enemy, ignore .movetarget.
When an enemy is killed, the monster will try to return to it's path.

.huntt_ime
Set to time + something when the player is in sight, but movement straight for
him is blocked.  This causes the monster to use wall following code for
movement direction instead of sighting on the player.

.ideal_yaw
A yaw angle of the intended direction, which will be turned towards at up
to 45 deg / state.  If the enemy is in view and hunt_time is not active,
this will be the exact line towards the enemy->v.

.pausetime
A monster will leave it's stand state and head towards it's .movetarget when
time > .pausetime.

walkmove(angle, speed) primitive is all or nothing
*/

float game_anglemod(float v)
{
	while (v >= 360)
		v = v - 360;
	while (v < 0)
		v = v + 360;
	return v;
}

/*
==============================================================================

MOVETARGET CODE

The angle of the movetarget effects standing and bowing direction, but has no effect on movement, which allways heads to the next target.

targetname
must be present.  The name of this movetarget.

target
the next spot to move to.  If not present, stop here for good.

pausetime
The number of seconds to spend standing or bowing for path_stand or path_bow

==============================================================================
*/


void movetarget_f(edict_t* self)
{
	if (!self->v.targetname)
		PF_objerror("monster_movetarget: no targetname");

	self->v.solid = SOLID_TRIGGER;
	self->v.touch = t_movetarget;
	PF_setsize(self, Vector3D{-8, -8, -8}, Vector3D{8, 8, 8});

}

/*QUAKED path_corner (0.5 0.3 0) (-8 -8 -8) (8 8 8)
Monsters will continue walking towards the next target corner.
*/
void path_corner(edict_t* self)
{
	movetarget_f(self);
}

LINK_ENTITY_TO_CLASS(path_corner);

/*
=============
t_movetarget

Something has bumped into a movetarget.  If it is a monster
moving towards it, change the next destination and continue.
==============
*/
void t_movetarget(edict_t* self, edict_t* other)
{
	if (other->v.movetarget != self)
		return;

	if (other->v.enemy)
		return;		// fighting, not following a path

	auto temp = self;
	self = other;
	other = temp;

	if (!strcmp(self->v.classname, "monster_ogre"))
		PF_sound(self, CHAN_VOICE, "ogre/ogdrag.wav", 1, ATTN_IDLE);// play chainsaw drag sound

//dprint ("t_movetarget\n");
	self->v.goalentity = self->v.movetarget = PF_Find(pr_global_struct->world, "targetname", other->v.target);
	self->v.ideal_yaw = PF_vectoyaw(AsVector(self->v.goalentity->v.origin) - AsVector(self->v.origin));
	if (!self->v.movetarget)
	{
		self->v.pausetime = pr_global_struct->time + 999999;
		self->v.th_stand(self);
		return;
	}
}



//============================================================================

/*
=============
range

returns the range catagorization of an entity reletive to self
0	melee range, will become hostile even if back is turned
1	visibility and infront, or visibility and show hostile
2	infront and show hostile
3	only triggered by damage
=============
*/
float range(edict_t* self, edict_t* targ)
{
	auto spot1 = AsVector(self->v.origin) + AsVector(self->v.view_ofs);
	auto spot2 = AsVector(targ->v.origin) + AsVector(targ->v.view_ofs);

	const float r = PF_vlen(spot1 - spot2);
	if (r < 120)
		return RANGE_MELEE;
	if (r < 500)
		return RANGE_NEAR;
	if (r < 1000)
		return RANGE_MID;
	return RANGE_FAR;
}

/*
=============
visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
bool visible(edict_t* self, edict_t* targ)
{
	auto spot1 = AsVector(self->v.origin) + AsVector(self->v.view_ofs);
	auto spot2 = AsVector(targ->v.origin) + AsVector(targ->v.view_ofs);
	PF_traceline(spot1, spot2, TRUE, self);	// see through other monsters

	if (pr_global_struct->trace_inopen && pr_global_struct->trace_inwater)
		return false;			// sight line crossed contents

	if (pr_global_struct->trace_fraction == 1)
		return true;
	return false;
}


/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
bool infront(edict_t* self, edict_t* targ)
{
	PF_makevectors(self->v.angles);
	vec3_t vec;
	PF_normalize(AsVector(targ->v.origin) - AsVector(self->v.origin), vec);
	float dot = DotProduct(vec, pr_global_struct->v_forward);

	if (dot > 0.3)
	{
		return true;
	}
	return false;
}


//============================================================================

/*
===========
ChangeYaw

Turns towards self->v.ideal_yaw at self->v.yaw_speed
Sets the global variable current_yaw
Called every 0.1 sec by monsters
============
*/
/*

void ChangeYaw(edict_t* self)
{
	local float		ideal, move;

//current_yaw = self->v.ideal_yaw;
// mod down the current angle
	current_yaw = game_anglemod( self->v.angles_y );
	ideal = self->v.ideal_yaw;

	if (current_yaw == ideal)
		return;

	move = ideal - current_yaw;
	if (ideal > current_yaw)
	{
		if (move > 180)
			move = move - 360;
	}
	else
	{
		if (move < -180)
			move = move + 360;
	}

	if (move > 0)
	{
		if (move > self->v.yaw_speed)
			move = self->v.yaw_speed;
	}
	else
	{
		if (move < 0-self->v.yaw_speed )
			move = 0-self->v.yaw_speed;
	}

	current_yaw = game_anglemod (current_yaw + move);

	self->v.angles_y = current_yaw;
}

*/


//============================================================================

void HuntTarget(edict_t* self)
{
	self->v.goalentity = self->v.enemy;
	self->v.think = self->v.th_run;
	self->v.ideal_yaw = PF_vectoyaw(AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin));
	self->v.nextthink = pr_global_struct->time + 0.1;
	SUB_AttackFinished(self, 1);	// wait a while before first attack
}

void SightSound(edict_t* self)
{
	if (!strcmp(self->v.classname, "monster_ogre"))
		PF_sound(self, CHAN_VOICE, "ogre/ogwake.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_knight"))
		PF_sound(self, CHAN_VOICE, "knight/ksight.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_shambler"))
		PF_sound(self, CHAN_VOICE, "shambler/ssight.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_demon1"))
		PF_sound(self, CHAN_VOICE, "demon/sight2.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_wizard"))
		PF_sound(self, CHAN_VOICE, "wizard/wsight.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_zombie"))
		PF_sound(self, CHAN_VOICE, "zombie/z_idle.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_dog"))
		PF_sound(self, CHAN_VOICE, "dog/dsight.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_hell_knight"))
		PF_sound(self, CHAN_VOICE, "hknight/sight1.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_tarbaby"))
		PF_sound(self, CHAN_VOICE, "blob/sight1.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_vomit"))
		PF_sound(self, CHAN_VOICE, "vomitus/v_sight1.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_enforcer"))
	{
		float rsnd = rint(random() * 3);
		if (rsnd == 1)
			PF_sound(self, CHAN_VOICE, "enforcer/sight1.wav", 1, ATTN_NORM);
		else if (rsnd == 2)
			PF_sound(self, CHAN_VOICE, "enforcer/sight2.wav", 1, ATTN_NORM);
		else if (rsnd == 0)
			PF_sound(self, CHAN_VOICE, "enforcer/sight3.wav", 1, ATTN_NORM);
		else
			PF_sound(self, CHAN_VOICE, "enforcer/sight4.wav", 1, ATTN_NORM);
	}
	else if (!strcmp(self->v.classname, "monster_army"))
		PF_sound(self, CHAN_VOICE, "soldier/sight1.wav", 1, ATTN_NORM);
	else if (!strcmp(self->v.classname, "monster_shalrath"))
		PF_sound(self, CHAN_VOICE, "shalrath/sight.wav", 1, ATTN_NORM);
}

void FoundTarget(edict_t* self)
{
	if (!strcmp(self->v.enemy->v.classname, "player"))
	{	// let other monsters see this monster for a while
		sight_entity = self;
		sight_entity_time = pr_global_struct->time;
	}

	self->v.show_hostile = pr_global_struct->time + 1;		// wake up other monsters

	SightSound(self);
	HuntTarget(self);
}

/*
===========
FindTarget

Self is currently not attacking anything, so try to find a target

Returns TRUE if an enemy was sighted

When a player fires a missile, the point of impact becomes a fakeplayer so
that monsters that see the impact will respond as if they had seen the
player.

To avoid spending too much time, only a single client (or fakeclient) is
checked each frame.  This means multi player games will have slightly
slower noticing monsters.
============
*/
float FindTarget(edict_t* self)
{
	edict_t* client;

	// if the first spawnflag bit is set, the monster will only wake up on
	// really seeing the player, not another monster getting angry

	// spawnflags & 3 is a big hack, because zombie crucified used the first
	// spawn flag prior to the ambush flag, and I forgot about it, so the second
	// spawn flag works as well
	if (sight_entity_time >= pr_global_struct->time - 0.1 && !(self->v.spawnflags & 3))
	{
		client = sight_entity;
		if (client->v.enemy == self->v.enemy)
			return false;
	}
	else
	{
		client = PF_checkclient(self);
		if (!client)
			return FALSE;	// current check entity isn't in PVS
	}

	if (client == self->v.enemy)
		return FALSE;

	if (client->v.flags & FL_NOTARGET)
		return FALSE;
	if (client->v.items & IT_INVISIBILITY)
		return FALSE;

	const float r = range(self, client);
	if (r == RANGE_FAR)
		return FALSE;

	if (!visible(self, client))
		return FALSE;

	if (r == RANGE_NEAR)
	{
		if (client->v.show_hostile < pr_global_struct->time && !infront(self, client))
			return FALSE;
	}
	else if (r == RANGE_MID)
	{
		if ( /* client.show_hostile < pr_global_struct->time || */ !infront(self, client))
			return FALSE;
	}

	//
	// got one
	//
	self->v.enemy = client;
	if (strcmp(self->v.enemy->v.classname, "player"))
	{
		self->v.enemy = self->v.enemy->v.enemy;
		if (!self->v.enemy || strcmp(self->v.enemy->v.classname, "player"))
		{
			self->v.enemy = pr_global_struct->world;
			return FALSE;
		}
	}

	FoundTarget(self);

	return TRUE;
}


//=============================================================================

void ai_forward(edict_t* self, float dist)
{
	PF_walkmove(self, self->v.angles[1], dist);
}

void ai_back(edict_t* self, float dist)
{
	PF_walkmove(self, (self->v.angles[1] + 180), dist);
}


/*
=============
ai_pain

stagger back a bit
=============
*/
void ai_pain(edict_t* self, float dist)
{
	ai_back(self, dist);
	/*
		local float	away;

		away = game_anglemod (vectoyaw (self->v.origin - self->v.enemy->v.origin)
		+ 180*(random()- 0.5) );

		walkmove (away, dist);
	*/
}

/*
=============
ai_painforward

stagger back a bit
=============
*/
void ai_painforward(edict_t* self, float dist)
{
	PF_walkmove(self, self->v.ideal_yaw, dist);
}

/*
=============
ai_walk

The monster is walking it's beat
=============
*/
void ai_walk(edict_t* self, float dist)
{
	if (!strcmp(self->v.classname, "monster_dragon"))
	{
		SV_MoveToGoal(self, dist);
		return;
	}
	// check for noticing a player
	if (FindTarget(self))
		return;

	SV_MoveToGoal(self, dist);
}


/*
=============
ai_stand

The monster is staying in one place for a while, with slight angle turns
=============
*/
void ai_stand(edict_t* self)
{
	if (FindTarget(self))
		return;

	if (pr_global_struct->time > self->v.pausetime)
	{
		self->v.th_walk(self);
		return;
	}

	// change angle slightly

}

/*
=============
ai_turn

don't move, but turn towards ideal_yaw
=============
*/
void ai_turn(edict_t* self)
{
	if (FindTarget(self))
		return;

	PF_changeyaw(self);
}

//=============================================================================

/*
=============
ChooseTurn
=============
*/
void ChooseTurn(edict_t* self, vec3_t dest3)
{
	auto dir = AsVector(self->v.origin) - AsVector(dest3);

	vec3_t newdir;

	newdir[0] = pr_global_struct->trace_plane_normal[1];
	newdir[1] = 0 - pr_global_struct->trace_plane_normal[0];
	newdir[2] = 0;

	if (DotProduct(dir, newdir) > 0)
	{
		dir[0] = 0 - pr_global_struct->trace_plane_normal[1];
		dir[1] = pr_global_struct->trace_plane_normal[0];
	}
	else
	{
		dir[0] = pr_global_struct->trace_plane_normal[1];
		dir[1] = 0 - pr_global_struct->trace_plane_normal[0];
	}

	dir[2] = 0;
	self->v.ideal_yaw = PF_vectoyaw(dir);
}

/*
============
FacingIdeal

============
*/
float FacingIdeal(edict_t* self)
{
	float delta = game_anglemod(self->v.angles[1] - self->v.ideal_yaw);
	if (delta > 45 && delta < 315)
		return false;
	return true;
}


//=============================================================================

bool	WizardCheckAttack(edict_t* self);
bool	DogCheckAttack(edict_t* self);

bool CheckAnyAttack(edict_t* self)
{
	if (!enemy_vis)
		return false;
	if (!strcmp(self->v.classname, "monster_army"))
		return SoldierCheckAttack(self);
	if (!strcmp(self->v.classname, "monster_ogre"))
		return OgreCheckAttack(self);
	if (!strcmp(self->v.classname, "monster_shambler"))
		return ShamCheckAttack(self);
	if (!strcmp(self->v.classname, "monster_demon1"))
		return DemonCheckAttack(self);
	if (!strcmp(self->v.classname, "monster_dog"))
		return DogCheckAttack(self);
	if (!strcmp(self->v.classname, "monster_wizard"))
		return WizardCheckAttack(self);
	return CheckAttack(self);
}


/*
=============
ai_run_melee

Turn and close until within an angle to launch a melee attack
=============
*/
void ai_run_melee(edict_t* self)
{
	self->v.ideal_yaw = enemy_yaw;
	PF_changeyaw(self);

	if (FacingIdeal(self))
	{
		self->v.th_melee(self);
		self->v.attack_state = AS_STRAIGHT;
	}
}


/*
=============
ai_run_missile

Turn in place until within an angle to launch a missile attack
=============
*/
void ai_run_missile(edict_t* self)
{
	self->v.ideal_yaw = enemy_yaw;
	PF_changeyaw(self);
	if (FacingIdeal(self))
	{
		self->v.th_missile(self);
		self->v.attack_state = AS_STRAIGHT;
	}
}


/*
=============
ai_run_slide

Strafe sideways, but stay at aproximately the same range
=============
*/
void ai_run_slide(edict_t* self, float dist)
{
	float	ofs;

	self->v.ideal_yaw = enemy_yaw;
	PF_changeyaw(self);
	if (self->v.lefty)
		ofs = 90;
	else
		ofs = -90;

	if (PF_walkmove(self, self->v.ideal_yaw + ofs, dist))
		return;

	self->v.lefty = 1 - self->v.lefty;

	PF_walkmove(self, self->v.ideal_yaw - ofs, dist);
}


/*
=============
ai_run

The monster has an enemy it is trying to kill
=============
*/
void ai_run(edict_t* self, float dist)
{
	// see if the enemy is dead
	if (self->v.enemy->v.health <= 0)
	{
		self->v.enemy = pr_global_struct->world;
		// FIXME: look all around for other targets
		if (self->v.oldenemy->v.health > 0)
		{
			self->v.enemy = self->v.oldenemy;
			HuntTarget(self);
		}
		else
		{
			if (self->v.movetarget)
				self->v.th_walk(self);
			else
				self->v.th_stand(self);
			return;
		}
	}

	self->v.show_hostile = pr_global_struct->time + 1;		// wake up other monsters

// check knowledge of enemy
	enemy_vis = visible(self, self->v.enemy);
	if (enemy_vis)
		self->v.search_time = pr_global_struct->time + 5;

	// look for other coop players
	if (pr_global_struct->coop && self->v.search_time < pr_global_struct->time)
	{
		if (FindTarget(self))
			return;
	}

	enemy_infront = infront(self, self->v.enemy);
	enemy_range = range(self, self->v.enemy);
	enemy_yaw = PF_vectoyaw(AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin));

	if (self->v.attack_state == AS_MISSILE)
	{
		//dprint ("ai_run_missile\n");
		ai_run_missile(self);
		return;
	}
	if (self->v.attack_state == AS_MELEE)
	{
		//dprint ("ai_run_melee\n");
		ai_run_melee(self);
		return;
	}

	if (CheckAnyAttack(self))
		return;					// beginning an attack

	if (self->v.attack_state == AS_SLIDING)
	{
		ai_run_slide(self, dist);
		return;
	}

	// head straight in
	SV_MoveToGoal(self, dist);		// done in C code...
}
