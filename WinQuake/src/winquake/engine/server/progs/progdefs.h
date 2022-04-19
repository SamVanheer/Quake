/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#pragma once

struct Animations;
struct edict_s;

typedef struct
{
	edict_s* world;
	float	time;
	float	frametime;
	float	force_retouch;
	const char*	mapname;
	float	deathmatch;
	float	coop;
	float	teamplay;
	int	serverflags;
	float	total_secrets;
	float	total_monsters;
	float	found_secrets;
	float	killed_monsters;
	float	parm1;
	float	parm2;
	float	parm3;
	float	parm4;
	float	parm5;
	float	parm6;
	float	parm7;
	float	parm8;
	float	parm9;
	float	parm10;
	float	parm11;
	float	parm12;
	float	parm13;
	float	parm14;
	float	parm15;
	float	parm16;
	vec3_t	v_forward;
	vec3_t	v_up;
	vec3_t	v_right;
	float	trace_allsolid;
	float	trace_startsolid;
	float	trace_fraction;
	vec3_t	trace_endpos;
	vec3_t	trace_plane_normal;
	float	trace_plane_dist;
	edict_s* trace_ent;
	float	trace_inopen;
	float	trace_inwater;
	edict_s* msg_entity;
} globalvars_t;

struct entvars_t
{
	float	modelindex;
	vec3_t	absmin;
	vec3_t	absmax;
	float	ltime;
	float	movetype;
	float	solid;
	vec3_t	origin;
	vec3_t	oldorigin;
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t	avelocity;
	vec3_t	punchangle;
	const char*	classname;
	const char*	model;
	float	frame;
	float	skin;
	int	effects;
	vec3_t	mins;
	vec3_t	maxs;
	vec3_t	size;
	void (*touch)(edict_s*, edict_s*);
	void (*use)(edict_s*, edict_s*);
	void (*think)(edict_s*);
	void (*blocked)(edict_s*, edict_s*);
	float	nextthink;
	edict_s* groundentity;
	float	health;
	float	frags;
	int	weapon;
	const char* weaponmodel;
	float	weaponframe;
	float	currentammo;
	float	ammo_shells;
	float	ammo_nails;
	float	ammo_rockets;
	float	ammo_cells;
	int	items;
	float	takedamage;
	edict_s* chain;
	float	deadflag;
	vec3_t	view_ofs;
	float	button0;
	float	button1;
	float	button2;
	float	impulse;
	float	fixangle;
	vec3_t	v_angle;
	float	idealpitch;
	const char* netname;
	edict_s* enemy;
	int	flags;
	float	colormap;
	float	team;
	float	max_health;
	float	teleport_time;
	float	armortype;
	float	armorvalue;
	float	waterlevel;
	float	watertype;
	float	ideal_yaw;
	float	yaw_speed;
	edict_s* aiment;
	edict_s* goalentity;
	int	spawnflags;
	const char*	target;
	const char*	targetname;
	float	dmg_take;
	float	dmg_save;
	edict_s* dmg_inflictor;
	edict_s* owner;
	vec3_t	movedir;
	const char* message;
	float	sounds;
	const char*	noise;
	const char*	noise1;
	const char*	noise2;
	const char*	noise3;

	//QuakeC only variables.
	//
	// world fields (FIXME: make globals)
	//
	const char*	wad;
	const char* 	map;
	float		worldtype;	// 0=medieval 1=metal 2=base

	//================================================

	const char* killtarget;

	//
	// quakeed fields
	//
	float		light_lev;		// not used by game, but parsed by light util
	float		style;


	//
	// monster ai
	//
	void (*th_stand)(edict_s*);
	void (*th_walk)(edict_s*);
	void (*th_run)(edict_s*);
	void (*th_missile)(edict_s*);
	void (*th_melee)(edict_s*);
	void (*th_pain)(edict_s*, edict_s* attacker, float damage);
	void (*th_die)(edict_s*);

	edict_s*	oldenemy;		// mad at this player before taking damage

	float	speed;

	float	lefty;

	float	search_time;
	float	attack_state;

	//
	// player only fields
	//
	float		walkframe;

	float 		attack_finished;
	float		pain_finished;

	float		invincible_finished;
	float		invisible_finished;
	float		super_damage_finished;
	float		radsuit_finished;

	float		invincible_time, invincible_sound;
	float		invisible_time, invisible_sound;
	float		super_time, super_sound;
	float		rad_time;
	float		fly_sound;

	float		axhitme;

	float		show_hostile;	// set to time+0.2 whenever a client fires a
							// weapon or takes damage.  Used to alert
							// monsters that otherwise would let the player go
	float		jump_flag;		// player jump flag
	float		swim_flag;		// player swimming sound flag
	float		air_finished;	// when time > air_finished, start drowning
	float		bubble_count;	// keeps track of the number of bubbles
	const char* deathtype;		// keeps track of how the player died

	//
	// object stuff
	//
	const char* mdl;
	vec3_t		mangle;			// angle at start

	//vec3_t		oldorigin;		// only used by secret door

	float		t_length, t_width;


	//
	// doors, etc
	//
	vec3_t		dest, dest1, dest2;
	float		wait;			// time from firing to restarting
	float		delay;			// time from activation to firing
	edict_s*	trigger_field;	// door's trigger entity
	const char* noise4;

	//
	// monsters
	//
	float 		pausetime;
	edict_s*	movetarget;


	//
	// doors
	//
	float		aflag;
	float		dmg;			// damage done by door when hit

	//
	// misc
	//
	float		cnt; 			// misc flag

	//
	// subs
	//
	void (*think1)(edict_s*);
	vec3_t		finaldest, finalangle;

	//
	// triggers
	//
	float		count;			// for counting triggers


	//
	// plats / doors / buttons
	//
	float	lip;
	float	state;
	vec3_t	pos1, pos2;		// top and bottom positions
	float	height;

	//
	// sounds
	//
	float	waitmin, waitmax;
	float	distance;
	float	volume;

	//client.qc
	float dmgtime;

	//items.qc
	float	healamount, healtype;

	//zombie.qc
	float inpain;

	//New variables
	const char* animation;
	const Animations* (*animations_get)(edict_s*);
};
