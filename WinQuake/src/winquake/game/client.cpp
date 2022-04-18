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
#include "client.h"

void Game::SetChangeParms(edict_t* self, float* parms)
{
	// remove items
	client->v.items = client->v.items - (((int)client->v.items) &
		(IT_KEY1 | IT_KEY2 | IT_INVISIBILITY | IT_INVULNERABILITY | IT_SUIT | IT_QUAD));

// cap super health
	if (client->v.health > 100)
		client->v.health = 100;
	if (client->v.health < 50)
		client->v.health = 50;
	parms[0] = client->v.items;
	parms[1] = client->v.health;
	parms[2] = client->v.armorvalue;
	if (client->v.ammo_shells < 25)
		parms[3] = 25;
	else
		parms[3] = client->v.ammo_shells;
	parms[4] = client->v.ammo_nails;
	parms[5] = client->v.ammo_rockets;
	parms[6] = client->v.ammo_cells;
	parms[7] = client->v.weapon;
	parms[8] = client->v.armortype * 100;
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

/*
============
ClientKill

Player entered the suicide command
============
*/
void Game::ClientKill(edict_t* self)
{
	//TODO
	/*
	bprint(client.netname);
	bprint(" suicides\n");
	set_suicide_frame();
	client.modelindex = modelindex_player;
	client.frags = client.frags - 2;	// extra penalty
	respawn();
	*/
}

/*
===========
PutClientInServer

called each time a player is spawned
============
*/
//void() DecodeLevelParms;
//void() PlayerDie;


{
	//TODO
	/*
	local	entity spot;

	self.classname = "player";
	self.health = 100;
	self.takedamage = DAMAGE_AIM;
	self.solid = SOLID_SLIDEBOX;
	self.movetype = MOVETYPE_WALK;
	self.show_hostile = 0;
	self.max_health = 100;
	self.flags = FL_CLIENT;
	self.air_finished = time + 12;
	self.dmg = 2;   		// initial water damage
	self.super_damage_finished = 0;
	self.radsuit_finished = 0;
	self.invisible_finished = 0;
	self.invincible_finished = 0;
	self.effects = 0;
	self.invincible_time = 0;

	DecodeLevelParms();

	W_SetCurrentAmmo();

	self.attack_finished = time;
	self.th_pain = player_pain;
	self.th_die = PlayerDie;

	self.deadflag = DEAD_NO;
void Game::PutClientInServer(edict_t* self)
	// paustime is set by teleporters to keep the player from moving a while
		self.pausetime = 0;

		spot = SelectSpawnPoint();

		self.origin = spot.origin + '0 0 1';
		self.angles = spot.angles;
		self.fixangle = TRUE;		// turn this way immediately

	// oh, this is a hack!
		setmodel(self, "progs/eyes.mdl");
		modelindex_eyes = self.modelindex;

		setmodel(self, "progs/player.mdl");
		modelindex_player = self.modelindex;

		setsize(self, VEC_HULL_MIN, VEC_HULL_MAX);

		self.view_ofs = '0 0 22';

		player_stand1();

		if (deathmatch || coop)
		{
			makevectors(self.angles);
			spawn_tfog(self.origin + v_forward * 20);
		}

		spawn_tdeath(self.origin, self);
		*/
}

/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void Game::PlayerPreThink(edict_t* self)
{
	//TODO
	/*
	local	float	mspeed, aspeed;
	local	float	r;

	if (intermission_running)
	{
		IntermissionThink();	// otherwise a button could be missed between
		return;					// the think tics
	}

	if (self.view_ofs == '0 0 0')
		return;		// intermission or finale

	makevectors(self.v_angle);		// is this still used

	CheckRules();
	WaterMove();

	if (self.waterlevel == 2)
		CheckWaterJump();

	if (self.deadflag >= DEAD_DEAD)
	{
		PlayerDeathThink();
		return;
	}

	if (self.deadflag == DEAD_DYING)
		return;	// dying, so do nothing

	if (self.button2)
	{
		PlayerJump();
	}
	else
		self.flags = self.flags | FL_JUMPRELEASED;

	// teleporters can force a non-moving pause time	
		if (time < self.pausetime)
			self.velocity = '0 0 0';
			*/
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void Game::PlayerPostThink(edict_t* self)
{
	//TODO
	/*
	local	float	mspeed, aspeed;
	local	float	r;

	if (self.view_ofs == '0 0 0')
		return;		// intermission or finale
	if (self.deadflag)
		return;

	// do weapon stuff

		W_WeaponFrame();

		// check to see if player landed and play landing sound	
			if ((self.jump_flag < -300) && (self.flags & FL_ONGROUND) && (self.health > 0))
			{
				if (self.watertype == CONTENT_WATER)
					sound(self, CHAN_BODY, "player/h2ojump.wav", 1, ATTN_NORM);
				else if (self.jump_flag < -650)
				{
					T_Damage(self, world, world, 5);
					sound(self, CHAN_VOICE, "player/land2.wav", 1, ATTN_NORM);
					self.deathtype = "falling";
				}
				else
					sound(self, CHAN_VOICE, "player/land.wav", 1, ATTN_NORM);

				self.jump_flag = 0;
			}

			if (!(self.flags & FL_ONGROUND))
				self.jump_flag = self.velocity_z;

			CheckPowerups();
			*/
}

/*
===========
ClientConnect

called when a player connects to a server
============
*/
void Game::ClientConnect(edict_t* self)
{
	//TODO
	/*
	bprint(client.netname);
	bprint(" entered the game\n");

	// a client connecting during an intermission can cause problems
		if (intermission_running)
			ExitIntermission();
			*/
};

/*
===========
ClientDisconnect

called when a player disconnects from a server
============
*/
void Game::ClientDisconnect(edict_t* self)
{
	//TODO
	/*
	if (gameover)
		return;
	// if the level end trigger has been activated, just return
	// since they aren't *really* leaving

	// let everyone else know
	bprint(client.netname);
	bprint(" left the game with ");
	bprint(ftos(client.frags));
	bprint(" frags\n");
	sound(client, CHAN_BODY, "player/tornoff2.wav", 1, ATTN_NONE);
	set_suicide_frame();
	*/
}
