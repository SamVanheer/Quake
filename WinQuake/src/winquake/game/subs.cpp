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
#include "Game.h"
#include "ai.h"
#include "subs.h"

void SUB_NullThink(edict_t*)
{
}

LINK_FUNCTION_TO_NAME(SUB_NullThink);

void SUB_NullTouch(edict_t*, edict_t*)
{
}

LINK_FUNCTION_TO_NAME(SUB_NullTouch);

void SUB_NullUse(edict_t*, edict_t*)
{
}

LINK_FUNCTION_TO_NAME(SUB_NullUse);

void SUB_NullPain(edict_t*, edict_t*, float)
{
}

LINK_FUNCTION_TO_NAME(SUB_NullPain);

void SUB_Remove(edict_t* self)
{
	PF_Remove(self);
}

LINK_FUNCTION_TO_NAME(SUB_Remove);

void SUB_TouchRemove(edict_t* self, edict_t* other)
{
	SUB_Remove(self);
}

LINK_FUNCTION_TO_NAME(SUB_TouchRemove);

vec3_t Up{0, -1, 0};
vec3_t Down{0, -2, 0};

/*
QuakeEd only writes a single float for angles (bad idea), so up and down are
just constant angles.
*/
void SetMovedir(edict_t* self)
{
	if (VectorCompare(self->v.angles, Up))
	{
		AsVector(self->v.movedir) = Vector3D{0, 0, 1};
	}
	else if (VectorCompare(self->v.angles, Down))
	{
		AsVector(self->v.movedir) = Vector3D{0, 0, -1};
	}
	else
	{
		PF_makevectors(self->v.angles);
		VectorCopy(pr_global_struct->v_forward, self->v.movedir);
	}

	VectorCopy(vec3_origin, self->v.angles);
}

/*
================
InitTrigger
================
*/
void InitTrigger(edict_t* self)
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (!VectorCompare(self->v.angles, vec3_origin))
		SetMovedir(self);
	self->v.solid = SOLID_TRIGGER;
	PF_setmodel(self, self->v.model);	// set size and link into world
	self->v.movetype = MOVETYPE_NONE;
	self->v.modelindex = 0;
	self->v.model = "";
}

/*
=============
SUB_CalcMove

calculate self.velocity and self.nextthink to reach dest from
self.origin traveling at speed
===============
*/
void SUB_CalcMoveEnt(edict_t* self, edict_t* ent, vec3_t tdest, float tspeed, void (*func)(edict_t*))
{
	SUB_CalcMove(ent, tdest, tspeed, func);
}

void SUB_CalcMove(edict_t* self, vec3_t tdest, float tspeed, void (*func)(edict_t*))
{
	if (!tspeed)
		PF_objerror("No speed is defined!");

	self->v.think1 = func;
	VectorCopy(tdest, self->v.finaldest);
	self->v.think = &SUB_CalcMoveDone;

	if (VectorCompare(tdest, self->v.origin))
	{
		VectorCopy(vec3_origin, self->v.velocity);
		self->v.nextthink = self->v.ltime + 0.1;
		return;
	}

	vec3_t vdestdelta;
	// set destdelta to the vector needed to move
	VectorSubtract(tdest, self->v.origin, vdestdelta);

	// calculate length of vector
	const float len = PF_vlen(vdestdelta);

	// divide by speed to get time to reach dest
	const float traveltime = len / tspeed;

	if (traveltime < 0.1)
	{
		VectorCopy(vec3_origin, self->v.velocity);
		self->v.nextthink = self->v.ltime + 0.1;
		return;
	}

	// set nextthink to trigger a think when dest is reached
	self->v.nextthink = self->v.ltime + traveltime;

	// scale the destdelta vector by the time spent traveling to get velocity
	VectorScale(vdestdelta, (1 / traveltime), self->v.velocity); // qcc won't take vec/float	
}

/*
============
After moving, set origin to exact final destination
============
*/
void SUB_CalcMoveDone(edict_t* self)
{
	PF_setorigin(self, self->v.finaldest);
	VectorCopy(vec3_origin, self->v.velocity);
	self->v.nextthink = -1;
	if (self->v.think1)
		self->v.think1(self);
}

LINK_FUNCTION_TO_NAME(SUB_CalcMoveDone);

