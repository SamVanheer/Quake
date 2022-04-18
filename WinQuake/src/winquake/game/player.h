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

const Animations* player_animations_get(edict_t* self);

void player_stand1(edict_t* self);
void player_run(edict_t* self);
void player_axe1(edict_t* self);
void player_axeb1(edict_t* self);
void player_axec1(edict_t* self);
void player_axed1(edict_t* self);
void player_shot1(edict_t* self);
void player_nail1(edict_t* self);
void player_light1(edict_t* self);
void player_rocket1(edict_t* self);
void player_pain(edict_t* self, edict_s* attacker, float damage);
