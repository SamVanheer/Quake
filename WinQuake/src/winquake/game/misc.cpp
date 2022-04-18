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
#include "misc.h"
#include "subs.h"
#include "weapons.h"

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
void info_null(edict_t* self)
{
	PF_Remove(self);
}

LINK_ENTITY_TO_CLASS(info_null);

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void info_notnull(edict_t* self)
{
}

LINK_ENTITY_TO_CLASS(info_notnull);

//============================================================================

constexpr int START_OFF = 1;

void light_use(edict_t* self, edict_t* other)
{
	if ((int)self->v.spawnflags & START_OFF)
	{
		PF_lightstyle(self->v.style, "m");
		self->v.spawnflags = self->v.spawnflags - START_OFF;
	}
	else
	{
		PF_lightstyle(self->v.style, "a");
		self->v.spawnflags = self->v.spawnflags + START_OFF;
	}
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
*/
void light(edict_t* self)
{
	if (!self->v.targetname)
	{	// inert light
		PF_Remove(self);
		return;
	}

	if (self->v.style >= 32)
	{
		self->v.use = light_use;
		if ((int)self->v.spawnflags & START_OFF)
			PF_lightstyle(self->v.style, "a");
		else
			PF_lightstyle(self->v.style, "m");
	}
}

LINK_ENTITY_TO_CLASS(light);

/*QUAKED light_fluoro (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
Makes steady fluorescent humming PF_sound
*/
void light_fluoro(edict_t* self)
{
	if (self->v.style >= 32)
	{
		self->v.use = light_use;
		if (self->v.spawnflags & START_OFF)
			PF_lightstyle(self->v.style, "a");
		else
			PF_lightstyle(self->v.style, "m");
	}

	PF_precache_sound("ambience/fl_hum1.wav");
	PF_ambientsound(self->v.origin, "ambience/fl_hum1.wav", 0.5, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(light_fluoro);

/*QUAKED light_fluorospark (0 1 0) (-8 -8 -8) (8 8 8)
Non-displayed light.
Default light value is 300
Default style is 10
Makes sparking, broken fluorescent PF_sound
*/
void light_fluorospark(edict_t* self)
{
	if (!self->v.style)
		self->v.style = 10;

	PF_precache_sound("ambience/buzz1.wav");
	PF_ambientsound(self->v.origin, "ambience/buzz1.wav", 0.5, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(light_fluorospark);

/*QUAKED light_globe (0 1 0) (-8 -8 -8) (8 8 8)
Sphere globe light.
Default light value is 300
Default style is 0
*/
void light_globe(edict_t* self)
{
	PF_precache_model("progs/s_light.spr");
	PF_setmodel(self, "progs/s_light.spr");
	PF_makestatic(self);
}

LINK_ENTITY_TO_CLASS(light_globe);

void FireAmbient(edict_t* self)
{
	PF_precache_sound("ambience/fire1.wav");
// attenuate fast
	PF_ambientsound(self->v.origin, "ambience/fire1.wav", 0.5, ATTN_STATIC);
}

/*QUAKED light_torch_small_walltorch (0 .5 0) (-10 -10 -20) (10 10 20)
Short wall torch
Default light value is 200
Default style is 0
*/
void light_torch_small_walltorch(edict_t* self)
{
	PF_precache_model("progs/flame.mdl");
	PF_setmodel(self, "progs/flame.mdl");
	FireAmbient(self);
	PF_makestatic(self);
}

LINK_ENTITY_TO_CLASS(light_torch_small_walltorch);

/*QUAKED light_flame_large_yellow (0 1 0) (-10 -10 -12) (12 12 18)
Large yellow flame ball
*/
void light_flame_large_yellow(edict_t* self)
{
	PF_precache_model("progs/flame2.mdl");
	PF_setmodel(self, "progs/flame2.mdl");
	self->v.frame = 1;
	FireAmbient(self);
	PF_makestatic(self);
}

LINK_ENTITY_TO_CLASS(light_flame_large_yellow);

/*QUAKED light_flame_small_yellow (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Small yellow flame ball
*/
void light_flame_small_yellow(edict_t* self)
{
	PF_precache_model("progs/flame2.mdl");
	PF_setmodel(self, "progs/flame2.mdl");
	FireAmbient(self);
	PF_makestatic(self);
}

LINK_ENTITY_TO_CLASS(light_flame_small_yellow);

/*QUAKED light_flame_small_white (0 1 0) (-10 -10 -40) (10 10 40) START_OFF
Small white flame ball
*/
void light_flame_small_white(edict_t* self)
{
	PF_precache_model("progs/flame2.mdl");
	PF_setmodel(self, "progs/flame2.mdl");
	FireAmbient(self);
	PF_makestatic(self);
}

LINK_ENTITY_TO_CLASS(light_flame_small_white);

//============================================================================


/*QUAKED misc_fireball (0 .5 .8) (-8 -8 -8) (8 8 8)
Lava Balls
*/

void fire_fly(edict_t* self);
void fire_touch(edict_t* self, edict_t* other);
void misc_fireball(edict_t* self)
{

	PF_precache_model("progs/lavaball.mdl");
	self->v.classname = "fireball";
	self->v.nextthink = pr_global_struct->time + (random() * 5);
	self->v.think = fire_fly;
	if (!self->v.speed)
		self->v.speed == 1000;
}

LINK_ENTITY_TO_CLASS(misc_fireball);

void fire_fly(edict_t* self)
{
	auto fireball = PF_Spawn();
	fireball->v.solid = SOLID_TRIGGER;
	fireball->v.movetype = MOVETYPE_TOSS;
	AsVector(fireball->v.velocity) = Vector3D{0, 0, 1000};
	fireball->v.velocity[0] = (random() * 100) - 50;
	fireball->v.velocity[1] = (random() * 100) - 50;
	fireball->v.velocity[2] = self->v.speed + (random() * 200);
	fireball->v.classname = "fireball";
	PF_setmodel(fireball, "progs/lavaball.mdl");
	PF_setsize(fireball, vec3_origin, vec3_origin);
	PF_setorigin(fireball, self->v.origin);
	fireball->v.nextthink = pr_global_struct->time + 5;
	fireball->v.think = SUB_Remove;
	fireball->v.touch = fire_touch;

	self->v.nextthink = pr_global_struct->time + (random() * 5) + 3;
	self->v.think = fire_fly;
}

void fire_touch(edict_t* self, edict_t* other)
{
	T_Damage(self, other, self, self, 20);
	PF_Remove(self);
}

//============================================================================

void barrel_explode(edict_t* self)
{
	self->v.takedamage = DAMAGE_NO;
	self->v.classname = "explo_box";
	// did say self->v.owner
	T_RadiusDamage(self, self, self, 160, pr_global_struct->world);
	PF_sound(self, CHAN_VOICE, "weapons/r_exp3.wav", 1, ATTN_NORM);
	PF_particle(self->v.origin, vec3_origin, 75, 255);

	self->v.origin[2] += 32;
	BecomeExplosion(self);
}

/*QUAKED misc_explobox (0 .5 .8) (0 0 0) (32 32 64)
TESTING THING
*/

void misc_explobox(edict_t* self)
{
	self->v.solid = SOLID_BBOX;
	self->v.movetype = MOVETYPE_NONE;
	PF_precache_model("maps/b_explob.bsp");
	PF_setmodel(self, "maps/b_explob.bsp");
	PF_precache_sound("weapons/r_exp3.wav");
	self->v.health = 20;
	self->v.th_die = barrel_explode;
	self->v.takedamage = DAMAGE_AIM;

	self->v.origin[2] += 2;
	const float oldz = self->v.origin[2];
	PF_droptofloor(self);
	if (oldz - self->v.origin[2] > 250)
	{
		dprint("item fell out of level at ");
		dprint(PF_vtos(self->v.origin));
		dprint("\n");
		PF_Remove(self);
	}
}

LINK_ENTITY_TO_CLASS(misc_explobox);

/*QUAKED misc_explobox2 (0 .5 .8) (0 0 0) (32 32 64)
Smaller exploding box, REGISTERED ONLY
*/

void misc_explobox2(edict_t* self)
{
	self->v.solid = SOLID_BBOX;
	self->v.movetype = MOVETYPE_NONE;
	PF_precache_model("maps/b_exbox2.bsp");
	PF_setmodel(self, "maps/b_exbox2.bsp");
	PF_precache_sound("weapons/r_exp3.wav");
	self->v.health = 20;
	self->v.th_die = barrel_explode;
	self->v.takedamage = DAMAGE_AIM;

	self->v.origin[2] += 2;
	const float oldz = self->v.origin[2];
	PF_droptofloor(self);
	if (oldz - self->v.origin[2] > 250)
	{
		dprint("item fell out of level at ");
		dprint(PF_vtos(self->v.origin));
		dprint("\n");
		PF_Remove(self);
	}
}

LINK_ENTITY_TO_CLASS(misc_explobox2);

//============================================================================

constexpr int SPAWNFLAG_SUPERSPIKE = 1;
constexpr int SPAWNFLAG_LASER = 2;

void LaunchLaser(edict_t* self, vec3_t org, vec3_t vec);

void LaunchLaser(edict_t* self, vec3_t org, vec3_t vec)
{
	//TODO move to enforcer.cpp
}

void spikeshooter_use(edict_t* self, edict_t* other)
{
	if (self->v.spawnflags & SPAWNFLAG_LASER)
	{
		PF_sound(self, CHAN_VOICE, "enforcer/enfire.wav", 1, ATTN_NORM);
		LaunchLaser(self, self->v.origin, self->v.movedir);
	}
	else
	{
		PF_sound(self, CHAN_VOICE, "weapons/spike2.wav", 1, ATTN_NORM);
		launch_spike(self, self->v.origin, self->v.movedir);
		AsVector(newmis->v.velocity) = AsVector(self->v.movedir) * 500;
		if (self->v.spawnflags & SPAWNFLAG_SUPERSPIKE)
			newmis->v.touch = superspike_touch;
	}
}

void shooter_think(edict_t* self)
{
	spikeshooter_use(self, nullptr);
	self->v.nextthink = pr_global_struct->time + self->v.wait;
	AsVector(newmis->v.velocity) = AsVector(self->v.movedir) * 500;
}

/*QUAKED trap_spikeshooter (0 .5 .8) (-8 -8 -8) (8 8 8) superspike laser
When triggered, fires a spike in the direction set in QuakeEd.
Laser is only for REGISTERED.
*/

void trap_spikeshooter(edict_t* self)
{
	SetMovedir(self);
	self->v.use = spikeshooter_use;
	if (self->v.spawnflags & SPAWNFLAG_LASER)
	{
		PF_precache_model("progs/laser.mdl");

		PF_precache_sound("enforcer/enfire.wav");
		PF_precache_sound("enforcer/enfstop.wav");
	}
	else
		PF_precache_sound("weapons/spike2.wav");
}

LINK_ENTITY_TO_CLASS(trap_spikeshooter);

/*QUAKED trap_shooter (0 .5 .8) (-8 -8 -8) (8 8 8) superspike laser
Continuously fires spikes.
"wait" time between spike (1.0 default)
"nextthink" delay before firing first spike, so multiple shooters can be stagered.
*/
void trap_shooter(edict_t* self)
{
	trap_spikeshooter(self);

	if (self->v.wait == 0)
		self->v.wait = 1;
	self->v.nextthink = self->v.nextthink + self->v.wait + self->v.ltime;
	self->v.think = shooter_think;
}

LINK_ENTITY_TO_CLASS(trap_shooter);

/*
===============================================================================


===============================================================================
*/


void make_bubbles(edict_t* self);
void bubble_remove(edict_t* self, edict_t* other);

/*QUAKED air_bubbles (0 .5 .8) (-8 -8 -8) (8 8 8)

testing air bubbles
*/

void air_bubbles(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model("progs/s_bubble.spr");
	self->v.nextthink = pr_global_struct->time + 1;
	self->v.think = make_bubbles;
}

LINK_ENTITY_TO_CLASS(air_bubbles);

void make_bubbles(edict_t* self)
{
	auto bubble = PF_Spawn();
	PF_setmodel(bubble, "progs/s_bubble.spr");
	PF_setorigin(bubble, self->v.origin);
	bubble->v.movetype = MOVETYPE_NOCLIP;
	bubble->v.solid = SOLID_NOT;
	AsVector(bubble->v.velocity) = Vector3D{0, 0, 15};
	bubble->v.nextthink = pr_global_struct->time + 0.5;
	bubble->v.think = bubble_bob;
	bubble->v.touch = bubble_remove;
	bubble->v.classname = "bubble";
	bubble->v.frame = 0;
	bubble->v.cnt = 0;
	PF_setsize(bubble, Vector3D{-8, -8, -8}, Vector3D{8, 8, 8});
	self->v.nextthink = pr_global_struct->time + random() + 0.5;
	self->v.think = make_bubbles;
}

void bubble_split(edict_t* self)
{
	auto bubble = PF_Spawn();
	PF_setmodel(bubble, "progs/s_bubble.spr");
	PF_setorigin(bubble, self->v.origin);
	bubble->v.movetype = MOVETYPE_NOCLIP;
	bubble->v.solid = SOLID_NOT;
	AsVector(bubble->v.velocity) = AsVector(self->v.velocity);
	bubble->v.nextthink = pr_global_struct->time + 0.5;
	bubble->v.think = bubble_bob;
	bubble->v.touch = bubble_remove;
	bubble->v.classname = "bubble";
	bubble->v.frame = 1;
	bubble->v.cnt = 10;
	PF_setsize(bubble, Vector3D{-8, -8, -8}, Vector3D{8, 8, 8});
	self->v.frame = 1;
	self->v.cnt = 10;
	if (self->v.waterlevel != 3)
		PF_Remove(self);
}

void bubble_remove(edict_t* self, edict_t* other)
{
	if (other->v.classname == self->v.classname)
	{
//		dprint ("bump");
		return;
	}
	PF_Remove(self);
}

void bubble_bob(edict_t* self)
{
	self->v.cnt = self->v.cnt + 1;
	if (self->v.cnt == 4)
		bubble_split(self);
	if (self->v.cnt == 20)
		PF_Remove(self);

	float rnd1 = self->v.velocity[0] + (-10 + (random() * 20));
	float rnd2 = self->v.velocity[1] + (-10 + (random() * 20));
	float rnd3 = self->v.velocity[2] + 10 + random() * 10;

	if (rnd1 > 10)
		rnd1 = 5;
	if (rnd1 < -10)
		rnd1 = -5;

	if (rnd2 > 10)
		rnd2 = 5;
	if (rnd2 < -10)
		rnd2 = -5;

	if (rnd3 < 10)
		rnd3 = 15;
	if (rnd3 > 30)
		rnd3 = 25;

	self->v.velocity[0] = rnd1;
	self->v.velocity[1] = rnd2;
	self->v.velocity[2] = rnd3;

	self->v.nextthink = pr_global_struct->time + 0.5;
	self->v.think = bubble_bob;
}

/*~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>
~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~*/

/*QUAKED viewthing (0 .5 .8) (-8 -8 -8) (8 8 8)

Just for the debugging level.  Don't use
*/

void viewthing(edict_t* self)

{
	self->v.movetype = MOVETYPE_NONE;
	self->v.solid = SOLID_NOT;
	PF_precache_model("progs/player.mdl");
	PF_setmodel(self, "progs/player.mdl");
}

LINK_ENTITY_TO_CLASS(viewthing);

/*
==============================================================================

SIMPLE BMODELS

==============================================================================
*/

void func_wall_use(edict_t* self, edict_t* other)
{	// change to alternate textures
	self->v.frame = 1 - self->v.frame;
}

/*QUAKED func_wall (0 .5 .8) ?
This is just a solid wall if not inhibitted
*/
void func_wall(edict_t* self)
{
	VectorCopy(vec3_origin, self->v.angles);
	self->v.movetype = MOVETYPE_PUSH;	// so it doesn't get pushed by anything
	self->v.solid = SOLID_BSP;
	self->v.use = func_wall_use;
	PF_setmodel(self, self->v.model);
}

LINK_ENTITY_TO_CLASS(func_wall);

/*QUAKED func_illusionary (0 .5 .8) ?
A simple entity that looks solid but lets you walk through it.
*/
void func_illusionary(edict_t* self)
{
	VectorCopy(vec3_origin, self->v.angles);
	self->v.movetype = MOVETYPE_NONE;
	self->v.solid = SOLID_NOT;
	PF_setmodel(self, self->v.model);
	PF_makestatic(self);
}

LINK_ENTITY_TO_CLASS(func_illusionary);

/*QUAKED func_episodegate (0 .5 .8) ? E1 E2 E3 E4
This bmodel will appear if the episode has allready been completed, so players can't reenter it.
*/
void func_episodegate(edict_t* self)
{
	if (!(pr_global_struct->serverflags & self->v.spawnflags))
		return;			// can still enter episode

	VectorCopy(vec3_origin, self->v.angles);
	self->v.movetype = MOVETYPE_PUSH;	// so it doesn't get pushed by anything
	self->v.solid = SOLID_BSP;
	self->v.use = func_wall_use;
	PF_setmodel(self, self->v.model);
}

LINK_ENTITY_TO_CLASS(func_episodegate);

/*QUAKED func_bossgate (0 .5 .8) ?
This bmodel appears unless players have all of the episode sigils.
*/
void func_bossgate(edict_t* self)
{
	if ((pr_global_struct->serverflags & 15) == 15)
		return;		// all episodes completed
	VectorCopy(vec3_origin, self->v.angles);
	self->v.movetype = MOVETYPE_PUSH;	// so it doesn't get pushed by anything
	self->v.solid = SOLID_BSP;
	self->v.use = func_wall_use;
	PF_setmodel(self, self->v.model);
}

LINK_ENTITY_TO_CLASS(func_bossgate);

//============================================================================
/*QUAKED ambient_suck_wind (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_suck_wind(edict_t* self)
{
	PF_precache_sound("ambience/suck1.wav");
	PF_ambientsound(self->v.origin, "ambience/suck1.wav", 1, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(ambient_suck_wind);

/*QUAKED ambient_drone (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_drone(edict_t* self)
{
	PF_precache_sound("ambience/drone6.wav");
	PF_ambientsound(self->v.origin, "ambience/drone6.wav", 0.5, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(ambient_drone);

/*QUAKED ambient_flouro_buzz (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_flouro_buzz(edict_t* self)
{
	PF_precache_sound("ambience/buzz1.wav");
	PF_ambientsound(self->v.origin, "ambience/buzz1.wav", 1, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(ambient_flouro_buzz);

/*QUAKED ambient_drip (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_drip(edict_t* self)
{
	PF_precache_sound("ambience/drip1.wav");
	PF_ambientsound(self->v.origin, "ambience/drip1.wav", 0.5, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(ambient_drip);

/*QUAKED ambient_comp_hum (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_comp_hum(edict_t* self)
{
	PF_precache_sound("ambience/comp1.wav");
	PF_ambientsound(self->v.origin, "ambience/comp1.wav", 1, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(ambient_comp_hum);

/*QUAKED ambient_thunder (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_thunder(edict_t* self)
{
	PF_precache_sound("ambience/thunder1.wav");
	PF_ambientsound(self->v.origin, "ambience/thunder1.wav", 0.5, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(ambient_thunder);

/*QUAKED ambient_light_buzz (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_light_buzz(edict_t* self)
{
	PF_precache_sound("ambience/fl_hum1.wav");
	PF_ambientsound(self->v.origin, "ambience/fl_hum1.wav", 0.5, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(ambient_light_buzz);

/*QUAKED ambient_swamp1 (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_swamp1(edict_t* self)
{
	PF_precache_sound("ambience/swamp1.wav");
	PF_ambientsound(self->v.origin, "ambience/swamp1.wav", 0.5, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(ambient_swamp1);

/*QUAKED ambient_swamp2 (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_swamp2(edict_t* self)
{
	PF_precache_sound("ambience/swamp2.wav");
	PF_ambientsound(self->v.origin, "ambience/swamp2.wav", 0.5, ATTN_STATIC);
}

LINK_ENTITY_TO_CLASS(ambient_swamp2);

//============================================================================

void noise_think(edict_t* self)
{
	self->v.nextthink = pr_global_struct->time + 0.5;
	PF_sound(self, 1, "enforcer/enfire.wav", 1, ATTN_NORM);
	PF_sound(self, 2, "enforcer/enfstop.wav", 1, ATTN_NORM);
	PF_sound(self, 3, "enforcer/sight1.wav", 1, ATTN_NORM);
	PF_sound(self, 4, "enforcer/sight2.wav", 1, ATTN_NORM);
	PF_sound(self, 5, "enforcer/sight3.wav", 1, ATTN_NORM);
	PF_sound(self, 6, "enforcer/sight4.wav", 1, ATTN_NORM);
	PF_sound(self, 7, "enforcer/pain1.wav", 1, ATTN_NORM);
}

/*QUAKED misc_noisemaker (1 0.5 0) (-10 -10 -10) (10 10 10)

For optimzation testing, starts a lot of sounds.
*/

void misc_noisemaker(edict_t* self)
{
	PF_precache_sound("enforcer/enfire.wav");
	PF_precache_sound("enforcer/enfstop.wav");
	PF_precache_sound("enforcer/sight1.wav");
	PF_precache_sound("enforcer/sight2.wav");
	PF_precache_sound("enforcer/sight3.wav");
	PF_precache_sound("enforcer/sight4.wav");
	PF_precache_sound("enforcer/pain1.wav");
	PF_precache_sound("enforcer/pain2.wav");
	PF_precache_sound("enforcer/death1.wav");
	PF_precache_sound("enforcer/idle1.wav");

	self->v.nextthink = pr_global_struct->time + 0.1 + random();
	self->v.think = noise_think;
}

LINK_ENTITY_TO_CLASS(misc_noisemaker);
