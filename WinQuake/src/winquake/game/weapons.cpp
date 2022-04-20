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
#include "player.h"
#include "subs.h"
#include "weapons.h"

// called by worldspawn
void W_Precache(edict_t* self)
{
	PF_precache_sound("weapons/r_exp3.wav");	// new rocket explosion
	PF_precache_sound("weapons/rocket1i.wav");	// spike gun
	PF_precache_sound("weapons/sgun1.wav");
	PF_precache_sound("weapons/guncock.wav");	// player shotgun
	PF_precache_sound("weapons/ric1.wav");	// ricochet (used in c code)
	PF_precache_sound("weapons/ric2.wav");	// ricochet (used in c code)
	PF_precache_sound("weapons/ric3.wav");	// ricochet (used in c code)
	PF_precache_sound("weapons/spike2.wav");	// super spikes
	PF_precache_sound("weapons/tink1.wav");	// spikes tink (used in c code)
	PF_precache_sound("weapons/grenade.wav");	// grenade launcher
	PF_precache_sound("weapons/bounce.wav");		// grenade bounce
	PF_precache_sound("weapons/shotgn2.wav");	// super shotgun
}

float crandom()
{
	return 2 * (random() - 0.5);
}

/*
================
W_FireAxe
================
*/
void W_FireAxe(edict_t* self)
{
	auto source = AsVector(self->v.origin) + Vector3D{0, 0, 16};
	PF_traceline(source, source + AsVector(pr_global_struct->v_forward) * 64, FALSE, self);
	if (pr_global_struct->trace_fraction == 1.0)
		return;

	auto org = AsVector(pr_global_struct->trace_endpos) - AsVector(pr_global_struct->v_forward) * 4;

	if (pr_global_struct->trace_ent->v.takedamage)
	{
		pr_global_struct->trace_ent->v.axhitme = 1;
		SpawnBlood(org, vec3_origin, 20);
		T_Damage(self, pr_global_struct->trace_ent, self, self, 20);
	}
	else
	{	// hit wall
		PF_sound(self, CHAN_WEAPON, "player/axhit2.wav", 1, ATTN_NORM);
		PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
		PF_WriteByte(MSG_BROADCAST, TE_GUNSHOT);
		PF_WriteCoord(MSG_BROADCAST, org[0]);
		PF_WriteCoord(MSG_BROADCAST, org[1]);
		PF_WriteCoord(MSG_BROADCAST, org[2]);
	}
}


//============================================================================


void wall_velocity(edict_t* self, vec3_t result)
{
	PF_normalize(self->v.velocity, result);
	PF_normalize(AsVector(result) + AsVector(pr_global_struct->v_up) * (random() - 0.5) + AsVector(pr_global_struct->v_right) * (random() - 0.5), result);
	AsVector(result) = AsVector(result) + 2 * AsVector(pr_global_struct->trace_plane_normal);
	AsVector(result) = AsVector(result) * 200;
}


/*
================
SpawnMeatSpray
================
*/
void SpawnMeatSpray(edict_t* self, vec3_t org, vec3_t vel)
{
	auto missile = PF_Spawn();
	missile->v.owner = self;
	missile->v.movetype = MOVETYPE_BOUNCE;
	missile->v.solid = SOLID_NOT;

	PF_makevectors(self->v.angles);

	VectorCopy(vel, missile->v.velocity);
	missile->v.velocity[2] = missile->v.velocity[2] + 250 + 50 * random();

	AsVector(missile->v.avelocity) = Vector3D{3000, 1000, 2000};

	// set missile duration
	missile->v.nextthink = pr_global_struct->time + 1;
	missile->v.think = SUB_Remove;

	PF_setmodel(missile, "progs/zom_gib.mdl");
	PF_setsize(missile, vec3_origin, vec3_origin);
	PF_setorigin(missile, org);
}

/*
================
SpawnBlood
================
*/
void SpawnBlood(vec3_t org, vec3_t vel, float damage)
{
	PF_particle(org, AsVector(vel) * 0.1f, 73, damage * 2);
}

/*
================
spawn_touchblood
================
*/
void spawn_touchblood(edict_t* self, float damage)
{
	vec3_t vel;
	wall_velocity(self, vel);
	AsVector(vel) = AsVector(vel) * 0.2f;
	SpawnBlood(AsVector(self->v.origin) + AsVector(vel) * 0.01f, vel, damage);
}


/*
================
SpawnChunk
================
*/
void SpawnChunk(vec3_t org, vec3_t vel)
{
	PF_particle(org, AsVector(vel) * 0.02f, 0, 10);
}

/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

