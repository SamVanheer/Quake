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

#include "quakedef.h"

/*
===============================================================================

						BUILT-IN FUNCTIONS

===============================================================================
*/

/*
=================
PF_errror

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

error(value)
=================
*/
void PF_error (const char* s)
{
	Host_Error ("Program error: %s", s);
}

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
void PF_objerror (const char* s)
{
	Host_Error ("Program error: %s", s);
}

/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void PF_makevectors (const float* angles)
{
	AngleVectors (angles, pr_global_struct->v_forward, pr_global_struct->v_right, pr_global_struct->v_up);
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
void PF_setorigin (edict_t* e, float* org)
{
	VectorCopy (org, e->v.origin);
	SV_LinkEdict (e, false);
}


void SetMinMaxSize (edict_t *e, const float *min, const float *max, bool rotate)
{
	vec3_t	rmin, rmax;
	
	for (int i=0 ; i<3 ; i++)
		if (min[i] > max[i])
			Host_Error("backwards mins/maxs");

	rotate = false;		// FIXME: implement rotation properly again

	if (!rotate)
	{
		VectorCopy (min, rmin);
		VectorCopy (max, rmax);
	}
	else
	{
	// find min / max for rotations
		const float* angles = e->v.angles;
		
		const float a = angles[1]/180 * M_PI;

		const float xvector[2] =
		{
			static_cast<float>(cos(a)),
			static_cast<float>(sin(a))
		};

		const float yvector[2] =
		{
			static_cast<float>(-sin(a)),
			static_cast<float>(cos(a))
		};
		
		float	bounds[2][3];
		VectorCopy (min, bounds[0]);
		VectorCopy (max, bounds[1]);
		
		rmin[0] = rmin[1] = rmin[2] = 9999;
		rmax[0] = rmax[1] = rmax[2] = -9999;

		vec3_t	base, transformed;
		
		for (int i=0 ; i<= 1 ; i++)
		{
			base[0] = bounds[i][0];
			for (int j=0 ; j<= 1 ; j++)
			{
				base[1] = bounds[j][1];
				for (int k=0 ; k<= 1 ; k++)
				{
					base[2] = bounds[k][2];
					
				// transform the point
					transformed[0] = xvector[0]*base[0] + yvector[0]*base[1];
					transformed[1] = xvector[1]*base[0] + yvector[1]*base[1];
					transformed[2] = base[2];
					
					for (int l=0 ; l<3 ; l++)
					{
						if (transformed[l] < rmin[l])
							rmin[l] = transformed[l];
						if (transformed[l] > rmax[l])
							rmax[l] = transformed[l];
					}
				}
			}
		}
	}
	
// set derived values
	VectorCopy (rmin, e->v.mins);
	VectorCopy (rmax, e->v.maxs);
	VectorSubtract (max, min, e->v.size);
	
	SV_LinkEdict (e, false);
}

/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void PF_setsize (edict_t* e, const float* min, const float* max)
{
	SetMinMaxSize (e, min, max, false);
}

/*
=================
PF_setmodel

setmodel(entity, model)
=================
*/
void PF_setmodel (edict_t* e, const char* m)
{
	const char **check = sv.model_precache;
	int i;

// check to see if model was properly precached
	for (i=0; *check ; i++, check++)
		if (!strcmp(*check, m))
			break;
			
	if (!*check)
		Host_Error("no precache: %s\n", m);

	e->v.model = m;
	e->v.modelindex = i; //SV_ModelIndex (m);

	auto mod = sv.models[ (int)e->v.modelindex];  // Mod_ForName (m, true);
	
	if (mod)
		SetMinMaxSize (e, mod->mins, mod->maxs, true);
	else
		SetMinMaxSize (e, vec3_origin, vec3_origin, true);
}

/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void PF_bprint (const char* s)
{
	SV_BroadcastPrintf ("%s", s);
}

/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void PF_sprint (edict_t* other, const char* s)
{
	const int entnum = NUM_FOR_EDICT(other);
	
	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("tried to sprint to a non-client\n");
		return;
	}
		
	auto client = &svs.clients[entnum-1];
		
	MSG_WriteChar (&client->message,svc_print);
	MSG_WriteString (&client->message, s);
}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void PF_centerprint (edict_t* other, const char* s)
{
	const int entnum = NUM_FOR_EDICT(other);
	
	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("tried to sprint to a non-client\n");
		return;
	}
		
	auto client = &svs.clients[entnum-1];
		
	MSG_WriteChar (&client->message,svc_centerprint);
	MSG_WriteString (&client->message, s);
}