/*
=============
SUB_CalcAngleMove

calculate self.avelocity and self.nextthink to reach destangle from
self.angles rotating

The calling function should make sure self.think is valid
===============
*/
void SUB_CalcAngleMoveEnt(edict_t* self, edict_t* ent, vec3_t destangle, float tspeed, void (*func)(edict_t*))
{
	SUB_CalcAngleMove(ent, destangle, tspeed, func);
}

void SUB_CalcAngleMove(edict_t* self, vec3_t destangle, float tspeed, void (*func)(edict_t*))
{
	if (!tspeed)
		PF_objerror("No speed is defined!");

	vec3_t destdelta;
	// set destdelta to the vector needed to move
	VectorSubtract(destangle, self->v.angles, destdelta);

	// calculate length of vector
	const float len = PF_vlen(destdelta);

	// divide by speed to get time to reach dest
	const float traveltime = len / tspeed;

	// set nextthink to trigger a think when dest is reached
	self->v.nextthink = self->v.ltime + traveltime;

	// scale the destdelta vector by the time spent traveling to get velocity
	VectorScale(destdelta, (1 / traveltime), self->v.avelocity);

	self->v.think1 = func;
	VectorCopy(destangle, self->v.finalangle);
	self->v.think = SUB_CalcAngleMoveDone;
}

/*
============
After rotating, set angle to exact final angle
============
*/
void SUB_CalcAngleMoveDone(edict_t* self)
{
	VectorCopy(self->v.finalangle, self->v.angles);
	VectorCopy(vec3_origin, self->v.avelocity);
	self->v.nextthink = -1;
	if (self->v.think1)
		self->v.think1(self);
}

LINK_FUNCTION_TO_NAME(SUB_CalcAngleMoveDone);

//=============================================================================

void DelayThink(edict_t* self)
{
	pr_global_struct->activator = self->v.enemy;
	SUB_UseTargets(self);
	PF_Remove(self);
}

LINK_FUNCTION_TO_NAME(DelayThink);

/*
==============================
SUB_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Removes all entities with a targetname that match self.killtarget,
and removes them, so some events can remove other triggers.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void SUB_UseTargets(edict_t* self, edict_t* other)
{
//
// check for a delay
//
	if (self->v.delay)
	{
	// create a temp object to fire at a later time
		auto t = PF_Spawn();
		t->v.classname = "DelayedUse";
		t->v.nextthink = pr_global_struct->time + self->v.delay;
		t->v.think = DelayThink;
		t->v.enemy = pr_global_struct->activator;
		t->v.message = self->v.message;
		t->v.killtarget = self->v.killtarget;
		t->v.target = self->v.target;
		return;
	}


	//
	// print the message
	//
	if (!strcmp(pr_global_struct->activator->v.classname, "player") && (self->v.message && strcmp(self->v.message, "")))
	{
		PF_centerprint(pr_global_struct->activator, self->v.message);
		if (!self->v.noise)
			PF_sound(pr_global_struct->activator, CHAN_VOICE, "misc/talk.wav", 1, ATTN_NORM);
	}

	//
	// kill the killtagets
	//
	if (self->v.killtarget)
	{
		auto t = pr_global_struct->world;
		do
		{
			t = PF_Find(t, "targetname", self->v.killtarget);
			if (t == pr_global_struct->world)
				return;
			PF_Remove(t);
		} while (1);
	}

	//
	// fire targets
	//
	if (self->v.target)
	{
		auto act = pr_global_struct->activator;
		auto t = pr_global_struct->world;
		do
		{
			t = PF_Find(t, "targetname", self->v.target);
			if (t == pr_global_struct->world)
			{
				return;
			}
			if (t->v.use != &SUB_NullUse)
			{
				if (t->v.use)
					t->v.use(t, self);
			}
			pr_global_struct->activator = act;
		} while (1);
	}


}

LINK_FUNCTION_TO_NAME(SUB_UseTargets);

/*

in nightmare mode, all attack_finished times become 0
some monsters refire twice automatically

*/

void SUB_AttackFinished(edict_t* self, float normal)
{
	self->v.cnt = 0;		// refire count for nightmare
	if (pr_global_struct->skill != 3)
		self->v.attack_finished = pr_global_struct->time + normal;
}

void SUB_CheckRefire(edict_t* self, void (*thinkst)(edict_t*))
{
	if (pr_global_struct->skill != 3)
		return;
	if (self->v.cnt == 1)
		return;
	if (!visible(self, self->v.enemy))
		return;
	self->v.cnt = 1;
	self->v.think = thinkst;
}
