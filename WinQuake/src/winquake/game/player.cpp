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
#include "client.h"
#include "Game.h"
#include "items.h"
#include "misc.h"
#include "player.h"
#include "subs.h"
#include "weapons.h"

void DeathBubbles(edict_t* self, float num_bubbles);

const Animations PlayerAnimations = MakeAnimations(
	{
		{"axrun", 6}, {"rockrun", 6},

		{"stand", 5}, {"axstnd", 12},

		{"axpain", 6}, {"pain", 6},

		{"axdeth", 9},
		{"deatha", 11},
		{"deathb", 9},
		{"deathc", 15},
		{"deathd", 9},
		{"deathe", 9},

		{"nailatt", 2},
		{"light", 2},
		{"rockatt", 6},
		{"shotatt", 6},
		{"axatt", 6},
		{"axattb", 6},
		{"axattc", 6},
		{"axattd", 6}
	});

void player_setanimation(edict_t* self, std::string_view name, int frame, decltype(entvars_t::think) thinkfunc)
{
	self->v.frame = PlayerAnimations.FindAnimationStart(name) + frame;
	self->v.nextthink = pr_global_struct->time + 0.1f;
	self->v.think = thinkfunc;
}

/*
==============================================================================
PLAYER
==============================================================================
*/

void player_stand1(edict_t* self)
{
	self->v.think = &player_stand1;
	self->v.nextthink = pr_global_struct->time + 0.1;

	self->v.weaponframe = 0;
	if (self->v.velocity[0] || self->v.velocity[1])
	{
		self->v.walkframe = 0;
		player_run(self);
		return;
	}

	if (self->v.weapon == IT_AXE)
	{
		if (self->v.walkframe >= 12)
			self->v.walkframe = 0;
		self->v.frame = PlayerAnimations.FindAnimationStart("axstnd") + self->v.walkframe;
	}
	else
	{
		if (self->v.walkframe >= 5)
			self->v.walkframe = 0;
		self->v.frame = PlayerAnimations.FindAnimationStart("stand") + self->v.walkframe;
	}
	++self->v.walkframe;
}

void player_run(edict_t* self)
{
	self->v.think = &player_run;
	self->v.nextthink = pr_global_struct->time + 0.1;

	self->v.weaponframe = 0;
	if (!self->v.velocity[0] && !self->v.velocity[1])
	{
		self->v.walkframe = 0;
		player_stand1(self);
		return;
	}

	if (self->v.weapon == IT_AXE)
	{
		if (self->v.walkframe == 6)
			self->v.walkframe = 0;
		self->v.frame = PlayerAnimations.FindAnimationStart("axrun") + self->v.walkframe;
	}
	else
	{
		if (self->v.walkframe == 6)
			self->v.walkframe = 0;
		self->v.frame += self->v.walkframe;
	}
	++self->v.walkframe;
}

void player_shot6(edict_t* self)
{
	player_setanimation(self, "shotatt", 5, player_run);
	self->v.weaponframe = 6;
}

void player_shot5(edict_t* self)
{
	player_setanimation(self, "shotatt", 4, player_shot6);
	self->v.weaponframe = 5;
}

void player_shot4(edict_t* self)
{
	player_setanimation(self, "shotatt", 3, player_shot5);
	self->v.weaponframe = 4;
}

void player_shot3(edict_t* self)
{
	player_setanimation(self, "shotatt", 2, player_shot4);
	self->v.weaponframe = 3;
}

void player_shot2(edict_t* self)
{
	player_setanimation(self, "shotatt", 1, player_shot3);
	self->v.weaponframe = 2;
}

void player_shot1(edict_t* self)
{
	player_setanimation(self, "shotatt", 0, player_shot2);
	self->v.weaponframe = 1;
	self->v.effects = self->v.effects | EF_MUZZLEFLASH;
}

void player_axe4(edict_t* self)
{
	player_setanimation(self, "shotatt", 3, player_run);
	self->v.weaponframe = 4;
}

void player_axe3(edict_t* self)
{
	player_setanimation(self, "shotatt", 2, player_axe4);
	self->v.weaponframe = 3;
	W_FireAxe(self);
}

