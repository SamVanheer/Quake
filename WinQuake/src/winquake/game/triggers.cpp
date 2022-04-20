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
#include "subs.h"
#include "triggers.h"

edict_t* stemp, * otemp, * s, * old;

void trigger_reactivate(edict_t* self)
{
	self->v.solid = SOLID_TRIGGER;
}

//=============================================================================

constexpr int SPAWNFLAG_NOMESSAGE = 1;
constexpr int SPAWNFLAG_NOTOUCH = 1;

// the wait time has passed, so set back up for another activation
void multi_wait(edict_t* self)
{
	if (self->v.max_health)
	{
		self->v.health = self->v.max_health;
		self->v.takedamage = DAMAGE_YES;
		self->v.solid = SOLID_BBOX;
	}
}


// the trigger was just touched/killed/used
// self->v.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger(edict_t* self)
{
	if (self->v.nextthink > pr_global_struct->time)
	{
		return;		// allready been triggered
	}

	if (!strcmp(self->v.classname, "trigger_secret"))
	{
		if (strcmp(self->v.enemy->v.classname, "player"))
			return;
		pr_global_struct->found_secrets = pr_global_struct->found_secrets + 1;
		PF_WriteByte(MSG_ALL, SVC_FOUNDSECRET);
	}

	if (self->v.noise)
		PF_sound(self, CHAN_VOICE, self->v.noise, 1, ATTN_NORM);

	// don't trigger again until reset
	self->v.takedamage = DAMAGE_NO;

	pr_global_struct->activator = self->v.enemy;

	SUB_UseTargets(self);

	if (self->v.wait > 0)
	{
		self->v.think = multi_wait;
		self->v.nextthink = pr_global_struct->time + self->v.wait;
	}
	else
	{	// we can't just remove (self) here, because this is a touch function
		// called wheil C code is looping through area links...
		self->v.touch = SUB_NullTouch;
		self->v.nextthink = pr_global_struct->time + 0.1;
		self->v.think = SUB_Remove;
	}
}

void multi_killed(edict_t* self)
{
	self->v.enemy = pr_global_struct->damage_attacker;
	multi_trigger(self);
}

void multi_use(edict_t* self, edict_t* other)
{
	self->v.enemy = pr_global_struct->activator;
	multi_trigger(self);
}

void multi_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;

	// if the trigger has an angles field, check player's facing direction
	if (!VectorCompare(self->v.movedir, vec3_origin))
	{
		PF_makevectors(other->v.angles);
		if (DotProduct(pr_global_struct->v_forward, self->v.movedir) < 0)
			return;		// not facing the right way
	}

	self->v.enemy = other;
	multi_trigger(self);
}

/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.  If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)	secret
2)	beep beep
3)	large switch
4)
set "message" to text string
*/
void trigger_multiple(edict_t* self)
{
	if (self->v.sounds == 1)
	{
		PF_precache_sound("misc/secret.wav");
		self->v.noise = "misc/secret.wav";
	}
	else if (self->v.sounds == 2)
	{
		PF_precache_sound("misc/talk.wav");
		self->v.noise = "misc/talk.wav";
	}
	else if (self->v.sounds == 3)
	{
		PF_precache_sound("misc/trigger1.wav");
		self->v.noise = "misc/trigger1.wav";
	}

	if (!self->v.wait)
		self->v.wait = 0.2f;
	self->v.use = multi_use;

	InitTrigger(self);

	if (self->v.health)
	{
		if (self->v.spawnflags & SPAWNFLAG_NOTOUCH)
			PF_objerror("health and notouch don't make sense\n");
		self->v.max_health = self->v.health;
		self->v.th_die = multi_killed;
		self->v.takedamage = DAMAGE_YES;
		self->v.solid = SOLID_BBOX;
		PF_setorigin(self, self->v.origin);	// make sure it links into the world
	}
	else
	{
		if (!(self->v.spawnflags & SPAWNFLAG_NOTOUCH))
		{
			self->v.touch = multi_touch;
		}
	}
}

LINK_ENTITY_TO_CLASS(trigger_multiple);

/*QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself->v.  You must set the key "target" to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.  Use "360" for an angle of 0.
sounds
1)	secret
2)	beep beep
3)	large switch
4)
set "message" to text string
*/
void trigger_once(edict_t* self)
{
	self->v.wait = -1;
	trigger_multiple(self);
}

LINK_ENTITY_TO_CLASS(trigger_once);

//=============================================================================

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.  It can contain killtargets, targets, delays, and messages.
*/
void trigger_relay(edict_t* self)
{
	self->v.use = SUB_UseTargets;
}

