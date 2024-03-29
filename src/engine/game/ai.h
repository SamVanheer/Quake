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

bool visible(edict_t* self, edict_t* targ);
bool infront(edict_t* self, edict_t* targ);
float range(edict_t* self, edict_t* targ);
void FoundTarget(edict_t* self);

void ai_forward(edict_t* self, float dist);
void ai_back(edict_t* self, float dist);
void ai_pain(edict_t* self, float dist);
void ai_painforward(edict_t* self, float dist);
void ai_walk(edict_t* self, float dist);
void ai_stand(edict_t* self);
void ai_turn(edict_t* self);
void ai_run(edict_t* self, float dist);
