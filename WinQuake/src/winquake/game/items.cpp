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
#include "items.h"
#include "subs.h"
#include "weapons.h"

/* ALL LIGHTS SHOULD BE 0 1 0 IN COLOR ALL OTHER ITEMS SHOULD
BE .8 .3 .4 IN COLOR */


void SUB_regen(edict_t* self)
{
	self->v.model = self->v.mdl;		// restore original model
	self->v.solid = SOLID_TRIGGER;	// allow it to be touched again
	PF_sound (self, CHAN_VOICE, "items/itembk2.wav", 1, ATTN_NORM);	// play respawn sound
	PF_setorigin (self, self->v.origin);
}



/*QUAKED noclass (0 0 0) (-8 -8 -8) (8 8 8)
prints a warning message when spawned
*/
void noclass(edict_t* self)
{
	dprint ("noclass spawned at");
	dprint (PF_vtos(self->v.origin));
	dprint ("\n");
	PF_Remove (self);
}

LINK_ENTITY_TO_CLASS(noclass);

/*
============
PlaceItem

plants the object on the floor
============
*/
void PlaceItem(edict_t* self)
{
	self->v.mdl = self->v.model;		// so it can be restored on respawn
	self->v.flags = FL_ITEM;		// make extra wide
	self->v.solid = SOLID_TRIGGER;
	self->v.movetype = MOVETYPE_TOSS;
	VectorCopy(vec3_origin, self->v.velocity);
	self->v.origin[2] += 6;
	if (!PF_droptofloor(self))
	{
		dprint ("Bonus item fell out of level at ");
		dprint (PF_vtos(self->v.origin));
		dprint ("\n");
		PF_Remove(self);
		return;
	}
}

/*
============
StartItem

Sets the clipping size and plants the object on the floor
============
*/
void StartItem(edict_t* self)
{
	self->v.nextthink = pr_global_struct->time + 0.2;	// items start after other solids
	self->v.think = PlaceItem;
}

/*
=========================================================================

HEALTH BOX

=========================================================================
*/
//
// T_Heal: add health to an entity, limiting health to max_health
// "ignore" will ignore max_health limit
//
bool T_Heal(edict_t* self, edict_t* e, float healamount, float ignore)
{
	if (e->v.health <= 0)
		return false;
	if ((!ignore) && (e->v.health >= e->v.max_health))
		return false;
	healamount = ceil(healamount);

	e->v.health = e->v.health + healamount;
	if ((!ignore) && (e->v.health >= e->v.max_health))
		e->v.health = e->v.max_health;
		
	if (e->v.health > 250)
		e->v.health = 250;
	return true;
}

/*QUAKED item_health (.3 .3 1) (0 0 0) (32 32 32) rotten megahealth
Health box. Normally gives 25 points.
Rotten box heals 5-10 points,
megahealth will add 100 health, then 
rot you down to your maximum health limit, 
one point per second.
*/

constexpr int H_ROTTEN = 1;
constexpr int H_MEGA = 2;

void health_touch(edict_t* self, edict_t* other);
void item_megahealth_rot(edict_t* self);

