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

struct edict_s;

typedef struct
{
	int	pad[28];
	edict_s*	self;
	edict_s* other;
	edict_s* world;
	float	time;
	float	frametime;
	float	force_retouch;
	const char*	mapname;
	float	deathmatch;
	float	coop;
	float	teamplay;
	float	serverflags;
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
	func_t	main;
	func_t	StartFrame;
	func_t	PlayerPreThink;
	func_t	PlayerPostThink;
	func_t	ClientKill;
	func_t	ClientConnect;
	func_t	PutClientInServer;
	func_t	ClientDisconnect;
	func_t	SetNewParms;
	func_t	SetChangeParms;
} globalvars_t;

typedef struct
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
	float	effects;
	vec3_t	mins;
	vec3_t	maxs;
	vec3_t	size;
	func_t	touch;
	func_t	use;
	func_t	think;
	func_t	blocked;
	float	nextthink;
	edict_s* groundentity;
	float	health;
	float	frags;
	float	weapon;
	const char* weaponmodel;
	float	weaponframe;
	float	currentammo;
	float	ammo_shells;
	float	ammo_nails;
	float	ammo_rockets;
	float	ammo_cells;
	float	items;
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
	float	flags;
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
	float	spawnflags;
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
} entvars_t;