void player_axe2(edict_t* self)
{
	player_setanimation(self, "shotatt", 1, player_axe3);
	self->v.weaponframe = 2;
}

void player_axe1(edict_t* self)
{
	player_setanimation(self, "shotatt", 0, player_axe2);
	self->v.weaponframe = 1;
}

void player_axeb4(edict_t* self)
{
	player_setanimation(self, "shotatt", 3, player_run);
	self->v.weaponframe = 8;
}

void player_axeb3(edict_t* self)
{
	player_setanimation(self, "shotatt", 2, player_axeb4);
	self->v.weaponframe = 7;
	W_FireAxe(self);
}

void player_axeb2(edict_t* self)
{
	player_setanimation(self, "shotatt", 1, player_axeb3);
	self->v.weaponframe = 6;
}

void player_axeb1(edict_t* self)
{
	player_setanimation(self, "shotatt", 0, player_axeb2);
	self->v.weaponframe = 5;
}

void player_axec4(edict_t* self)
{
	player_setanimation(self, "shotatt", 3, player_run);
	self->v.weaponframe = 4;
}

void player_axec3(edict_t* self)
{
	player_setanimation(self, "shotatt", 2, player_axec4);
	self->v.weaponframe = 3;
	W_FireAxe(self);
}

void player_axec2(edict_t* self)
{
	player_setanimation(self, "shotatt", 1, player_axec3);
	self->v.weaponframe = 2;
}

void player_axec1(edict_t* self)
{
	player_setanimation(self, "shotatt", 0, player_axec2);
	self->v.weaponframe = 1;
}

void player_axed4(edict_t* self)
{
	player_setanimation(self, "shotatt", 3, player_run);
	self->v.weaponframe = 6;
}

void player_axed3(edict_t* self)
{
	player_setanimation(self, "shotatt", 2, player_axed4);
	self->v.weaponframe = 7;
	W_FireAxe(self);
}

void player_axed2(edict_t* self)
{
	player_setanimation(self, "shotatt", 1, player_axed3);
	self->v.weaponframe = 6;
}

void player_axed1(edict_t* self)
{
	player_setanimation(self, "shotatt", 0, player_axed2);
	self->v.weaponframe = 5;
}

//============================================================================
void player_nail2(edict_t* self);

void player_nail1(edict_t* self)
{
	player_setanimation(self, "nailatt", 0, player_nail2);

	self->v.effects = self->v.effects | EF_MUZZLEFLASH;

	if (!self->v.button0)
	{
		player_run(self);
		return;
	}
	self->v.weaponframe = self->v.weaponframe + 1;
	if (self->v.weaponframe == 9)
		self->v.weaponframe = 1;
	SuperDamageSound(self);
	W_FireSpikes(self, 4);
	self->v.attack_finished = pr_global_struct->time + 0.2;
}

void player_nail2(edict_t* self)
{
	player_setanimation(self, "nailatt", 1, player_nail1);

	self->v.effects = self->v.effects | EF_MUZZLEFLASH;

	if (!self->v.button0)
	{
		player_run(self); return;
	}
	self->v.weaponframe = self->v.weaponframe + 1;
	if (self->v.weaponframe == 9)
		self->v.weaponframe = 1;
	SuperDamageSound(self);
	W_FireSpikes(self, -4);
	self->v.attack_finished = pr_global_struct->time + 0.2;
}

//============================================================================
void player_light2(edict_t* self);

void player_light1(edict_t* self)
{
	player_setanimation(self, "light", 0, player_light2);

	self->v.effects = self->v.effects | EF_MUZZLEFLASH;

	if (!self->v.button0)
	{
		player_run(self); return;
	}
	self->v.weaponframe = self->v.weaponframe + 1;
	if (self->v.weaponframe == 5)
		self->v.weaponframe = 1;
	SuperDamageSound(self);
	W_FireLightning(self);
	self->v.attack_finished = pr_global_struct->time + 0.2;
}

void player_light2(edict_t* self)
{
	player_setanimation(self, "light", 1, player_light1);

	self->v.effects = self->v.effects | EF_MUZZLEFLASH;

	if (!self->v.button0)
	{
		player_run(self); return;
	}
	self->v.weaponframe = self->v.weaponframe + 1;
	if (self->v.weaponframe == 5)
		self->v.weaponframe = 1;
	SuperDamageSound(self);
	W_FireLightning(self);
	self->v.attack_finished = pr_global_struct->time + 0.2;
}

