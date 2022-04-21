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
#include "Game.h"
#include "plats.h"
#include "subs.h"

void plat_center_touch(edict_t* self, edict_t* other);
void plat_outside_touch(edict_t* self, edict_t* other);
void plat_go_down(edict_t* self);

constexpr int PLAT_LOW_TRIGGER = 1;

void plat_spawn_inside_trigger(edict_t* self)
{
//
// middle trigger
//	
	auto trigger = PF_Spawn();
	trigger->v.touch = plat_center_touch;
	trigger->v.movetype = MOVETYPE_NONE;
	trigger->v.solid = SOLID_TRIGGER;
	trigger->v.enemy = self;
	
	auto tmin = AsVector(self->v.mins) + Vector3D{25, 25, 0};
	auto tmax = AsVector(self->v.maxs) - Vector3D{25, 25, -8};
	tmin[2] = tmax[2] - (self->v.pos1[2] - self->v.pos2[2] + 8);
	if (self->v.spawnflags & PLAT_LOW_TRIGGER)
		tmax[2] = tmin[2] + 8;
	
	if (self->v.size[0] <= 50)
	{
		tmin[0] = (self->v.mins[0] + self->v.maxs[0]) / 2;
		tmax[0] = tmin[0] + 1;
	}
	if (self->v.size[1] <= 50)
	{
		tmin[1] = (self->v.mins[1] + self->v.maxs[1]) / 2;
		tmax[1] = tmin[1] + 1;
	}
	
	PF_setsize (trigger, tmin, tmax);
}

void plat_hit_top(edict_t* self)
{
	PF_sound (self, CHAN_VOICE, self->v.noise1, 1, ATTN_NORM);
	self->v.state = STATE_TOP;
	self->v.think = plat_go_down;
	self->v.nextthink = self->v.ltime + 3;
}

LINK_FUNCTION_TO_NAME(plat_hit_top);

void plat_hit_bottom(edict_t* self)
{
	PF_sound (self, CHAN_VOICE, self->v.noise1, 1, ATTN_NORM);
	self->v.state = STATE_BOTTOM;
}

LINK_FUNCTION_TO_NAME(plat_hit_bottom);

void plat_go_down(edict_t* self)
{
	PF_sound (self, CHAN_VOICE, self->v.noise, 1, ATTN_NORM);
	self->v.state = STATE_DOWN;
	SUB_CalcMove (self, self->v.pos2, self->v.speed, plat_hit_bottom);
}

LINK_FUNCTION_TO_NAME(plat_go_down);

void plat_go_up(edict_t* self)
{
	PF_sound (self, CHAN_VOICE, self->v.noise, 1, ATTN_NORM);
	self->v.state = STATE_UP;
	SUB_CalcMove (self, self->v.pos1, self->v.speed, plat_hit_top);
}

void plat_center_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
		
	if (other->v.health <= 0)
		return;

	self = self->v.enemy;
	if (self->v.state == STATE_BOTTOM)
		plat_go_up (self);
	else if (self->v.state == STATE_TOP)
		self->v.nextthink = self->v.ltime + 1;	// delay going down
}

LINK_FUNCTION_TO_NAME(plat_center_touch);

void plat_outside_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;

	if (other->v.health <= 0)
		return;
		
//dprint ("plat_outside_touch\n");
	self = self->v.enemy;
	if (self->v.state == STATE_TOP)
		plat_go_down (self);
}

LINK_FUNCTION_TO_NAME(plat_outside_touch);

void plat_trigger_use(edict_t* self, edict_t* other)
{
	if (self->v.think)
		return;		// allready activated
	plat_go_down(self);
}

LINK_FUNCTION_TO_NAME(plat_trigger_use);

void plat_crush(edict_t* self, edict_t* other)
{
//dprint ("plat_crush\n");

	T_Damage (self, other, self, self, 1);
	
	if (self->v.state == STATE_UP)
		plat_go_down (self);
	else if (self->v.state == STATE_DOWN)
		plat_go_up (self);
	else
		PF_objerror ("plat_crush: bad self->v.state\n");
}

