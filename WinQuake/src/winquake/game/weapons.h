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

void W_Precache(edict_t* self);
float crandom();
void W_FireAxe(edict_t* self);
void SpawnBlood(vec3_t org, vec3_t vel, float damage);
void BecomeExplosion(edict_t* self);
void W_FireLightning(edict_t* self);
void launch_spike(edict_t* self, vec3_t org, vec3_t dir);
void W_FireSpikes(edict_t* self, float ox);
void spike_touch(edict_t* self, edict_t* other);
void superspike_touch(edict_t* self, edict_t* other);
void W_SetCurrentAmmo(edict_t* self);
int W_BestWeapon(edict_t* self);
void W_WeaponFrame(edict_t* self);
void SuperDamageSound(edict_t* self);