edict_t* multi_ent;
float	multi_damage;

void ClearMultiDamage(edict_t* self)
{
	multi_ent = pr_global_struct->world;
	multi_damage = 0;
}

void ApplyMultiDamage(edict_t* self)
{
	if (!multi_ent)
		return;
	T_Damage(self, multi_ent, self, self, multi_damage);
}

void AddMultiDamage(edict_t* self, edict_t* hit, float damage)
{
	if (!hit)
		return;

	if (hit != multi_ent)
	{
		ApplyMultiDamage(self);
		multi_damage = damage;
		multi_ent = hit;
	}
	else
		multi_damage = multi_damage + damage;
}

/*
==============================================================================

BULLETS

==============================================================================
*/

/*
================
TraceAttack
================
*/
void TraceAttack(edict_t* self, float damage, vec3_t dir)
{
	vec3_t vel;
	PF_normalize(AsVector(dir) + AsVector(pr_global_struct->v_up) * crandom() + AsVector(pr_global_struct->v_right) * crandom(), vel);
	AsVector(vel) = AsVector(vel) + 2 * AsVector(pr_global_struct->trace_plane_normal);
	AsVector(vel) = AsVector(vel) * 200;

	auto org = AsVector(pr_global_struct->trace_endpos) - AsVector(dir) * 4;

	if (pr_global_struct->trace_ent->v.takedamage)
	{
		SpawnBlood(org, AsVector(vel) * 0.2f, damage);
		AddMultiDamage(self, pr_global_struct->trace_ent, damage);
	}
	else
	{
		PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
		PF_WriteByte(MSG_BROADCAST, TE_GUNSHOT);
		PF_WriteCoord(MSG_BROADCAST, org[0]);
		PF_WriteCoord(MSG_BROADCAST, org[1]);
		PF_WriteCoord(MSG_BROADCAST, org[2]);
	}
}

/*
================
FireBullets

Used by shotgun, super shotgun, and enemy soldier firing
Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void FireBullets(edict_t* self, float shotcount, vec3_t dir, vec3_t spread)
{
	PF_makevectors(self->v.v_angle);

	auto src = AsVector(self->v.origin) + AsVector(pr_global_struct->v_forward) * 10;
	src[2] = self->v.absmin[2] + self->v.size[2] * 0.7;

	ClearMultiDamage(self);
	Vector3D direction;
	while (shotcount > 0)
	{
		direction = AsVector(dir) + crandom() * spread[0] * AsVector(pr_global_struct->v_right) + crandom() * spread[1] * AsVector(pr_global_struct->v_up);

		PF_traceline(src, src + direction * 2048, FALSE, self);
		if (pr_global_struct->trace_fraction != 1.0)
			TraceAttack(self, 4, direction);

		shotcount = shotcount - 1;
	}
	ApplyMultiDamage(self);
}

/*
================
W_FireShotgun
================
*/
void W_FireShotgun(edict_t* self)
{
	vec3_t dir;

	PF_sound(self, CHAN_WEAPON, "weapons/guncock.wav", 1, ATTN_NORM);

	self->v.punchangle[0] = -2;

	self->v.currentammo = self->v.ammo_shells = self->v.ammo_shells - 1;
	PF_aim(self, 100000, dir);
	FireBullets(self, 6, dir, Vector3D{0.04f, 0.04f, 0});
}


/*
================
W_FireSuperShotgun
================
*/
void W_FireSuperShotgun(edict_t* self)
{
	vec3_t dir;

	if (self->v.currentammo == 1)
	{
		W_FireShotgun(self);
		return;
	}

	PF_sound(self, CHAN_WEAPON, "weapons/shotgn2.wav", 1, ATTN_NORM);

	self->v.punchangle[0] = -4;

	self->v.currentammo = self->v.ammo_shells = self->v.ammo_shells - 2;
	PF_aim(self, 100000, dir);
	FireBullets(self, 14, dir, Vector3D{0.14f, 0.08f, 0});
}


/*
==============================================================================

ROCKETS

==============================================================================
*/

void s_explode6(edict_t* self)
{
	self->v.frame = 5;
	self->v.nextthink = pr_global_struct->time + 0.1f;
	self->v.think = &SUB_Remove;
}

void s_explode5(edict_t* self)
{
	self->v.frame = 4;
	self->v.nextthink = pr_global_struct->time + 0.1f;
	self->v.think = &s_explode6;
}

void s_explode4(edict_t* self)
{
	self->v.frame = 3;
	self->v.nextthink = pr_global_struct->time + 0.1f;
	self->v.think = &s_explode5;
}