LINK_FUNCTION_TO_NAME(plat_crush);

void plat_use(edict_t* self, edict_t* other)
{
	self->v.use = SUB_NullUse;
	if (self->v.state != STATE_UP)
		PF_objerror ("plat_use: not in up state");
	plat_go_down(self);
}

LINK_FUNCTION_TO_NAME(plat_use);

/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
speed	default 150

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in the extended position until it is trigger, when it will lower and become a normal plat.

If the "height" key is set, that will determine the amount the plat moves, instead of being implicitly determined by the model's height.
Set "sounds" to one of the following:
1) base fast
2) chain slow
*/


void func_plat(edict_t* self)

{
	if (!self->v.t_length)
		self->v.t_length = 80;
	if (!self->v.t_width)
		self->v.t_width = 10;

	if (self->v.sounds == 0)
		self->v.sounds = 2;
// FIX THIS TO LOAD A GENERIC PLAT SOUND

	if (self->v.sounds == 1)
	{
		PF_precache_sound ("plats/plat1.wav");
		PF_precache_sound ("plats/plat2.wav");
		self->v.noise = "plats/plat1.wav";
		self->v.noise1 = "plats/plat2.wav";
	}

	if (self->v.sounds == 2)
	{
		PF_precache_sound ("plats/medplat1.wav");
		PF_precache_sound ("plats/medplat2.wav");
		self->v.noise = "plats/medplat1.wav";
		self->v.noise1 = "plats/medplat2.wav";
	}


	AsVector(self->v.mangle) = AsVector(self->v.angles);
	AsVector(self->v.angles) = AsVector(vec3_origin);

	self->v.classname = "plat";
	self->v.solid = SOLID_BSP;
	self->v.movetype = MOVETYPE_PUSH;
	PF_setorigin (self, self->v.origin);	
	PF_setmodel (self, self->v.model);
	PF_setsize (self, self->v.mins , self->v.maxs);

	self->v.blocked = plat_crush;
	if (!self->v.speed)
		self->v.speed = 150;

// pos1 is the top position, pos2 is the bottom
	AsVector(self->v.pos1) = AsVector(self->v.origin);
	AsVector(self->v.pos2) = AsVector(self->v.origin);
	if (self->v.height)
		self->v.pos2[2] = self->v.origin[2] - self->v.height;
	else
		self->v.pos2[2] = self->v.origin[2] - self->v.size[2] + 8;

	self->v.use = plat_trigger_use;

	plat_spawn_inside_trigger (self);	// the "start moving" trigger	

	if (self->v.targetname)
	{
		self->v.state = STATE_UP;
		self->v.use = plat_use;
	}
	else
	{
		PF_setorigin (self, self->v.pos2);
		self->v.state = STATE_BOTTOM;
	}
}

LINK_ENTITY_TO_CLASS(func_plat);

//============================================================================

void train_next(edict_t* self);
void func_train_find(edict_t* self);

void train_blocked(edict_t* self, edict_t* other)
{
	if (pr_global_struct->time < self->v.attack_finished)
		return;
	self->v.attack_finished = pr_global_struct->time + 0.5;
	T_Damage (self, other, self, self, self->v.dmg);
}

LINK_FUNCTION_TO_NAME(train_blocked);

void train_use(edict_t* self, edict_t* other)
{
	if (self->v.think != func_train_find)
		return;		// already activated
	train_next(self);
}

LINK_FUNCTION_TO_NAME(train_use);

void train_wait(edict_t* self)
{
	if (self->v.wait)
	{
		self->v.nextthink = self->v.ltime + self->v.wait;
		PF_sound (self, CHAN_VOICE, self->v.noise, 1, ATTN_NORM);
	}
	else
		self->v.nextthink = self->v.ltime + 0.1;
	
	self->v.think = train_next;
}