//============================================================================

void player_rocket6(edict_t* self)
{
	player_setanimation(self, "rocketatt", 5, player_run);
	self->v.weaponframe = 6;
}

void player_rocket5(edict_t* self)
{
	player_setanimation(self, "rocketatt", 4, player_rocket6);
	self->v.weaponframe = 5;
}

void player_rocket4(edict_t* self)
{
	player_setanimation(self, "rocketatt", 3, player_rocket5);
	self->v.weaponframe = 4;
}

void player_rocket3(edict_t* self)
{
	player_setanimation(self, "rocketatt", 2, player_rocket4);
	self->v.weaponframe = 3;
}

void player_rocket2(edict_t* self)
{
	player_setanimation(self, "rocketatt", 1, player_rocket3);
	self->v.weaponframe = 2;
}

void player_rocket1(edict_t* self)
{
	player_setanimation(self, "rocketatt", 0, player_rocket2);
	self->v.weaponframe = 1;
	self->v.effects = self->v.effects | EF_MUZZLEFLASH;
}

void PainSound(edict_t* self)
{
	if (self->v.health < 0)
		return;

	if (!strcmp(damage_attacker->v.classname, "teledeath"))
	{
		PF_sound(self, CHAN_VOICE, "player/teledth1.wav", 1, ATTN_NONE);
		return;
	}

	// water pain sounds
	if (self->v.watertype == CONTENT_WATER && self->v.waterlevel == 3)
	{
		DeathBubbles(self, 1);
		if (random() > 0.5)
			PF_sound(self, CHAN_VOICE, "player/drown1.wav", 1, ATTN_NORM);
		else
			PF_sound(self, CHAN_VOICE, "player/drown2.wav", 1, ATTN_NORM);
		return;
	}

	// slime pain sounds
	if (self->v.watertype == CONTENT_SLIME)
	{
		// FIX ME	put in some steam here
		if (random() > 0.5)
			PF_sound(self, CHAN_VOICE, "player/lburn1.wav", 1, ATTN_NORM);
		else
			PF_sound(self, CHAN_VOICE, "player/lburn2.wav", 1, ATTN_NORM);
		return;
	}

	if (self->v.watertype == CONTENT_LAVA)
	{
		if (random() > 0.5)
			PF_sound(self, CHAN_VOICE, "player/lburn1.wav", 1, ATTN_NORM);
		else
			PF_sound(self, CHAN_VOICE, "player/lburn2.wav", 1, ATTN_NORM);
		return;
	}

	if (self->v.pain_finished > pr_global_struct->time)
	{
		self->v.axhitme = 0;
		return;
	}
	self->v.pain_finished = pr_global_struct->time + 0.5;

	// don't make multiple pain sounds right after each other

	// ax pain sound
	if (self->v.axhitme == 1)
	{
		self->v.axhitme = 0;
		PF_sound(self, CHAN_VOICE, "player/axhit1.wav", 1, ATTN_NORM);
		return;
	}


	const float rs = rint((random() * 5) + 1);

	self->v.noise = "";
	if (rs == 1)
		self->v.noise = "player/pain1.wav";
	else if (rs == 2)
		self->v.noise = "player/pain2.wav";
	else if (rs == 3)
		self->v.noise = "player/pain3.wav";
	else if (rs == 4)
		self->v.noise = "player/pain4.wav";
	else if (rs == 5)
		self->v.noise = "player/pain5.wav";
	else
		self->v.noise = "player/pain6.wav";

	PF_sound(self, CHAN_VOICE, self->v.noise, 1, ATTN_NORM);
	return;
}

void player_pain6(edict_t* self)
{
	player_setanimation(self, "pain", 5, &player_run);
}

void player_pain5(edict_t* self)
{
	player_setanimation(self, "pain", 4, &player_pain6);
}

void player_pain4(edict_t* self)
{
	player_setanimation(self, "pain", 3, &player_pain5);
}

void player_pain3(edict_t* self)
{
	player_setanimation(self, "pain", 2, &player_pain4);
}

