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
#include "buttons.h"
#include "Game.h"
#include "subs.h"

// button and multiple button

void button_wait(edict_t* self);
void button_return(edict_t* self);

void button_wait(edict_t* self)
{
	self->v.state = STATE_TOP;
	self->v.nextthink = self->v.ltime + self->v.wait;
	self->v.think = button_return;
	pr_global_struct->activator = self->v.enemy;
	SUB_UseTargets(self);
	self->v.frame = 1;			// use alternate textures
}

LINK_FUNCTION_TO_NAME(button_wait);

void button_done(edict_t* self)
{
	self->v.state = STATE_BOTTOM;
}

LINK_FUNCTION_TO_NAME(button_done);

void button_return(edict_t* self)
{
	self->v.state = STATE_DOWN;
	SUB_CalcMove(self, self->v.pos1, self->v.speed, button_done);
	self->v.frame = 0;			// use normal textures
	if (self->v.health)
		self->v.takedamage = DAMAGE_YES;	// can be shot again
}

LINK_FUNCTION_TO_NAME(button_return);

void button_blocked(edict_t* self, edict_t* other)
{	// do nothing, just don't ome all the way back out
}

LINK_FUNCTION_TO_NAME(button_blocked);

void button_fire(edict_t* self)
{
	if (self->v.state == STATE_UP || self->v.state == STATE_TOP)
		return;

	PF_sound(self, CHAN_VOICE, self->v.noise, 1, ATTN_NORM);

	self->v.state = STATE_UP;
	SUB_CalcMove(self, self->v.pos2, self->v.speed, button_wait);
}

void button_use(edict_t* self, edict_t* other)
{
	self->v.enemy = pr_global_struct->activator;
	button_fire(self);
}

LINK_FUNCTION_TO_NAME(button_use);

void button_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	self->v.enemy = other;
	button_fire(self);
}

LINK_FUNCTION_TO_NAME(button_touch);

void button_killed(edict_t* self)
{
	self->v.enemy = pr_global_struct->damage_attacker;
	self->v.health = self->v.max_health;
	self->v.takedamage = DAMAGE_NO;	// wil be reset upon return
	button_fire(self);
}

LINK_FUNCTION_TO_NAME(button_killed);

/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"sounds"
0) steam metal
1) wooden clunk
2) metallic click
3) in-out
*/
void func_button(edict_t* self)
{
	if (self->v.sounds == 0)
	{
		PF_precache_sound("buttons/airbut1.wav");
		self->v.noise = "buttons/airbut1.wav";
	}
	if (self->v.sounds == 1)
	{
		PF_precache_sound("buttons/switch21.wav");
		self->v.noise = "buttons/switch21.wav";
	}
	if (self->v.sounds == 2)
	{
		PF_precache_sound("buttons/switch02.wav");
		self->v.noise = "buttons/switch02.wav";
	}
	if (self->v.sounds == 3)
	{
		PF_precache_sound("buttons/switch04.wav");
		self->v.noise = "buttons/switch04.wav";
	}

	SetMovedir(self);

	self->v.movetype = MOVETYPE_PUSH;
	self->v.solid = SOLID_BSP;
	PF_setmodel(self, self->v.model);

	self->v.blocked = button_blocked;
	self->v.use = button_use;

	if (self->v.health)
	{
		self->v.max_health = self->v.health;
		self->v.th_die = button_killed;
		self->v.takedamage = DAMAGE_YES;
	}
	else
		self->v.touch = button_touch;

	if (!self->v.speed)
		self->v.speed = 40;
	if (!self->v.wait)
		self->v.wait = 1;
	if (!self->v.lip)
		self->v.lip = 4;

	self->v.state = STATE_BOTTOM;

	AsVector(self->v.pos1) = AsVector(self->v.origin);
	AsVector(self->v.pos2) = AsVector(self->v.pos1) + AsVector(self->v.movedir) * (fabs(DotProduct(self->v.movedir, self->v.size)) - self->v.lip);
}

LINK_ENTITY_TO_CLASS(func_button);
