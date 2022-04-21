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
#include "game_world.h"
#include "client.h"
#include "combat.h"
#include "player.h"
#include "subs.h"
#include "triggers.h"
#include "weapons.h"

/*
=============================================================================

				LEVEL CHANGING / INTERMISSION

=============================================================================
*/

/*QUAKED info_intermission (1 0.5 0.5) (-16 -16 -16) (16 16 16)
This is the camera point for the intermission.
Use mangle instead of angle, so you can set pitch or roll as well as yaw.  'pitch roll yaw'
*/
void info_intermission(edict_t* self)
{
}

LINK_ENTITY_TO_CLASS(info_intermission);

void Game::SetChangeParms(edict_t* self, float* parms)
{
	// remove items
	self->v.items &= ~(IT_KEY1 | IT_KEY2 | IT_INVISIBILITY | IT_INVULNERABILITY | IT_SUIT | IT_QUAD);

	// cap super health
	if (self->v.health > 100)
		self->v.health = 100;
	if (self->v.health < 50)
		self->v.health = 50;
	parms[0] = self->v.items;
	parms[1] = self->v.health;
	parms[2] = self->v.armorvalue;
	if (self->v.ammo_shells < 25)
		parms[3] = 25;
	else
		parms[3] = self->v.ammo_shells;
	parms[4] = self->v.ammo_nails;
	parms[5] = self->v.ammo_rockets;
	parms[6] = self->v.ammo_cells;
	parms[7] = self->v.weapon;
	parms[8] = self->v.armortype * 100;
}

void Game::SetNewParms(float* parms)
{
	parms[0] = IT_SHOTGUN | IT_AXE;
	parms[1] = 100;
	parms[2] = 0;
	parms[3] = 25;
	parms[4] = 0;
	parms[5] = 0;
	parms[5] = 0;
	parms[7] = 1;
	parms[8] = 0;
}

void DecodeLevelParms(edict_t* self)
{
	if (pr_global_struct->serverflags)
	{
		//TODO: hack: assumes parm1 is followed by each parm variable
		if (!strcmp(pr_global_struct->world->v.model, "maps/start.bsp"))
			g_Game->SetNewParms(&pr_global_struct->parm1);		// take away all stuff on starting new episode
	}

	self->v.items = pr_global_struct->parm1;
	self->v.health = pr_global_struct->parm2;
	self->v.armorvalue = pr_global_struct->parm3;
	self->v.ammo_shells = pr_global_struct->parm4;
	self->v.ammo_nails = pr_global_struct->parm5;
	self->v.ammo_rockets = pr_global_struct->parm6;
	self->v.ammo_cells = pr_global_struct->parm7;
	self->v.weapon = pr_global_struct->parm8;
	self->v.armortype = pr_global_struct->parm9 * 0.01;
}

/*
============
FindIntermission

Returns the entity to view from
============
*/
edict_t* FindIntermission(edict_t* self)
{
	// look for info_intermission first
	auto spot = PF_Find(pr_global_struct->world, "classname", "info_intermission");
	if (spot != pr_global_struct->world)
	{	// pick a random one
		float cyc = PF_random() * 4;
		while (cyc > 1)
		{
			spot = PF_Find(spot, "classname", "info_intermission");
			if (spot == pr_global_struct->world)
				spot = PF_Find(spot, "classname", "info_intermission");
			cyc = cyc - 1;
		}
		return spot;
	}

	// then look for the start position
	spot = PF_Find(pr_global_struct->world, "classname", "info_player_start");
	if (spot != pr_global_struct->world)
		return spot;

	// testinfo_player_start is only found in regioned levels
	spot = PF_Find(pr_global_struct->world, "classname", "testplayerstart");
	if (spot != pr_global_struct->world)
		return spot;

	PF_objerror("FindIntermission: no spot");
	return nullptr;
}

void GotoNextMap(edict_t* self)
{
	if (PF_cvar("samelevel"))	// if samelevel is set, stay on same level
		PF_changelevel(pr_global_struct->mapname);
	else
		PF_changelevel(pr_global_struct->nextmap);
}

