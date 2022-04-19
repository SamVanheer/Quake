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

inline float enemy_vis, enemy_infront, enemy_range;
inline float enemy_yaw;

float SoldierCheckAttack(edict_t* self);
float OgreCheckAttack(edict_t* self);
float ShamCheckAttack(edict_t* self);
float DemonCheckAttack(edict_t* self);
bool CheckAttack(edict_t* self);

void ai_face(edict_t* self);
void ai_charge(edict_t* self, float d);
void ai_charge_side(edict_t* self);
void ai_melee(edict_t* self);
void ai_melee_side(edict_t* self);
