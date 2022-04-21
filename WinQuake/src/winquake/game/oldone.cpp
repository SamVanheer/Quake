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
#include "animation.h"
#include "Game.h"
#include "player.h"

/*
==============================================================================

OLD ONE

==============================================================================
*/

void old_thrash1(edict_t* self);

void finale_1(edict_t* self);
void finale_2(edict_t* self);
void finale_3(edict_t* self);
void finale_4(edict_t* self);

void oldone_idle_frame(edict_t* self, const Animation* animation, int frame)
{
}

void oldone_shake_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
	case 13:
	case 14: PF_lightstyle(0, "m"); break;
	case 1:
	case 2:
	case 12: PF_lightstyle(0, "k"); break;
	case 3:
	case 11: PF_lightstyle(0, "i"); break;
	case 4:
	case 10:
	case 15: PF_lightstyle(0, "g"); break;
	case 5:
	case 9: PF_lightstyle(0, "e"); break;
	case 6:
	case 8:
	case 16: PF_lightstyle(0, "c"); break;
	case 7:
	case 18: PF_lightstyle(0, "a"); break;
	case 17: PF_lightstyle(0, "b"); break;
	}

	if (frame == 14)
	{
		self->v.cnt = self->v.cnt + 1;
		if (self->v.cnt != 3)
			self->v.think = old_thrash1;
	}

	if (animation->ReachedEnd(frame))
	{
		finale_4(self);
	}
}

const Animations OldOneAnimations = MakeAnimations(
	{
		{"old", 46, &oldone_idle_frame, AnimationFlag::Looping},
		{"shake", 20, &oldone_shake_frame}
	});

const Animations* old_animations_get(edict_t* self)
{
	return &OldOneAnimations;
}

LINK_FUNCTION_TO_NAME(old_animations_get);

//void() old_stand     =[      $old1,       old_stand    ] {}

void old_idle1(edict_t* self)
{
	animation_set(self, "old");
}

LINK_FUNCTION_TO_NAME(old_idle1);

void old_thrash1(edict_t* self)
{
	animation_set(self, "shake");
}

LINK_FUNCTION_TO_NAME(old_thrash1);

//============================================================================

void finale_1(edict_t* self)
{
	//TODO: figure out what other is supposed to be here.
	edict_t* other = pr_global_struct->world;

	pr_global_struct->intermission_exittime = pr_global_struct->time + 10000000;	// never allow exit
	pr_global_struct->intermission_running = 1;

	// find the intermission spot
	auto pos = PF_Find(pr_global_struct->world, "classname", "info_intermission");
	if (pos == pr_global_struct->world)
		PF_error("no info_intermission");
	auto pl = PF_Find(pr_global_struct->world, "classname", "misc_teleporttrain");
	if (pl == pr_global_struct->world)
		PF_error("no teleporttrain");
	PF_Remove(pl);

	PF_WriteByte(MSG_ALL, SVC_FINALE);
	PF_WriteString(MSG_ALL, "");

	pl = PF_Find(pr_global_struct->world, "classname", "player");
	while (pl != pr_global_struct->world)
	{
		AsVector(pl->v.view_ofs) = AsVector(vec3_origin);
		AsVector(pl->v.angles) = AsVector(other->v.v_angle) = AsVector(pos->v.mangle);
		pl->v.fixangle = 1;		// turn this way immediately
		pl->v.map = self->v.map;
		pl->v.nextthink = pr_global_struct->time + 0.5;
		pl->v.takedamage = DAMAGE_NO;
		pl->v.solid = SOLID_NOT;
		pl->v.movetype = MOVETYPE_NONE;
		pl->v.modelindex = 0;
		PF_setorigin(pl, pos->v.origin);
		pl = PF_Find(pl, "classname", "player");
	}

	// make fake versions of all players as standins, and move the real
	// players to the intermission spot

	// wait for 1 second
	auto timer = PF_Spawn();
	timer->v.nextthink = pr_global_struct->time + 1;
	timer->v.think = finale_2;
}

LINK_FUNCTION_TO_NAME(finale_1);

