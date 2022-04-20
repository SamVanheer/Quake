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

/**
*	@brief Size of game variable types, as multiples of 4.
*/
constexpr int type_size[etypes_count] = {1,sizeof(const char*) / 4,1,3,1,sizeof(decltype(entvars_t::think)) / 4, 1};

bool ED_ParseEpair(void* base, const fielddescription& key, const char* s);

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

#define OBJ_FIELD(name) {#name, DeduceType<decltype(globalvars_t::name)>(), offsetof(globalvars_t, name)}

const fielddescription GlobalvarsFields[] =
{
	OBJ_FIELD(world),
	OBJ_FIELD(time),
	OBJ_FIELD(frametime),
	OBJ_FIELD(force_retouch),
	OBJ_FIELD(mapname),
	OBJ_FIELD(deathmatch),
	OBJ_FIELD(coop),
	OBJ_FIELD(teamplay),
	OBJ_FIELD(serverflags),
	OBJ_FIELD(total_secrets),
	OBJ_FIELD(total_monsters),
	OBJ_FIELD(found_secrets),
	OBJ_FIELD(killed_monsters),
	OBJ_FIELD(parm1),
	OBJ_FIELD(parm2),
	OBJ_FIELD(parm3),
	OBJ_FIELD(parm4),
	OBJ_FIELD(parm5),
	OBJ_FIELD(parm6),
	OBJ_FIELD(parm7),
	OBJ_FIELD(parm8),
	OBJ_FIELD(parm9),
	OBJ_FIELD(parm10),
	OBJ_FIELD(parm11),
	OBJ_FIELD(parm12),
	OBJ_FIELD(parm13),
	OBJ_FIELD(parm14),
	OBJ_FIELD(parm15),
	OBJ_FIELD(parm16),
	OBJ_FIELD(v_forward),
	OBJ_FIELD(v_up),
	OBJ_FIELD(v_right),
	OBJ_FIELD(trace_allsolid),
	OBJ_FIELD(trace_startsolid),
	OBJ_FIELD(trace_fraction),
	OBJ_FIELD(trace_endpos),
	OBJ_FIELD(trace_plane_normal),
	OBJ_FIELD(trace_plane_dist),
	OBJ_FIELD(trace_ent),
	OBJ_FIELD(trace_inopen),
	OBJ_FIELD(trace_inwater),
	OBJ_FIELD(msg_entity),

	OBJ_FIELD(gameover),

	OBJ_FIELD(activator),

	OBJ_FIELD(damage_attacker),
	OBJ_FIELD(framecount),

	OBJ_FIELD(game_skill),

	OBJ_FIELD(intermission_running),
	OBJ_FIELD(intermission_exittime),

	OBJ_FIELD(sight_entity),
	OBJ_FIELD(sight_entity_time),

	OBJ_FIELD(nextmap),

	OBJ_FIELD(hknight_type),

	OBJ_FIELD(le2),
	OBJ_FIELD(lightning_end),

	OBJ_FIELD(shub)
};

#undef OBJ_FIELD
#define OBJ_FIELD(name) {#name, DeduceType<decltype(entvars_t::name)>(), offsetof(entvars_t, name)}