LINK_ENTITY_TO_CLASS(trigger_relay);

//=============================================================================

/*QUAKED trigger_secret (.5 .5 .5) ?
secret counter trigger
sounds
1)	secret
2)	beep beep
3)
4)
set "message" to text string
*/
void trigger_secret(edict_t* self)
{
	pr_global_struct->total_secrets = pr_global_struct->total_secrets + 1;
	self->v.wait = -1;
	if (!self->v.message)
		self->v.message = "You found a secret area!";
	if (!self->v.sounds)
		self->v.sounds = 1;

	if (self->v.sounds == 1)
	{
		PF_precache_sound("misc/secret.wav");
		self->v.noise = "misc/secret.wav";
	}
	else if (self->v.sounds == 2)
	{
		PF_precache_sound("misc/talk.wav");
		self->v.noise = "misc/talk.wav";
	}

	trigger_multiple(self);
}

LINK_ENTITY_TO_CLASS(trigger_secret);

//=============================================================================


void counter_use(edict_t* self, edict_t* other)
{
	self->v.count = self->v.count - 1;
	if (self->v.count < 0)
		return;

	if (self->v.count != 0)
	{
		if (!strcmp(pr_global_struct->activator->v.classname, "player")
			&& (self->v.spawnflags & SPAWNFLAG_NOMESSAGE) == 0)
		{
			if (self->v.count >= 4)
				PF_centerprint(pr_global_struct->activator, "There are more to go...");
			else if (self->v.count == 3)
				PF_centerprint(pr_global_struct->activator, "Only 3 more to go...");
			else if (self->v.count == 2)
				PF_centerprint(pr_global_struct->activator, "Only 2 more to go...");
			else
				PF_centerprint(pr_global_struct->activator, "Only 1 more to go...");
		}
		return;
	}

	if (!strcmp(pr_global_struct->activator->v.classname, "player")
		&& (self->v.spawnflags & SPAWNFLAG_NOMESSAGE) == 0)
		PF_centerprint(pr_global_struct->activator, "Sequence completed!");
	self->v.enemy = pr_global_struct->activator;
	multi_trigger(self);
}

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.

If nomessage is not set, t will print "1 more.. " etc when triggered and "sequence complete" when finished.

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself->v.
*/
void trigger_counter(edict_t* self)
{
	self->v.wait = -1;
	if (!self->v.count)
		self->v.count = 2;

	self->v.use = counter_use;
}

LINK_ENTITY_TO_CLASS(trigger_counter);

/*
==============================================================================

TELEPORT TRIGGERS

==============================================================================
*/

constexpr int PLAYER_ONLY = 1;
constexpr int SILENT = 2;

void play_teleport(edict_t* self)
{
	const char* tmpstr;

	const float v = random() * 5;
	if (v < 1)
		tmpstr = "misc/r_tele1.wav";
	else if (v < 2)
		tmpstr = "misc/r_tele2.wav";
	else if (v < 3)
		tmpstr = "misc/r_tele3.wav";
	else if (v < 4)
		tmpstr = "misc/r_tele4.wav";
	else
		tmpstr = "misc/r_tele5.wav";

	PF_sound(self, CHAN_VOICE, tmpstr, 1, ATTN_NORM);
	PF_Remove(self);
}

void spawn_tfog(edict_t* self, vec3_t org)
{
	s = PF_Spawn();
	VectorCopy(org, s->v.origin);
	s->v.nextthink = pr_global_struct->time + 0.2;
	s->v.think = play_teleport;

	PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte(MSG_BROADCAST, TE_TELEPORT);
	PF_WriteCoord(MSG_BROADCAST, org[0]);
	PF_WriteCoord(MSG_BROADCAST, org[1]);
	PF_WriteCoord(MSG_BROADCAST, org[2]);
}


void tdeath_touch(edict_t* self, edict_t* other)
{
	if (other == self->v.owner)
		return;

	// frag anyone who teleports in on top of an invincible player
	if (!strcmp(other->v.classname, "player"))
	{
		if (other->v.invincible_finished > pr_global_struct->time)
			self->v.classname = "teledeath2";
		if (strcmp(self->v.owner->v.classname, "player"))
		{	// other monsters explode themselves
			T_Damage(self, self->v.owner, self, self, 50000);
			return;
		}

	}

	if (other->v.health)
	{
		T_Damage(self, other, self, self, 50000);
	}
}