void finale_2(edict_t* self)
{
	// start a teleport splash inside shub

	auto o = AsVector(pr_global_struct->shub->v.origin) - Vector3D{0, 100, 0};
	PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte(MSG_BROADCAST, TE_TELEPORT);
	PF_WriteCoord(MSG_BROADCAST, o[0]);
	PF_WriteCoord(MSG_BROADCAST, o[1]);
	PF_WriteCoord(MSG_BROADCAST, o[2]);

	PF_sound(pr_global_struct->shub, CHAN_VOICE, "misc/r_tele1.wav", 1, ATTN_NORM);

	self->v.nextthink = pr_global_struct->time + 2;
	self->v.think = finale_3;
}

LINK_FUNCTION_TO_NAME(finale_2);

void finale_3(edict_t* self)
{
	// start shub thrashing wildly
	pr_global_struct->shub->v.think = old_thrash1;
	PF_sound(pr_global_struct->shub, CHAN_VOICE, "boss2/death.wav", 1, ATTN_NORM);
	PF_lightstyle(0, "abcdefghijklmlkjihgfedcb");
}

LINK_FUNCTION_TO_NAME(finale_3);

void finale_4(edict_t* self)
{
	// throw tons of meat chunks	
	PF_sound(self, CHAN_VOICE, "boss2/pop2.wav", 1, ATTN_NORM);

	Vector3D oldo = AsVector(self->v.origin);

	float z = 16;
	while (z <= 144)
	{
		float x = -64;
		while (x <= 64)
		{
			float y = -64;
			while (y <= 64)
			{
				self->v.origin[0] = oldo[0] + x;
				self->v.origin[1] = oldo[1] + y;
				self->v.origin[2] = oldo[2] + z;

				const float r = PF_random();
				if (r < 0.3)
					ThrowGib(self, "progs/gib1.mdl", -999);
				else if (r < 0.6)
					ThrowGib(self, "progs/gib2.mdl", -999);
				else
					ThrowGib(self, "progs/gib3.mdl", -999);
				y = y + 32;
			}
			x = x + 32;
		}
		z = z + 96;
	}
	// start the end text
	PF_WriteByte(MSG_ALL, SVC_FINALE);
	PF_WriteString(MSG_ALL, "Congratulations and well done! You have\nbeaten the hideous Shub-Niggurath, and\nher hundreds of ugly changelings and\nmonsters. You have proven that your\nskill and your cunning are greater than\nall the powers of Quake. You are the\nmaster now. Id Software salutes you.");

	// put a player model down
	auto n = PF_Spawn();
	PF_setmodel(n, "progs/player.mdl");
	oldo = oldo - Vector3D{32, 264, 0};
	PF_setorigin(n, oldo);
	AsVector(n->v.angles) = Vector3D{0, 290, 0};
	n->v.frame = 1;

	PF_Remove(self);

	// switch cd track
	PF_WriteByte(MSG_ALL, SVC_CDTRACK);
	PF_WriteByte(MSG_ALL, 3);
	PF_WriteByte(MSG_ALL, 3);
	PF_lightstyle(0, "m");
}

LINK_FUNCTION_TO_NAME(finale_4);

//============================================================================


/*QUAKED monster_oldone (1 0 0) (-16 -16 -24) (16 16 32)
*/
void monster_oldone(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}

	PF_precache_model("progs/oldone.mdl");

	PF_precache_sound("boss2/death.wav");
	PF_precache_sound("boss2/idle.wav");
	PF_precache_sound("boss2/sight.wav");
	PF_precache_sound("boss2/pop2.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel(self, "progs/oldone.mdl");
	PF_setsize(self, Vector3D{-160, -128, -24}, Vector3D{160, 128, 256});

	self->v.health = 40000;		// kill by telefrag
	self->v.think = old_idle1;
	self->v.nextthink = pr_global_struct->time + 0.1;
	self->v.takedamage = DAMAGE_YES;
	self->v.th_die = finale_1;
	self->v.animations_get = &old_animations_get;
	pr_global_struct->shub = self;

	pr_global_struct->total_monsters = pr_global_struct->total_monsters + 1;
}

LINK_ENTITY_TO_CLASS(monster_oldone);