void s_explode3(edict_t* self)
{
	self->v.frame = 2;
	self->v.nextthink = pr_global_struct->time + 0.1f;
	self->v.think = &s_explode4;
}

void s_explode2(edict_t* self)
{
	self->v.frame = 1;
	self->v.nextthink = pr_global_struct->time + 0.1f;
	self->v.think = &s_explode3;
}

void s_explode1(edict_t* self)
{
	self->v.frame = 0;
	self->v.nextthink = pr_global_struct->time + 0.1f;
	self->v.think = &s_explode2;
}

void BecomeExplosion(edict_t* self)
{
	self->v.movetype = MOVETYPE_NONE;
	VectorCopy(vec3_origin, self->v.velocity);
	self->v.touch = SUB_NullTouch;
	PF_setmodel(self, "progs/s_explod.spr");
	self->v.solid = SOLID_NOT;
	s_explode1(self);
}

void T_MissileTouch(edict_t* self, edict_t* other)
{
	if (other == self->v.owner)
		return;		// don't explode on owner

	if (PF_pointcontents(self->v.origin) == CONTENT_SKY)
	{
		PF_Remove(self);
		return;
	}

	float damg = 100 + random() * 20;

	if (other->v.health)
	{
		if (!strcmp(other->v.classname, "monster_shambler"))
			damg = damg * 0.5;	// mostly immune
		T_Damage(self, other, self, self->v.owner, damg);
	}

	// don't do radius damage to the other, because all the damage
	// was done in the impact
	T_RadiusDamage(self, self, self->v.owner, 120, other);

	//	PF_sound (self, CHAN_WEAPON, "weapons/r_exp3.wav", 1, ATTN_NORM);
	vec3_t direction;
	PF_normalize(self->v.velocity, direction);
	AsVector(self->v.origin) = AsVector(self->v.origin) - 8 * AsVector(direction);

	PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte(MSG_BROADCAST, TE_EXPLOSION);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[0]);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[1]);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[2]);

	BecomeExplosion(self);
}



/*
================
W_FireRocket
================
*/
void W_FireRocket(edict_t* self)
{
	self->v.currentammo = self->v.ammo_rockets = self->v.ammo_rockets - 1;

	PF_sound(self, CHAN_WEAPON, "weapons/sgun1.wav", 1, ATTN_NORM);

	self->v.punchangle[0] = -2;

	auto missile = PF_Spawn();
	missile->v.owner = self;
	missile->v.movetype = MOVETYPE_FLYMISSILE;
	missile->v.solid = SOLID_BBOX;

	// set missile speed	

	PF_makevectors(self->v.v_angle);
	PF_aim(self, 1000, missile->v.velocity);
	AsVector(missile->v.velocity) = AsVector(missile->v.velocity) * 1000;
	PF_vectoangles(missile->v.velocity, missile->v.angles);

	missile->v.touch = T_MissileTouch;

	// set missile duration
	missile->v.nextthink = pr_global_struct->time + 5;
	missile->v.think = SUB_Remove;

	PF_setmodel(missile, "progs/missile.mdl");
	PF_setsize(missile, vec3_origin, vec3_origin);
	PF_setorigin(missile, AsVector(self->v.origin) + AsVector(pr_global_struct->v_forward) * 8 + Vector3D{0, 0, 16});
}

/*
===============================================================================

LIGHTNING

===============================================================================
*/

/*
=================
LightningDamage
=================
*/
void LightningDamage(edict_t* self, vec3_t p1, vec3_t p2, edict_t* from, float damage)
{
	//TODO: in QuakeC other is an int, so this is either world or whatever was last written to other.
	//Think functions have other set to world before being executed so this logic is probably faulty.
	auto other = pr_global_struct->world;

	auto f = AsVector(p2) - AsVector(p1);
	PF_normalize(f, f);
	f[0] = 0 - f[1];
	f[1] = f[0];
	f[2] = 0;
	f = f * 16;

	auto e1 = pr_global_struct->world;
	auto e2 = pr_global_struct->world;

	PF_traceline(p1, p2, FALSE, self);
	if (pr_global_struct->trace_ent->v.takedamage)
	{
		PF_particle(pr_global_struct->trace_endpos, Vector3D{0, 0, 100}, 225, damage * 4);
		T_Damage(self, pr_global_struct->trace_ent, from, from, damage);
		if (!strcmp(self->v.classname, "player"))
		{
			if (!strcmp(other->v.classname, "player"))
				pr_global_struct->trace_ent->v.velocity[2] = pr_global_struct->trace_ent->v.velocity[2] + 400;
		}
	}
	e1 = pr_global_struct->trace_ent;

	PF_traceline(AsVector(p1) + f, AsVector(p2) + f, FALSE, self);
	if (pr_global_struct->trace_ent != e1 && pr_global_struct->trace_ent->v.takedamage)
	{
		PF_particle(pr_global_struct->trace_endpos, Vector3D{0, 0, 100}, 225, damage * 4);
		T_Damage(self, pr_global_struct->trace_ent, from, from, damage);
	}
	e2 = pr_global_struct->trace_ent;

	PF_traceline(AsVector(p1) - f, AsVector(p2) - f, FALSE, self);
	if (pr_global_struct->trace_ent != e1 && pr_global_struct->trace_ent != e2 && pr_global_struct->trace_ent->v.takedamage)
	{
		PF_particle(pr_global_struct->trace_endpos, Vector3D{0, 0, 100}, 225, damage * 4);
		T_Damage(self, pr_global_struct->trace_ent, from, from, damage);
	}
}