void ExitIntermission(edict_t* self)
{
	// skip any text in deathmatch
	if (pr_global_struct->deathmatch)
	{
		GotoNextMap(self);
		return;
	}

	pr_global_struct->intermission_exittime = pr_global_struct->time + 1;
	++pr_global_struct->intermission_running;

	//
	// run some text if at the end of an episode
	//
	if (pr_global_struct->intermission_running == 2)
	{
		if (!strcmp(pr_global_struct->world->v.model, "maps/e1m7.bsp"))
		{
			PF_WriteByte(MSG_ALL, SVC_CDTRACK);
			PF_WriteByte(MSG_ALL, 2);
			PF_WriteByte(MSG_ALL, 3);
			if (!PF_cvar("registered"))
			{
				PF_WriteByte(MSG_ALL, SVC_FINALE);
				PF_WriteString(MSG_ALL, "As the corpse of the monstrous entity\nChthon sinks back into the lava whence\nit rose, you grip the Rune of Earth\nMagic tightly. Now that you have\nconquered the Dimension of the Doomed,\nrealm of Earth Magic, you are ready to\ncomplete your task in the other three\nhaunted lands of Quake. Or are you? If\nyou don't register Quake, you'll never\nknow what awaits you in the Realm of\nBlack Magic, the Netherworld, and the\nElder World!");
			}
			else
			{
				PF_WriteByte(MSG_ALL, SVC_FINALE);
				PF_WriteString(MSG_ALL, "As the corpse of the monstrous entity\nChthon sinks back into the lava whence\nit rose, you grip the Rune of Earth\nMagic tightly. Now that you have\nconquered the Dimension of the Doomed,\nrealm of Earth Magic, you are ready to\ncomplete your task. A Rune of magic\npower lies at the end of each haunted\nland of Quake. Go forth, seek the\ntotality of the four Runes!");
			}
			return;
		}
		else if (!strcmp(pr_global_struct->world->v.model, "maps/e2m6.bsp"))
		{
			PF_WriteByte(MSG_ALL, SVC_CDTRACK);
			PF_WriteByte(MSG_ALL, 2);
			PF_WriteByte(MSG_ALL, 3);

			PF_WriteByte(MSG_ALL, SVC_FINALE);
			PF_WriteString(MSG_ALL, "The Rune of Black Magic throbs evilly in\nyour hand and whispers dark thoughts\ninto your brain. You learn the inmost\nlore of the Hell-Mother; Shub-Niggurath!\nYou now know that she is behind all the\nterrible plotting which has led to so\nmuch death and horror. But she is not\ninviolate! Armed with this Rune, you\nrealize that once all four Runes are\ncombined, the gate to Shub-Niggurath's\nPit will open, and you can face the\nWitch-Goddess herself in her frightful\notherworld cathedral.");
			return;
		}
		else if (!strcmp(pr_global_struct->world->v.model, "maps/e3m6.bsp"))
		{
			PF_WriteByte(MSG_ALL, SVC_CDTRACK);
			PF_WriteByte(MSG_ALL, 2);
			PF_WriteByte(MSG_ALL, 3);

			PF_WriteByte(MSG_ALL, SVC_FINALE);
			PF_WriteString(MSG_ALL, "The charred viscera of diabolic horrors\nbubble viscously as you seize the Rune\nof Hell Magic. Its heat scorches your\nhand, and its terrible secrets blight\nyour mind. Gathering the shreds of your\ncourage, you shake the devil's shackles\nfrom your soul, and become ever more\nhard and determined to destroy the\nhideous creatures whose mere existence\nthreatens the souls and psyches of all\nthe population of Earth.");
			return;
		}
		else if (!strcmp(pr_global_struct->world->v.model, "maps/e4m7.bsp"))
		{
			PF_WriteByte(MSG_ALL, SVC_CDTRACK);
			PF_WriteByte(MSG_ALL, 2);
			PF_WriteByte(MSG_ALL, 3);

			PF_WriteByte(MSG_ALL, SVC_FINALE);
			PF_WriteString(MSG_ALL, "Despite the awful might of the Elder\nWorld, you have achieved the Rune of\nElder Magic, capstone of all types of\narcane wisdom. Beyond good and evil,\nbeyond life and death, the Rune\npulsates, heavy with import. Patient and\npotent, the Elder Being Shub-Niggurath\nweaves her dire plans to clear off all\nlife from the Earth, and bring her own\nfoul offspring to our world! For all the\ndwellers in these nightmare dimensions\nare her descendants! Once all Runes of\nmagic power are united, the energy\nbehind them will blast open the Gateway\nto Shub-Niggurath, and you can travel\nthere to foil the Hell-Mother's plots\nin person.");
			return;
		}

		GotoNextMap(self);
	}

	if (pr_global_struct->intermission_running == 3)
	{
		if (!PF_cvar("registered"))
		{	// shareware episode has been completed, go to sell screen
			PF_WriteByte(MSG_ALL, SVC_SELLSCREEN);
			return;
		}

		if (((int)pr_global_struct->serverflags & 15) == 15)
		{
			PF_WriteByte(MSG_ALL, SVC_FINALE);
			PF_WriteString(MSG_ALL, "Now, you have all four Runes. You sense\ntremendous invisible forces moving to\nunseal ancient barriers. Shub-Niggurath\nhad hoped to use the Runes Herself to\nclear off the Earth, but now instead,\nyou will use them to enter her home and\nconfront her as an avatar of avenging\nEarth-life. If you defeat her, you will\nbe remembered forever as the savior of\nthe planet. If she conquers, it will be\nas if you had never been born.");
			return;
		}

	}

	GotoNextMap(self);
}

/*
============
IntermissionThink

When the player presses attack or jump, change to the next level
============
*/
void IntermissionThink(edict_t* self)
{
	if (pr_global_struct->time < pr_global_struct->intermission_exittime)
		return;

	if (!self->v.button0 && !self->v.button1 && !self->v.button2)
		return;

	ExitIntermission(self);
}

void execute_changelevel(edict_t* self)
{
	pr_global_struct->intermission_running = 1;

	// enforce a wait time before allowing changelevel
	if (pr_global_struct->deathmatch)
		pr_global_struct->intermission_exittime = pr_global_struct->time + 5;
	else
		pr_global_struct->intermission_exittime = pr_global_struct->time + 2;

	PF_WriteByte(MSG_ALL, SVC_CDTRACK);
	PF_WriteByte(MSG_ALL, 3);
	PF_WriteByte(MSG_ALL, 3);

	auto pos = FindIntermission(self);

	auto other = PF_Find(pr_global_struct->world, "classname", "player");
	while (other != pr_global_struct->world)
	{
		VectorCopy(vec3_origin, other->v.view_ofs);
		VectorCopy(pos->v.mangle, other->v.angles);
		VectorCopy(pos->v.mangle, other->v.v_angle);
		other->v.fixangle = TRUE;		// turn this way immediately
		other->v.nextthink = pr_global_struct->time + 0.5;
		other->v.takedamage = DAMAGE_NO;
		other->v.solid = SOLID_NOT;
		other->v.movetype = MOVETYPE_NONE;
		other->v.modelindex = 0;
		PF_setorigin(other, pos->v.origin);
		other = PF_Find(other, "classname", "player");
	}

	PF_WriteByte(MSG_ALL, SVC_INTERMISSION);
}

