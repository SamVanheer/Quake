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

#pragma once

void SUB_NullThink(edict_t*);
void SUB_NullTouch(edict_t*, edict_t*);
void SUB_NullUse(edict_t*, edict_t*);
void SUB_NullPain(edict_t*, edict_t*, float);
void SUB_Remove(edict_t* self);
void SUB_TouchRemove(edict_t* self, edict_t* other);
void SetMovedir(edict_t* self);
void InitTrigger(edict_t* self);
void SUB_CalcMove(edict_t* self, vec3_t tdest, float tspeed, void (*func)(edict_t*));
void SUB_CalcMoveDone(edict_t* self);
void SUB_CalcAngleMove(edict_t* self, vec3_t destangle, float tspeed, void (*func)(edict_t*));
void SUB_CalcAngleMoveDone(edict_t* self);
void SUB_UseTargets(edict_t* self, edict_t* other = nullptr);
void SUB_AttackFinished(edict_t* self, float normal);
void SUB_CheckRefire(edict_t* self, void (*thinkst)(edict_t*));