void W_FireLightning(edict_t* self)
{
	if (self->v.ammo_cells < 1)
	{
		self->v.weapon = W_BestWeapon(self);
		W_SetCurrentAmmo(self);
		return;
	}

	// explode if under water
	if (self->v.waterlevel > 1)
	{
		T_RadiusDamage(self, self, self, 35 * self->v.ammo_cells, pr_global_struct->world);
		self->v.ammo_cells = 0;
		W_SetCurrentAmmo(self);
		return;
	}

	if (self->v.t_width < pr_global_struct->time)
	{
		PF_sound(self, CHAN_WEAPON, "weapons/lhit.wav", 1, ATTN_NORM);
		self->v.t_width = pr_global_struct->time + 0.6;
	}
	self->v.punchangle[0] = -2;

	self->v.currentammo = self->v.ammo_cells = self->v.ammo_cells - 1;

	auto org = AsVector(self->v.origin) + Vector3D{0, 0, 16};

	PF_traceline(org, AsVector(org) + AsVector(pr_global_struct->v_forward) * 600, TRUE, self);

	PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte(MSG_BROADCAST, TE_LIGHTNING2);
	PF_WriteEntity(MSG_BROADCAST, self);
	PF_WriteCoord(MSG_BROADCAST, org[0]);
	PF_WriteCoord(MSG_BROADCAST, org[1]);
	PF_WriteCoord(MSG_BROADCAST, org[2]);
	PF_WriteCoord(MSG_BROADCAST, pr_global_struct->trace_endpos[0]);
	PF_WriteCoord(MSG_BROADCAST, pr_global_struct->trace_endpos[1]);
	PF_WriteCoord(MSG_BROADCAST, pr_global_struct->trace_endpos[2]);

	LightningDamage(self, self->v.origin, AsVector(pr_global_struct->trace_endpos) + AsVector(pr_global_struct->v_forward) * 4, self, 30);
}


//=============================================================================


void GrenadeExplode(edict_t* self)
{
	T_RadiusDamage(self, self, self->v.owner, 120, pr_global_struct->world);

	PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte(MSG_BROADCAST, TE_EXPLOSION);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[0]);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[1]);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[2]);

	BecomeExplosion(self);
}

void GrenadeTouch(edict_t* self, edict_t* other)
{
	if (other == self->v.owner)
		return;		// don't explode on owner
	if (other->v.takedamage == DAMAGE_AIM)
	{
		GrenadeExplode(self);
		return;
	}
	PF_sound(self, CHAN_WEAPON, "weapons/bounce.wav", 1, ATTN_NORM);	// bounce sound
	if (VectorCompare(self->v.velocity, vec3_origin))
	{
		VectorCopy(vec3_origin, self->v.avelocity);
	}
}

/*
================
W_FireGrenade
================
*/
void W_FireGrenade(edict_t* self)
{
	self->v.currentammo = self->v.ammo_rockets = self->v.ammo_rockets - 1;

	PF_sound(self, CHAN_WEAPON, "weapons/grenade.wav", 1, ATTN_NORM);

	self->v.punchangle[0] = -2;

	auto missile = PF_Spawn();
	missile->v.owner = self;
	missile->v.movetype = MOVETYPE_BOUNCE;
	missile->v.solid = SOLID_BBOX;
	missile->v.classname = "grenade";

	// set missile speed	

	PF_makevectors(self->v.v_angle);

	if (self->v.v_angle[0])
		AsVector(missile->v.velocity) = AsVector(pr_global_struct->v_forward) * 600
		+ AsVector(pr_global_struct->v_up) * 200
		+ crandom() * AsVector(pr_global_struct->v_right) * 10
		+ crandom() * AsVector(pr_global_struct->v_up) * 10;
	else
	{
		PF_aim(self, 10000, missile->v.velocity);
		AsVector(missile->v.velocity) = AsVector(missile->v.velocity) * 600;
		missile->v.velocity[2] = 200;
	}

	AsVector(missile->v.avelocity) = Vector3D{300, 300, 300};

	PF_vectoangles(missile->v.velocity, missile->v.angles);

	missile->v.touch = GrenadeTouch;

	// set missile duration
	missile->v.nextthink = pr_global_struct->time + 2.5;
	missile->v.think = GrenadeExplode;

	PF_setmodel(missile, "progs/grenade.mdl");
	PF_setsize(missile, Vector3D{0, 0, 0}, Vector3D{0, 0, 0});
	PF_setorigin(missile, self->v.origin);
}


