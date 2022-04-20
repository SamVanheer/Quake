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
// sv_edict.c -- entity dictionary

#include "quakedef.h"
#include "game/IGame.h"

globalvars_t globalVars;
globalvars_t	*pr_global_struct = &globalVars;

/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict (edict_t *e)
{
	memset (&e->v, 0, sizeof(entvars_t));
	e->free = false;
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *ED_Alloc (void)
{
	int			i;
	edict_t		*e;

	for ( i=svs.maxclients+1 ; i<sv.num_edicts ; i++)
	{
		e = EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && ( e->freetime < 2 || sv.time - e->freetime > 0.5 ) )
		{
			ED_ClearEdict (e);
			return e;
		}
	}
	
	if (i == MAX_EDICTS)
		Sys_Error ("ED_Alloc: no free edicts");
		
	sv.num_edicts++;
	e = EDICT_NUM(i);
	ED_ClearEdict (e);

	return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free (edict_t *ed)
{
	SV_UnlinkEdict (ed);		// unlink from world bsp

	ed->free = true;
	ed->v.model = 0;
	ed->v.takedamage = 0;
	ed->v.modelindex = 0;
	ed->v.colormap = 0;
	ed->v.skin = 0;
	ed->v.frame = 0;
	VectorCopy (vec3_origin, ed->v.origin);
	VectorCopy (vec3_origin, ed->v.angles);
	ed->v.nextthink = -1;
	ed->v.solid = 0;
	
	ed->freetime = sv.time;
}

//===========================================================================

//TODO: don't rely on a hardcoded list
//Quick and dirty field list so entity spawning works.
struct fielddescription
{
	const char* Name;
	const etype_t Type;
	eval_t entvars_t::* const Member;
};

//Automatically deduce the type. Saves having to update it.
template<typename T>
constexpr etype_t DeduceType();

template<>
constexpr etype_t DeduceType<const char*>()
{
	return ev_string;
}

template<>
constexpr etype_t DeduceType<float>()
{
	return ev_float;
}

template<>
constexpr etype_t DeduceType<vec3_t>()
{
	return ev_vector;
}

template<>
constexpr etype_t DeduceType<edict_t*>()
{
	return ev_entity;
}

template<>
constexpr etype_t DeduceType<int>()
{
	return ev_int;
}

template<>
constexpr etype_t DeduceType<decltype(entvars_t::think)>()
{
	return ev_function;
}

template<>
constexpr etype_t DeduceType<decltype(entvars_t::touch)>()
{
	return ev_function;
}

template<>
constexpr etype_t DeduceType<decltype(entvars_t::th_pain)>()
{
	return ev_function;
}

template<>
constexpr etype_t DeduceType<decltype(entvars_t::animations_get)>()
{
	return ev_function;
}

#define ENT_FIELD(name) {#name, DeduceType<decltype(entvars_t::name)>(), reinterpret_cast<decltype(fielddescription::Member)>(&entvars_t::name)}

const fielddescription EntvarsFields[] =
{
	ENT_FIELD(modelindex),
	ENT_FIELD(absmin),
	ENT_FIELD(absmax),
	ENT_FIELD(ltime),
	ENT_FIELD(movetype),
	ENT_FIELD(solid),
	ENT_FIELD(origin),
	ENT_FIELD(oldorigin),
	ENT_FIELD(velocity),
	ENT_FIELD(angles),
	ENT_FIELD(avelocity),
	ENT_FIELD(punchangle),
	ENT_FIELD(classname),
	ENT_FIELD(model),
	ENT_FIELD(frame),
	ENT_FIELD(skin),
	ENT_FIELD(effects),
	ENT_FIELD(mins),
	ENT_FIELD(maxs),
	ENT_FIELD(size),
	ENT_FIELD(touch),
	ENT_FIELD(use),
	ENT_FIELD(think),
	ENT_FIELD(blocked),
	ENT_FIELD(nextthink),
	ENT_FIELD(groundentity),
	ENT_FIELD(health),
	ENT_FIELD(frags),
	ENT_FIELD(weapon),
	ENT_FIELD(weaponmodel),
	ENT_FIELD(weaponframe),
	ENT_FIELD(currentammo),
	ENT_FIELD(ammo_shells),
	ENT_FIELD(ammo_nails),
	ENT_FIELD(ammo_rockets),
	ENT_FIELD(ammo_cells),
	ENT_FIELD(items),
	ENT_FIELD(takedamage),
	ENT_FIELD(chain),
	ENT_FIELD(deadflag),
	ENT_FIELD(view_ofs),
	ENT_FIELD(button0),
	ENT_FIELD(button1),
	ENT_FIELD(button2),
	ENT_FIELD(impulse),
	ENT_FIELD(fixangle),
	ENT_FIELD(v_angle),
	ENT_FIELD(idealpitch),
	ENT_FIELD(netname),
	ENT_FIELD(enemy),
	ENT_FIELD(flags),
	ENT_FIELD(colormap),
	ENT_FIELD(team),
	ENT_FIELD(max_health),
	ENT_FIELD(teleport_time),
	ENT_FIELD(armortype),
	ENT_FIELD(armorvalue),
	ENT_FIELD(waterlevel),
	ENT_FIELD(watertype),
	ENT_FIELD(ideal_yaw),
	ENT_FIELD(yaw_speed),
	ENT_FIELD(aiment),
	ENT_FIELD(goalentity),
	ENT_FIELD(spawnflags),
	ENT_FIELD(target),
	ENT_FIELD(targetname),
	ENT_FIELD(dmg_take),
	ENT_FIELD(dmg_save),
	ENT_FIELD(dmg_inflictor),
	ENT_FIELD(owner),
	ENT_FIELD(movedir),
	ENT_FIELD(message),
	ENT_FIELD(sounds),
	ENT_FIELD(noise),
	ENT_FIELD(noise1),
	ENT_FIELD(noise2),
	ENT_FIELD(noise3),

	ENT_FIELD(wad),
	ENT_FIELD(map),
	ENT_FIELD(worldtype),

	ENT_FIELD(killtarget),

	ENT_FIELD(light_lev),
	ENT_FIELD(style),

	ENT_FIELD(touch),

	ENT_FIELD(th_stand),
	ENT_FIELD(th_walk),
	ENT_FIELD(th_run),
	ENT_FIELD(th_missile),
	ENT_FIELD(th_melee),
	ENT_FIELD(th_pain),
	ENT_FIELD(th_die),

	ENT_FIELD(oldenemy),

	ENT_FIELD(speed),
	ENT_FIELD(lefty),

	ENT_FIELD(search_time),
	ENT_FIELD(attack_state),

	ENT_FIELD(walkframe),

	ENT_FIELD(attack_finished),
	ENT_FIELD(pain_finished),

	ENT_FIELD(invincible_finished),
	ENT_FIELD(invisible_finished),
	ENT_FIELD(super_damage_finished),
	ENT_FIELD(radsuit_finished),

	ENT_FIELD(invincible_time),
	ENT_FIELD(invincible_sound),
	ENT_FIELD(invisible_time),
	ENT_FIELD(invisible_sound),
	ENT_FIELD(super_time),
	ENT_FIELD(super_sound),
	ENT_FIELD(rad_time),
	ENT_FIELD(fly_sound),

	ENT_FIELD(axhitme),

	ENT_FIELD(show_hostile),

	ENT_FIELD(jump_flag),
	ENT_FIELD(swim_flag),
	ENT_FIELD(air_finished),
	ENT_FIELD(bubble_count),
	ENT_FIELD(deathtype),

	ENT_FIELD(mdl),
	ENT_FIELD(mangle),

	ENT_FIELD(t_length),
	ENT_FIELD(t_width),

	ENT_FIELD(dest),
	ENT_FIELD(dest1),
	ENT_FIELD(dest2),

	ENT_FIELD(wait),
	ENT_FIELD(delay),

	ENT_FIELD(trigger_field),
	ENT_FIELD(noise4),

	ENT_FIELD(pausetime),
	ENT_FIELD(movetarget),


	ENT_FIELD(aflag),
	ENT_FIELD(dmg),

	ENT_FIELD(cnt),

	ENT_FIELD(think1),
	ENT_FIELD(finaldest),
	ENT_FIELD(finalangle),

	ENT_FIELD(count),


	ENT_FIELD(lip),
	ENT_FIELD(state),
	ENT_FIELD(pos1),
	ENT_FIELD(pos2),
	ENT_FIELD(height),

	ENT_FIELD(waitmin),
	ENT_FIELD(waitmax),
	ENT_FIELD(distance),
	ENT_FIELD(volume),

	ENT_FIELD(dmgtime),

	ENT_FIELD(healamount),
	ENT_FIELD(healtype),

	ENT_FIELD(inpain),

	ENT_FIELD(animation),
	ENT_FIELD(animations_get),

	ENT_FIELD(fish_swim_mode)
};

/*
============
ED_FindField
============
*/
const fielddescription* ED_FindField (const char *name)
{
	for (const auto& field : EntvarsFields)
	{
		if (!strcmp(field.Name, name))
		{
			return &field;
		}
	}

	return nullptr;
}

eval_t *GetEdictFieldValue(edict_t *ed, const char *field)
{
	//TODO: reimplement this functionality by merging in features from expansion packs.
	return nullptr;
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write (FILE *f, edict_t *ed)
{
	//TODO: reimplement
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts ()
{
	//TODO: reimplement
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f ()
{
	//TODO: reimplement
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count ()
{
	int active = 0, models = 0, solid = 0, step = 0;

	for (int i=0 ; i<sv.num_edicts ; i++)
	{
		auto ent = EDICT_NUM(i);
		if (ent->free)
			continue;
		active++;
		if (ent->v.solid)
			solid++;
		if (ent->v.model)
			models++;
		if (ent->v.movetype == MOVETYPE_STEP)
			step++;
	}

	Con_Printf ("num_edicts:%3i\n", sv.num_edicts);
	Con_Printf ("active    :%3i\n", active);
	Con_Printf ("view      :%3i\n", models);
	Con_Printf ("touch     :%3i\n", solid);
	Con_Printf ("step      :%3i\n", step);

}

/*
==============================================================================

					ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void ED_WriteGlobals (FILE *f)
{
	//TODO: reimplement
}

/*
=============
ED_ParseGlobals
=============
*/
void ED_ParseGlobals (char *data)
{
	//TODO: reimplement
}

//============================================================================


/*
=============
ED_NewString
=============
*/
char *ED_NewString (const char *string)
{	
	const int l = strlen(string) + 1;
	auto pszNew = reinterpret_cast<char*>( Hunk_Alloc (l) );
	auto new_p = pszNew;

	for (int i=0 ; i< l ; i++)
	{
		if (string[i] == '\\' && i < l-1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}
	
	return pszNew;
}


/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
bool ED_ParseEpair (entvars_t *base, const fielddescription* key, const char *s)
{
	auto d = &(base->*key->Member);
	
	switch (key->Type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		d->string = ED_NewString (s);
		break;
		
	case ev_float:
		d->_float = atof (s);
		break;

	case ev_int:
		d->_int = atoi(s);
		break;
		
	case ev_vector:
	{
		char string[128];
		strcpy(string, s);
		auto v = string;
		auto w = string;
		for (int i = 0; i < 3; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			d->vector[i] = atof(w);
			w = v = v + 1;
		}
		break;
	}
		
	case ev_entity:
		d->edict = EDICT_NUM(atoi (s));
		break;
		
	default:
		break;
	}
	return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
char *ED_ParseEdict (char *data, edict_t *ent)
{
	char keyname[256];

	bool init = false;

// clear it
	if (ent != sv.edicts)	// hack
		memset (&ent->v, 0, sizeof(entvars_t));

// go through all the dictionary pairs
	while (1)
	{	
	// parse key
		data = COM_Parse (data);
		if (com_token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");
		
		// anglehack is to allow QuakeEd to write single scalar angles
		// and allow them to be turned into vectors. (FIXME...)
		const bool anglehack = !strcmp(com_token, "angle");

		if (anglehack)
		{
			strcpy (com_token, "angles");
		}

		// FIXME: change light to _light to get rid of this hack
		if (!strcmp(com_token, "light"))
			strcpy (com_token, "light_lev");	// hack for single light def

		strcpy (keyname, com_token);

		// another hack to fix heynames with trailing spaces
		int n = strlen(keyname);
		while (n && keyname[n-1] == ' ')
		{
			keyname[n-1] = 0;
			n--;
		}

	// parse value	
		data = COM_Parse (data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		init = true;	

// keynames with a leading underscore are used for utility comments,
// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;
		
		auto key = ED_FindField (keyname);
		if (!key)
		{
			Con_Printf ("'%s' is not a field\n", keyname);
			continue;
		}

		if (anglehack)
		{
			char	temp[32];
			strcpy (temp, com_token);
			sprintf (com_token, "0 %s 0", temp);
		}

		if (!ED_ParseEpair (&ent->v, key, com_token))
			Host_Error ("ED_ParseEdict: parse error");
	}

	if (!init)
		ent->free = true;

	return data;
}

/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile (char *data)
{	
	edict_t* ent = NULL;
	int inhibit = 0;
	pr_global_struct->time = sv.time;
	
// parse ents
	while (1)
	{
// parse the opening brace	
		data = COM_Parse (data);
		if (!data)
			break;
		if (com_token[0] != '{')
			Sys_Error ("ED_LoadFromFile: found %s when expecting {",com_token);

		if (!ent)
		{
			ent = EDICT_NUM(0);
			//HACKHACK: set the global world pointer. TODO: remove
			pr_global_struct->world = ent;
		}
		else
			ent = ED_Alloc ();
		data = ED_ParseEdict (data, ent);

// remove things from different skill levels or deathmatch
		if (deathmatch.value)
		{
			if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH))
			{
				ED_Free (ent);	
				inhibit++;
				continue;
			}
		}
		else if ((current_skill == 0 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_EASY))
				|| (current_skill == 1 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM))
				|| (current_skill >= 2 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_HARD)) )
		{
			ED_Free (ent);	
			inhibit++;
			continue;
		}

//
// immediately call spawn function
//
		if (!ent->v.classname || !strcmp(ent->v.classname, ""))
		{
			Con_Printf ("No classname for entity\n");
			ED_Free (ent);
			continue;
		}

	// look for the spawn function
		if (!g_Game->SpawnEntity(ent, ent->v.classname))
		{
			Con_Printf("No spawn function for: %s\n", ent->v.classname);
			ED_Free(ent);
			continue;
		}
	}	

	Con_DPrintf ("%i entities inhibited\n", inhibit);
}

void PR_LoadProgs()
{
	//Zero out globals
	memset(pr_global_struct, 0, sizeof(*pr_global_struct));

	g_Game->NewMapStarted();
}

/*
===============
PR_Init
===============
*/
void PR_Init (void)
{
	Cmd_AddCommand ("edictcount", ED_Count);
}

edict_t *EDICT_NUM(int n)
{
	if (n < 0 || n >= sv.max_edicts)
		Sys_Error ("EDICT_NUM: bad number %i", n);
	return sv.edicts + n;
}

int NUM_FOR_EDICT(edict_t *e)
{
	const int b = e - sv.edicts;
	
	if (b < 0 || b >= sv.num_edicts)
		Sys_Error ("NUM_FOR_EDICT: bad pointer");
	return b;
}
