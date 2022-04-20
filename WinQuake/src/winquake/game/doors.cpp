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
#include "doors.h"
#include "Game.h"
#include "subs.h"

constexpr int DOOR_START_OPEN = 1;
constexpr int DOOR_DONT_LINK = 4;
constexpr int DOOR_GOLD_KEY = 8;
constexpr int DOOR_SILVER_KEY = 16;
constexpr int DOOR_TOGGLE = 32;

/*

Doors are similar to buttons, but can spawn a fat trigger field around them
to open without a touch, and they link together to form simultanious
double/quad doors.

Door.owner is the master door.  If there is only one door, it points to itself.
If multiple doors, all will point to a single one.

Door.enemy chains from the master door through all doors linked in the chain.

*/

/*
=============================================================================

THINK FUNCTIONS

=============================================================================
*/

void door_go_down(edict_t* self);
void door_go_up(edict_t* self);

void door_blocked(edict_t* self, edict_t* other)
{
	T_Damage(self, other, self, self, self->v.dmg);

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if (self->v.wait >= 0)
	{
		if (self->v.state == STATE_DOWN)
			door_go_up(self);
		else
			door_go_down(self);
	}
}


void door_hit_top(edict_t* self)
{
	PF_sound(self, CHAN_VOICE, self->v.noise1, 1, ATTN_NORM);
	self->v.state = STATE_TOP;
	if (self->v.spawnflags & DOOR_TOGGLE)
		return;		// don't come down automatically
	self->v.think = door_go_down;
	self->v.nextthink = self->v.ltime + self->v.wait;
}

void door_hit_bottom(edict_t* self)
{
	PF_sound(self, CHAN_VOICE, self->v.noise1, 1, ATTN_NORM);
	self->v.state = STATE_BOTTOM;
}

void door_go_down(edict_t* self)
{
	PF_sound(self, CHAN_VOICE, self->v.noise2, 1, ATTN_NORM);
	if (self->v.max_health)
	{
		self->v.takedamage = DAMAGE_YES;
		self->v.health = self->v.max_health;
	}

	self->v.state = STATE_DOWN;
	SUB_CalcMove(self, self->v.pos1, self->v.speed, door_hit_bottom);
}

void door_go_up(edict_t* self)
{
	if (self->v.state == STATE_UP)
		return;		// allready going up

	if (self->v.state == STATE_TOP)
	{	// reset top wait time
		self->v.nextthink = self->v.ltime + self->v.wait;
		return;
	}

	PF_sound(self, CHAN_VOICE, self->v.noise2, 1, ATTN_NORM);
	self->v.state = STATE_UP;
	SUB_CalcMove(self, self->v.pos2, self->v.speed, door_hit_top);

	SUB_UseTargets(self);
}


/*
=============================================================================

ACTIVATION FUNCTIONS

=============================================================================
*/

void door_fire(edict_t* self)
{
	if (self->v.owner != self)
		PF_objerror("door_fire: self->v.owner != self");

	// play use key sound

	if (self->v.items)
		PF_sound(self, CHAN_VOICE, self->v.noise4, 1, ATTN_NORM);

	self->v.message = nullptr;		// no more message

	if (self->v.spawnflags & DOOR_TOGGLE)
	{
		if (self->v.state == STATE_UP || self->v.state == STATE_TOP)
		{
			auto starte = self;
			auto next = self;
			do
			{
				door_go_down(next);
				next = next->v.enemy;
			} while ((next != starte) && (next != pr_global_struct->world));
			return;
		}
	}

	// trigger all paired doors
	auto starte = self;
	auto next = self;
	do
	{
		door_go_up(next);
		next = next->v.enemy;
	} while ((next != starte) && (next != pr_global_struct->world));
}


void door_use(edict_t* self, edict_t* other = nullptr)
{
	self->v.message = "";			// door message are for touch only
	self->v.owner->v.message = "";
	self->v.enemy->v.message = "";
	door_fire(self->v.owner);
}


void door_trigger_touch(edict_t* self, edict_t* other)
{
	if (other->v.health <= 0)
		return;

	if (pr_global_struct->time < self->v.attack_finished)
		return;
	self->v.attack_finished = pr_global_struct->time + 1;

	activator = other;

	auto owner = self->v.owner;
	door_use(owner);
}


void door_killed(edict_t* self)
{
	auto owner = self->v.owner;
	owner->v.health = owner->v.max_health;
	owner->v.takedamage = DAMAGE_NO;	// wil be reset upon return
	door_use(owner);
}