LINK_FUNCTION_TO_NAME(execute_changelevel);

void changelevel_touch(edict_t* self, edict_t* other)
{
	if (strcmp(other->v.classname, "player"))
		return;

	if (PF_cvar("noexit"))
	{
		T_Damage(self, other, self, self, 50000);
		return;
	}
	bprint(other->v.netname);
	bprint(" exited the level\n");

	pr_global_struct->nextmap = self->v.map;

	SUB_UseTargets(self);

	if (((int)self->v.spawnflags & 1) && (pr_global_struct->deathmatch == 0))
	{	// NO_INTERMISSION
		GotoNextMap(self);
		return;
	}

	self->v.touch = &SUB_NullTouch;

	// we can't move people right now, because touch functions are called
	// in the middle of C movement code, so set a think time to do it
	self->v.think = &execute_changelevel;
	self->v.nextthink = pr_global_struct->time + 0.1;
}

LINK_FUNCTION_TO_NAME(changelevel_touch);

/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/
void trigger_changelevel(edict_t* self)
{
	if (!self->v.map)
		PF_objerror("chagnelevel trigger doesn't have map");

	InitTrigger(self);
	self->v.touch = &changelevel_touch;
}

LINK_ENTITY_TO_CLASS(trigger_changelevel);

/*
=============================================================================

				PLAYER GAME EDGE FUNCTIONS

=============================================================================
*/

void set_suicide_frame(edict_t*);

// called by ClientKill and DeadThink
void respawn(edict_t* self)
{
	if (pr_global_struct->coop)
	{
		// make a copy of the dead body for appearances sake
		CopyToBodyQue(self);
		// get the spawn parms as they were at level start
		PF_setspawnparms(self);
		// respawn		
		g_Game->PutClientInServer(self);
	}
	else if (pr_global_struct->deathmatch)
	{
		// make a copy of the dead body for appearances sake
		CopyToBodyQue(self);
		// set default spawn parms
		g_Game->SetNewParms(&pr_global_struct->parm1);
		// respawn		
		g_Game->PutClientInServer(self);
	}
	else
	{	// restart the entire server
		PF_localcmd("restart\n");
	}
}

/*
============
ClientKill

Player entered the suicide command
============
*/
void Game::ClientKill(edict_t* self)
{
	bprint(self->v.netname);
	bprint(" suicides\n");
	set_suicide_frame(self);
	self->v.modelindex = modelindex_player;
	self->v.frags = self->v.frags - 2;	// extra penalty
	respawn(self);
}

/*
============
SelectSpawnPoint

Returns the entity to spawn at
============
*/
edict_t* SelectSpawnPoint()
{
	// testinfo_player_start is only found in regioned levels
	auto spot = PF_Find(pr_global_struct->world, "classname", "testplayerstart");
	if (spot != pr_global_struct->world)
		return spot;

	// choose a info_player_deathmatch point
	if (pr_global_struct->coop)
	{
		lastspawn = PF_Find(lastspawn, "classname", "info_player_coop");
		if (lastspawn == pr_global_struct->world)
			lastspawn = PF_Find(lastspawn, "classname", "info_player_start");
		if (lastspawn != pr_global_struct->world)
			return lastspawn;
	}
	else if (pr_global_struct->deathmatch)
	{
		lastspawn = PF_Find(lastspawn, "classname", "info_player_deathmatch");
		if (lastspawn == pr_global_struct->world)
			lastspawn = PF_Find(lastspawn, "classname", "info_player_deathmatch");
		if (lastspawn != pr_global_struct->world)
			return lastspawn;
	}

	if (pr_global_struct->serverflags)
	{	// return with a rune to start
		spot = PF_Find(pr_global_struct->world, "classname", "info_player_start2");
		if (spot != pr_global_struct->world)
			return spot;
	}

	spot = PF_Find(pr_global_struct->world, "classname", "info_player_start");
	if (spot == pr_global_struct->world)
		PF_error("PutClientInServer: no info_player_start on level");

	return spot;
};

/*
===========
PutClientInServer

called each time a player is spawned
============
*/
void PlayerDie(edict_t*);