void item_health(edict_t* self)
{	
	self->v.touch = health_touch;

	if (self->v.spawnflags & H_ROTTEN)
	{
		PF_precache_model("maps/b_bh10.bsp");

		PF_precache_sound("items/r_item1.wav");
		PF_setmodel(self, "maps/b_bh10.bsp");
		self->v.noise = "items/r_item1.wav";
		self->v.healamount = 15;
		self->v.healtype = 0;
	}
	else
	if (self->v.spawnflags & H_MEGA)
	{
		PF_precache_model("maps/b_bh100.bsp");
		PF_precache_sound("items/r_item2.wav");
		PF_setmodel(self, "maps/b_bh100.bsp");
		self->v.noise = "items/r_item2.wav";
		self->v.healamount = 100;
		self->v.healtype = 2;
	}
	else
	{
		PF_precache_model("maps/b_bh25.bsp");
		PF_precache_sound("items/health1.wav");
		PF_setmodel(self, "maps/b_bh25.bsp");
		self->v.noise = "items/health1.wav";
		self->v.healamount = 25;
		self->v.healtype = 1;
	}
	PF_setsize(self, Vector3D{0, 0, 0}, Vector3D{32, 32, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_health);

void health_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	
	if (self->v.healtype == 2) // Megahealth?  Ignore max_health...
	{
		if (other->v.health >= 250)
			return;
		if (!T_Heal(self, other, self->v.healamount, 1))
			return;
	}
	else
	{
		if (!T_Heal(self, other, self->v.healamount, 0))
			return;
	}
	
	PF_sprint(other, "You receive ");
	auto s = PF_ftos(self->v.healamount);
	PF_sprint(other, s);
	PF_sprint(other, " health\n");
	
// health touch sound
	PF_sound(other, CHAN_ITEM, self->v.noise, 1, ATTN_NORM);

	PF_stuffcmd (other, "bf\n");
	
	self->v.model = nullptr;
	self->v.solid = SOLID_NOT;

	// Megahealth = rot down the player's super health
	if (self->v.healtype == 2)
	{
		other->v.items = other->v.items | IT_SUPERHEALTH;
		self->v.nextthink = pr_global_struct->time + 5;
		self->v.think = item_megahealth_rot;
		self->v.owner = other;
	}
	else
	{
		if (pr_global_struct->deathmatch != 2)		// deathmatch 2 is the silly old rules
		{
			if (pr_global_struct->deathmatch)
				self->v.nextthink = pr_global_struct->time + 20;
			self->v.think = SUB_regen;
		}
	}
	
	activator = other;
	SUB_UseTargets(self);				// fire all targets / killtargets
}	

void item_megahealth_rot(edict_t* self)
{
	auto other = self->v.owner;
	
	if (other->v.health > other->v.max_health)
	{
		other->v.health = other->v.health - 1;
		self->v.nextthink = pr_global_struct->time + 1;
		return;
	}

// it is possible for a player to die and respawn between rots, so don't
// just blindly subtract the flag off
	other->v.items = other->v.items - (other->v.items & IT_SUPERHEALTH);
	
	if (pr_global_struct->deathmatch == 1)	// deathmatch 2 is silly old rules
	{
		self->v.nextthink = pr_global_struct->time + 20;
		self->v.think = SUB_regen;
	}
}

/*
===============================================================================

ARMOR

===============================================================================
*/

void armor_touch(edict_t* self, edict_t* other)
{
	if (other->v.health <= 0)
		return;
	if (strcmp(other->v.classname, "player"))
		return;

	float type = 0, value = 0, bit = 0;

	if (!strcmp(self->v.classname, "item_armor1"))
	{
		type = 0.3f;
		value = 100;
		bit = IT_ARMOR1;
	}
	if (!strcmp(self->v.classname, "item_armor2"))
	{
		type = 0.6f;
		value = 150;
		bit = IT_ARMOR2;
	}
	if (!strcmp(self->v.classname, "item_armorInv"))
	{
		type = 0.8f;
		value = 200;
		bit = IT_ARMOR3;
	}
	if (other->v.armortype*other->v.armorvalue >= type*value)
		return;
		
	other->v.armortype = type;
	other->v.armorvalue = value;
	other->v.items = other->v.items - (other->v.items & (IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)) + bit;

	self->v.solid = SOLID_NOT;
	self->v.model = nullptr;
	if (pr_global_struct->deathmatch == 1)
		self->v.nextthink = pr_global_struct->time + 20;
	self->v.think = SUB_regen;

	PF_sprint(other, "You got armor\n");
// armor touch sound
	PF_sound(other, CHAN_ITEM, "items/armor1.wav", 1, ATTN_NORM);
	PF_stuffcmd (other, "bf\n");
	
	activator = other;
	SUB_UseTargets(self);				// fire all targets / killtargets
}


/*QUAKED item_armor1 (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void item_armor1(edict_t* self)
{
	self->v.touch = armor_touch;
	PF_precache_model ("progs/armor.mdl");
	PF_setmodel (self, "progs/armor.mdl");
	self->v.skin = 0;
	PF_setsize(self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_armor1);

/*QUAKED item_armor2 (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void item_armor2(edict_t* self)
{
	self->v.touch = armor_touch;
	PF_precache_model ("progs/armor.mdl");
	PF_setmodel (self, "progs/armor.mdl");
	self->v.skin = 1;
	PF_setsize (self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_armor2);

/*QUAKED item_armorInv (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void item_armorInv(edict_t* self)
{
	self->v.touch = armor_touch;
	PF_precache_model ("progs/armor.mdl");
	PF_setmodel (self, "progs/armor.mdl");
	self->v.skin = 2;
	PF_setsize (self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_armorInv);

/*
===============================================================================

WEAPONS

===============================================================================
*/

void bound_other_ammo(edict_t* self, edict_t* other)
{
	if (other->v.ammo_shells > 100)
		other->v.ammo_shells = 100;
	if (other->v.ammo_nails > 200)
		other->v.ammo_nails = 200;
	if (other->v.ammo_rockets > 100)
		other->v.ammo_rockets = 100;		
	if (other->v.ammo_cells > 100)
		other->v.ammo_cells = 100;		
}


float RankForWeapon(edict_t* self, float w)
{
	if (w == IT_LIGHTNING)
		return 1;
	if (w == IT_ROCKET_LAUNCHER)
		return 2;
	if (w == IT_SUPER_NAILGUN)
		return 3;
	if (w == IT_GRENADE_LAUNCHER)
		return 4;
	if (w == IT_SUPER_SHOTGUN)
		return 5;
	if (w == IT_NAILGUN)
		return 6;
	return 7;
}

/*
=============
Deathmatch_Weapon

Deathmatch weapon change rules for picking up a weapon

.float		ammo_shells, ammo_nails, ammo_rockets, ammo_cells;
=============
*/
void Deathmatch_Weapon(edict_t* self, float old, float newWeapon)
{
// change self->v.weapon if desired
	const float or = RankForWeapon (self, self->v.weapon);
	const float nr = RankForWeapon (self, newWeapon);
	if ( nr < or )
		self->v.weapon = newWeapon;
}

/*
=============
weapon_touch
=============
*/
void weapon_touch(edict_t* self, edict_t* other)
{
	float	hadammo;
	int newWeapon = 0;

	if (!(other->v.flags & FL_CLIENT))
		return;

// if the player was using his best weapon, change up to the new one if better		
	auto stemp = self;
	self = other;
	/*float best = */W_BestWeapon(self);
	self = stemp;

	const bool leave = pr_global_struct->deathmatch == 2 || pr_global_struct->coop;
	
	if (!strcmp(self->v.classname, "weapon_nailgun"))
	{
		if (leave && (other->v.items & IT_NAILGUN) )
			return;
		hadammo = other->v.ammo_nails;			
		newWeapon = IT_NAILGUN;
		other->v.ammo_nails = other->v.ammo_nails + 30;
	}
	else if (!strcmp(self->v.classname, "weapon_supernailgun"))
	{
		if (leave && (other->v.items & IT_SUPER_NAILGUN) )
			return;
		hadammo = other->v.ammo_rockets;			
		newWeapon = IT_SUPER_NAILGUN;
		other->v.ammo_nails = other->v.ammo_nails + 30;
	}
	else if (!strcmp(self->v.classname, "weapon_supershotgun"))
	{
		if (leave && (other->v.items & IT_SUPER_SHOTGUN) )
			return;
		hadammo = other->v.ammo_rockets;			
		newWeapon = IT_SUPER_SHOTGUN;
		other->v.ammo_shells = other->v.ammo_shells + 5;
	}
	else if (!strcmp(self->v.classname, "weapon_rocketlauncher"))
	{
		if (leave && (other->v.items & IT_ROCKET_LAUNCHER) )
			return;
		hadammo = other->v.ammo_rockets;			
		newWeapon = IT_ROCKET_LAUNCHER;
		other->v.ammo_rockets = other->v.ammo_rockets + 5;
	}
	else if (!strcmp(self->v.classname, "weapon_grenadelauncher"))
	{
		if (leave && (other->v.items & IT_GRENADE_LAUNCHER) )
			return;
		hadammo = other->v.ammo_rockets;			
		newWeapon = IT_GRENADE_LAUNCHER;
		other->v.ammo_rockets = other->v.ammo_rockets + 5;
	}
	else if (!strcmp(self->v.classname, "weapon_lightning"))
	{
		if (leave && (other->v.items & IT_LIGHTNING) )
			return;
		hadammo = other->v.ammo_rockets;			
		newWeapon = IT_LIGHTNING;
		other->v.ammo_cells = other->v.ammo_cells + 15;
	}
	else
		PF_objerror ("weapon_touch: unknown classname");

	PF_sprint (other, "You got the ");
	PF_sprint (other, self->v.netname);
	PF_sprint (other, "\n");
// weapon touch sound
	PF_sound (other, CHAN_ITEM, "weapons/pkup.wav", 1, ATTN_NORM);
	PF_stuffcmd (other, "bf\n");

	bound_other_ammo (self, other);

// change to the weapon
	float old = other->v.items;
	other->v.items = other->v.items | newWeapon;
	
	stemp = self;
	self = other;

	if (!pr_global_struct->deathmatch)
		self->v.weapon = newWeapon;
	else
		Deathmatch_Weapon (self, old, newWeapon);

	W_SetCurrentAmmo(self);

	self = stemp;

	if (leave)
		return;

// PF_Remove it in single player, or setup for respawning in deathmatch
	self->v.model = nullptr;
	self->v.solid = SOLID_NOT;
	if (pr_global_struct->deathmatch == 1)
		self->v.nextthink = pr_global_struct->time + 30;
	self->v.think = SUB_regen;
	
	activator = other;
	SUB_UseTargets(self);				// fire all targets / killtargets
}


/*QUAKED weapon_supershotgun (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void weapon_supershotgun(edict_t* self)
{
	PF_precache_model ("progs/g_shot.mdl");
	PF_setmodel (self, "progs/g_shot.mdl");
	self->v.weapon = IT_SUPER_SHOTGUN;
	self->v.netname = "Double-barrelled Shotgun";
	self->v.touch = weapon_touch;
	PF_setsize (self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(weapon_supershotgun);

/*QUAKED weapon_nailgun (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void weapon_nailgun(edict_t* self)
{
	PF_precache_model ("progs/g_nail.mdl");
	PF_setmodel (self, "progs/g_nail.mdl");
	self->v.weapon = IT_NAILGUN;
	self->v.netname = "nailgun";
	self->v.touch = weapon_touch;
	PF_setsize (self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(weapon_nailgun);

/*QUAKED weapon_supernailgun (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void weapon_supernailgun(edict_t* self)
{
	PF_precache_model ("progs/g_nail2.mdl");
	PF_setmodel (self, "progs/g_nail2.mdl");
	self->v.weapon = IT_SUPER_NAILGUN;
	self->v.netname = "Super Nailgun";
	self->v.touch = weapon_touch;
	PF_setsize (self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(weapon_supernailgun);

/*QUAKED weapon_grenadelauncher (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void weapon_grenadelauncher(edict_t* self)
{
	PF_precache_model ("progs/g_rock.mdl");
	PF_setmodel (self, "progs/g_rock.mdl");
	self->v.weapon = 3;
	self->v.netname = "Grenade Launcher";
	self->v.touch = weapon_touch;
	PF_setsize (self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(weapon_grenadelauncher);

/*QUAKED weapon_rocketlauncher (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void weapon_rocketlauncher(edict_t* self)
{
	PF_precache_model ("progs/g_rock2.mdl");
	PF_setmodel (self, "progs/g_rock2.mdl");
	self->v.weapon = 3;
	self->v.netname = "Rocket Launcher";
	self->v.touch = weapon_touch;
	PF_setsize (self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(weapon_rocketlauncher);

/*QUAKED weapon_lightning (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void weapon_lightning(edict_t* self)
{
	PF_precache_model ("progs/g_light.mdl");
	PF_setmodel (self, "progs/g_light.mdl");
	self->v.weapon = 3;
	self->v.netname = "Thunderbolt";
	self->v.touch = weapon_touch;
	PF_setsize (self, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(weapon_lightning);

/*
===============================================================================

AMMO

===============================================================================
*/

void ammo_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	if (other->v.health <= 0)
		return;

// if the player was using his best weapon, change up to the new one if better		
	const float best = W_BestWeapon(other);


// shotgun
	if (self->v.weapon == 1)
	{
		if (other->v.ammo_shells >= 100)
			return;
		other->v.ammo_shells = other->v.ammo_shells + self->v.aflag;
	}

// spikes
	if (self->v.weapon == 2)
	{
		if (other->v.ammo_nails >= 200)
			return;
		other->v.ammo_nails = other->v.ammo_nails + self->v.aflag;
	}

//	rockets
	if (self->v.weapon == 3)
	{
		if (other->v.ammo_rockets >= 100)
			return;
		other->v.ammo_rockets = other->v.ammo_rockets + self->v.aflag;
	}

//	cells
	if (self->v.weapon == 4)
	{
		if (other->v.ammo_cells >= 200)
			return;
		other->v.ammo_cells = other->v.ammo_cells + self->v.aflag;
	}

	bound_other_ammo (self, other);
	
	PF_sprint (other, "You got the ");
	PF_sprint (other, self->v.netname);
	PF_sprint (other, "\n");
// ammo touch sound
	PF_sound (other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM);
	PF_stuffcmd (other, "bf\n");

// change to a better weapon if appropriate

	if ( other->v.weapon == best )
	{
		other->v.weapon = W_BestWeapon(other);
		W_SetCurrentAmmo (other);
	}

// if changed current ammo, update it
	W_SetCurrentAmmo(other);

// PF_Remove it in single player, or setup for respawning in deathmatch
	self->v.model = nullptr;
	self->v.solid = SOLID_NOT;
	if (pr_global_struct->deathmatch == 1)
		self->v.nextthink = pr_global_struct->time + 30;
	
	self->v.think = SUB_regen;

	activator = other;
	SUB_UseTargets(self);				// fire all targets / killtargets
}




