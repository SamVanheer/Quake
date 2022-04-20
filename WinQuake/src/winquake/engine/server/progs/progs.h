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

#include "pr_comp.h"			// defs shared with qcc
#include "progdefs.h"			// generated by program cdefs

//TODO: get rid of this
typedef union eval_s
{
	const char*		string;
	float			_float;
	float			vector[3];
	func_t			function;
	int				_int;
	edict_s*		edict;
} eval_t;	

struct fielddescription
{
	const char* Name;
	const etype_t Type;
	const std::size_t Offset;
};

#define	MAX_ENT_LEAFS	16
typedef struct edict_s
{
	bool		free;
	link_t		area;				// linked to a division node or leaf
	
	int			num_leafs;
	short		leafnums[MAX_ENT_LEAFS];

	entity_state_t	baseline;
	
	float		freetime;			// sv.time when the object was freed
	entvars_t	v;					// C exported fields from progs
// other fields from progs come immediately after
} edict_t;
#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

//============================================================================

extern	inline char* pr_strings = "";
extern	globalvars_t	*pr_global_struct;

//============================================================================

void PR_Init (void);

void PR_LoadProgs(void);

edict_t *ED_Alloc (void);
void ED_Free (edict_t *ed);

char	*ED_NewString (const char *string);
// returns a copy of the string allocated from the server's string heap

void ED_Write (FILE *f, edict_t *ed);

template<typename T>
T* ED_GetValueAddress(void* base, const fielddescription& field)
{
	return reinterpret_cast<T*>(reinterpret_cast<byte*>(base) + field.Offset);
}

template<typename T>
void ED_SetValue(void* base, const fielddescription& field, T value)
{
	auto address = ED_GetValueAddress<T>(base, field);
	*address = value;
}

char *ED_ParseEdict (char *data, edict_t *ent);

void ED_WriteGlobals (FILE *f);
void ED_ParseGlobals (char *data);

void ED_LoadFromFile (char *data);

edict_t *EDICT_NUM(int n);
int NUM_FOR_EDICT(edict_t *e);

//============================================================================

void ED_PrintEdicts (void);
void ED_PrintNum (int ent);

eval_t *GetEdictFieldValue(edict_t *ed, const char *field);

//commands

void PF_error(const char* s);
void PF_objerror(const char* s);
void PF_makevectors(float* angles);
void PF_setorigin(edict_t* e, float* org);
void PF_setsize(edict_t* e, const float* min, const float* max);
void PF_setmodel(edict_t* e, char* m);
void PF_bprint(char* s);
void PF_sprint(edict_t* other, const char* s);
void PF_centerprint(edict_t* other, const char* s);
void PF_normalize(float* value1, float* result);
float PF_vlen(float* value1);
float PF_vectoyaw(float* value1);
void PF_vectoangles(float* value1, float* result);
float PF_random(void);
void PF_particle(float* org, float* dir, float color, float count);
void PF_ambientsound(float* pos, char* samp, float vol, float attenuation);
void PF_sound(edict_t* entity, int channel, char* sample, float volumeFloat, float attenuation);
void PF_traceline(float* v1, float* v2, int nomonsters, edict_t* ent);
edict_t* PF_checkclient(edict_t* self);
void PF_stuffcmd(edict_t* ent, const char* str);
void PF_localcmd(const char* str);
float PF_cvar(const char* str);
void PF_cvar_set(const char* var, const char* val);
edict_t* PF_findradius(const float* org, float rad);
const char* PF_ftos(float v);
float PF_fabs(float v);
const char* PF_vtos(const float* v);
edict_t* PF_Spawn();
void PF_Remove(edict_t* ed);
edict_t* PF_Find(edict_t* ent, const char* f, const char* s);
void PF_precache_file(const char* s);
void PF_precache_sound(char* s);
void PF_precache_model(char* s);
bool PF_walkmove(edict_t* ent, float yaw, float dist);
int PF_droptofloor(edict_t* ent);
void PF_lightstyle(int style, char* val);
float PF_rint(float f);
float PF_floor(float value);
float PF_ceil(float value);
bool PF_checkbottom(edict_t* ent);
float PF_pointcontents(float* v);
edict_t* PF_nextent(edict_t* ent);
void PF_aim(edict_t* ent, float speed, float* aimDirection);
void PF_changeyaw(edict_t* ent);
void PF_WriteByte(int dest, int value, edict_t* ent = nullptr);
void PF_WriteChar(int dest, int value, edict_t* ent = nullptr);
void PF_WriteShort(int dest, int value, edict_t* ent = nullptr);
void PF_WriteLong(int dest, int value, edict_t* ent = nullptr);
void PF_WriteAngle(int dest, float value, edict_t* ent = nullptr);
void PF_WriteCoord(int dest, float value, edict_t* ent = nullptr);
void PF_WriteString(int dest, const char* string, edict_t* ent = nullptr);
void PF_WriteEntity(int dest, edict_t* ent, edict_t* destEnt = nullptr);
void PF_makestatic(edict_t* ent);
void PF_setspawnparms(edict_t* ent);
void PF_changelevel(const char* mapname);

inline void bprint(const char* s)
{
	PF_bprint(const_cast<char*>(s));
}

void dprint(const char* s);

inline void PF_setmodel(edict_t* e, const char* m)
{
	PF_setmodel(e, const_cast<char*>(m));
}

inline float random()
{
	return PF_random();
}

inline void PF_sound(edict_t* entity, int channel, const char* sample, float volumeFloat, float attenuation)
{
	PF_sound(entity, channel, const_cast<char*>(sample), volumeFloat, attenuation);
}