void Game::PutClientInServer(edict_t* self)
{
	self->v.classname = "player";
	self->v.health = 100;
	self->v.takedamage = DAMAGE_AIM;
	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_WALK;
	self->v.show_hostile = 0;
	self->v.max_health = 100;
	self->v.flags = FL_CLIENT;
	self->v.air_finished = pr_global_struct->time + 12;
	self->v.dmg = 2;   		// initial water damage
	self->v.super_damage_finished = 0;
	self->v.radsuit_finished = 0;
	self->v.invisible_finished = 0;
	self->v.invincible_finished = 0;
	self->v.effects = 0;
	self->v.invincible_time = 0;
	self->v.animations_get = &player_animations_get;

	DecodeLevelParms(self);
	
	W_SetCurrentAmmo(self);

	self->v.attack_finished = pr_global_struct->time;
	self->v.th_pain = &player_pain;
	self->v.th_die = PlayerDie;

	self->v.deadflag = DEAD_NO;
	// paustime is set by teleporters to keep the player from moving a while
	self->v.pausetime = 0;

	auto spot = SelectSpawnPoint();

	VectorCopy(spot->v.origin, self->v.origin);
	self->v.origin[2] += 1;
	VectorCopy(spot->v.angles, self->v.angles);
	self->v.fixangle = TRUE;		// turn this way immediately

// oh, this is a hack!
	PF_setmodel(self, "progs/eyes.mdl");
	modelindex_eyes = self->v.modelindex;

	PF_setmodel(self, "progs/player.mdl");
	modelindex_player = self->v.modelindex;

	PF_setsize(self, VEC_HULL_MIN, VEC_HULL_MAX);

	self->v.view_ofs[0] = 0;
	self->v.view_ofs[1] = 0;
	self->v.view_ofs[2] = 22;

	player_stand1(self);

	if (pr_global_struct->deathmatch || pr_global_struct->coop)
	{
		PF_makevectors(self->v.angles);
		vec3_t fogPos;
		VectorMA(self->v.origin, 20, pr_global_struct->v_forward, fogPos);
		spawn_tfog(self, fogPos);
	}

	spawn_tdeath(self, self->v.origin, self);
}

/*
=============================================================================

				QUAKED FUNCTIONS

=============================================================================
*/


/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 24)
The normal starting point for a level.
*/
void info_player_start(edict_t* self)
{
}

LINK_ENTITY_TO_CLASS(info_player_start);

/*QUAKED info_player_start2 (1 0 0) (-16 -16 -24) (16 16 24)
Only used on start map for the return point from an episode.
*/
void info_player_start2(edict_t* self)
{
}

LINK_ENTITY_TO_CLASS(info_player_start2);

/*
saved out by quaked in region mode
*/
void testplayerstart(edict_t* self)
{
}

LINK_ENTITY_TO_CLASS(testplayerstart);

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 24)
potential spawning position for deathmatch games
*/
void info_player_deathmatch(edict_t* self)
{
}

LINK_ENTITY_TO_CLASS(info_player_deathmatch);

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 24)
potential spawning position for coop games
*/
void info_player_coop(edict_t* self)
{
}

LINK_ENTITY_TO_CLASS(info_player_coop);

/*
===============================================================================

RULES

===============================================================================
*/

void PrintClientScore(edict_t* self, edict_t* c)
{
	if (c->v.frags > -10 && c->v.frags < 0)
		bprint(" ");
	else if (c->v.frags >= 0)
	{
		if (c->v.frags < 100)
			bprint(" ");
		if (c->v.frags < 10)
			bprint(" ");
	}
	bprint(PF_ftos(c->v.frags));
	bprint(" ");
	bprint(c->v.netname);
	bprint("\n");
}

void DumpScore(edict_t* self)
{
	if (pr_global_struct->world->v.chain)
		PF_error("DumpScore: world.chain is set");

	// build a sorted lis
	auto e = PF_Find(pr_global_struct->world, "classname", "player");
	auto sort = pr_global_struct->world;
	while (e != pr_global_struct->world)
	{
		if (sort == pr_global_struct->world)
		{
			sort = e;
			e->v.chain = pr_global_struct->world;
		}
		else
		{
			if (e->v.frags > sort->v.frags)
			{
				e->v.chain = sort;
				sort = e;
			}
			else
			{
				auto walk = sort;
				do
				{
					if (!walk->v.chain)
					{
						e->v.chain = pr_global_struct->world;
						walk->v.chain = e;
					}
					else if (walk->v.chain->v.frags < e->v.frags)
					{
						e->v.chain = walk->v.chain;
						walk->v.chain = e;
					}
					else
						walk = walk->v.chain;
				} while (walk->v.chain != e);
			}
		}

		e = PF_Find(e, "classname", "player");
	}

	// print the list

	PF_bprint("\n");
	while (sort)
	{
		PrintClientScore(self, sort);
		sort = sort->v.chain;
	}
	PF_bprint("\n");
}

/*
go to the next level for deathmatch
*/
void NextLevel(edict_t* self)
{
	// find a trigger changelevel
	auto o = PF_Find(pr_global_struct->world, "classname", "trigger_changelevel");
	if (o == pr_global_struct->world || !strcmp(pr_global_struct->mapname, "start"))
	{	// go back to same map if no trigger_changelevel
		o = PF_Spawn();
		o->v.map = pr_global_struct->mapname;
	}

	pr_global_struct->nextmap = o->v.map;

	if (o->v.nextthink < pr_global_struct->time)
	{
		o->v.think = &execute_changelevel;
		o->v.nextthink = pr_global_struct->time + 0.1;
	}
}

/*
============
CheckRules

Exit deathmatch games upon conditions
============
*/
void CheckRules(edict_t* self)
{
	if (pr_global_struct->gameover)	// someone else quit the game already
		return;

	float timelimit = PF_cvar("timelimit") * 60;
	float fraglimit = PF_cvar("fraglimit");

	if (timelimit && pr_global_struct->time >= timelimit)
	{
		NextLevel(self);
		/*
				pr_global_struct->gameover = TRUE;
				bprint ("\n\n\n==============================\n");
				bprint ("game exited after ");
				bprint (ftos(timelimit/60));
				bprint (" minutes\n");
				DumpScore ();
				localcmd ("killserver\n");
		*/
		return;
	}

	if (fraglimit && self->v.frags >= fraglimit)
	{
		NextLevel(self);
		/*
				pr_global_struct->gameover = TRUE;
				bprint ("\n\n\n==============================\n");
				bprint ("game exited after ");
				bprint (ftos(self->v.frags));
				bprint (" frags\n");
				DumpScore ();
				localcmd ("killserver\n");
		*/
		return;
	}
}