/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void PF_normalize (const float* value1, float* result)
{
	float flNew = sqrt(value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2]);
	
	if (flNew == 0)
		result[0] = result[1] = result[2] = 0;
	else
	{
		flNew = 1/flNew;
		result[0] = value1[0] * flNew;
		result[1] = value1[1] * flNew;
		result[2] = value1[2] * flNew;
	}	
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
float PF_vlen (float* value1)
{
	return sqrt(value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2]);
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
float PF_vectoyaw (float* value1)
{
	float yaw;

	if (value1[1] == 0 && value1[0] == 0)
		yaw = 0;
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	return yaw;
}


/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
void PF_vectoangles (const float* value1, float* result)
{
	float yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		const float forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (int) (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	result[0] = pitch;
	result[1] = yaw;
	result[2] = 0;
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
float PF_random (void)
{
	return (rand ()&0x7fff) / ((float)0x7fff);
}

/*
=================
PF_particle

particle(origin, color, count)
=================
*/
void PF_particle (const float* org, const float* dir, float color, float count)
{
	SV_StartParticle (org, dir, color, count);
}

/*
=================
PF_ambientsound

=================
*/
void PF_ambientsound (float* pos, const char* samp, float vol, float attenuation)
{
	const char** check = sv.sound_precache;
	int soundnum;
	
// check to see if samp was properly precached
	for (soundnum=0; *check ; check++, soundnum++)
		if (!strcmp(*check,samp))
			break;
			
	if (!*check)
	{
		Con_Printf ("no precache: %s\n", samp);
		return;
	}

// add an svc_spawnambient command to the level signon packet

	MSG_WriteByte (&sv.signon,svc_spawnstaticsound);
	for (int i=0 ; i<3 ; i++)
		MSG_WriteCoord(&sv.signon, pos[i]);

	MSG_WriteByte (&sv.signon, soundnum);

	MSG_WriteByte (&sv.signon, vol*255);
	MSG_WriteByte (&sv.signon, attenuation*64);

}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void PF_sound (edict_t* entity, int channel, const char* sample, float volumeFloat, float attenuation)
{
	const int volume = volumeFloat * 255;
	
	if (volume < 0 || volume > 255)
		Sys_Error ("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	SV_StartSound (entity, channel, sample, volume, attenuation);
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void PF_traceline (const float* v1, const float* v2, int nomonsters, edict_t* ent)
{
	trace_t	trace = SV_Move (v1, vec3_origin, vec3_origin, v2, nomonsters, ent);

	pr_global_struct->trace_allsolid = static_cast<int>(trace.allsolid);
	pr_global_struct->trace_startsolid = static_cast<int>(trace.startsolid);
	pr_global_struct->trace_fraction = trace.fraction;
	pr_global_struct->trace_inwater = static_cast<int>(trace.inwater);
	pr_global_struct->trace_inopen = static_cast<int>(trace.inopen);
	VectorCopy (trace.endpos, pr_global_struct->trace_endpos);
	VectorCopy (trace.plane.normal, pr_global_struct->trace_plane_normal);
	pr_global_struct->trace_plane_dist =  trace.plane.dist;	
	if (trace.ent)
		pr_global_struct->trace_ent = trace.ent;
	else
		pr_global_struct->trace_ent = sv.edicts;
}

//============================================================================

byte	checkpvs[MAX_MAP_LEAFS/8];

int PF_newcheckclient (int check)
{
	edict_t	*ent;
	vec3_t	org;

// cycle to the next one

	if (check < 1)
		check = 1;
	if (check > svs.maxclients)
		check = svs.maxclients;

	int i = check == svs.maxclients ? 1 : check + 1;

	for ( ;  ; i++)
	{
		if (i == svs.maxclients+1)
			i = 1;

		ent = EDICT_NUM(i);

		if (i == check)
			break;	// didn't find anything else

		if (ent->free)
			continue;
		if (ent->v.health <= 0)
			continue;
		if ((int)ent->v.flags & FL_NOTARGET)
			continue;

	// anything that is a client, or has a client as an enemy
		break;
	}

// get the PVS for the entity
	VectorAdd (ent->v.origin, ent->v.view_ofs, org);
	auto leaf = Mod_PointInLeaf (org, sv.worldmodel);
	auto pvs = Mod_LeafPVS (leaf, sv.worldmodel);
	memcpy (checkpvs, pvs, (sv.worldmodel->numleafs+7)>>3 );

	return i;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define	MAX_CHECK	16
int c_invis, c_notvis;
edict_t* PF_checkclient (edict_t* self)
{
	vec3_t	view;
	
// find a new check if on a new frame
	if (sv.time - sv.lastchecktime >= 0.1)
	{
		sv.lastcheck = PF_newcheckclient (sv.lastcheck);
		sv.lastchecktime = sv.time;
	}

// return check if it might be visible	
	auto ent = EDICT_NUM(sv.lastcheck);
	if (ent->free || ent->v.health <= 0)
	{
		return sv.edicts;
	}

// if current entity can't possibly see the check entity, return 0
	VectorAdd (self->v.origin, self->v.view_ofs, view);
	auto leaf = Mod_PointInLeaf (view, sv.worldmodel);
	const int l = (leaf - sv.worldmodel->leafs) - 1;
	if ( (l<0) || !(checkpvs[l>>3] & (1<<(l&7)) ) )
	{
c_notvis++;
		return sv.edicts;
	}

// might be able to see it
c_invis++;
	return ent;
}

//============================================================================


/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF_stuffcmd (edict_t* ent, const char* str)
{
	const int entnum = NUM_FOR_EDICT(ent);
	if (entnum < 1 || entnum > svs.maxclients)
		Host_Error("Parm 0 not a client");
	
	auto old = host_client;
	host_client = &svs.clients[entnum-1];
	Host_ClientCommands ("%s", str);
	host_client = old;
}

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

localcmd (string)
=================
*/
void PF_localcmd (const char* str)
{
	Cbuf_AddText (str);
}

/*
=================
PF_cvar

float cvar (string)
=================
*/
float PF_cvar (const char* str)
{
	return Cvar_VariableValue (str);
}

/*
=================
PF_cvar_set

float cvar (string)
=================
*/
void PF_cvar_set (const char* var, const char* val)
{	
	Cvar_Set (var, val);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
edict_t* PF_findradius (const float* org, float rad)
{
	vec3_t	eorg;

	auto chain = sv.edicts;

	auto ent = sv.edicts + 1;
	for (int i=1 ; i<sv.num_edicts ; i++, ++ent)
	{
		if (ent->free)
			continue;
		if (ent->v.solid == SOLID_NOT)
			continue;
		for (int j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (ent->v.origin[j] + (ent->v.mins[j] + ent->v.maxs[j])*0.5);			
		if (Length(eorg) > rad)
			continue;
			
		ent->v.chain = chain;
		chain = ent;
	}

	return chain;
}

char	pr_string_temp[128];

const char* PF_ftos (float v)
{
	if (v == (int)v)
		sprintf (pr_string_temp, "%d",(int)v);
	else
		sprintf (pr_string_temp, "%5.1f",v);
	return pr_string_temp;
}

float PF_fabs (float v)
{
	return fabs(v);
}

const char* PF_vtos (const float* v)
{
	sprintf (pr_string_temp, "'%5.1f %5.1f %5.1f'", v[0], v[1], v[2]);
	return pr_string_temp;
}

edict_t* PF_Spawn ()
{
	auto ent = ED_Alloc();

	//Mimic QuakeC behavior where a null string is an empty string.
	//TODO: verify that !classname checks are updated everywhere.
	ent->v.classname = "";

	return ent;
}

void PF_Remove (edict_t* ed)
{
	ED_Free (ed);
}

//Quick and dirty list of fields used by find
struct fieldtable_t
{
	const char* name;
	const char* entvars_t::* member;
};

constexpr fieldtable_t entvarsFields[] =
{
	{"classname", &entvars_t::classname},
	{"targetname", &entvars_t::targetname},
	{"target", &entvars_t::target}
};

// entity (entity start, .string field, string match) find = #5;
edict_t* PF_Find (edict_t* ent, const char* f, const char* s)
{
	int e = NUM_FOR_EDICT(ent);

	if (!s)
		Host_Error("PF_Find: bad search string");

	decltype(fieldtable_t::member) member = nullptr;

	for (const auto& field : entvarsFields)
	{
		if (!strcmp(f, field.name))
		{
			member = field.member;
			break;
		}
	}

	//TODO: QuakeC evaluates the world to false in conditional checks, so maybe just return nullptr here instead.

	if (!member)
	{
		//Can't find anything.
		return sv.edicts;
	}
		
	for (e++ ; e < sv.num_edicts ; e++)
	{
		auto ed = EDICT_NUM(e);
		if (ed->free)
			continue;

		auto t = ed->v.*member;

		if (!t)
			continue;
		if (!strcmp(t, s))
		{
			return ed;
		}
	}

	return sv.edicts;
}

void PR_CheckEmptyString (const char *s)
{
	if (s[0] <= ' ')
		Host_Error("Bad string");
}

void PF_precache_file (const char* s)
{
	// precache_file is only used to copy files with qcc, it does nothing
}

void PF_precache_sound (const char* s)
{
	if (sv.state != ss_loading)
		Host_Error("PF_Precache_*: Precache can only be done in spawn functions");
		
	PR_CheckEmptyString (s);
	
	for (int i=0 ; i<MAX_SOUNDS ; i++)
	{
		if (!sv.sound_precache[i])
		{
			sv.sound_precache[i] = s;
			return;
		}
		if (!strcmp(sv.sound_precache[i], s))
			return;
	}
	Host_Error("PF_precache_sound: overflow");
}

void PF_precache_model (const char* s)
{
	if (sv.state != ss_loading)
		Host_Error("PF_Precache_*: Precache can only be done in spawn functions");
	
	PR_CheckEmptyString (s);

	for (int i=0 ; i<MAX_MODELS ; i++)
	{
		if (!sv.model_precache[i])
		{
			sv.model_precache[i] = s;
			sv.models[i] = Mod_ForName (s, true);
			return;
		}
		if (!strcmp(sv.model_precache[i], s))
			return;
	}
	Host_Error("PF_precache_model: overflow");
}

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
bool PF_walkmove (edict_t* ent, float yaw, float dist)
{
	if ( !( (int)ent->v.flags & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
	{
		return false;
	}

	yaw = yaw*M_PI*2 / 360;
	
	vec3_t move;

	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;
	
	return static_cast<int>(SV_movestep(ent, move, true));
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
int PF_droptofloor (edict_t* ent)
{
	vec3_t		end;

	VectorCopy (ent->v.origin, end);
	end[2] -= 256;
	
	const auto trace = SV_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

	if (trace.fraction == 1 || trace.allsolid)
		return 0;
	else
	{
		VectorCopy (trace.endpos, ent->v.origin);
		SV_LinkEdict (ent, false);
		ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
		ent->v.groundentity = trace.ent;
		return 1;
	}
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void PF_lightstyle (int style, const char* val)
{
// change the string in sv
	sv.lightstyles[style] = val;
	
// send message to all clients on this server
	if (sv.state != ss_active)
		return;

	auto client = svs.clients;
	
	for (int j = 0; j < svs.maxclients; j++, client++)
	{
		if (client->active || client->spawned)
		{
			MSG_WriteChar(&client->message, svc_lightstyle);
			MSG_WriteChar(&client->message, style);
			MSG_WriteString(&client->message, val);
		}
	}
}

float PF_rint (float f)
{
	if (f > 0)
		return (int)(f + 0.5);
	else
		return (int)(f - 0.5);
}
float PF_floor (float value)
{
	return floor(value);
}
float PF_ceil (float value)
{
	return ceil(value);
}


/*
=============
PF_checkbottom
=============
*/
bool PF_checkbottom (edict_t* ent)
{
	return SV_CheckBottom (ent);
}

/*
=============
PF_pointcontents
=============
*/
float PF_pointcontents (const float* v)
{
	return SV_PointContents (v);	
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
edict_t* PF_nextent (edict_t* ent)
{
	int i = NUM_FOR_EDICT(ent);
	while (1)
	{
		i++;
		if (i == sv.num_edicts)
		{
			break;
		}
		ent = EDICT_NUM(i);
		if (!ent->free)
		{
			return ent;
		}
	}

	return sv.edicts;
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
cvar_t	sv_aim = {"sv_aim", "0.93"};
void PF_aim (edict_t* ent, float speed, float* aimDirection)
{
	vec3_t	start, dir, end, bestdir;
	trace_t	tr;
	float	dist, bestdist;

	VectorCopy (ent->v.origin, start);
	start[2] += 20;

// try sending a trace straight
	VectorCopy (pr_global_struct->v_forward, dir);
	VectorMA (start, 2048, dir, end);
	tr = SV_Move (start, vec3_origin, vec3_origin, end, false, ent);
	if (tr.ent && tr.ent->v.takedamage == DAMAGE_AIM
	&& (!teamplay.value || ent->v.team <=0 || ent->v.team != tr.ent->v.team) )
	{
		VectorCopy (pr_global_struct->v_forward, aimDirection);
		return;
	}


// try all possible entities
	VectorCopy (dir, bestdir);
	bestdist = sv_aim.value;
	edict_t* bestent = NULL;
	
	auto check = sv.edicts + 1;
	for (int i=1 ; i<sv.num_edicts ; i++, ++check )
	{
		if (check->v.takedamage != DAMAGE_AIM)
			continue;
		if (check == ent)
			continue;
		if (teamplay.value && ent->v.team > 0 && ent->v.team == check->v.team)
			continue;	// don't aim at teammate
		for (int j=0 ; j<3 ; j++)
			end[j] = check->v.origin[j]
			+ 0.5*(check->v.mins[j] + check->v.maxs[j]);
		VectorSubtract (end, start, dir);
		VectorNormalize (dir);
		dist = DotProduct (dir, pr_global_struct->v_forward);
		if (dist < bestdist)
			continue;	// to far to turn
		tr = SV_Move (start, vec3_origin, vec3_origin, end, false, ent);
		if (tr.ent == check)
		{	// can shoot at this one
			bestdist = dist;
			bestent = check;
		}
	}
	
	if (bestent)
	{
		VectorSubtract (bestent->v.origin, ent->v.origin, dir);
		dist = DotProduct (dir, pr_global_struct->v_forward);
		VectorScale (pr_global_struct->v_forward, dist, end);
		end[2] = dir[2];
		VectorNormalize (end);
		VectorCopy (end, aimDirection);
	}
	else
	{
		VectorCopy (bestdir, aimDirection);
	}
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void PF_changeyaw (edict_t* ent)
{
	const float current = anglemod( ent->v.angles[1] );
	const float ideal = ent->v.ideal_yaw;
	const float speed = ent->v.yaw_speed;
	
	if (current == ideal)
		return;
	float move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}
	
	ent->v.angles[1] = anglemod (current + move);
}

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define	MSG_BROADCAST	0		// unreliable to all
#define	MSG_ONE			1		// reliable to one (msg_entity)
#define	MSG_ALL			2		// reliable to all
#define	MSG_INIT		3		// write to the init string

sizebuf_t *WriteDest (int dest, edict_t* ent)
{
	switch (dest)
	{
	case MSG_BROADCAST:
		return &sv.datagram;
	
	case MSG_ONE:
	{
		const int entnum = NUM_FOR_EDICT(ent);
		if (entnum < 1 || entnum > svs.maxclients)
			Host_Error("WriteDest: not a client");
		return &svs.clients[entnum - 1].message;
	}
		
	case MSG_ALL:
		return &sv.reliable_datagram;
	
	case MSG_INIT:
		return &sv.signon;

	default:
		Host_Error("WriteDest: bad destination");
		break;
	}
	
	return NULL;
}

void PF_WriteByte (int dest, int value, edict_t* ent)
{
	MSG_WriteByte (WriteDest(dest, ent), value);
}

void PF_WriteChar (int dest, int value, edict_t* ent)
{
	MSG_WriteChar (WriteDest(dest, ent), value);
}

void PF_WriteShort (int dest, int value, edict_t* ent)
{
	MSG_WriteShort (WriteDest(dest, ent), value);
}

void PF_WriteLong (int dest, int value, edict_t* ent)
{
	MSG_WriteLong (WriteDest(dest, ent), value);
}

void PF_WriteAngle (int dest, float value, edict_t* ent)
{
	MSG_WriteAngle (WriteDest(dest, ent), value);
}

void PF_WriteCoord (int dest, float value, edict_t* ent)
{
	MSG_WriteCoord (WriteDest(dest, ent), value);
}

void PF_WriteString (int dest, const char* string, edict_t* ent)
{
	MSG_WriteString (WriteDest(dest, ent), string);
}

void PF_WriteEntity (int dest, edict_t* ent, edict_t* destEnt)
{
	MSG_WriteShort (WriteDest(dest, destEnt), NUM_FOR_EDICT(ent));
}

//=============================================================================

int SV_ModelIndex (const char *name);

void PF_makestatic (edict_t* ent)
{
	MSG_WriteByte (&sv.signon,svc_spawnstatic);

	MSG_WriteByte (&sv.signon, SV_ModelIndex(ent->v.model));

	MSG_WriteByte (&sv.signon, ent->v.frame);
	MSG_WriteByte (&sv.signon, ent->v.colormap);
	MSG_WriteByte (&sv.signon, ent->v.skin);
	for (int i=0 ; i<3 ; i++)
	{
		MSG_WriteCoord(&sv.signon, ent->v.origin[i]);
		MSG_WriteAngle(&sv.signon, ent->v.angles[i]);
	}

// throw the entity away now
	ED_Free (ent);
}

//=============================================================================

/*
==============
PF_setspawnparms
==============
*/
void PF_setspawnparms (edict_t* ent)
{
	const int clientIndex = NUM_FOR_EDICT(ent);
	if (clientIndex < 1 || clientIndex > svs.maxclients)
		Host_Error("Entity is not a client");

	// copy spawn parms out of the client_t
	auto client = svs.clients + (clientIndex -1);

	for (int i=0 ; i< NUM_SPAWN_PARMS ; i++)
		(&pr_global_struct->parm1)[i] = client->spawn_parms[i];
}

/*
==============
PF_changelevel
==============
*/
void PF_changelevel (const char* mapname)
{
// make sure we don't issue two changelevels
	if (svs.changelevel_issued)
		return;
	svs.changelevel_issued = true;
	
	Cbuf_AddText (va("changelevel %s\n", mapname));
}

void dprint(const char* s)
{
	Con_DPrintf(s);
}