LINK_FUNCTION_TO_NAME(train_wait);

void train_next(edict_t* self)
{
	auto targ = PF_Find (pr_global_struct->world, "targetname", self->v.target);
	self->v.target = targ->v.target;
	if (!self->v.target)
		PF_objerror ("train_next: no next target");
	if (targ->v.wait)
		self->v.wait = targ->v.wait;
	else
		self->v.wait = 0;
	PF_sound (self, CHAN_VOICE, self->v.noise1, 1, ATTN_NORM);
	SUB_CalcMove (self, AsVector(targ->v.origin) - AsVector(self->v.mins), self->v.speed, train_wait);
}

LINK_FUNCTION_TO_NAME(train_next);

void func_train_find(edict_t* self)

{
	auto targ = PF_Find(pr_global_struct->world, "targetname", self->v.target);
	self->v.target = targ->v.target;
	PF_setorigin (self, AsVector(targ->v.origin) - AsVector(self->v.mins));
	if (!self->v.targetname)
	{	// not triggered, so start immediately
		self->v.nextthink = self->v.ltime + 0.1;
		self->v.think = train_next;
	}
}

LINK_FUNCTION_TO_NAME(func_train_find);

/*QUAKED func_train (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
sounds
1) ratchet metal

*/
void func_train(edict_t* self)
{	
	if (!self->v.speed)
		self->v.speed = 100;
	if (!self->v.target)
		PF_objerror ("func_train without a target");
	if (!self->v.dmg)
		self->v.dmg = 2;

	if (self->v.sounds == 0)
	{
		self->v.noise = ("misc/null.wav");
		PF_precache_sound ("misc/null.wav");
		self->v.noise1 = ("misc/null.wav");
		PF_precache_sound ("misc/null.wav");
	}

	if (self->v.sounds == 1)
	{
		self->v.noise = ("plats/train2.wav");
		PF_precache_sound ("plats/train2.wav");
		self->v.noise1 = ("plats/train1.wav");
		PF_precache_sound ("plats/train1.wav");
	}

	self->v.cnt = 1;
	self->v.solid = SOLID_BSP;
	self->v.movetype = MOVETYPE_PUSH;
	self->v.blocked = train_blocked;
	self->v.use = train_use;
	self->v.classname = "train";

	PF_setmodel (self, self->v.model);
	PF_setsize (self, self->v.mins , self->v.maxs);
	PF_setorigin (self, self->v.origin);

// start trains on the second frame, to make sure their targets have had
// a chance to spawn
	self->v.nextthink = self->v.ltime + 0.1;
	self->v.think = func_train_find;
}

LINK_ENTITY_TO_CLASS(func_train);

/*QUAKED misc_teleporttrain (0 .5 .8) (-8 -8 -8) (8 8 8)
This is used for the final bos
*/
void misc_teleporttrain(edict_t* self)
{	
	if (!self->v.speed)
		self->v.speed = 100;
	if (!self->v.target)
		PF_objerror ("func_train without a target");

	self->v.cnt = 1;
	self->v.solid = SOLID_NOT;
	self->v.movetype = MOVETYPE_PUSH;
	self->v.blocked = train_blocked;
	self->v.use = train_use;
	AsVector(self->v.avelocity) = Vector3D{100, 200, 300};

	self->v.noise = ("misc/null.wav");
	PF_precache_sound ("misc/null.wav");
	self->v.noise1 = ("misc/null.wav");
	PF_precache_sound ("misc/null.wav");

	PF_precache_model ("progs/teleport.mdl");
	PF_setmodel (self, "progs/teleport.mdl");
	PF_setsize (self, self->v.mins , self->v.maxs);
	PF_setorigin (self, self->v.origin);

// start trains on the second frame, to make sure their targets have had
// a chance to spawn
	self->v.nextthink = self->v.ltime + 0.1;
	self->v.think = func_train_find;
}

LINK_ENTITY_TO_CLASS(misc_teleporttrain);