constexpr int WEAPON_BIG2 = 1;

/*QUAKED item_shells (0 .5 .8) (0 0 0) (32 32 32) big
*/

void item_shells(edict_t* self)
{
	self->v.touch = ammo_touch;

	if (self->v.spawnflags & WEAPON_BIG2)
	{
		PF_precache_model ("maps/b_shell1.bsp");
		PF_setmodel (self, "maps/b_shell1.bsp");
		self->v.aflag = 40;
	}
	else
	{
		PF_precache_model ("maps/b_shell0.bsp");
		PF_setmodel (self, "maps/b_shell0.bsp");
		self->v.aflag = 20;
	}
	self->v.weapon = 1;
	self->v.netname = "shells";
	PF_setsize (self, vec3_origin, Vector3D{32, 32, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_shells);

/*QUAKED item_spikes (0 .5 .8) (0 0 0) (32 32 32) big
*/

void item_spikes(edict_t* self)
{
	self->v.touch = ammo_touch;

	if (self->v.spawnflags & WEAPON_BIG2)
	{
		PF_precache_model ("maps/b_nail1.bsp");
		PF_setmodel (self, "maps/b_nail1.bsp");
		self->v.aflag = 50;
	}
	else
	{
		PF_precache_model ("maps/b_nail0.bsp");
		PF_setmodel (self, "maps/b_nail0.bsp");
		self->v.aflag = 25;
	}
	self->v.weapon = 2;
	self->v.netname = "nails";
	PF_setsize (self, vec3_origin, Vector3D{32, 32, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_spikes);

/*QUAKED item_rockets (0 .5 .8) (0 0 0) (32 32 32) big
*/

void item_rockets(edict_t* self)
{
	self->v.touch = ammo_touch;

	if (self->v.spawnflags & WEAPON_BIG2)
	{
		PF_precache_model ("maps/b_rock1.bsp");
		PF_setmodel (self, "maps/b_rock1.bsp");
		self->v.aflag = 10;
	}
	else
	{
		PF_precache_model ("maps/b_rock0.bsp");
		PF_setmodel (self, "maps/b_rock0.bsp");
		self->v.aflag = 5;
	}
	self->v.weapon = 3;
	self->v.netname = "rockets";
	PF_setsize (self, vec3_origin, Vector3D{32, 32, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_rockets);

/*QUAKED item_cells (0 .5 .8) (0 0 0) (32 32 32) big
*/

void item_cells(edict_t* self)
{
	self->v.touch = ammo_touch;

	if (self->v.spawnflags & WEAPON_BIG2)
	{
		PF_precache_model ("maps/b_batt1.bsp");
		PF_setmodel (self, "maps/b_batt1.bsp");
		self->v.aflag = 12;
	}
	else
	{
		PF_precache_model ("maps/b_batt0.bsp");
		PF_setmodel (self, "maps/b_batt0.bsp");
		self->v.aflag = 6;
	}
	self->v.weapon = 4;
	self->v.netname = "cells";
	PF_setsize (self, vec3_origin, Vector3D{32, 32, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_cells);

/*QUAKED item_weapon (0 .5 .8) (0 0 0) (32 32 32) shotgun rocket spikes big
DO NOT USE THIS!!!! IT WILL BE REMOVED!
*/

constexpr int WEAPON_SHOTGUN = 1;
constexpr int WEAPON_ROCKET = 2;
constexpr int WEAPON_SPIKES = 4;
constexpr int WEAPON_BIG = 8;
void item_weapon(edict_t* self)
{
	self->v.touch = ammo_touch;

	if (self->v.spawnflags & WEAPON_SHOTGUN)
	{
		if (self->v.spawnflags & WEAPON_BIG)
		{
			PF_precache_model ("maps/b_shell1.bsp");
			PF_setmodel (self, "maps/b_shell1.bsp");
			self->v.aflag = 40;
		}
		else
		{
			PF_precache_model ("maps/b_shell0.bsp");
			PF_setmodel (self, "maps/b_shell0.bsp");
			self->v.aflag = 20;
		}
		self->v.weapon = 1;
		self->v.netname = "shells";
	}

	if (self->v.spawnflags & WEAPON_SPIKES)
	{
		if (self->v.spawnflags & WEAPON_BIG)
		{
			PF_precache_model ("maps/b_nail1.bsp");
			PF_setmodel (self, "maps/b_nail1.bsp");
			self->v.aflag = 40;
		}
		else
		{
			PF_precache_model ("maps/b_nail0.bsp");
			PF_setmodel (self, "maps/b_nail0.bsp");
			self->v.aflag = 20;
		}
		self->v.weapon = 2;
		self->v.netname = "spikes";
	}

	if (self->v.spawnflags & WEAPON_ROCKET)
	{
		if (self->v.spawnflags & WEAPON_BIG)
		{
			PF_precache_model ("maps/b_rock1.bsp");
			PF_setmodel (self, "maps/b_rock1.bsp");
			self->v.aflag = 10;
		}
		else
		{
			PF_precache_model ("maps/b_rock0.bsp");
			PF_setmodel (self, "maps/b_rock0.bsp");
			self->v.aflag = 5;
		}
		self->v.weapon = 3;
		self->v.netname = "rockets";
	}
	
	PF_setsize (self, vec3_origin, Vector3D{32, 32, 56});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_weapon);

/*
===============================================================================

KEYS

===============================================================================
*/

void key_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	if (other->v.health <= 0)
		return;
	if (other->v.items & self->v.items)
		return;

	PF_sprint (other, "You got the ");
	PF_sprint (other, self->v.netname);
	PF_sprint (other,"\n");

	PF_sound (other, CHAN_ITEM, self->v.noise, 1, ATTN_NORM);
	PF_stuffcmd (other, "bf\n");
	other->v.items = other->v.items | self->v.items;

	if (!pr_global_struct->coop)
	{	
		self->v.solid = SOLID_NOT;
		self->v.model = nullptr;
	}

	activator = other;
	SUB_UseTargets(self);				// fire all targets / killtargets
}


void key_setPF_sounds(edict_t* self)
{
	if (pr_global_struct->world->v.worldtype == 0)
	{
		PF_precache_sound ("misc/medkey.wav");
		self->v.noise = "misc/medkey.wav";
	}
	if (pr_global_struct->world->v.worldtype == 1)
	{
		PF_precache_sound ("misc/runekey.wav");
		self->v.noise = "misc/runekey.wav";
	}
	if (pr_global_struct->world->v.worldtype == 2)
	{
		PF_precache_sound ("misc/basekey.wav");
		self->v.noise = "misc/basekey.wav";
	}
}

/*QUAKED item_key1 (0 .5 .8) (-16 -16 -24) (16 16 32)
SILVER key
In order for keys to work
you MUST set your maps
worldtype to one of the
following:
0: medieval
1: metal
2: base
*/

void item_key1(edict_t* self)
{
	if (pr_global_struct->world->v.worldtype == 0)
	{
		PF_precache_model ("progs/w_s_key.mdl");
		PF_setmodel (self, "progs/w_s_key.mdl");
		self->v.netname = "silver key";
	}
	else if (pr_global_struct->world->v.worldtype == 1)
	{
		PF_precache_model ("progs/m_s_key.mdl");
		PF_setmodel (self, "progs/m_s_key.mdl");
		self->v.netname = "silver runekey";
	}
	else if (pr_global_struct->world->v.worldtype == 2)
	{
		PF_precache_model ("progs/b_s_key.mdl");
		PF_setmodel (self, "progs/b_s_key.mdl");
		self->v.netname = "silver keycard";
	}
	key_setPF_sounds(self);
	self->v.touch = key_touch;
	self->v.items = IT_KEY1;
	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 32});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_key1);

/*QUAKED item_key2 (0 .5 .8) (-16 -16 -24) (16 16 32)
GOLD key
In order for keys to work
you MUST set your maps
worldtype to one of the
following:
0: medieval
1: metal
2: base
*/

void item_key2(edict_t* self)
{
	if (pr_global_struct->world->v.worldtype == 0)
	{
		PF_precache_model ("progs/w_g_key.mdl");
		PF_setmodel (self, "progs/w_g_key.mdl");
		self->v.netname = "gold key";
	}
	if (pr_global_struct->world->v.worldtype == 1)
	{
		PF_precache_model ("progs/m_g_key.mdl");
		PF_setmodel (self, "progs/m_g_key.mdl");
		self->v.netname = "gold runekey";
	}
	if (pr_global_struct->world->v.worldtype == 2)
	{
		PF_precache_model ("progs/b_g_key.mdl");
		PF_setmodel (self, "progs/b_g_key.mdl");
		self->v.netname = "gold keycard";
	}
	key_setPF_sounds(self);
	self->v.touch = key_touch;
	self->v.items = IT_KEY2;
	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 32});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_key2);

/*
===============================================================================

END OF LEVEL RUNES

===============================================================================
*/

void sigil_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	if (other->v.health <= 0)
		return;

	PF_centerprint (other, "You got the rune!");

	PF_sound (other, CHAN_ITEM, self->v.noise, 1, ATTN_NORM);
	PF_stuffcmd (other, "bf\n");
	self->v.solid = SOLID_NOT;
	self->v.model = nullptr;
	pr_global_struct->serverflags = pr_global_struct->serverflags | (self->v.spawnflags & 15);
	self->v.classname = "";		// so rune doors won't find it
	
	activator = other;
	SUB_UseTargets(self);				// fire all targets / killtargets
}


/*QUAKED item_sigil (0 .5 .8) (-16 -16 -24) (16 16 32) E1 E2 E3 E4
End of level sigil, pick up to end episode and return to jrstart.
*/

void item_sigil(edict_t* self)
{
	if (!self->v.spawnflags)
		PF_objerror ("no spawnflags");

	PF_precache_sound ("misc/runekey.wav");
	self->v.noise = "misc/runekey.wav";

	if (self->v.spawnflags & 1)
	{
		PF_precache_model ("progs/end1.mdl");
		PF_setmodel (self, "progs/end1.mdl");
	}
	if (self->v.spawnflags & 2)
	{
		PF_precache_model ("progs/end2.mdl");
		PF_setmodel (self, "progs/end2.mdl");
	}
	if (self->v.spawnflags & 4)
	{
		PF_precache_model ("progs/end3.mdl");
		PF_setmodel (self, "progs/end3.mdl");
	}
	if (self->v.spawnflags & 8)
	{
		PF_precache_model ("progs/end4.mdl");
		PF_setmodel (self, "progs/end4.mdl");
	}
	
	self->v.touch = sigil_touch;
	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 32});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_sigil);

