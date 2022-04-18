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
#include "weapons.h"

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

void InitBodyQue();

//=======================
/*QUAKED worldspawn (0 0 0) ?
Only used for the world entity.
Set message to the level name.
Set sounds to the cd track to play.

World Types:
0: medieval
1: metal
2: base
*/
//=======================
void worldspawn(edict_t* self)
{
	lastspawn = pr_global_struct->world;
	InitBodyQue();

	// custom map attributes
	if (!strcmp(self->v.model, "maps/e1m8.bsp"))
		PF_cvar_set("sv_gravity", "100");
	else
		PF_cvar_set("sv_gravity", "800");

	// the area based ambient sounds MUST be the first precache_sounds

	// player precaches	
	W_Precache(self);			// get weapon precaches

	// sounds used from C physics code
	PF_precache_sound("demon/dland2.wav");		// landing thud
	PF_precache_sound("misc/h2ohit1.wav");		// landing splash

	// setup precaches allways needed
	PF_precache_sound("items/itembk2.wav");		// item respawn sound
	PF_precache_sound("player/plyrjmp8.wav");		// player jump
	PF_precache_sound("player/land.wav");			// player landing
	PF_precache_sound("player/land2.wav");		// player hurt landing
	PF_precache_sound("player/drown1.wav");		// drowning pain
	PF_precache_sound("player/drown2.wav");		// drowning pain
	PF_precache_sound("player/gasp1.wav");		// gasping for air
	PF_precache_sound("player/gasp2.wav");		// taking breath
	PF_precache_sound("player/h2odeath.wav");		// drowning death

	PF_precache_sound("misc/talk.wav");			// talk
	PF_precache_sound("player/teledth1.wav");		// telefrag
	PF_precache_sound("misc/r_tele1.wav");		// teleport sounds
	PF_precache_sound("misc/r_tele2.wav");
	PF_precache_sound("misc/r_tele3.wav");
	PF_precache_sound("misc/r_tele4.wav");
	PF_precache_sound("misc/r_tele5.wav");
	PF_precache_sound("weapons/lock4.wav");		// ammo pick up
	PF_precache_sound("weapons/pkup.wav");		// weapon up
	PF_precache_sound("items/armor1.wav");		// armor up
	PF_precache_sound("weapons/lhit.wav");		//lightning
	PF_precache_sound("weapons/lstart.wav");		//lightning start
	PF_precache_sound("items/damage3.wav");

	PF_precache_sound("misc/power.wav");			//lightning for boss

	// player gib sounds
	PF_precache_sound("player/gib.wav");			// player gib sound
	PF_precache_sound("player/udeath.wav");		// player gib sound
	PF_precache_sound("player/tornoff2.wav");		// gib sound

	// player pain sounds

	PF_precache_sound("player/pain1.wav");
	PF_precache_sound("player/pain2.wav");
	PF_precache_sound("player/pain3.wav");
	PF_precache_sound("player/pain4.wav");
	PF_precache_sound("player/pain5.wav");
	PF_precache_sound("player/pain6.wav");

	// player death sounds
	PF_precache_sound("player/death1.wav");
	PF_precache_sound("player/death2.wav");
	PF_precache_sound("player/death3.wav");
	PF_precache_sound("player/death4.wav");
	PF_precache_sound("player/death5.wav");

	// ax sounds	
	PF_precache_sound("weapons/ax1.wav");			// ax swoosh
	PF_precache_sound("player/axhit1.wav");		// ax hit meat
	PF_precache_sound("player/axhit2.wav");		// ax hit world

	PF_precache_sound("player/h2ojump.wav");		// player jumping into water
	PF_precache_sound("player/slimbrn2.wav");		// player enter slime
	PF_precache_sound("player/inh2o.wav");		// player enter water
	PF_precache_sound("player/inlava.wav");		// player enter lava
	PF_precache_sound("misc/outwater.wav");		// leaving water sound

	PF_precache_sound("player/lburn1.wav");		// lava burn
	PF_precache_sound("player/lburn2.wav");		// lava burn

	PF_precache_sound("misc/water1.wav");			// swimming
	PF_precache_sound("misc/water2.wav");			// swimming

	PF_precache_model("progs/player.mdl");
	PF_precache_model("progs/eyes.mdl");
	PF_precache_model("progs/h_player.mdl");
	PF_precache_model("progs/gib1.mdl");
	PF_precache_model("progs/gib2.mdl");
	PF_precache_model("progs/gib3.mdl");

	PF_precache_model("progs/s_bubble.spr");	// drowning bubbles
	PF_precache_model("progs/s_explod.spr");	// sprite explosion

	PF_precache_model("progs/v_axe.mdl");
	PF_precache_model("progs/v_shot.mdl");
	PF_precache_model("progs/v_nail.mdl");
	PF_precache_model("progs/v_rock.mdl");
	PF_precache_model("progs/v_shot2.mdl");
	PF_precache_model("progs/v_nail2.mdl");
	PF_precache_model("progs/v_rock2.mdl");

	PF_precache_model("progs/bolt.mdl");		// for lightning gun
	PF_precache_model("progs/bolt2.mdl");		// for lightning gun
	PF_precache_model("progs/bolt3.mdl");		// for boss shock
	PF_precache_model("progs/lavaball.mdl");	// for testing

	PF_precache_model("progs/missile.mdl");
	PF_precache_model("progs/grenade.mdl");
	PF_precache_model("progs/spike.mdl");
	PF_precache_model("progs/s_spike.mdl");

	PF_precache_model("progs/backpack.mdl");

	PF_precache_model("progs/zom_gib.mdl");

	PF_precache_model("progs/v_light.mdl");


	//
	// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
	//

	// 0 normal
	PF_lightstyle(0, "m");

	// 1 FLICKER (first variety)
	PF_lightstyle(1, "mmnmmommommnonmmonqnmmo");

	// 2 SLOW STRONG PULSE
	PF_lightstyle(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	// 3 CANDLE (first variety)
	PF_lightstyle(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	// 4 FAST STROBE
	PF_lightstyle(4, "mamamamamama");

	// 5 GENTLE PULSE 1
	PF_lightstyle(5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	// 6 FLICKER (second variety)
	PF_lightstyle(6, "nmonqnmomnmomomno");

	// 7 CANDLE (second variety)
	PF_lightstyle(7, "mmmaaaabcdefgmmmmaaaammmaamm");

	// 8 CANDLE (third variety)
	PF_lightstyle(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	// 9 SLOW STROBE (fourth variety)
	PF_lightstyle(9, "aaaaaaaazzzzzzzz");

	// 10 FLUORESCENT FLICKER
	PF_lightstyle(10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	PF_lightstyle(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	PF_lightstyle(63, "a");
}

LINK_ENTITY_TO_CLASS(worldspawn);

/*
==============================================================================

BODY QUE

==============================================================================
*/

edict_t* bodyque_head;

void bodyque(edict_t* self)
{	// just here so spawn functions don't complain after the world
	// creates bodyques
}

void InitBodyQue()
{
	bodyque_head = PF_Spawn();
	bodyque_head->v.classname = "bodyque";
	bodyque_head->v.owner = PF_Spawn();
	bodyque_head->v.owner->v.classname = "bodyque";
	bodyque_head->v.owner->v.owner = PF_Spawn();
	bodyque_head->v.owner->v.owner->v.classname = "bodyque";
	bodyque_head->v.owner->v.owner->v.owner = PF_Spawn();
	bodyque_head->v.owner->v.owner->v.owner->v.classname = "bodyque";
	bodyque_head->v.owner->v.owner->v.owner->v.owner = bodyque_head;
}


// make a body que entry for the given ent so the ent can be
// respawned elsewhere
void CopyToBodyQue(edict_t* ent)
{
	VectorCopy(ent->v.angles, bodyque_head->v.angles);
	bodyque_head->v.model = ent->v.model;
	bodyque_head->v.modelindex = ent->v.modelindex;
	bodyque_head->v.frame = ent->v.frame;
	bodyque_head->v.colormap = ent->v.colormap;
	bodyque_head->v.movetype = ent->v.movetype;
	VectorCopy(ent->v.velocity, bodyque_head->v.velocity);
	bodyque_head->v.flags = 0;
	PF_setorigin(bodyque_head, ent->v.origin);
	PF_setsize(bodyque_head, ent->v.mins, ent->v.maxs);
	bodyque_head = bodyque_head->v.owner;
}