void spawn_tdeath(edict_t* self, vec3_t org, edict_t* death_owner)
{
	auto death = PF_Spawn();
	death->v.classname = "teledeath";
	death->v.movetype = MOVETYPE_NONE;
	death->v.solid = SOLID_TRIGGER;
	VectorCopy(vec3_origin, death->v.angles);
	PF_setsize(death, AsVector(death_owner->v.mins) - Vector3D{1, 1, 1}, AsVector(death_owner->v.maxs) + Vector3D{1, 1, 1});
	PF_setorigin(death, org);
	death->v.touch = tdeath_touch;
	death->v.nextthink = pr_global_struct->time + 0.2;
	death->v.think = SUB_Remove;
	death->v.owner = death_owner;

	pr_global_struct->force_retouch = 2;		// make sure even still objects get hit
}

void teleport_touch(edict_t* self, edict_t* other)
{
	if (self->v.targetname)
	{
		if (self->v.nextthink < pr_global_struct->time)
		{
			return;		// not fired yet
		}
	}

	if (self->v.spawnflags & PLAYER_ONLY)
	{
		if (strcmp(other->v.classname, "player"))
			return;
	}

	// only teleport living creatures
	if (other->v.health <= 0 || other->v.solid != SOLID_SLIDEBOX)
		return;

	SUB_UseTargets(self);

	// put a tfog where the player was
	spawn_tfog(self, other->v.origin);

	auto t = PF_Find(pr_global_struct->world, "targetname", self->v.target);
	if (t == pr_global_struct->world)
		PF_objerror("couldn't find target");

	// PF_Spawn a tfog flash in front of the destination
	PF_makevectors(t->v.mangle);
	auto org = AsVector(t->v.origin) + 32 * AsVector(pr_global_struct->v_forward);

	spawn_tfog(self, org);
	spawn_tdeath(self, t->v.origin, other);

	// move the player and lock him down for a little while
	if (!other->v.health)
	{
		AsVector(other->v.origin) = AsVector(t->v.origin);
		AsVector(other->v.velocity) = (AsVector(pr_global_struct->v_forward) * other->v.velocity[0]) + (AsVector(pr_global_struct->v_forward) * other->v.velocity[1]);
		return;
	}

	PF_setorigin(other, t->v.origin);
	AsVector(other->v.angles) = AsVector(t->v.mangle);
	if (!strcmp(other->v.classname, "player"))
	{
		other->v.fixangle = 1;		// turn this way immediately
		other->v.teleport_time = pr_global_struct->time + 0.7;
		AsVector(other->v.velocity) = AsVector(pr_global_struct->v_forward) * 300;
	}
	other->v.flags &= ~FL_ONGROUND;
}

/*QUAKED info_teleport_destination (.5 .5 .5) (-8 -8 -8) (8 8 32)
This is the destination marker for a teleporter.  It should have a "targetname" field with the same value as a teleporter's "target" field.
*/
void info_teleport_destination(edict_t* self)
{
	// this does nothing, just serves as a target spot
	AsVector(self->v.mangle) = AsVector(self->v.angles);
	VectorCopy(vec3_origin, self->v.angles);
	self->v.model = "";
	AsVector(self->v.origin) = AsVector(self->v.origin) + Vector3D{0, 0, 27};
	if (!self->v.targetname)
		PF_objerror("no targetname");
}

LINK_ENTITY_TO_CLASS(info_teleport_destination);

void teleport_use(edict_t* self, edict_t* other)
{
	self->v.nextthink = pr_global_struct->time + 0.2;
	pr_global_struct->force_retouch = 2;		// make sure even still objects get hit
	self->v.think = SUB_NullThink;
}

/*QUAKED trigger_teleport (.5 .5 .5) ? PLAYER_ONLY SILENT
Any object touching this will be transported to the corresponding info_teleport_destination entity. You must set the "target" field, and create an object with a "targetname" field that matches.

If the trigger_teleport has a targetname, it will only teleport entities when it has been fired.
*/
void trigger_teleport(edict_t* self)
{
	InitTrigger(self);
	self->v.touch = teleport_touch;
	// find the destination 
	if (!self->v.target)
		PF_objerror("no target");
	self->v.use = teleport_use;

	if (!(self->v.spawnflags & SILENT))
	{
		PF_precache_sound("ambience/hum1.wav");
		auto o = (AsVector(self->v.mins) + AsVector(self->v.maxs)) * 0.5;
		PF_ambientsound(o, "ambience/hum1.wav", 0.5, ATTN_STATIC);
	}
}

LINK_ENTITY_TO_CLASS(trigger_teleport);

/*
==============================================================================

trigger_setskill

==============================================================================
*/

void trigger_skill_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;

	PF_cvar_set("skill", self->v.message);
}