/*
===============================================================================

POWERUPS

===============================================================================
*/
void powerup_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	if (other->v.health <= 0)
		return;

	PF_sprint (other, "You got the ");
	PF_sprint (other, self->v.netname);
	PF_sprint (other,"\n");

	if (pr_global_struct->deathmatch)
	{
		self->v.mdl = self->v.model;
		
		if ((!strcmp(self->v.classname, "item_artifact_invulnerability")) ||
			(!strcmp(self->v.classname, "item_artifact_invisibility")))
			self->v.nextthink = pr_global_struct->time + 60*5;
		else
			self->v.nextthink = pr_global_struct->time + 60;
		
		self->v.think = SUB_regen;
	}	

	PF_sound (other, CHAN_VOICE, self->v.noise, 1, ATTN_NORM);
	PF_stuffcmd (other, "bf\n");
	self->v.solid = SOLID_NOT;
	other->v.items = other->v.items | self->v.items;
	self->v.model = nullptr;

// do the apropriate action
	if (!strcmp(self->v.classname, "item_artifact_envirosuit"))
	{
		other->v.rad_time = 1;
		other->v.radsuit_finished = pr_global_struct->time + 30;
	}
	
	if (!strcmp(self->v.classname, "item_artifact_invulnerability"))
	{
		other->v.invincible_time = 1;
		other->v.invincible_finished = pr_global_struct->time + 30;
	}
	
	if (!strcmp(self->v.classname, "item_artifact_invisibility"))
	{
		other->v.invisible_time = 1;
		other->v.invisible_finished = pr_global_struct->time + 30;
	}

	if (!strcmp(self->v.classname, "item_artifact_super_damage"))
	{
		other->v.super_time = 1;
		other->v.super_damage_finished = pr_global_struct->time + 30;
	}	

	activator = other;
	SUB_UseTargets(self);				// fire all targets / killtargets
}