/*
================
door_touch

Prints messages and opens key doors
================
*/
void door_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	if (self->v.owner->v.attack_finished > pr_global_struct->time)
		return;

	self->v.owner->v.attack_finished = pr_global_struct->time + 2;

	if (self->v.owner->v.message && strcmp(self->v.owner->v.message, ""))
	{
		PF_centerprint(other, self->v.owner->v.message);
		PF_sound(other, CHAN_VOICE, "misc/talk.wav", 1, ATTN_NORM);
	}

	// key door stuff
	if (!self->v.items)
		return;

	// FIXME: blink key on player's status bar
	if ((self->v.items & other->v.items) != self->v.items)
	{
		if (self->v.owner->v.items == IT_KEY1)
		{
			if (pr_global_struct->world->v.worldtype == 2)
			{
				PF_centerprint(other, "You need the silver keycard");
				PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
			}
			else if (pr_global_struct->world->v.worldtype == 1)
			{
				PF_centerprint(other, "You need the silver runekey");
				PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
			}
			else if (pr_global_struct->world->v.worldtype == 0)
			{
				PF_centerprint(other, "You need the silver key");
				PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
			}
		}
		else
		{
			if (pr_global_struct->world->v.worldtype == 2)
			{
				PF_centerprint(other, "You need the gold keycard");
				PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
			}
			else if (pr_global_struct->world->v.worldtype == 1)
			{
				PF_centerprint(other, "You need the gold runekey");
				PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
			}
			else if (pr_global_struct->world->v.worldtype == 0)
			{
				PF_centerprint(other, "You need the gold key");
				PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
			}
		}
		return;
	}

	other->v.items &= ~self->v.items;
	self->v.touch = SUB_NullTouch;
	if (self->v.enemy)
		self->v.enemy->v.touch = SUB_NullTouch;	// get paired door
	door_use(self);
}

/*
=============================================================================

SPAWNING FUNCTIONS

=============================================================================
*/


edict_t* spawn_field(edict_t* self, vec3_t fmins, vec3_t fmaxs)
{
	auto trigger = PF_Spawn();
	trigger->v.movetype = MOVETYPE_NONE;
	trigger->v.solid = SOLID_TRIGGER;
	trigger->v.owner = self;
	trigger->v.touch = door_trigger_touch;

	PF_setsize(trigger, AsVector(fmins) - Vector3D{60, 60, 8}, AsVector(fmaxs) + Vector3D{60, 60, 8});
	return trigger;
}


bool EntitiesTouching(edict_t* e1, edict_t* e2)
{
	if (e1->v.mins[0] > e2->v.maxs[0])
		return false;
	if (e1->v.mins[1] > e2->v.maxs[1])
		return false;
	if (e1->v.mins[2] > e2->v.maxs[2])
		return false;
	if (e1->v.maxs[0] < e2->v.mins[0])
		return false;
	if (e1->v.maxs[1] < e2->v.mins[1])
		return false;
	if (e1->v.maxs[2] < e2->v.mins[2])
		return false;
	return true;
}


/*
=============
LinkDoors


=============
*/
void LinkDoors(edict_t* self)
{
	if (self->v.enemy)
		return;		// already linked by another door
	if (self->v.spawnflags & 4)
	{
		self->v.owner = self->v.enemy = self;
		return;		// don't want to link this door
	}

	Vector3D cmins = AsVector(self->v.mins);
	Vector3D cmaxs = AsVector(self->v.maxs);

	auto starte = self;
	auto current = self;
	auto t = self;

	do
	{
		current->v.owner = starte;			// master door

		if (current->v.health)
			starte->v.health = current->v.health;
		if (current->v.targetname)
			starte->v.targetname = current->v.targetname;
		if (current->v.message && strcmp(current->v.message, ""))
			starte->v.message = current->v.message;

		t = PF_Find(t, "classname", current->v.classname);
		if (t == pr_global_struct->world)
		{
			current->v.enemy = starte;		// make the chain a loop

		// shootable, fired, or key doors just needed the owner/enemy links,
		// they don't spawn a field

			current = current->v.owner;

			if (current->v.health)
				return;
			if (current->v.targetname)
				return;
			if (current->v.items)
				return;

			current->v.owner->v.trigger_field = spawn_field(current, cmins, cmaxs);

			return;
		}

		if (EntitiesTouching(current, t))
		{
			if (t->v.enemy)
				PF_objerror("cross connected doors");

			current->v.enemy = t;
			current = t;

			if (t->v.mins[0] < cmins[0])
				cmins[0] = t->v.mins[0];
			if (t->v.mins[1] < cmins[1])
				cmins[1] = t->v.mins[1];
			if (t->v.mins[2] < cmins[2])
				cmins[2] = t->v.mins[2];
			if (t->v.maxs[0] > cmaxs[0])
				cmaxs[0] = t->v.maxs[0];
			if (t->v.maxs[1] > cmaxs[1])
				cmaxs[1] = t->v.maxs[1];
			if (t->v.maxs[2] > cmaxs[2])
				cmaxs[2] = t->v.maxs[2];
		}
	} while (1);

}