//=============================================================================

/*
===============
launch_spike

Used for both the player and the ogre
===============
*/
edict_t* launch_spike(edict_t* self, vec3_t org, vec3_t dir)
{
	auto newmis = PF_Spawn();
	newmis->v.owner = self;
	newmis->v.movetype = MOVETYPE_FLYMISSILE;
	newmis->v.solid = SOLID_BBOX;

	PF_vectoangles(dir, newmis->v.angles);

	newmis->v.touch = spike_touch;
	newmis->v.classname = "spike";
	newmis->v.think = SUB_Remove;
	newmis->v.nextthink = pr_global_struct->time + 6;
	PF_setmodel(newmis, "progs/spike.mdl");
	PF_setsize(newmis, VEC_ORIGIN, VEC_ORIGIN);
	PF_setorigin(newmis, org);

	AsVector(newmis->v.velocity) = AsVector(dir) * 1000;

	return newmis;
}

void W_FireSuperSpikes(edict_t* self)
{
	vec3_t dir;

	PF_sound(self, CHAN_WEAPON, "weapons/spike2.wav", 1, ATTN_NORM);
	self->v.attack_finished = pr_global_struct->time + 0.2;
	self->v.currentammo = self->v.ammo_nails = self->v.ammo_nails - 2;
	PF_aim(self, 1000, dir);
	auto newmis = launch_spike(self, AsVector(self->v.origin) + Vector3D{0, 0, 16}, dir);
	newmis->v.touch = superspike_touch;
	PF_setmodel(newmis, "progs/s_spike.mdl");
	PF_setsize(newmis, VEC_ORIGIN, VEC_ORIGIN);
	self->v.punchangle[0] = -2;
}

void W_FireSpikes(edict_t* self, float ox)
{
	vec3_t dir;

	PF_makevectors(self->v.v_angle);

	if (self->v.ammo_nails >= 2 && self->v.weapon == IT_SUPER_NAILGUN)
	{
		W_FireSuperSpikes(self);
		return;
	}

	if (self->v.ammo_nails < 1)
	{
		self->v.weapon = W_BestWeapon(self);
		W_SetCurrentAmmo(self);
		return;
	}

	PF_sound(self, CHAN_WEAPON, "weapons/rocket1i.wav", 1, ATTN_NORM);
	self->v.attack_finished = pr_global_struct->time + 0.2;
	self->v.currentammo = self->v.ammo_nails = self->v.ammo_nails - 1;
	PF_aim(self, 1000, dir);
	launch_spike(self, AsVector(self->v.origin) + Vector3D{0, 0, 16} + AsVector(pr_global_struct->v_right) * ox, dir);

	self->v.punchangle[0] = -2;
}

void spike_touch(edict_t* self, edict_t* other)
{
	if (other == self->v.owner)
		return;

	if (other->v.solid == SOLID_TRIGGER)
		return;	// trigger field, do nothing

	if (PF_pointcontents(self->v.origin) == CONTENT_SKY)
	{
		PF_Remove(self);
		return;
	}

	// hit something that bleeds
	if (other->v.takedamage)
	{
		spawn_touchblood(self, 9);
		T_Damage(self, other, self, self->v.owner, 9);
	}
	else
	{
		PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);

		if (!strcmp(self->v.classname, "wizspike"))
			PF_WriteByte(MSG_BROADCAST, TE_WIZSPIKE);
		else if (!strcmp(self->v.classname, "knightspike"))
			PF_WriteByte(MSG_BROADCAST, TE_KNIGHTSPIKE);
		else
			PF_WriteByte(MSG_BROADCAST, TE_SPIKE);
		PF_WriteCoord(MSG_BROADCAST, self->v.origin[0]);
		PF_WriteCoord(MSG_BROADCAST, self->v.origin[1]);
		PF_WriteCoord(MSG_BROADCAST, self->v.origin[2]);
	}

	PF_Remove(self);

}