const fielddescription EntvarsFields[] =
{
	OBJ_FIELD(modelindex),
	OBJ_FIELD(absmin),
	OBJ_FIELD(absmax),
	OBJ_FIELD(ltime),
	OBJ_FIELD(movetype),
	OBJ_FIELD(solid),
	OBJ_FIELD(origin),
	OBJ_FIELD(oldorigin),
	OBJ_FIELD(velocity),
	OBJ_FIELD(angles),
	OBJ_FIELD(avelocity),
	OBJ_FIELD(punchangle),
	OBJ_FIELD(classname),
	OBJ_FIELD(model),
	OBJ_FIELD(frame),
	OBJ_FIELD(skin),
	OBJ_FIELD(effects),
	OBJ_FIELD(mins),
	OBJ_FIELD(maxs),
	OBJ_FIELD(size),
	OBJ_FIELD(touch),
	OBJ_FIELD(use),
	OBJ_FIELD(think),
	OBJ_FIELD(blocked),
	OBJ_FIELD(nextthink),
	OBJ_FIELD(groundentity),
	OBJ_FIELD(health),
	OBJ_FIELD(frags),
	OBJ_FIELD(weapon),
	OBJ_FIELD(weaponmodel),
	OBJ_FIELD(weaponframe),
	OBJ_FIELD(currentammo),
	OBJ_FIELD(ammo_shells),
	OBJ_FIELD(ammo_nails),
	OBJ_FIELD(ammo_rockets),
	OBJ_FIELD(ammo_cells),
	OBJ_FIELD(items),
	OBJ_FIELD(takedamage),
	OBJ_FIELD(chain),
	OBJ_FIELD(deadflag),
	OBJ_FIELD(view_ofs),
	OBJ_FIELD(button0),
	OBJ_FIELD(button1),
	OBJ_FIELD(button2),
	OBJ_FIELD(impulse),
	OBJ_FIELD(fixangle),
	OBJ_FIELD(v_angle),
	OBJ_FIELD(idealpitch),
	OBJ_FIELD(netname),
	OBJ_FIELD(enemy),
	OBJ_FIELD(flags),
	OBJ_FIELD(colormap),
	OBJ_FIELD(team),
	OBJ_FIELD(max_health),
	OBJ_FIELD(teleport_time),
	OBJ_FIELD(armortype),
	OBJ_FIELD(armorvalue),
	OBJ_FIELD(waterlevel),
	OBJ_FIELD(watertype),
	OBJ_FIELD(ideal_yaw),
	OBJ_FIELD(yaw_speed),
	OBJ_FIELD(aiment),
	OBJ_FIELD(goalentity),
	OBJ_FIELD(spawnflags),
	OBJ_FIELD(target),
	OBJ_FIELD(targetname),
	OBJ_FIELD(dmg_take),
	OBJ_FIELD(dmg_save),
	OBJ_FIELD(dmg_inflictor),
	OBJ_FIELD(owner),
	OBJ_FIELD(movedir),
	OBJ_FIELD(message),
	OBJ_FIELD(sounds),
	OBJ_FIELD(noise),
	OBJ_FIELD(noise1),
	OBJ_FIELD(noise2),
	OBJ_FIELD(noise3),

	OBJ_FIELD(wad),
	OBJ_FIELD(map),
	OBJ_FIELD(worldtype),

	OBJ_FIELD(killtarget),

	OBJ_FIELD(light_lev),
	OBJ_FIELD(style),

	OBJ_FIELD(touch),

	OBJ_FIELD(th_stand),
	OBJ_FIELD(th_walk),
	OBJ_FIELD(th_run),
	OBJ_FIELD(th_missile),
	OBJ_FIELD(th_melee),
	OBJ_FIELD(th_pain),
	OBJ_FIELD(th_die),

	OBJ_FIELD(oldenemy),

	OBJ_FIELD(speed),
	OBJ_FIELD(lefty),

	OBJ_FIELD(search_time),
	OBJ_FIELD(attack_state),

	OBJ_FIELD(walkframe),

	OBJ_FIELD(attack_finished),
	OBJ_FIELD(pain_finished),

	OBJ_FIELD(invincible_finished),
	OBJ_FIELD(invisible_finished),
	OBJ_FIELD(super_damage_finished),
	OBJ_FIELD(radsuit_finished),

	OBJ_FIELD(invincible_time),
	OBJ_FIELD(invincible_sound),
	OBJ_FIELD(invisible_time),
	OBJ_FIELD(invisible_sound),
	OBJ_FIELD(super_time),
	OBJ_FIELD(super_sound),
	OBJ_FIELD(rad_time),
	OBJ_FIELD(fly_sound),

	OBJ_FIELD(axhitme),

	OBJ_FIELD(show_hostile),

	OBJ_FIELD(jump_flag),
	OBJ_FIELD(swim_flag),
	OBJ_FIELD(air_finished),
	OBJ_FIELD(bubble_count),
	OBJ_FIELD(deathtype),

	OBJ_FIELD(mdl),
	OBJ_FIELD(mangle),

	OBJ_FIELD(t_length),
	OBJ_FIELD(t_width),

	OBJ_FIELD(dest),
	OBJ_FIELD(dest1),
	OBJ_FIELD(dest2),

	OBJ_FIELD(wait),
	OBJ_FIELD(delay),

	OBJ_FIELD(trigger_field),
	OBJ_FIELD(noise4),

	OBJ_FIELD(pausetime),
	OBJ_FIELD(movetarget),


	OBJ_FIELD(aflag),
	OBJ_FIELD(dmg),

	OBJ_FIELD(cnt),

	OBJ_FIELD(think1),
	OBJ_FIELD(finaldest),
	OBJ_FIELD(finalangle),

	OBJ_FIELD(count),


	OBJ_FIELD(lip),
	OBJ_FIELD(state),
	OBJ_FIELD(pos1),
	OBJ_FIELD(pos2),
	OBJ_FIELD(height),

	OBJ_FIELD(waitmin),
	OBJ_FIELD(waitmax),
	OBJ_FIELD(distance),
	OBJ_FIELD(volume),

	OBJ_FIELD(dmgtime),

	OBJ_FIELD(healamount),
	OBJ_FIELD(healtype),

	OBJ_FIELD(inpain),

	OBJ_FIELD(animation),
	OBJ_FIELD(animations_get),

	OBJ_FIELD(animation_mode)
};