//============================================================================

void PlayerDeathThink(edict_t* self)
{
	if (((int)self->v.flags & FL_ONGROUND))
	{
		float forward = PF_vlen(self->v.velocity);
		forward = forward - 20;
		if (forward <= 0)
		{
			VectorCopy(vec3_origin, self->v.velocity);
		}
		else
		{
			PF_normalize(self->v.velocity, self->v.velocity);
			VectorScale(self->v.velocity, forward, self->v.velocity);
		}
	}

	// wait for all buttons released
	if (self->v.deadflag == DEAD_DEAD)
	{
		if (self->v.button2 || self->v.button1 || self->v.button0)
			return;
		self->v.deadflag = DEAD_RESPAWNABLE;
		return;
	}

	// wait for any button down
	if (!self->v.button2 && !self->v.button1 && !self->v.button0)
		return;

	self->v.button0 = 0;
	self->v.button1 = 0;
	self->v.button2 = 0;
	respawn(self);
}

void PlayerJump(edict_t* self)
{
	if ((int)self->v.flags & FL_WATERJUMP)
		return;

	if (self->v.waterlevel >= 2)
	{
		if (self->v.watertype == CONTENT_WATER)
			self->v.velocity[2] = 100;
		else if (self->v.watertype == CONTENT_SLIME)
			self->v.velocity[2] = 80;
		else
			self->v.velocity[2] = 50;

		// play swiming sound
		if (self->v.swim_flag < pr_global_struct->time)
		{
			self->v.swim_flag = pr_global_struct->time + 1;
			if (PF_random() < 0.5)
				PF_sound(self, CHAN_BODY, "misc/water1.wav", 1, ATTN_NORM);
			else
				PF_sound(self, CHAN_BODY, "misc/water2.wav", 1, ATTN_NORM);
		}

		return;
	}

	if (!((int)self->v.flags & FL_ONGROUND))
		return;

	if (!((int)self->v.flags & FL_JUMPRELEASED))
		return;		// don't pogo stick

	self->v.flags &= ~FL_JUMPRELEASED;

	self->v.flags &= ~FL_ONGROUND;	// don't stairwalk

	self->v.button2 = 0;
	// player jumping sound
	PF_sound(self, CHAN_BODY, "player/plyrjmp8.wav", 1, ATTN_NORM);
	self->v.velocity[2] += 270;
}

/*
===========
WaterMove

============
*/
void WaterMove(edict_t* self)
{
	//dprint (ftos(self->v.waterlevel));
	if (self->v.movetype == MOVETYPE_NOCLIP)
		return;
	if (self->v.health < 0)
		return;

	if (self->v.waterlevel != 3)
	{
		if (self->v.air_finished < pr_global_struct->time)
			PF_sound(self, CHAN_VOICE, "player/gasp2.wav", 1, ATTN_NORM);
		else if (self->v.air_finished < pr_global_struct->time + 9)
			PF_sound(self, CHAN_VOICE, "player/gasp1.wav", 1, ATTN_NORM);
		self->v.air_finished = pr_global_struct->time + 12;
		self->v.dmg = 2;
	}
	else if (self->v.air_finished < pr_global_struct->time)
	{	// drown!
		if (self->v.pain_finished < pr_global_struct->time)
		{
			self->v.dmg = self->v.dmg + 2;
			if (self->v.dmg > 15)
				self->v.dmg = 10;
			T_Damage(self, self, pr_global_struct->world, pr_global_struct->world, self->v.dmg);
			self->v.pain_finished = pr_global_struct->time + 1;
		}
	}

	if (!self->v.waterlevel)
	{
		if ((int)self->v.flags & FL_INWATER)
		{
			// play leave water sound
			PF_sound(self, CHAN_BODY, "misc/outwater.wav", 1, ATTN_NORM);
			self->v.flags &= ~FL_INWATER;
		}
		return;
	}

	if (self->v.watertype == CONTENT_LAVA)
	{	// do damage
		if (self->v.dmgtime < pr_global_struct->time)
		{
			if (self->v.radsuit_finished > pr_global_struct->time)
				self->v.dmgtime = pr_global_struct->time + 1;
			else
				self->v.dmgtime = pr_global_struct->time + 0.2;

			T_Damage(self, self, pr_global_struct->world, pr_global_struct->world, 10 * self->v.waterlevel);
		}
	}
	else if (self->v.watertype == CONTENT_SLIME)
	{	// do damage
		if (self->v.dmgtime < pr_global_struct->time && self->v.radsuit_finished < pr_global_struct->time)
		{
			self->v.dmgtime = pr_global_struct->time + 1;
			T_Damage(self, self, pr_global_struct->world, pr_global_struct->world, 4 * self->v.waterlevel);
		}
	}

	if (!((int)self->v.flags & FL_INWATER))
	{

		// player enter water sound

		if (self->v.watertype == CONTENT_LAVA)
			PF_sound(self, CHAN_BODY, "player/inlava.wav", 1, ATTN_NORM);
		if (self->v.watertype == CONTENT_WATER)
			PF_sound(self, CHAN_BODY, "player/inh2o.wav", 1, ATTN_NORM);
		if (self->v.watertype == CONTENT_SLIME)
			PF_sound(self, CHAN_BODY, "player/slimbrn2.wav", 1, ATTN_NORM);

		self->v.flags |= FL_INWATER;
		self->v.dmgtime = 0;
	}

	if (!((int)self->v.flags & FL_WATERJUMP))
	{
		VectorMA(self->v.velocity, -0.8 * self->v.waterlevel * pr_global_struct->frametime, self->v.velocity, self->v.velocity);
	}
}