/*QUAKED func_door (0 .5 .8) ? START_OPEN x DOOR_DONT_LINK GOLD_KEY SILVER_KEY TOGGLE
if two doors touch, they are assumed to be connected and operate as a unit.

TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN causes the door to move to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not usefull for touch or takedamage doors).

Key doors are allways wait -1.

"message"	is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
0)	no PF_sound
1)	stone
2)	base
3)	stone chain
4)	screechy metal
*/

void func_door(edict_t* self)

{

	if (pr_global_struct->world->v.worldtype == 0)
	{
		PF_precache_sound("doors/medtry.wav");
		PF_precache_sound("doors/meduse.wav");
		self->v.noise3 = "doors/medtry.wav";
		self->v.noise4 = "doors/meduse.wav";
	}
	else if (pr_global_struct->world->v.worldtype == 1)
	{
		PF_precache_sound("doors/runetry.wav");
		PF_precache_sound("doors/runeuse.wav");
		self->v.noise3 = "doors/runetry.wav";
		self->v.noise4 = "doors/runeuse.wav";
	}
	else if (pr_global_struct->world->v.worldtype == 2)
	{
		PF_precache_sound("doors/basetry.wav");
		PF_precache_sound("doors/baseuse.wav");
		self->v.noise3 = "doors/basetry.wav";
		self->v.noise4 = "doors/baseuse.wav";
	}
	else
	{
		dprint("no worldtype set!\n");
	}
	if (self->v.sounds == 0)
	{
		PF_precache_sound("misc/null.wav");
		PF_precache_sound("misc/null.wav");
		self->v.noise1 = "misc/null.wav";
		self->v.noise2 = "misc/null.wav";
	}
	if (self->v.sounds == 1)
	{
		PF_precache_sound("doors/drclos4.wav");
		PF_precache_sound("doors/doormv1.wav");
		self->v.noise1 = "doors/drclos4.wav";
		self->v.noise2 = "doors/doormv1.wav";
	}
	if (self->v.sounds == 2)
	{
		PF_precache_sound("doors/hydro1.wav");
		PF_precache_sound("doors/hydro2.wav");
		self->v.noise2 = "doors/hydro1.wav";
		self->v.noise1 = "doors/hydro2.wav";
	}
	if (self->v.sounds == 3)
	{
		PF_precache_sound("doors/stndr1.wav");
		PF_precache_sound("doors/stndr2.wav");
		self->v.noise2 = "doors/stndr1.wav";
		self->v.noise1 = "doors/stndr2.wav";
	}
	if (self->v.sounds == 4)
	{
		PF_precache_sound("doors/ddoor1.wav");
		PF_precache_sound("doors/ddoor2.wav");
		self->v.noise1 = "doors/ddoor2.wav";
		self->v.noise2 = "doors/ddoor1.wav";
	}


	SetMovedir(self);

	self->v.max_health = self->v.health;
	self->v.solid = SOLID_BSP;
	self->v.movetype = MOVETYPE_PUSH;
	PF_setorigin(self, self->v.origin);
	PF_setmodel(self, self->v.model);
	self->v.classname = "door";

	self->v.blocked = door_blocked;
	self->v.use = door_use;

	if (self->v.spawnflags & DOOR_SILVER_KEY)
		self->v.items = IT_KEY1;
	if (self->v.spawnflags & DOOR_GOLD_KEY)
		self->v.items = IT_KEY2;

	if (!self->v.speed)
		self->v.speed = 100;
	if (!self->v.wait)
		self->v.wait = 3;
	if (!self->v.lip)
		self->v.lip = 8;
	if (!self->v.dmg)
		self->v.dmg = 2;

	AsVector(self->v.pos1) = AsVector(self->v.origin);
	AsVector(self->v.pos2) = AsVector(self->v.pos1) + AsVector(self->v.movedir) * (fabs(DotProduct(self->v.movedir, self->v.size)) - self->v.lip);

	// DOOR_START_OPEN is to allow an entity to be lighted in the closed position
	// but spawn in the open position
	if (self->v.spawnflags & DOOR_START_OPEN)
	{
		PF_setorigin(self, self->v.pos2);
		AsVector(self->v.pos2) = AsVector(self->v.pos1);
		AsVector(self->v.pos1) = AsVector(self->v.origin);
	}

	self->v.state = STATE_BOTTOM;

	if (self->v.health)
	{
		self->v.takedamage = DAMAGE_YES;
		self->v.th_die = door_killed;
	}

	if (self->v.items)
		self->v.wait = -1;

	self->v.touch = door_touch;

	// LinkDoors can't be done until all of the doors have been spawned, so
	// the sizes can be detected properly.
	self->v.think = LinkDoors;
	self->v.nextthink = self->v.ltime + 0.1;
}