/*QUAKED trigger_setskill (.5 .5 .5) ?
sets skill level to the value of "message".
Only used on start map.
*/
void trigger_setskill(edict_t* self)
{
	InitTrigger(self);
	self->v.touch = trigger_skill_touch;
}

LINK_ENTITY_TO_CLASS(trigger_setskill);

/*
==============================================================================

ONLY REGISTERED TRIGGERS

==============================================================================
*/

void trigger_onlyregistered_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	if (self->v.attack_finished > pr_global_struct->time)
		return;

	self->v.attack_finished = pr_global_struct->time + 2;
	if (PF_cvar("registered"))
	{
		self->v.message = "";
		SUB_UseTargets(self);
		PF_Remove(self);
	}
	else
	{
		if (self->v.message && strcmp(self->v.message, ""))
		{
			PF_centerprint(other, self->v.message);
			PF_sound(other, CHAN_BODY, "misc/talk.wav", 1, ATTN_NORM);
		}
	}
}

/*QUAKED trigger_onlyregistered (.5 .5 .5) ?
Only fires if playing the registered version, otherwise prints the message
*/
void trigger_onlyregistered(edict_t* self)
{
	PF_precache_sound("misc/talk.wav");
	InitTrigger(self);
	self->v.touch = trigger_onlyregistered_touch;
}

LINK_ENTITY_TO_CLASS(trigger_onlyregistered);

//============================================================================

void hurt_on(edict_t* self)
{
	self->v.solid = SOLID_TRIGGER;
	self->v.nextthink = -1;
}

void hurt_touch(edict_t* self, edict_t* other)
{
	if (other->v.takedamage)
	{
		self->v.solid = SOLID_NOT;
		T_Damage(self, other, self, self, self->v.dmg);
		self->v.think = hurt_on;
		self->v.nextthink = pr_global_struct->time + 1;
	}

	return;
}

/*QUAKED trigger_hurt (.5 .5 .5) ?
Any object touching this will be hurt
set dmg to damage amount
defalt dmg = 5
*/
void trigger_hurt(edict_t* self)
{
	InitTrigger(self);
	self->v.touch = hurt_touch;
	if (!self->v.dmg)
		self->v.dmg = 5;
}

LINK_ENTITY_TO_CLASS(trigger_hurt);

//============================================================================

constexpr int PUSH_ONCE = 1;

void trigger_push_touch(edict_t* self, edict_t* other)
{
	if (!strcmp(other->v.classname, "grenade"))
		AsVector(other->v.velocity) = self->v.speed * AsVector(self->v.movedir) * 10;
	else if (other->v.health > 0)
	{
		AsVector(other->v.velocity) = self->v.speed * AsVector(self->v.movedir) * 10;
		if (!strcmp(other->v.classname, "player"))
		{
			if (other->v.fly_sound < pr_global_struct->time)
			{
				other->v.fly_sound = pr_global_struct->time + 1.5;
				PF_sound(other, CHAN_AUTO, "ambience/windfly.wav", 1, ATTN_NORM);
			}
		}
	}
	if (self->v.spawnflags & PUSH_ONCE)
		PF_Remove(self);
}


/*QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE
Pushes the player
*/
void trigger_push(edict_t* self)
{
	InitTrigger(self);
	PF_precache_sound("ambience/windfly.wav");
	self->v.touch = trigger_push_touch;
	if (!self->v.speed)
		self->v.speed = 1000;
}

LINK_ENTITY_TO_CLASS(trigger_push);

//============================================================================

void trigger_monsterjump_touch(edict_t* self, edict_t* other)
{
	if ((other->v.flags & (FL_MONSTER | FL_FLY | FL_SWIM)) != FL_MONSTER)
		return;

	// set XY even if not on ground, so the jump will clear lips
	other->v.velocity[0] = self->v.movedir[0] * self->v.speed;
	other->v.velocity[1] = self->v.movedir[1] * self->v.speed;

	if (!(other->v.flags & FL_ONGROUND))
		return;

	other->v.flags &= ~FL_ONGROUND;

	other->v.velocity[2] = self->v.height;
}

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle
"speed" default to 200, the speed thrown forward
"height" default to 200, the speed thrown upwards
*/
void trigger_monsterjump(edict_t* self)
{
	if (!self->v.speed)
		self->v.speed = 200;
	if (!self->v.height)
		self->v.height = 200;
	if (VectorCompare(self->v.angles, vec3_origin))
		AsVector(self->v.angles) = Vector3D{0, 360, 0};
	InitTrigger(self);
	self->v.touch = trigger_monsterjump_touch;
}

LINK_ENTITY_TO_CLASS(trigger_monsterjump);