void CheckWaterJump(edict_t* self)
{
// check for a jump-out-of-water
	PF_makevectors(self->v.angles);
	Vector3D start = AsVector(self->v.origin);
	start[2] += 8;
	pr_global_struct->v_forward[2] = 0;
	PF_normalize(pr_global_struct->v_forward, pr_global_struct->v_forward);
	auto end = start + AsVector(pr_global_struct->v_forward) * 24;
	PF_traceline(start, end, TRUE, self);
	if (pr_global_struct->trace_fraction < 1)
	{	// solid at waist
		start[2] = start[2] + self->v.maxs[2] - 8;
		end = start + AsVector(pr_global_struct->v_forward) * 24;
		AsVector(self->v.movedir) = AsVector(pr_global_struct->trace_plane_normal) * -50;
		PF_traceline(start, end, TRUE, self);
		if (pr_global_struct->trace_fraction == 1)
		{	// open at eye level
			self->v.flags |= FL_WATERJUMP;
			self->v.velocity[2] = 225;
			self->v.flags &= ~FL_JUMPRELEASED;
			self->v.teleport_time = pr_global_struct->time + 2;	// safety net
			return;
		}
	}
}

/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void Game::PlayerPreThink(edict_t* self)
{
	if (pr_global_struct->intermission_running)
	{
		IntermissionThink(self);	// otherwise a button could be missed between
		return;					// the think tics
	}

	if (VectorCompare(self->v.view_ofs, vec3_origin))
		return;		// intermission or finale

	PF_makevectors(self->v.v_angle);		// is this still used

	CheckRules(self);
	WaterMove(self);

	if (self->v.waterlevel == 2)
		CheckWaterJump(self);

	if (self->v.deadflag >= DEAD_DEAD)
	{
		PlayerDeathThink(self);
		return;
	}

	if (self->v.deadflag == DEAD_DYING)
		return;	// dying, so do nothing

	if (self->v.button2)
	{
		PlayerJump(self);
	}
	else
		self->v.flags |= FL_JUMPRELEASED;

	// teleporters can force a non-moving pause time
	if (pr_global_struct->time < self->v.pausetime)
	{
		VectorCopy(vec3_origin, self->v.velocity);
	}
}