void superspike_touch(edict_t* self, edict_t* other)
{
	if (other == self->v.owner)
		return;

	if (other->v.solid == SOLID_TRIGGER)
		return;	// trigger field, do nothing

	if (PF_pointcontents(self->v.origin) == CONTENT_SKY)
	{
		PF_Remove(self);
		return;
	}

	// hit something that bleeds
	if (other->v.takedamage)
	{
		spawn_touchblood(self, 18);
		T_Damage(self, other, self, self->v.owner, 18);
	}
	else
	{
		PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
		PF_WriteByte(MSG_BROADCAST, TE_SUPERSPIKE);
		PF_WriteCoord(MSG_BROADCAST, self->v.origin[0]);
		PF_WriteCoord(MSG_BROADCAST, self->v.origin[1]);
		PF_WriteCoord(MSG_BROADCAST, self->v.origin[2]);
	}

	PF_Remove(self);

}


/*
===============================================================================

PLAYER WEAPON USE

===============================================================================
*/

void W_SetCurrentAmmo(edict_t* self)
{
	player_run(self);		// get out of any weapon firing states

	self->v.items &= ~(IT_SHELLS | IT_NAILS | IT_ROCKETS | IT_CELLS);

	if (self->v.weapon == IT_AXE)
	{
		self->v.currentammo = 0;
		self->v.weaponmodel = "progs/v_axe.mdl";
		self->v.weaponframe = 0;
	}
	else if (self->v.weapon == IT_SHOTGUN)
	{
		self->v.currentammo = self->v.ammo_shells;
		self->v.weaponmodel = "progs/v_shot.mdl";
		self->v.weaponframe = 0;
		self->v.items = self->v.items | IT_SHELLS;
	}
	else if (self->v.weapon == IT_SUPER_SHOTGUN)
	{
		self->v.currentammo = self->v.ammo_shells;
		self->v.weaponmodel = "progs/v_shot2.mdl";
		self->v.weaponframe = 0;
		self->v.items = self->v.items | IT_SHELLS;
	}
	else if (self->v.weapon == IT_NAILGUN)
	{
		self->v.currentammo = self->v.ammo_nails;
		self->v.weaponmodel = "progs/v_nail.mdl";
		self->v.weaponframe = 0;
		self->v.items = self->v.items | IT_NAILS;
	}
	else if (self->v.weapon == IT_SUPER_NAILGUN)
	{
		self->v.currentammo = self->v.ammo_nails;
		self->v.weaponmodel = "progs/v_nail2.mdl";
		self->v.weaponframe = 0;
		self->v.items = self->v.items | IT_NAILS;
	}
	else if (self->v.weapon == IT_GRENADE_LAUNCHER)
	{
		self->v.currentammo = self->v.ammo_rockets;
		self->v.weaponmodel = "progs/v_rock.mdl";
		self->v.weaponframe = 0;
		self->v.items = self->v.items | IT_ROCKETS;
	}
	else if (self->v.weapon == IT_ROCKET_LAUNCHER)
	{
		self->v.currentammo = self->v.ammo_rockets;
		self->v.weaponmodel = "progs/v_rock2.mdl";
		self->v.weaponframe = 0;
		self->v.items = self->v.items | IT_ROCKETS;
	}
	else if (self->v.weapon == IT_LIGHTNING)
	{
		self->v.currentammo = self->v.ammo_cells;
		self->v.weaponmodel = "progs/v_light.mdl";
		self->v.weaponframe = 0;
		self->v.items = self->v.items | IT_CELLS;
	}
	else
	{
		self->v.currentammo = 0;
		self->v.weaponmodel = "";
		self->v.weaponframe = 0;
	}
}

int W_BestWeapon(edict_t* self)
{
	int it = self->v.items;

	if (self->v.ammo_cells >= 1 && (it & IT_LIGHTNING))
		return IT_LIGHTNING;
	else if (self->v.ammo_nails >= 2 && (it & IT_SUPER_NAILGUN))
		return IT_SUPER_NAILGUN;
	else if (self->v.ammo_shells >= 2 && (it & IT_SUPER_SHOTGUN))
		return IT_SUPER_SHOTGUN;
	else if (self->v.ammo_nails >= 1 && (it & IT_NAILGUN))
		return IT_NAILGUN;
	else if (self->v.ammo_shells >= 1 && (it & IT_SHOTGUN))
		return IT_SHOTGUN;

	/*
		if(self->v.ammo_rockets >= 1 && (it & IT_ROCKET_LAUNCHER) )
			return IT_ROCKET_LAUNCHER;
		else if(self->v.ammo_rockets >= 1 && (it & IT_GRENADE_LAUNCHER) )
			return IT_GRENADE_LAUNCHER;

	*/

	return IT_AXE;
}