/*QUAKED item_artifact_invulnerability (0 .5 .8) (-16 -16 -24) (16 16 32)
Player is invulnerable for 30 seconds
*/
void item_artifact_invulnerability(edict_t* self)
{
	self->v.touch = powerup_touch;

	PF_precache_model ("progs/invulner.mdl");
	PF_precache_sound ("items/protect.wav");
	PF_precache_sound ("items/protect2.wav");
	PF_precache_sound ("items/protect3.wav");
	self->v.noise = "items/protect.wav";
	PF_setmodel (self, "progs/invulner.mdl");
	self->v.netname = "Pentagram of Protection";
	self->v.items = IT_INVULNERABILITY;
	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 32});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_artifact_invulnerability);

/*QUAKED item_artifact_envirosuit (0 .5 .8) (-16 -16 -24) (16 16 32)
Player takes no damage from water or slime for 30 seconds
*/
void item_artifact_envirosuit(edict_t* self)
{
	self->v.touch = powerup_touch;

	PF_precache_model ("progs/suit.mdl");
	PF_precache_sound ("items/suit.wav");
	PF_precache_sound ("items/suit2.wav");
	self->v.noise = "items/suit.wav";
	PF_setmodel (self, "progs/suit.mdl");
	self->v.netname = "Biosuit";
	self->v.items = IT_SUIT;
	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 32});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_artifact_envirosuit);