void player_pain2(edict_t* self)
{
	player_setanimation(self, "pain", 1, &player_pain3);
}

void player_pain1(edict_t* self)
{
	player_setanimation(self, "pain", 0, &player_pain2);
	PainSound(self);
	self->v.weaponframe = 0;
}

void player_axpain6(edict_t* self)
{
	player_setanimation(self, "axpain", 5, &player_run);
}

void player_axpain5(edict_t* self)
{
	player_setanimation(self, "axpain", 4, &player_axpain6);
}

void player_axpain4(edict_t* self)
{
	player_setanimation(self, "axpain", 3, &player_axpain5);
}

void player_axpain3(edict_t* self)
{
	player_setanimation(self, "axpain", 2, &player_axpain4);
}

void player_axpain2(edict_t* self)
{
	player_setanimation(self, "axpain", 1, &player_axpain3);
}

void player_axpain1(edict_t* self)
{
	player_setanimation(self, "axpain", 0, &player_axpain2);
	PainSound(self);
	self->v.weaponframe = 0;
}

void player_pain(edict_t* self, edict_s* attacker, float damage)
{
	if (self->v.weaponframe)
		return;

	if (self->v.invisible_finished > pr_global_struct->time)
		return;		// eyes don't have pain frames

	if (self->v.weapon == IT_AXE)
		player_axpain1(self);
	else
		player_pain1(self);
}

void player_diea1(edict_t* self);
void player_dieb1(edict_t* self);
void player_diec1(edict_t* self);
void player_died1(edict_t* self);
void player_diee1(edict_t* self);
void player_die_ax1(edict_t* self);

void DeathBubblesSpawn(edict_t* self)
{
	if (self->v.owner->v.waterlevel != 3)
		return;
	auto bubble = PF_Spawn();
	PF_setmodel(bubble, "progs/s_bubble.spr");
	PF_setorigin(bubble, AsVector(self->v.owner->v.origin) + Vector3D{0, 0, 24});
	bubble->v.movetype = MOVETYPE_NOCLIP;
	bubble->v.solid = SOLID_NOT;
	AsVector(bubble->v.velocity) = Vector3D{0, 0, 15};
	bubble->v.nextthink = pr_global_struct->time + 0.5;
	bubble->v.think = bubble_bob;
	bubble->v.classname = "bubble";
	bubble->v.frame = 0;
	bubble->v.cnt = 0;
	PF_setsize(bubble, Vector3D{-8, -8, -8}, Vector3D{8, 8, 8});
	self->v.nextthink = pr_global_struct->time + 0.1;
	self->v.think = DeathBubblesSpawn;
	self->v.air_finished = self->v.air_finished + 1;
	if (self->v.air_finished >= self->v.bubble_count)
		PF_Remove(self);
}

void DeathBubbles(edict_t* self, float num_bubbles)
{
	auto bubble_spawner = PF_Spawn();
	PF_setorigin(bubble_spawner, self->v.origin);
	bubble_spawner->v.movetype = MOVETYPE_NONE;
	bubble_spawner->v.solid = SOLID_NOT;
	bubble_spawner->v.nextthink = pr_global_struct->time + 0.1;
	bubble_spawner->v.think = DeathBubblesSpawn;
	bubble_spawner->v.air_finished = 0;
	bubble_spawner->v.owner = self;
	bubble_spawner->v.bubble_count = num_bubbles;
	return;
}