LINK_ENTITY_TO_CLASS(func_door);

/*
=============================================================================

SECRET DOORS

=============================================================================
*/

void fd_secret_move1(edict_t* self);
void fd_secret_move2(edict_t* self);
void fd_secret_move3(edict_t* self);
void fd_secret_move4(edict_t* self);
void fd_secret_move5(edict_t* self);
void fd_secret_move6(edict_t* self);
void fd_secret_done(edict_t* self);

constexpr int SECRET_OPEN_ONCE = 1;		// stays open
constexpr int SECRET_1ST_LEFT = 2;		// 1st move is left of arrow
constexpr int SECRET_1ST_DOWN = 4;		// 1st move is down from arrow
constexpr int SECRET_NO_SHOOT = 8;		// only opened by trigger
constexpr int SECRET_YES_SHOOT = 16;	// shootable even if targeted


void fd_secret_use(edict_t* self, edict_t* other)
{
	self->v.health = 10000;

	// exit if still moving around...
	if (!VectorCompare(self->v.origin, self->v.oldorigin))
		return;

	self->v.message = nullptr;		// no more message

	SUB_UseTargets(self);				// fire all targets / killtargets

	if (!(self->v.spawnflags & SECRET_NO_SHOOT))
	{
		self->v.th_pain = SUB_NullPain;
		self->v.takedamage = DAMAGE_NO;
	}
	AsVector(self->v.velocity) = AsVector(vec3_origin);

	// Make a sound, wait a little...

	PF_sound(self, CHAN_VOICE, self->v.noise1, 1, ATTN_NORM);
	self->v.nextthink = self->v.ltime + 0.1;

	const float temp = 1 - (self->v.spawnflags & SECRET_1ST_LEFT);	// 1 or -1
	PF_makevectors(self->v.mangle);

	if (!self->v.t_width)
	{
		if (self->v.spawnflags & SECRET_1ST_DOWN)
			self->v.t_width = fabs(DotProduct(pr_global_struct->v_up, self->v.size));
		else
			self->v.t_width = fabs(DotProduct(pr_global_struct->v_right, self->v.size));
	}

	if (!self->v.t_length)
		self->v.t_length = fabs(DotProduct(pr_global_struct->v_forward, self->v.size));

	if (self->v.spawnflags & SECRET_1ST_DOWN)
		AsVector(self->v.dest1) = AsVector(self->v.origin) - AsVector(pr_global_struct->v_up) * self->v.t_width;
	else
		AsVector(self->v.dest1) = AsVector(self->v.origin) + AsVector(pr_global_struct->v_right) * (self->v.t_width * temp);

	AsVector(self->v.dest2) = AsVector(self->v.dest1) + AsVector(pr_global_struct->v_forward) * self->v.t_length;
	SUB_CalcMove(self, self->v.dest1, self->v.speed, fd_secret_move1);
	PF_sound(self, CHAN_VOICE, self->v.noise2, 1, ATTN_NORM);
}

void fd_secret_pain(edict_t* self, edict_t* attacker, float damage)
{
	fd_secret_use(self, attacker);
}

// Wait after first movement...
void fd_secret_move1(edict_t* self)
{
	self->v.nextthink = self->v.ltime + 1.0;
	self->v.think = fd_secret_move2;
	PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
}

// Start moving sideways w/sound...
void fd_secret_move2(edict_t* self)
{
	PF_sound(self, CHAN_VOICE, self->v.noise2, 1, ATTN_NORM);
	SUB_CalcMove(self, self->v.dest2, self->v.speed, fd_secret_move3);
}

// Wait here until time to go back...
void fd_secret_move3(edict_t* self)
{
	PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
	if (!(self->v.spawnflags & SECRET_OPEN_ONCE))
	{
		self->v.nextthink = self->v.ltime + self->v.wait;
		self->v.think = fd_secret_move4;
	}
}