/*
================
CheckPowerups

Check for turning off powerups
================
*/
void CheckPowerups(edict_t* self)
{
	if (self->v.health <= 0)
		return;

	// invisibility
	if (self->v.invisible_finished)
	{
		// sound and screen flash when items starts to run out
		if (self->v.invisible_sound < pr_global_struct->time)
		{
			PF_sound(self, CHAN_AUTO, "items/inv3.wav", 0.5, ATTN_IDLE);
			self->v.invisible_sound = pr_global_struct->time + ((random() * 3) + 1);
		}


		if (self->v.invisible_finished < pr_global_struct->time + 3)
		{
			if (self->v.invisible_time == 1)
			{
				PF_sprint(self, "Ring of Shadows magic is fading\n");
				PF_stuffcmd(self, "bf\n");
				PF_sound(self, CHAN_AUTO, "items/inv2.wav", 1, ATTN_NORM);
				self->v.invisible_time = pr_global_struct->time + 1;
			}

			if (self->v.invisible_time < pr_global_struct->time)
			{
				self->v.invisible_time = pr_global_struct->time + 1;
				PF_stuffcmd(self, "bf\n");
			}
		}

		if (self->v.invisible_finished < pr_global_struct->time)
		{	// just stopped
			self->v.items &= ~IT_INVISIBILITY;
			self->v.invisible_finished = 0;
			self->v.invisible_time = 0;
		}

		// use the eyes
		self->v.frame = 0;
		self->v.modelindex = modelindex_eyes;
	}
	else
		self->v.modelindex = modelindex_player;	// don't use eyes

// invincibility
	if (self->v.invincible_finished)
	{
		// sound and screen flash when items starts to run out
		if (self->v.invincible_finished < pr_global_struct->time + 3)
		{
			if (self->v.invincible_time == 1)
			{
				PF_sprint(self, "Protection is almost burned out\n");
				PF_stuffcmd(self, "bf\n");
				PF_sound(self, CHAN_AUTO, "items/protect2.wav", 1, ATTN_NORM);
				self->v.invincible_time = pr_global_struct->time + 1;
			}

			if (self->v.invincible_time < pr_global_struct->time)
			{
				self->v.invincible_time = pr_global_struct->time + 1;
				PF_stuffcmd(self, "bf\n");
			}
		}

		if (self->v.invincible_finished < pr_global_struct->time)
		{	// just stopped
			self->v.items &= ~IT_INVULNERABILITY;
			self->v.invincible_time = 0;
			self->v.invincible_finished = 0;
		}
		if (self->v.invincible_finished > pr_global_struct->time)
			self->v.effects |= EF_DIMLIGHT;
		else
			self->v.effects &= ~EF_DIMLIGHT;
	}

	// super damage
	if (self->v.super_damage_finished)
	{

		// sound and screen flash when items starts to run out

		if (self->v.super_damage_finished < pr_global_struct->time + 3)
		{
			if (self->v.super_time == 1)
			{
				PF_sprint(self, "Quad Damage is wearing off\n");
				PF_stuffcmd(self, "bf\n");
				PF_sound(self, CHAN_AUTO, "items/damage2.wav", 1, ATTN_NORM);
				self->v.super_time = pr_global_struct->time + 1;
			}

			if (self->v.super_time < pr_global_struct->time)
			{
				self->v.super_time = pr_global_struct->time + 1;
				PF_stuffcmd(self, "bf\n");
			}
		}

		if (self->v.super_damage_finished < pr_global_struct->time)
		{	// just stopped
			self->v.items &= ~IT_QUAD;
			self->v.super_damage_finished = 0;
			self->v.super_time = 0;
		}
		if (self->v.super_damage_finished > pr_global_struct->time)
			self->v.effects |= EF_DIMLIGHT;
		else
			self->v.effects &= ~EF_DIMLIGHT;
	}

	// suit	
	if (self->v.radsuit_finished)
	{
		self->v.air_finished = pr_global_struct->time + 12;		// don't drown

// sound and screen flash when items starts to run out
		if (self->v.radsuit_finished < pr_global_struct->time + 3)
		{
			if (self->v.rad_time == 1)
			{
				PF_sprint(self, "Air supply in Biosuit expiring\n");
				PF_stuffcmd(self, "bf\n");
				PF_sound(self, CHAN_AUTO, "items/suit2.wav", 1, ATTN_NORM);
				self->v.rad_time = pr_global_struct->time + 1;
			}

			if (self->v.rad_time < pr_global_struct->time)
			{
				self->v.rad_time = pr_global_struct->time + 1;
				PF_stuffcmd(self, "bf\n");
			}
		}

		if (self->v.radsuit_finished < pr_global_struct->time)
		{	// just stopped
			self->v.items &= ~IT_SUIT;
			self->v.rad_time = 0;
			self->v.radsuit_finished = 0;
		}
	}

}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void Game::PlayerPostThink(edict_t* self)
{
	if (VectorCompare(self->v.view_ofs, vec3_origin))
		return;		// intermission or finale
	if (self->v.deadflag)
		return;

	// do weapon stuff

	W_WeaponFrame(self);

	// check to see if player landed and play landing sound
	if ((self->v.jump_flag < -300) && ((int)self->v.flags & FL_ONGROUND) && (self->v.health > 0))
	{
		if (self->v.watertype == CONTENT_WATER)
			PF_sound(self, CHAN_BODY, "player/h2ojump.wav", 1, ATTN_NORM);
		else if (self->v.jump_flag < -650)
		{
			T_Damage(self, self, pr_global_struct->world, pr_global_struct->world, 5);
			PF_sound(self, CHAN_VOICE, "player/land2.wav", 1, ATTN_NORM);
			self->v.deathtype = "falling";
		}
		else
			PF_sound(self, CHAN_VOICE, "player/land.wav", 1, ATTN_NORM);

		self->v.jump_flag = 0;
	}

	if (!((int)self->v.flags & FL_ONGROUND))
		self->v.jump_flag = self->v.velocity[2];

	CheckPowerups(self);
}

/*
===========
ClientConnect

called when a player connects to a server
============
*/
void Game::ClientConnect(edict_t* self)
{
	bprint(self->v.netname);
	bprint(" entered the game\n");

	// a client connecting during an intermission can cause problems
	if (pr_global_struct->intermission_running)
		ExitIntermission(self);
};

/*
===========
ClientDisconnect

called when a player disconnects from a server
============
*/
void Game::ClientDisconnect(edict_t* self)
{
	if (pr_global_struct->gameover)
		return;
	// if the level end trigger has been activated, just return
	// since they aren't *really* leaving

	// let everyone else know
	bprint(self->v.netname);
	bprint(" left the game with ");
	bprint(PF_ftos(self->v.frags));
	bprint(" frags\n");
	PF_sound(self, CHAN_BODY, "player/tornoff2.wav", 1, ATTN_NONE);
	set_suicide_frame(self);
}