/*QUAKED item_artifact_invisibility (0 .5 .8) (-16 -16 -24) (16 16 32)
Player is invisible for 30 seconds
*/
void item_artifact_invisibility(edict_t* self)
{
	self->v.touch = powerup_touch;

	PF_precache_model ("progs/invisibl.mdl");
	PF_precache_sound ("items/inv1.wav");
	PF_precache_sound ("items/inv2.wav");
	PF_precache_sound ("items/inv3.wav");
	self->v.noise = "items/inv1.wav";
	PF_setmodel (self, "progs/invisibl.mdl");
	self->v.netname = "Ring of Shadows";
	self->v.items = IT_INVISIBILITY;
	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 32});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_artifact_invisibility);

/*QUAKED item_artifact_super_damage (0 .5 .8) (-16 -16 -24) (16 16 32)
The next attack from the player will do 4x damage
*/
void item_artifact_super_damage(edict_t* self)
{
	self->v.touch = powerup_touch;

	PF_precache_model ("progs/quaddama.mdl");
	PF_precache_sound ("items/damage.wav");
	PF_precache_sound ("items/damage2.wav");
	PF_precache_sound ("items/damage3.wav");
	self->v.noise = "items/damage.wav";
	PF_setmodel (self, "progs/quaddama.mdl");
	self->v.netname = "Quad Damage";
	self->v.items = IT_QUAD;
	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 32});
	StartItem (self);
}

