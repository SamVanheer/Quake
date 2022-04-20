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

#include "quakedef.h"
#include "ai.h"
#include "animation.h"
#include "combat.h"
#include "fight.h"
#include "Game.h"
#include "monsters.h"
#include "subs.h"
#include "weapons.h"

/*
==============================================================================

BLOB

==============================================================================
*/

void Tar_JumpTouch(edict_t* self, edict_t* other);
void tbaby_fly1(edict_t* self);
void tbaby_run1(edict_t* self);

enum class TarBabyWalkMode
{
	Stand = 0,
	Hang,
	Walk
};

void tarbaby_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void tarbaby_hang_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void tarbaby_walk_core_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame < 10)
	{
		ai_turn(self);
	}
	else
	{
		ai_walk(self, 2);
	}
}

void tarbaby_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (static_cast<TarBabyWalkMode>(self->v.animation_mode))
	{
	default:
	case TarBabyWalkMode::Stand:
		tarbaby_stand_frame(self, animation, frame);
		break;

	case TarBabyWalkMode::Hang:
		tarbaby_hang_frame(self, animation, frame);
		break;

	case TarBabyWalkMode::Walk:
		tarbaby_walk_core_frame(self, animation, frame);
		break;
	}
}

void tarbaby_run_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame < 10)
	{
		ai_face(self);
	}
	else
	{
		ai_run(self, 2);
	}
}

void tarbaby_fly_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.cnt = self->v.cnt + 1;
		if (self->v.cnt == 4)
		{
			//dprint ("spawn hop\n");
			animation_set(self, "jump", 4);
		}
	}
}

void tarbaby_jump_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame < 4)
	{
		ai_face(self);
	}
	else if (frame == 4)
	{
		self->v.movetype = MOVETYPE_BOUNCE;
		self->v.touch = Tar_JumpTouch;
		PF_makevectors(self->v.angles);
		self->v.origin[2] = self->v.origin[2] + 1;
		AsVector(self->v.velocity) = AsVector(pr_global_struct->v_forward) * 600 + Vector3D{0, 0, 200};
		self->v.velocity[2] = self->v.velocity[2] + random() * 150;
		if (self->v.flags & FL_ONGROUND)
			self->v.flags &= ~FL_ONGROUND;
		self->v.cnt = 0;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = &tbaby_fly1;
	}
}

void tarbaby_exp_finish(edict_t* self)
{
	self->v.nextthink = pr_global_struct->time + 0.1f;
	self->v.think = &tbaby_run1;

	T_RadiusDamage(self, self, self, 120, pr_global_struct->world);

	PF_sound(self, CHAN_VOICE, "blob/death1.wav", 1, ATTN_NORM);

	Vector3D dir;
	PF_normalize(self->v.velocity, dir);
	AsVector(self->v.origin) = AsVector(self->v.origin) - 8 * dir;

	PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte(MSG_BROADCAST, TE_TAREXPLOSION);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[0]);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[1]);
	PF_WriteCoord(MSG_BROADCAST, self->v.origin[2]);

	BecomeExplosion(self);
}

void tarbaby_exp_frame(edict_t* self, const Animation* animation, int frame)
{
	self->v.takedamage = DAMAGE_NO;
	self->v.think = &tarbaby_exp_finish;
}

const Animations TarBabyAnimations = MakeAnimations(
	{
		{"walk", 25, &tarbaby_walk_frame, AnimationFlag::Looping},
		{"run", 25, &tarbaby_run_frame, AnimationFlag::Looping},
		{"jump", 6, &tarbaby_jump_frame, AnimationFlag::Looping},
		{"fly", 4, &tarbaby_fly_frame, AnimationFlag::Looping},
		{"exp", 1, &tarbaby_exp_frame}
	});

void tbaby_stand1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(TarBabyWalkMode::Stand);
	animation_set(self, "walk");
}

void tbaby_hang1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(TarBabyWalkMode::Hang);
	animation_set(self, "walk");
}

void tbaby_walk1(edict_t* self)
{
	self->v.animation_mode = static_cast<int>(TarBabyWalkMode::Walk);
	animation_set(self, "walk");
}

void tbaby_run1(edict_t* self)
{
	animation_set(self, "run");
}

//============================================================================

void tbaby_jump1(edict_t* self);

void Tar_JumpTouch(edict_t* self, edict_t* other)
{
	if (other->v.takedamage && other->v.classname != self->v.classname)
	{
		if ( PF_vlen(self->v.velocity) > 400 )
		{
			const float ldmg = 10 + 10*random();
			T_Damage (self, other, self, self, ldmg);	
			PF_sound (self, CHAN_WEAPON, "blob/hit1.wav", 1, ATTN_NORM);
		}
	}
	else
		PF_sound(self, CHAN_WEAPON, "blob/land1.wav", 1, ATTN_NORM);


	if (!PF_checkbottom(self))
	{
		if (self->v.flags & FL_ONGROUND)
		{	// jump randomly to not get hung up
//dprint ("popjump\n");
	self->v.touch = SUB_NullTouch;
	self->v.think = tbaby_run1;
	self->v.movetype = MOVETYPE_STEP;
	self->v.nextthink = pr_global_struct->time + 0.1;

//			self->v.velocity_x = (random() - 0.5) * 600;
//			self->v.velocity_y = (random() - 0.5) * 600;
//			self->v.velocity_z = 200;
//			self->v.flags &= ~FL_ONGROUND;
		}
		return;	// not on ground yet
	}

	self->v.touch = SUB_NullTouch;
	self->v.think = tbaby_jump1;
	self->v.nextthink = pr_global_struct->time + 0.1;
}

void tbaby_fly1(edict_t* self)
{
	animation_set(self, "fly");
}

void tbaby_jump1(edict_t* self)
{
	animation_set(self, "jump");
}

//=============================================================================

void tbaby_die1(edict_t* self)
{
	animation_set(self, "exp");
}

//=============================================================================

/*QUAKED monster_tarbaby (1 0 0) (-16 -16 -24) (16 16 24) Ambush
*/
void monster_tarbaby(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model ("progs/tarbaby.mdl");

	PF_precache_sound ("blob/death1.wav");
	PF_precache_sound ("blob/hit1.wav");
	PF_precache_sound ("blob/land1.wav");
	PF_precache_sound ("blob/sight1.wav");
	
	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel (self, "progs/tarbaby.mdl");

	PF_setsize (self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 40});
	self->v.health = 80;

	self->v.th_stand = tbaby_stand1;
	self->v.th_walk = tbaby_walk1;
	self->v.th_run = tbaby_run1;
	self->v.th_missile = tbaby_jump1;
	self->v.th_melee = tbaby_jump1;
	self->v.th_die = tbaby_die1;
	self->v.animations_get = [](edict_t* self) { return &TarBabyAnimations; };
	
	walkmonster_start (self);
}

LINK_ENTITY_TO_CLASS(monster_tarbaby);
