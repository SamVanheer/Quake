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
#include "ai.h"
#include "Game.h"
#include "monsters.h"
#include "subs.h"

/* ALL MONSTERS SHOULD BE 1 0 0 IN COLOR */

// name =[framenum,	nexttime, nextthink] {code}
// expands to:
// name ()
// {
//		self->v.frame=framenum;
//		self->v.nextthink = time + nexttime;
//		self->v.think = nextthink
//		<code>
// };


/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void monster_use(edict_t* self, edict_t* other)
{
	if (self->v.enemy)
		return;
	if (self->v.health <= 0)
		return;
	if ((int)pr_global_struct->activator->v.items & IT_INVISIBILITY)
		return;
	if ((int)pr_global_struct->activator->v.flags & FL_NOTARGET)
		return;
	if (strcmp(pr_global_struct->activator->v.classname, "player"))
		return;

	// delay reaction so if the monster is teleported, its sound is still
	// heard
	self->v.enemy = pr_global_struct->activator;
	self->v.nextthink = pr_global_struct->time + 0.1;
	self->v.think = FoundTarget;
}

/*
================
monster_death_use

When a mosnter dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use(edict_t* self)
{
// fall to ground
	if ((int)self->v.flags & FL_FLY)
		self->v.flags &= ~FL_FLY;
	if ((int)self->v.flags & FL_SWIM)
		self->v.flags &= ~FL_SWIM;

	if (!self->v.target)
		return;

	pr_global_struct->activator = self->v.enemy;
	SUB_UseTargets(self);
}

//============================================================================

void walkmonster_start_go(edict_t* self)
{
	self->v.origin[2] = self->v.origin[2] + 1;	// raise off floor a bit
	PF_droptofloor(self);

	if (!PF_walkmove(self, 0,0))
	{
		dprint("walkmonster in wall at: ");
		dprint(PF_vtos(self->v.origin));
		dprint("\n");
	}

	self->v.takedamage = DAMAGE_AIM;

	self->v.ideal_yaw = self->v.angles[1];
	if (!self->v.yaw_speed)
		self->v.yaw_speed = 20;
	AsVector(self->v.view_ofs) = Vector3D{0, 0, 25};
	self->v.use = monster_use;

	self->v.flags |= FL_MONSTER;

	if (self->v.target)
	{
		self->v.goalentity = self->v.movetarget = PF_Find(pr_global_struct->world, "targetname", self->v.target);
		self->v.ideal_yaw = PF_vectoyaw(AsVector(self->v.goalentity->v.origin) - AsVector(self->v.origin));
		if (self->v.movetarget == pr_global_struct->world)
		{
			dprint("Monster can't find target at ");
			dprint(PF_vtos(self->v.origin));
			dprint("\n");
		}
		// this used to be an objerror
		if (!strcmp(self->v.movetarget->v.classname, "path_corner"))
			self->v.th_walk(self);
		else
			self->v.pausetime = (float)99999999;
			self->v.th_stand(self);
	}
	else
	{
		self->v.pausetime = (float)99999999;
		self->v.th_stand(self);
	}

	// spread think times so they don't all happen at same time
	self->v.nextthink = self->v.nextthink + random() * 0.5;
}

void walkmonster_start(edict_t* self)
{
	// delay drop to floor to make sure all doors have been spawned
	// spread think times so they don't all happen at same time
	self->v.nextthink = self->v.nextthink + random() * 0.5;
	self->v.think = walkmonster_start_go;
	pr_global_struct->total_monsters = pr_global_struct->total_monsters + 1;
}

void flymonster_start_go(edict_t* self)
{
	self->v.takedamage = DAMAGE_AIM;

	self->v.ideal_yaw = self->v.angles[1];
	if (!self->v.yaw_speed)
		self->v.yaw_speed = 10;
	AsVector(self->v.view_ofs) = Vector3D{0, 0, 25};
	self->v.use = monster_use;

	self->v.flags |= FL_FLY | FL_MONSTER;

	if (!PF_walkmove(self, 0,0))
	{
		dprint("flymonster in wall at: ");
		dprint(PF_vtos(self->v.origin));
		dprint("\n");
	}

	if (self->v.target)
	{
		self->v.goalentity = self->v.movetarget = PF_Find(pr_global_struct->world, "targetname", self->v.target);
		if (self->v.movetarget == pr_global_struct->world)
		{
			dprint("Monster can't find target at ");
			dprint(PF_vtos(self->v.origin));
			dprint("\n");
		}
		// this used to be an objerror
		if (!strcmp(self->v.movetarget->v.classname, "path_corner"))
			self->v.th_walk(self);
		else
			self->v.pausetime = (float)99999999;
			self->v.th_stand(self);
	}
	else
	{
		self->v.pausetime = (float)99999999;
		self->v.th_stand(self);
	}
}

void flymonster_start(edict_t* self)
{
	// spread think times so they don't all happen at same time
	self->v.nextthink = self->v.nextthink + random() * 0.5;
	self->v.think = flymonster_start_go;
	pr_global_struct->total_monsters = pr_global_struct->total_monsters + 1;
}

void swimmonster_start_go(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}

	self->v.takedamage = DAMAGE_AIM;
	pr_global_struct->total_monsters = pr_global_struct->total_monsters + 1;

	self->v.ideal_yaw = self->v.angles[1];
	if (!self->v.yaw_speed)
		self->v.yaw_speed = 10;
	AsVector(self->v.view_ofs) = Vector3D{0, 0, 10};
	self->v.use = monster_use;

	self->v.flags |= FL_SWIM | FL_MONSTER;

	if (self->v.target)
	{
		self->v.goalentity = self->v.movetarget = PF_Find(pr_global_struct->world, "targetname", self->v.target);
		if (self->v.movetarget == pr_global_struct->world)
		{
			dprint("Monster can't find target at ");
			dprint(PF_vtos(self->v.origin));
			dprint("\n");
		}
		// this used to be an objerror
		self->v.ideal_yaw = PF_vectoyaw(AsVector(self->v.goalentity->v.origin) - AsVector(self->v.origin));
		self->v.th_walk(self);
	}
	else
	{
		self->v.pausetime = (float)99999999;
		self->v.th_stand(self);
	}

	// spread think times so they don't all happen at same time
	self->v.nextthink = self->v.nextthink + random() * 0.5;
}

void swimmonster_start(edict_t* self)
{
	// spread think times so they don't all happen at same time
	self->v.nextthink = self->v.nextthink + random() * 0.5;
	self->v.think = swimmonster_start_go;
	pr_global_struct->total_monsters = pr_global_struct->total_monsters + 1;
}