#undef OBJ_FIELD

template<size_t Size>
const fielddescription* ED_FindFieldInTable(const char* name, const fielddescription (&fields)[Size])
{
	for (const auto& field : fields)
	{
		if (!strcmp(field.Name, name))
		{
			return &field;
		}
	}

	return nullptr;
}

/*
============
ED_FindField
============
*/
const fielddescription* ED_FindGlobal(const char* name)
{
	return ED_FindFieldInTable(name, GlobalvarsFields);
}

/*
============
ED_FindField
============
*/
const fielddescription* ED_FindField (const char *name)
{
	return ED_FindFieldInTable(name, EntvarsFields);
}

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char* PR_ValueString(void* base, const fielddescription& field)
{
	static char	line[256];

	switch (field.Type)
	{
	case ev_string:
		sprintf(line, "%s", ED_GetValue<const char*>(base, field));
		break;
	case ev_entity:
	{
		auto ent = ED_GetValue<edict_t*>(base, field);

		//Entity printing has to use world for null pointers to match the original.
		if (!ent)
		{
			ent = pr_global_struct->world;
		}

		sprintf(line, "entity %i", NUM_FOR_EDICT(ent));
		break;
	}
		//TODO
		/*
	case ev_function:
		f = pr_functions + val->function;
		sprintf(line, "%s()", pr_strings + f->s_name);
		break;
		*/
	case ev_void:
		sprintf(line, "void");
		break;
	case ev_float:
		sprintf(line, "%5.1f", ED_GetValue<float>(base, field));
		break;
	case ev_vector:
	{
		auto vec = ED_GetValueAddress<float>(base, field);
		sprintf(line, "'%5.1f %5.1f %5.1f'", vec[0], vec[1], vec[2]);
		break;
	}
	case ev_int:
		sprintf(line, "%5d", ED_GetValue<int>(base, field));
		break;
	default:
		sprintf(line, "bad type %i", field.Type);
		break;
	}

	return line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
char* PR_UglyValueString(void* base, const fielddescription& field)
{
	static char	line[256];

	switch (field.Type)
	{
	case ev_string:
		sprintf(line, "%s", ED_GetValue<const char*>(base, field));
		break;
	case ev_entity:
	{
		auto ent = ED_GetValue<edict_t*>(base, field);

		//Entity printing has to use world for null pointers to match the original.
		if (!ent)
		{
			ent = pr_global_struct->world;
		}

		sprintf(line, "%i", NUM_FOR_EDICT(ent));
		break;
	}
		//TODO
		/*
	case ev_function:
		f = pr_functions + val->function;
		sprintf(line, "%s", pr_strings + f->s_name);
		break;
		*/
	case ev_void:
		sprintf(line, "void");
		break;
	case ev_float:
		sprintf(line, "%f", ED_GetValue<float>(base, field));
		break;
	case ev_vector:
	{
		auto vec = ED_GetValueAddress<float>(base, field);
		sprintf(line, "%f %f %f", vec[0], vec[1], vec[2]);
		break;
	}
	case ev_int:
		sprintf(line, "%d", ED_GetValue<int>(base, field));
		break;
	default:
		sprintf(line, "bad type %i", field.Type);
		break;
	}

	return line;
}

/*
=============
ED_Print

For debugging
=============
*/
void ED_Print(edict_t* ed)
{
	int		l;
	int* v;
	int		j;

	if (ed->free)
	{
		Con_Printf("FREE\n");
		return;
	}

	Con_Printf("\nEDICT %i:\n", NUM_FOR_EDICT(ed));
	for (const auto& field : EntvarsFields)
	{
		v = ED_GetValueAddress<int>(&ed->v, field);

		// if the value is still all 0, skip the field
		for (j = 0; j < type_size[field.Type]; j++)
			if (v[j])
				break;
		if (j == type_size[field.Type])
			continue;

		Con_Printf("%s", field.Name);
		l = strlen(field.Name);
		while (l++ < 15)
			Con_Printf(" ");

		Con_Printf("%s\n", PR_ValueString(&ed->v, field));
	}
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write (FILE *f, edict_t *ed)
{
	int* v;
	int		j;

	fprintf(f, "{\n");

	if (ed->free)
	{
		fprintf(f, "}\n");
		return;
	}

	for (const auto& field : EntvarsFields)
	{
		v = ED_GetValueAddress<int>(&ed->v, field);

		// if the value is still all 0, skip the field
		for (j = 0; j < type_size[field.Type]; j++)
			if (v[j])
				break;
		if (j == type_size[field.Type])
			continue;

		fprintf(f, "\"%s\" ", field.Name);
		fprintf(f, "\"%s\"\n", PR_UglyValueString(&ed->v, field));
	}

	fprintf(f, "}\n");
}

void ED_PrintNum(int ent)
{
	ED_Print(EDICT_NUM(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts(void)
{
	int		i;

	Con_Printf("%i entities\n", sv.num_edicts);
	for (i = 0; i < sv.num_edicts; i++)
		ED_PrintNum(i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f(void)
{
	int		i;

	i = Q_atoi(Cmd_Argv(1));
	if (i >= sv.num_edicts)
	{
		Con_Printf("Bad edict number\n");
		return;
	}
	ED_PrintNum(i);
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
	fprintf(f, "{\n");
	for (const auto& field : GlobalvarsFields)
	{
		if (field.Type != ev_string
			&& field.Type != ev_float
			&& field.Type != ev_entity
			&& field.Type != ev_int)
			continue;

		fprintf(f, "\"%s\" ", field.Name);
		fprintf(f, "\"%s\"\n", PR_UglyValueString(pr_global_struct, field));
	}
	fprintf(f, "}\n");
}

/*
=============
ED_ParseGlobals
=============
*/
void ED_ParseGlobals (char *data)
{
	char	keyname[64];

	while (1)
	{
		// parse key
		data = COM_Parse(data);
		if (com_token[0] == '}')
			break;
		if (!data)
			Sys_Error("ED_ParseEntity: EOF without closing brace");

		strcpy(keyname, com_token);

		// parse value	
		data = COM_Parse(data);
		if (!data)
			Sys_Error("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Sys_Error("ED_ParseEntity: closing brace without data");

		auto key = ED_FindGlobal(keyname);
		if (!key)
		{
			Con_Printf("'%s' is not a global\n", keyname);
			continue;
		}

		if (!ED_ParseEpair(pr_global_struct, *key, com_token))
			Host_Error("ED_ParseGlobals: parse error");
	}
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
bool ED_ParseEpair (void *base, const fielddescription& key, const char *s)
{
	switch (key.Type)
	{
	case ev_string:
		ED_SetValue(base, key, ED_NewString(s));
		break;
		
	case ev_float:
		ED_SetValue(base, key, static_cast<float>(atof (s)));
		break;

	case ev_int:
		ED_SetValue(base, key, atoi(s));
		break;
		
	case ev_vector:
	{
		float* vec = ED_GetValueAddress<float>(base, key);

		char string[128];
		strcpy(string, s);
		auto v = string;
		auto w = string;
		for (int i = 0; i < 3; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			vec[i] = atof(w);
			w = v = v + 1;
		}
		break;
	}
		
	case ev_entity:
		ED_SetValue(base, key, EDICT_NUM(atoi (s)));
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

		if (!ED_ParseEpair (&ent->v, *key, com_token))
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
	Cmd_AddCommand("edict", ED_PrintEdict_f);
	Cmd_AddCommand("edicts", ED_PrintEdicts);
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