bool W_CheckNoAmmo(edict_t* self)
{
	if (self->v.currentammo > 0)
		return true;

	if (self->v.weapon == IT_AXE)
		return true;

	self->v.weapon = W_BestWeapon(self);

	W_SetCurrentAmmo(self);

	// drop the weapon down
	return false;
}

/*
============
W_Attack

An attack impulse can be triggered now
============
*/
void W_Attack(edict_t* self)
{
	if (!W_CheckNoAmmo(self))
		return;

	PF_makevectors(self->v.v_angle);			// calculate forward angle for velocity
	self->v.show_hostile = pr_global_struct->time + 1;	// wake monsters up

	if (self->v.weapon == IT_AXE)
	{
		PF_sound(self, CHAN_WEAPON, "weapons/ax1.wav", 1, ATTN_NORM);
		const float r = random();
		if (r < 0.25)
			player_axe1(self);
		else if (r < 0.5)
			player_axeb1(self);
		else if (r < 0.75)
			player_axec1(self);
		else
			player_axed1(self);
		self->v.attack_finished = pr_global_struct->time + 0.5;
	}
	else if (self->v.weapon == IT_SHOTGUN)
	{
		player_shot1(self);
		W_FireShotgun(self);
		self->v.attack_finished = pr_global_struct->time + 0.5;
	}
	else if (self->v.weapon == IT_SUPER_SHOTGUN)
	{
		player_shot1(self);
		W_FireSuperShotgun(self);
		self->v.attack_finished = pr_global_struct->time + 0.7;
	}
	else if (self->v.weapon == IT_NAILGUN)
	{
		player_nail1(self);
	}
	else if (self->v.weapon == IT_SUPER_NAILGUN)
	{
		player_nail1(self);
	}
	else if (self->v.weapon == IT_GRENADE_LAUNCHER)
	{
		player_rocket1(self);
		W_FireGrenade(self);
		self->v.attack_finished = pr_global_struct->time + 0.6;
	}
	else if (self->v.weapon == IT_ROCKET_LAUNCHER)
	{
		player_rocket1(self);
		W_FireRocket(self);
		self->v.attack_finished = pr_global_struct->time + 0.8;
	}
	else if (self->v.weapon == IT_LIGHTNING)
	{
		player_light1(self);
		self->v.attack_finished = pr_global_struct->time + 0.1;
		PF_sound(self, CHAN_AUTO, "weapons/lstart.wav", 1, ATTN_NORM);
	}
}

/*
============
W_ChangeWeapon

============
*/
void W_ChangeWeapon(edict_t* self)
{
	int fl = 0;

	float am = 0;

	if (self->v.impulse == 1)
	{
		fl = IT_AXE;
	}
	else if (self->v.impulse == 2)
	{
		fl = IT_SHOTGUN;
		if (self->v.ammo_shells < 1)
			am = 1;
	}
	else if (self->v.impulse == 3)
	{
		fl = IT_SUPER_SHOTGUN;
		if (self->v.ammo_shells < 2)
			am = 1;
	}
	else if (self->v.impulse == 4)
	{
		fl = IT_NAILGUN;
		if (self->v.ammo_nails < 1)
			am = 1;
	}
	else if (self->v.impulse == 5)
	{
		fl = IT_SUPER_NAILGUN;
		if (self->v.ammo_nails < 2)
			am = 1;
	}
	else if (self->v.impulse == 6)
	{
		fl = IT_GRENADE_LAUNCHER;
		if (self->v.ammo_rockets < 1)
			am = 1;
	}
	else if (self->v.impulse == 7)
	{
		fl = IT_ROCKET_LAUNCHER;
		if (self->v.ammo_rockets < 1)
			am = 1;
	}
	else if (self->v.impulse == 8)
	{
		fl = IT_LIGHTNING;
		if (self->v.ammo_cells < 1)
			am = 1;
	}

	self->v.impulse = 0;

	if (!(self->v.items & fl))
	{	// don't have the weapon or the ammo
		PF_sprint(self, "no weapon.\n");
		return;
	}

	if (am)
	{	// don't have the ammo
		PF_sprint(self, "not enough ammo.\n");
		return;
	}

	//
	// set weapon, set ammo
	//
	self->v.weapon = fl;
	W_SetCurrentAmmo(self);
}