// Move backward...
void fd_secret_move4(edict_t* self)
{
	PF_sound(self, CHAN_VOICE, self->v.noise2, 1, ATTN_NORM);
	SUB_CalcMove(self, self->v.dest1, self->v.speed, fd_secret_move5);
}

// Wait 1 second...
void fd_secret_move5(edict_t* self)
{
	self->v.nextthink = self->v.ltime + 1.0;
	self->v.think = fd_secret_move6;
	PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
}

void fd_secret_move6(edict_t* self)
{
	PF_sound(self, CHAN_VOICE, self->v.noise2, 1, ATTN_NORM);
	SUB_CalcMove(self, self->v.oldorigin, self->v.speed, fd_secret_done);
}

void fd_secret_done(edict_t* self)
{
	if (!self->v.targetname || self->v.spawnflags & SECRET_YES_SHOOT)
	{
		self->v.health = 10000;
		self->v.takedamage = DAMAGE_YES;
		self->v.th_pain = fd_secret_pain;
	}
	PF_sound(self, CHAN_VOICE, self->v.noise3, 1, ATTN_NORM);
}

void secret_blocked(edict_t* self, edict_t* other)
{
	if (pr_global_struct->time < self->v.attack_finished)
		return;
	self->v.attack_finished = pr_global_struct->time + 0.5;
	T_Damage(self, other, self, self, self->v.dmg);
}

/*
================
secret_touch

Prints messages
================
*/
void secret_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	if (self->v.attack_finished > pr_global_struct->time)
		return;

	self->v.attack_finished = pr_global_struct->time + 2;

	if (self->v.message)
	{
		PF_centerprint(other, self->v.message);
		PF_sound(other, CHAN_BODY, "misc/talk.wav", 1, ATTN_NORM);
	}
}


/*QUAKED func_door_secret (0 .5 .8) ? open_once 1st_left 1st_down no_shoot always_shoot
Basic secret door. Slides back, then to the side. Angle determines direction.
wait  = # of seconds before coming back
1st_left = 1st move is left of arrow
1st_down = 1st move is down from arrow
always_shoot = even if targeted, keep shootable
t_width = override WIDTH to move back (or height if going down)
t_length = override LENGTH to move sideways
"dmg"		damage to inflict when blocked (2 default)

If a secret door has a targetname, it will only be opened by it's botton or trigger, not by damage.
"sounds"
1) medieval
2) metal
3) base
*/

void func_door_secret(edict_t* self)
{
	if (self->v.sounds == 0)
		self->v.sounds = 3;
	if (self->v.sounds == 1)
	{
		PF_precache_sound("doors/latch2.wav");
		PF_precache_sound("doors/winch2.wav");
		PF_precache_sound("doors/drclos4.wav");
		self->v.noise1 = "doors/latch2.wav";
		self->v.noise2 = "doors/winch2.wav";
		self->v.noise3 = "doors/drclos4.wav";
	}
	if (self->v.sounds == 2)
	{
		PF_precache_sound("doors/airdoor1.wav");
		PF_precache_sound("doors/airdoor2.wav");
		self->v.noise2 = "doors/airdoor1.wav";
		self->v.noise1 = "doors/airdoor2.wav";
		self->v.noise3 = "doors/airdoor2.wav";
	}
	if (self->v.sounds == 3)
	{
		PF_precache_sound("doors/basesec1.wav");
		PF_precache_sound("doors/basesec2.wav");
		self->v.noise2 = "doors/basesec1.wav";
		self->v.noise1 = "doors/basesec2.wav";
		self->v.noise3 = "doors/basesec2.wav";
	}

	if (!self->v.dmg)
		self->v.dmg = 2;

	// Magic formula...
	AsVector(self->v.mangle) = AsVector(self->v.angles);
	AsVector(self->v.angles) = AsVector(vec3_origin);
	self->v.solid = SOLID_BSP;
	self->v.movetype = MOVETYPE_PUSH;
	self->v.classname = "door";
	PF_setmodel(self, self->v.model);
	PF_setorigin(self, self->v.origin);

	self->v.touch = secret_touch;
	self->v.blocked = secret_blocked;
	self->v.speed = 50;
	self->v.use = fd_secret_use;
	if (!self->v.targetname || self->v.spawnflags & SECRET_YES_SHOOT)
	{
		self->v.health = 10000;
		self->v.takedamage = DAMAGE_YES;
		self->v.th_pain = fd_secret_pain;
	}
	AsVector(self->v.oldorigin) = AsVector(self->v.origin);
	if (!self->v.wait)
		self->v.wait = 5;		// 5 seconds before closing
}

LINK_ENTITY_TO_CLASS(func_door_secret);