LINK_ENTITY_TO_CLASS(item_artifact_super_damage);

/*
===============================================================================

PLAYER BACKPACKS

===============================================================================
*/

void BackpackTouch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;
	if (other->v.health <= 0)
		return;
		
// if the player was using his best weapon, change up to the new one if better		
	auto stemp = self;
	self = other;
	const float best = W_BestWeapon(self);
	self = stemp;

// change weapons
	other->v.ammo_shells = other->v.ammo_shells + self->v.ammo_shells;
	other->v.ammo_nails = other->v.ammo_nails + self->v.ammo_nails;
	other->v.ammo_rockets = other->v.ammo_rockets + self->v.ammo_rockets;
	other->v.ammo_cells = other->v.ammo_cells + self->v.ammo_cells;

	other->v.items = other->v.items | self->v.items;
	
	bound_other_ammo (self, other);

	PF_sprint (other, "You get ");

	const char* s;

	if (self->v.ammo_shells)
	{
		s = PF_ftos(self->v.ammo_shells);
		PF_sprint (other, s);
		PF_sprint (other, " shells  ");
	}
	if (self->v.ammo_nails)
	{
		s = PF_ftos(self->v.ammo_nails);
		PF_sprint (other, s);
		PF_sprint (other, " nails ");
	}
	if (self->v.ammo_rockets)
	{
		s = PF_ftos(self->v.ammo_rockets);
		PF_sprint (other, s);
		PF_sprint (other, " rockets  ");
	}
	if (self->v.ammo_cells)
	{
		s = PF_ftos(self->v.ammo_cells);
		PF_sprint (other, s);
		PF_sprint (other, " cells  ");
	}
	
	PF_sprint (other, "\n");