void DeathSound(edict_t* self)
{
	// water death sounds
	if (self->v.waterlevel == 3)
	{
		DeathBubbles(self, 20);
		PF_sound(self, CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}

	const float rs = rint((random() * 4) + 1);
	if (rs == 1)
		self->v.noise = "player/death1.wav";
	if (rs == 2)
		self->v.noise = "player/death2.wav";
	if (rs == 3)
		self->v.noise = "player/death3.wav";
	if (rs == 4)
		self->v.noise = "player/death4.wav";
	if (rs == 5)
		self->v.noise = "player/death5.wav";

	PF_sound(self, CHAN_VOICE, self->v.noise, 1, ATTN_NONE);
	return;
}

void PlayerDead(edict_t* self)
{
	self->v.nextthink = -1;
	// allow respawn after a certain time
	self->v.deadflag = DEAD_DEAD;
}

void VelocityForDamage(edict_t* self, float dm, vec3_t result)
{
	result[0] = 100 * crandom();
	result[1] = 100 * crandom();
	result[2] = 200 + 100 * PF_random();

	if (dm > -50)
	{
		//		dprint ("level 1\n");
		VectorScale(result, 0.7f, result);
	}
	else if (dm > -200)
	{
		//		dprint ("level 3\n");
		VectorScale(result, 2, result);
	}
	else
		VectorScale(result, 10, result);
}

void ThrowGib(edict_t* self, char* gibname, float dm)
{
	auto ent = PF_Spawn();
	VectorCopy(self->v.origin, ent->v.origin);
	PF_setmodel(ent, gibname);
	PF_setsize(ent, vec3_origin, vec3_origin);
	VelocityForDamage(self, dm, ent->v.velocity);
	ent->v.movetype = MOVETYPE_BOUNCE;
	ent->v.solid = SOLID_NOT;
	ent->v.avelocity[0] = PF_random() * 600;
	ent->v.avelocity[1] = PF_random() * 600;
	ent->v.avelocity[2] = PF_random() * 600;
	ent->v.think = &SUB_Remove;
	ent->v.ltime = pr_global_struct->time;
	ent->v.nextthink = pr_global_struct->time + 10 + PF_random() * 10;
	ent->v.frame = 0;
	ent->v.flags = 0;
}

void ThrowHead(edict_t* self, char* gibname, float dm)
{
	PF_setmodel(self, gibname);
	self->v.frame = 0;
	self->v.nextthink = -1;
	self->v.movetype = MOVETYPE_BOUNCE;
	self->v.takedamage = DAMAGE_NO;
	self->v.solid = SOLID_NOT;
	self->v.view_ofs[0] = 0;
	self->v.view_ofs[1] = 0;
	self->v.view_ofs[2] = 8;
	PF_setsize(self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	VelocityForDamage(self, dm, self->v.velocity);
	self->v.origin[2] -= 24;
	self->v.flags = self->v.flags - ((int)self->v.flags & FL_ONGROUND);
	AsVector(self->v.avelocity) = crandom() * Vector3D { 0, 600, 0 };
}

void GibPlayer(edict_t* self)
{
	ThrowHead(self, "progs/h_player.mdl", self->v.health);
	ThrowGib(self, "progs/gib1.mdl", self->v.health);
	ThrowGib(self, "progs/gib2.mdl", self->v.health);
	ThrowGib(self, "progs/gib3.mdl", self->v.health);

	self->v.deadflag = DEAD_DEAD;

	if (!strcmp(damage_attacker->v.classname, "teledeath"))
	{
		PF_sound(self, CHAN_VOICE, "player/teledth1.wav", 1, ATTN_NONE);
		return;
	}

	if (!strcmp(damage_attacker->v.classname, "teledeath2"))
	{
		PF_sound(self, CHAN_VOICE, "player/teledth1.wav", 1, ATTN_NONE);
		return;
	}

	if (PF_random() < 0.5)
		PF_sound(self, CHAN_VOICE, "player/gib.wav", 1, ATTN_NONE);
	else
		PF_sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NONE);
}

void PlayerDie(edict_t* self)
{
	self->v.items = self->v.items - ((int)self->v.items & IT_INVISIBILITY);
	self->v.invisible_finished = 0;	// don't die as eyes
	self->v.invincible_finished = 0;
	self->v.super_damage_finished = 0;
	self->v.radsuit_finished = 0;
	self->v.modelindex = modelindex_player;	// don't use eyes

	if (pr_global_struct->deathmatch || pr_global_struct->coop)
	{
		DropBackpack(self);
	}

	self->v.weaponmodel = "";
	self->v.view_ofs[0] = 0;
	self->v.view_ofs[1] = 0;
	self->v.view_ofs[2] = -8;
	self->v.deadflag = DEAD_DYING;
	self->v.solid = SOLID_NOT;
	self->v.flags = self->v.flags - ((int)self->v.flags & FL_ONGROUND);
	self->v.movetype = MOVETYPE_TOSS;
	if (self->v.velocity[2] < 10)
		self->v.velocity[2] += PF_random() * 300;

	if (self->v.health < -40)
	{
		GibPlayer(self);
		return;
	}

	DeathSound(self);

	self->v.angles[0] = 0;
	self->v.angles[2] = 0;

	if (self->v.weapon == IT_AXE)
	{
		player_die_ax1(self);
		return;
	}

	float i = PF_cvar("temp1");
	if (!i)
		i = 1 + floor(PF_random() * 6);

	if (i == 1)
		player_diea1(self);
	else if (i == 2)
		player_dieb1(self);
	else if (i == 3)
		player_diec1(self);
	else if (i == 4)
		player_died1(self);
	else
		player_diee1(self);
}

void set_suicide_frame(edict_t* self)
{	// used by klill command and diconnect command
	if (strcmp(self->v.model, "progs/player.mdl"))
		return;	// allready gibbed
	self->v.frame = PlayerAnimations.FindAnimationEnd("deatha");
	self->v.solid = SOLID_NOT;
	self->v.movetype = MOVETYPE_TOSS;
	self->v.deadflag = DEAD_DEAD;
	self->v.nextthink = -1;
}

void player_diea11(edict_t* self)
{
	player_setanimation(self, "deatha", 10, &player_diea11);
	PlayerDead(self);
}

void player_diea10(edict_t* self)
{
	player_setanimation(self, "deatha", 9, &player_diea11);
}

void player_diea9(edict_t* self)
{
	player_setanimation(self, "deatha", 8, &player_diea10);
}

void player_diea8(edict_t* self)
{
	player_setanimation(self, "deatha", 7, &player_diea9);
}

void player_diea7(edict_t* self)
{
	player_setanimation(self, "deatha", 6, &player_diea8);
}

void player_diea6(edict_t* self)
{
	player_setanimation(self, "deatha", 5, &player_diea7);
}

void player_diea5(edict_t* self)
{
	player_setanimation(self, "deatha", 4, &player_diea6);
}

void player_diea4(edict_t* self)
{
	player_setanimation(self, "deatha", 3, &player_diea5);
}

void player_diea3(edict_t* self)
{
	player_setanimation(self, "deatha", 2, &player_diea4);
}

void player_diea2(edict_t* self)
{
	player_setanimation(self, "deatha", 1, &player_diea3);
}

void player_diea1(edict_t* self)
{
	player_setanimation(self, "deatha", 0, &player_diea2);
}

void player_dieb9(edict_t* self)
{
	player_setanimation(self, "deathb", 8, &player_dieb9);
	PlayerDead(self);
}

void player_dieb8(edict_t* self)
{
	player_setanimation(self, "deathb", 7, &player_dieb9);
}

void player_dieb7(edict_t* self)
{
	player_setanimation(self, "deathb", 6, &player_dieb8);
}

void player_dieb6(edict_t* self)
{
	player_setanimation(self, "deathb", 5, &player_dieb7);
}

void player_dieb5(edict_t* self)
{
	player_setanimation(self, "deathb", 4, &player_dieb6);
}

void player_dieb4(edict_t* self)
{
	player_setanimation(self, "deathb", 3, &player_dieb5);
}

void player_dieb3(edict_t* self)
{
	player_setanimation(self, "deathb", 2, &player_dieb4);
}

void player_dieb2(edict_t* self)
{
	player_setanimation(self, "deathb", 1, &player_dieb3);
}

void player_dieb1(edict_t* self)
{
	player_setanimation(self, "deathb", 0, &player_dieb2);
}

void player_diec15(edict_t* self)
{
	player_setanimation(self, "deathc", 14, &player_diec15);
	PlayerDead(self);
}

void player_diec14(edict_t* self)
{
	player_setanimation(self, "deathc", 13, &player_diec15);
}

void player_diec13(edict_t* self)
{
	player_setanimation(self, "deathc", 12, &player_diec14);
}

void player_diec12(edict_t* self)
{
	player_setanimation(self, "deathc", 11, &player_diec13);
}

void player_diec11(edict_t* self)
{
	player_setanimation(self, "deathc", 10, &player_diec12);
}

void player_diec10(edict_t* self)
{
	player_setanimation(self, "deathc", 9, &player_diec11);
}

void player_diec9(edict_t* self)
{
	player_setanimation(self, "deathc", 8, &player_diec10);
}

void player_diec8(edict_t* self)
{
	player_setanimation(self, "deathc", 7, &player_diec9);
}

void player_diec7(edict_t* self)
{
	player_setanimation(self, "deathc", 6, &player_diec8);
}

void player_diec6(edict_t* self)
{
	player_setanimation(self, "deathc", 5, &player_diec7);
}

void player_diec5(edict_t* self)
{
	player_setanimation(self, "deathc", 4, &player_diec6);
}

void player_diec4(edict_t* self)
{
	player_setanimation(self, "deathc", 3, &player_diec5);
}

void player_diec3(edict_t* self)
{
	player_setanimation(self, "deathc", 2, &player_diec4);
}

void player_diec2(edict_t* self)
{
	player_setanimation(self, "deathc", 1, &player_diec3);
}

void player_diec1(edict_t* self)
{
	player_setanimation(self, "deathc", 0, &player_diec2);
}

void player_died9(edict_t* self)
{
	player_setanimation(self, "deathd", 8, &player_died9);
	PlayerDead(self);
}

void player_died8(edict_t* self)
{
	player_setanimation(self, "deathd", 7, &player_died9);
}

void player_died7(edict_t* self)
{
	player_setanimation(self, "deathd", 6, &player_died8);
}

void player_died6(edict_t* self)
{
	player_setanimation(self, "deathd", 5, &player_died7);
}

void player_died5(edict_t* self)
{
	player_setanimation(self, "deathd", 4, &player_died6);
}

void player_died4(edict_t* self)
{
	player_setanimation(self, "deathd", 3, &player_died5);
}

void player_died3(edict_t* self)
{
	player_setanimation(self, "deathd", 2, &player_died4);
}

void player_died2(edict_t* self)
{
	player_setanimation(self, "deathd", 1, &player_died3);
}

void player_died1(edict_t* self)
{
	player_setanimation(self, "deathd", 0, &player_died2);
}

void player_diee9(edict_t* self)
{
	player_setanimation(self, "deathe", 8, &player_diee9);
	PlayerDead(self);
}

void player_diee8(edict_t* self)
{
	player_setanimation(self, "deathe", 7, &player_diee9);
}

void player_diee7(edict_t* self)
{
	player_setanimation(self, "deathe", 6, &player_diee8);
}

void player_diee6(edict_t* self)
{
	player_setanimation(self, "deathe", 5, &player_diee7);
}

void player_diee5(edict_t* self)
{
	player_setanimation(self, "deathe", 4, &player_diee6);
}

void player_diee4(edict_t* self)
{
	player_setanimation(self, "deathe", 3, &player_diee5);
}

void player_diee3(edict_t* self)
{
	player_setanimation(self, "deathe", 2, &player_diee4);
}

void player_diee2(edict_t* self)
{
	player_setanimation(self, "deathe", 1, &player_diee3);
}

void player_diee1(edict_t* self)
{
	player_setanimation(self, "deathe", 0, &player_diee2);
}

void player_die_ax9(edict_t* self)
{
	player_setanimation(self, "axdeth", 8, &player_die_ax9);
	PlayerDead(self);
}

void player_die_ax8(edict_t* self)
{
	player_setanimation(self, "axdeth", 7, &player_die_ax9);
}

void player_die_ax7(edict_t* self)
{
	player_setanimation(self, "axdeth", 6, &player_die_ax8);
}

void player_die_ax6(edict_t* self)
{
	player_setanimation(self, "axdeth", 5, &player_die_ax7);
}

void player_die_ax5(edict_t* self)
{
	player_setanimation(self, "axdeth", 4, &player_die_ax6);
}

void player_die_ax4(edict_t* self)
{
	player_setanimation(self, "axdeth", 3, &player_die_ax5);
}

void player_die_ax3(edict_t* self)
{
	player_setanimation(self, "axdeth", 2, &player_die_ax4);
}

void player_die_ax2(edict_t* self)
{
	player_setanimation(self, "axdeth", 1, &player_die_ax3);
}

void player_die_ax1(edict_t* self)
{
	player_setanimation(self, "axdeth", 0, &player_die_ax2);
}