/*
===========
ClientObituary

called when a player dies
============
*/
void ClientObituary(edict_t* self, edict_t* targ, edict_t* attacker)
{
	float rnum = PF_random();

	if (!strcmp(targ->v.classname, "player"))
	{
		if (!strcmp(attacker->v.classname, "teledeath"))
		{
			bprint(targ->v.netname);
			bprint(" was telefragged by ");
			bprint(attacker->v.owner->v.netname);
			bprint("\n");

			attacker->v.owner->v.frags = attacker->v.owner->v.frags + 1;
			return;
		}

		if (!strcmp(attacker->v.classname, "teledeath2"))
		{
			bprint("Satan's power deflects ");
			bprint(targ->v.netname);
			bprint("'s telefrag\n");

			targ->v.frags = targ->v.frags - 1;
			return;
		}

		if (!strcmp(attacker->v.classname, "player"))
		{
			if (targ == attacker)
			{
				// killed self
				attacker->v.frags = attacker->v.frags - 1;
				bprint(targ->v.netname);

				if (targ->v.weapon == 64 && targ->v.waterlevel > 1)
				{
					bprint(" discharges into the water.\n");
					return;
				}
				if (targ->v.weapon == 16)
					bprint(" tries to put the pin back in\n");
				else if (rnum)
					bprint(" becomes bored with life\n");
				else
					bprint(" checks if his weapon is loaded\n");
				return;
			}
			else
			{
				attacker->v.frags = attacker->v.frags + 1;

				const char* deathstring = "", * deathstring2 = "";

				rnum = attacker->v.weapon;
				if (rnum == IT_AXE)
				{
					deathstring = " was ax-murdered by ";
					deathstring2 = "\n";
				}
				if (rnum == IT_SHOTGUN)
				{
					deathstring = " chewed on ";
					deathstring2 = "'s boomstick\n";
				}
				if (rnum == IT_SUPER_SHOTGUN)
				{
					deathstring = " ate 2 loads of ";
					deathstring2 = "'s buckshot\n";
				}
				if (rnum == IT_NAILGUN)
				{
					deathstring = " was nailed by ";
					deathstring2 = "\n";
				}
				if (rnum == IT_SUPER_NAILGUN)
				{
					deathstring = " was punctured by ";
					deathstring2 = "\n";
				}
				if (rnum == IT_GRENADE_LAUNCHER)
				{
					deathstring = " eats ";
					deathstring2 = "'s pineapple\n";
					if (targ->v.health < -40)
					{
						deathstring = " was gibbed by ";
						deathstring2 = "'s grenade\n";
					}
				}
				if (rnum == IT_ROCKET_LAUNCHER)
				{
					deathstring = " rides ";
					deathstring2 = "'s rocket\n";
					if (targ->v.health < -40)
					{
						deathstring = " was gibbed by ";
						deathstring2 = "'s rocket\n";
					}
				}
				if (rnum == IT_LIGHTNING)
				{
					deathstring = " accepts ";
					if (attacker->v.waterlevel > 1)
						deathstring2 = "'s discharge\n";
					else
						deathstring2 = "'s shaft\n";
				}
				bprint(targ->v.netname);
				bprint(deathstring);
				bprint(attacker->v.netname);
				bprint(deathstring2);
			}
			return;
		}
		else
		{
			targ->v.frags = targ->v.frags - 1;		// killed self
			rnum = targ->v.watertype;

			bprint(targ->v.netname);
			if (rnum == -3)
			{
				if (random() < 0.5)
					bprint(" sleeps with the fishes\n");
				else
					bprint(" sucks it down\n");
				return;
			}
			else if (rnum == -4)
			{
				if (random() < 0.5)
					bprint(" gulped a load of slime\n");
				else
					bprint(" can't exist on slime alone\n");
				return;
			}
			else if (rnum == -5)
			{
				if (targ->v.health < -15)
				{
					bprint(" burst into flames\n");
					return;
				}
				if (random() < 0.5)
					bprint(" turned into hot slag\n");
				else
					bprint(" visits the Volcano God\n");
				return;
			}

			if ((int)attacker->v.flags & FL_MONSTER)
			{
				if (!strcmp(attacker->v.classname, "monster_army"))
					bprint(" was shot by a Grunt\n");
				if (!strcmp(attacker->v.classname, "monster_demon1"))
					bprint(" was eviscerated by a Fiend\n");
				if (!strcmp(attacker->v.classname, "monster_dog"))
					bprint(" was mauled by a Rottweiler\n");
				if (!strcmp(attacker->v.classname, "monster_dragon"))
					bprint(" was fried by a Dragon\n");
				if (!strcmp(attacker->v.classname, "monster_enforcer"))
					bprint(" was blasted by an Enforcer\n");
				if (!strcmp(attacker->v.classname, "monster_fish"))
					bprint(" was fed to the Rotfish\n");
				if (!strcmp(attacker->v.classname, "monster_hell_knight"))
					bprint(" was slain by a Death Knight\n");
				if (!strcmp(attacker->v.classname, "monster_knight"))
					bprint(" was slashed by a Knight\n");
				if (!strcmp(attacker->v.classname, "monster_ogre"))
					bprint(" was destroyed by an Ogre\n");
				if (!strcmp(attacker->v.classname, "monster_oldone"))
					bprint(" became one with Shub-Niggurath\n");
				if (!strcmp(attacker->v.classname, "monster_shalrath"))
					bprint(" was exploded by a Vore\n");
				if (!strcmp(attacker->v.classname, "monster_shambler"))
					bprint(" was smashed by a Shambler\n");
				if (!strcmp(attacker->v.classname, "monster_tarbaby"))
					bprint(" was slimed by a Spawn\n");
				if (!strcmp(attacker->v.classname, "monster_vomit"))
					bprint(" was vomited on by a Vomitus\n");
				if (!strcmp(attacker->v.classname, "monster_wizard"))
					bprint(" was scragged by a Scrag\n");
				if (!strcmp(attacker->v.classname, "monster_zombie"))
					bprint(" joins the Zombies\n");

				return;
			}
			if (!strcmp(attacker->v.classname, "explo_box"))
			{
				bprint(" blew up\n");
				return;
			}
			if (attacker->v.solid == SOLID_BSP && attacker != pr_global_struct->world)
			{
				bprint(" was squished\n");
				return;
			}
			if (!strcmp(targ->v.deathtype, "falling"))
			{
				targ->v.deathtype = "";
				bprint(" fell to his death\n");
				return;
			}
			if (!strcmp(attacker->v.classname, "trap_shooter") || !strcmp(attacker->v.classname, "trap_spikeshooter"))
			{
				bprint(" was spiked\n");
				return;
			}
			if (!strcmp(attacker->v.classname, "fireball"))
			{
				bprint(" ate a lavaball\n");
				return;
			}
			if (!strcmp(attacker->v.classname, "trigger_changelevel"))
			{
				bprint(" tried to leave\n");
				return;
			}

			bprint(" died\n");
		}
	}
}