/*
============
CheatCommand
============
*/
void CheatCommand(edict_t* self)
{
	if (pr_global_struct->deathmatch || pr_global_struct->coop)
		return;

	self->v.ammo_rockets = 100;
	self->v.ammo_nails = 200;
	self->v.ammo_shells = 100;
	self->v.items = self->v.items |
		IT_AXE |
		IT_SHOTGUN |
		IT_SUPER_SHOTGUN |
		IT_NAILGUN |
		IT_SUPER_NAILGUN |
		IT_GRENADE_LAUNCHER |
		IT_ROCKET_LAUNCHER |
		IT_KEY1 | IT_KEY2;

	self->v.ammo_cells = 200;
	self->v.items = self->v.items | IT_LIGHTNING;

	self->v.weapon = IT_ROCKET_LAUNCHER;
	self->v.impulse = 0;
	W_SetCurrentAmmo(self);
}

/*
============
CycleWeaponCommand

Go to the next weapon with ammo
============
*/
void CycleWeaponCommand(edict_t* self)
{
	int am;

	self->v.impulse = 0;

	while (1)
	{
		am = 0;

		if (self->v.weapon == IT_LIGHTNING)
		{
			self->v.weapon = IT_AXE;
		}
		else if (self->v.weapon == IT_AXE)
		{
			self->v.weapon = IT_SHOTGUN;
			if (self->v.ammo_shells < 1)
				am = 1;
		}
		else if (self->v.weapon == IT_SHOTGUN)
		{
			self->v.weapon = IT_SUPER_SHOTGUN;
			if (self->v.ammo_shells < 2)
				am = 1;
		}
		else if (self->v.weapon == IT_SUPER_SHOTGUN)
		{
			self->v.weapon = IT_NAILGUN;
			if (self->v.ammo_nails < 1)
				am = 1;
		}
		else if (self->v.weapon == IT_NAILGUN)
		{
			self->v.weapon = IT_SUPER_NAILGUN;
			if (self->v.ammo_nails < 2)
				am = 1;
		}
		else if (self->v.weapon == IT_SUPER_NAILGUN)
		{
			self->v.weapon = IT_GRENADE_LAUNCHER;
			if (self->v.ammo_rockets < 1)
				am = 1;
		}
		else if (self->v.weapon == IT_GRENADE_LAUNCHER)
		{
			self->v.weapon = IT_ROCKET_LAUNCHER;
			if (self->v.ammo_rockets < 1)
				am = 1;
		}
		else if (self->v.weapon == IT_ROCKET_LAUNCHER)
		{
			self->v.weapon = IT_LIGHTNING;
			if (self->v.ammo_cells < 1)
				am = 1;
		}

		if ((self->v.items & self->v.weapon) && am == 0)
		{
			W_SetCurrentAmmo(self);
			return;
		}
	}

}

/*
============
ServerflagsCommand

Just for development
============
*/
void ServerflagsCommand(edict_t* self)
{
	pr_global_struct->serverflags = pr_global_struct->serverflags * 2 + 1;
}

void QuadCheat(edict_t* self)
{
	if (pr_global_struct->deathmatch || pr_global_struct->coop)
		return;
	self->v.super_time = 1;
	self->v.super_damage_finished = pr_global_struct->time + 30;
	self->v.items = self->v.items | IT_QUAD;
	dprint("quad cheat\n");
}

/*
============
ImpulseCommands

============
*/
void ImpulseCommands(edict_t* self)
{
	if (self->v.impulse >= 1 && self->v.impulse <= 8)
		W_ChangeWeapon(self);

	if (self->v.impulse == 9)
		CheatCommand(self);
	if (self->v.impulse == 10)
		CycleWeaponCommand(self);
	if (self->v.impulse == 11)
		ServerflagsCommand(self);

	if (self->v.impulse == 255)
		QuadCheat(self);

	self->v.impulse = 0;
}

/*
============
W_WeaponFrame

Called every frame so impulse events can be handled as well as possible
============
*/
void W_WeaponFrame(edict_t* self)
{
	if (pr_global_struct->time < self->v.attack_finished)
		return;

	ImpulseCommands(self);

	// check for attack
	if (self->v.button0)
	{
		SuperDamageSound(self);
		W_Attack(self);
	}
}

/*
========
SuperDamageSound

Plays PF_sound if needed
========
*/
void SuperDamageSound(edict_t* self)
{
	if (self->v.super_damage_finished > pr_global_struct->time)
	{
		if (self->v.super_sound < pr_global_struct->time)
		{
			self->v.super_sound = pr_global_struct->time + 1;
			PF_sound(self, CHAN_BODY, "items/damage3.wav", 1, ATTN_NORM);
		}
	}
	return;
}