// backpack touch sound
	PF_sound (other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM);
	PF_stuffcmd (other, "bf\n");

// change to a better weapon if appropriate
	if ( other->v.weapon == best )
	{
		stemp = self;
		self = other;
		self->v.weapon = W_BestWeapon(self);
		self = stemp;
	}

	
	PF_Remove(self);
	
	self = other;
	W_SetCurrentAmmo (self);
}

/*
===============
DropBackpack
===============
*/
void DropBackpack(edict_t* self)
{
	if (!(self->v.ammo_shells + self->v.ammo_nails + self->v.ammo_rockets + self->v.ammo_cells))
		return;	// nothing in it

	auto item = PF_Spawn();
	AsVector(item->v.origin) = AsVector(self->v.origin) - Vector3D{0, 0, 24};
	
	item->v.items = self->v.weapon;

	item->v.ammo_shells = self->v.ammo_shells;
	item->v.ammo_nails = self->v.ammo_nails;
	item->v.ammo_rockets = self->v.ammo_rockets;
	item->v.ammo_cells = self->v.ammo_cells;

	item->v.velocity[2] = 300;
	item->v.velocity[0] = -100 + (random() * 200);
	item->v.velocity[1] = -100 + (random() * 200);
	
	item->v.flags = FL_ITEM;
	item->v.solid = SOLID_TRIGGER;
	item->v.movetype = MOVETYPE_TOSS;
	PF_setmodel (item, "progs/backpack.mdl");
	PF_setsize (item, Vector3D{-16, -16, 0}, Vector3D{16, 16, 56});
	item->v.touch = BackpackTouch;
	
	item->v.nextthink = pr_global_struct->time + 120;	// PF_Remove after 2 minutes
	item->v.think = SUB_Remove;
}
