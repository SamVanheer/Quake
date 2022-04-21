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
#include "player.h"
#include "subs.h"

/*
==============================================================================

ZOMBIE

==============================================================================
*/
void zombie_run1(edict_t* self);
void ZombieFireGrenade(edict_t* self, vec3_t st);

void zombie_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_stand(self);
}

void zombie_cruc_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		if (PF_random() < 0.1)
			PF_sound(self, CHAN_VOICE, "zombie/idle_w2.wav", 1, ATTN_STATIC);
	}
	else
	{
		self->v.nextthink = pr_global_struct->time + 0.1 + PF_random() * 0.1;
	}
}

void zombie_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	default:
		ai_walk(self, 0);
		break;

	case 1:
	case 3:
	case 10:
	case 11:
		ai_walk(self, 2);
		break;

	case 2:
		ai_walk(self, 3);
		break;

	case 4:
	case 12:
		ai_walk(self, 1);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		if (PF_random() < 0.2)
			PF_sound(self, CHAN_VOICE, "zombie/z_idle.wav", 1, ATTN_IDLE);
	}
}

void zombie_run_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 0)
	{
		self->v.inpain = 0;
	}

	switch (frame)
	{
	case 0:
	case 1:
	case 3:
		ai_run(self, 1);
		break;

	default:
		ai_run(self, 0);
		break;

	case 4:
	case 8:
	case 12:
		ai_run(self, 2);
		break;

	case 5:
	case 16:
		ai_run(self, 3);
		break;

	case 6:
	case 7:
	case 13:
		ai_run(self, 4);
		break;

	case 14:
		ai_run(self, 6);
		break;

	case 15:
		ai_run(self, 7);
		break;

	case 17:
		ai_run(self, 8);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		if (PF_random() < 0.2)
			PF_sound(self, CHAN_VOICE, "zombie/z_idle.wav", 1, ATTN_IDLE);
		if (PF_random() > 0.8)
			PF_sound(self, CHAN_VOICE, "zombie/z_idle1.wav", 1, ATTN_IDLE);
	}
}

void zombie_attack_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_face(self);

	if (animation->ReachedEnd(frame))
	{
		self->v.think = zombie_run1;
		ZombieFireGrenade(self, Vector3D{-10, -22, 30});
	}
}

void zombie_attackb_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_face(self);

	if (animation->ReachedEnd(frame))
	{
		self->v.think = zombie_run1;
		ZombieFireGrenade(self, Vector3D{-10, -24, 29});
	}
}

void zombie_attackc_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_face(self);

	if (animation->ReachedEnd(frame))
	{
		self->v.think = zombie_run1;
		ZombieFireGrenade(self, Vector3D{-12, -19, 29});
	}
}

void zombie_paina_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		PF_sound(self, CHAN_VOICE, "zombie/z_pain.wav", 1, ATTN_NORM);
		break;

	case 1:
		ai_painforward(self, 3);
		break;

	case 2:
		ai_painforward(self, 1);
		break;

	case 3:
		ai_pain(self, 1);
		break;

	case 4:
		ai_pain(self, 3);
		break;

	case 5:
		ai_pain(self, 1);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = zombie_run1;
	}
}

void zombie_painb_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		PF_sound(self, CHAN_VOICE, "zombie/z_pain.wav", 1, ATTN_NORM);
		break;

	case 1:
		ai_pain(self, 2);
		break;

	case 2:
		ai_pain(self, 7);
		break;

	case 3:
		ai_pain(self, 6);
		break;

	case 4:
		ai_pain(self, 2);
		break;

	case 8:
		PF_sound(self, CHAN_BODY, "zombie/z_fall.wav", 1, ATTN_NORM);
		break;

	case 24:
		ai_painforward(self, 1);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = zombie_run1;
	}
}

void zombie_painc_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		PF_sound(self, CHAN_VOICE, "zombie/z_pain.wav", 1, ATTN_NORM);
		break;

	case 2:
		ai_pain(self, 3);
		break;

	case 3:
		ai_pain(self, 1);
		break;

	case 10:
	case 11:
		ai_painforward(self, 1);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = zombie_run1;
	}
}

void zombie_paind_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		PF_sound(self, CHAN_VOICE, "zombie/z_pain.wav", 1, ATTN_NORM);
		break;

	case 8:
		ai_pain(self, 1);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = zombie_run1;
	}
}

void zombie_paine_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		PF_sound(self, CHAN_VOICE, "zombie/z_pain.wav", 1, ATTN_NORM);
		self->v.health = 60;
		break;

	case 1:
		ai_pain(self, 8);
		break;

	case 2:
		ai_pain(self, 5);
		break;

	case 3:
		ai_pain(self, 3);
		break;

	case 4:
	case 6:
	case 7:
	case 27:
		ai_pain(self, 1);
		break;

	case 5:
	case 8:
		ai_pain(self, 2);
		break;

	case 24:
		ai_pain(self, 5);
		break;

	case 25:
		ai_pain(self, 3);
		break;

	case 26:
		ai_pain(self, 1);
		break;

	case 9:
		PF_sound(self, CHAN_BODY, "zombie/z_fall.wav", 1, ATTN_NORM);
		self->v.solid = SOLID_NOT;
		break;

	case 10:
		self->v.nextthink = self->v.nextthink + 5;
		self->v.health = 60;
		break;

	case 11:
		// see if ok to stand up
		self->v.health = 60;
		PF_sound(self, CHAN_VOICE, "zombie/z_idle.wav", 1, ATTN_IDLE);
		self->v.solid = SOLID_SLIDEBOX;
		if (!PF_walkmove(self, 0, 0))
		{
			self->v.frame = 10;
			self->v.solid = SOLID_NOT;
			return;
		}
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = zombie_run1;
	}
}

const Animations ZombieAnimations = MakeAnimations(
	{
		{"stand", 15, &zombie_stand_frame, AnimationFlag::Looping},
		{"walk", 19, &zombie_walk_frame},
		{"run", 18, &zombie_run_frame, AnimationFlag::Looping},
		{"atta", 13, &zombie_attack_frame},
		{"attb", 14, &zombie_attackb_frame},
		{"attc", 12, &zombie_attackc_frame},
		{"paina", 12, &zombie_paina_frame},
		{"painb", 28, &zombie_painb_frame},
		{"painc", 18, &zombie_painc_frame},
		{"paind", 13, &zombie_paind_frame},
		{"paine", 30, &zombie_paine_frame},
		{"cruc_", 6, &zombie_cruc_frame, AnimationFlag::Looping}
	});

const Animations* zombie_animations_get(edict_t* self)
{
	return &ZombieAnimations;
}

LINK_FUNCTION_TO_NAME(zombie_animations_get);

constexpr int SPAWN_CRUCIFIED = 1;

//=============================================================================
void zombie_stand1(edict_t* self)
{
	animation_set(self, "stand");
}

LINK_FUNCTION_TO_NAME(zombie_stand1);

void zombie_cruc1(edict_t* self)
{
	animation_set(self, "cruc_");
}

void zombie_walk1(edict_t* self)
{
	animation_set(self, "walk");
}

LINK_FUNCTION_TO_NAME(zombie_walk1);

void zombie_run1(edict_t* self)
{
	animation_set(self, "run");
}

LINK_FUNCTION_TO_NAME(zombie_run1);

void zombie_atta1(edict_t* self)
{
	animation_set(self, "atta");
}

void zombie_attb1(edict_t* self)
{
	animation_set(self, "attb");
}

void zombie_attc1(edict_t* self)
{
	animation_set(self, "attc");
}

void zombie_paina1(edict_t* self)
{
	animation_set(self, "paina");
}

void zombie_painb1(edict_t* self)
{
	animation_set(self, "painb");
}

void zombie_painc1(edict_t* self)
{
	animation_set(self, "painc");
}

void zombie_paind1(edict_t* self)
{
	animation_set(self, "paind");
}

void zombie_paine1(edict_t* self)
{
	animation_set(self, "paine");
}

/*
=============================================================================

ATTACKS

=============================================================================
*/

void ZombieGrenadeTouch(edict_t* self, edict_t* other)
{
	if (other == self->v.owner)
		return;		// don't explode on owner
	if (other->v.takedamage)
	{
		T_Damage(self, other, self, self->v.owner, 10);
		PF_sound(self, CHAN_WEAPON, "zombie/z_hit.wav", 1, ATTN_NORM);
		PF_Remove(self);
		return;
	}
	PF_sound(self, CHAN_WEAPON, "zombie/z_miss.wav", 1, ATTN_NORM);	// bounce sound
	AsVector(self->v.velocity) = AsVector(vec3_origin);
	AsVector(self->v.avelocity) = AsVector(vec3_origin);
	self->v.touch = SUB_TouchRemove;
}

LINK_FUNCTION_TO_NAME(ZombieGrenadeTouch);

/*
================
ZombieFireGrenade
================
*/
void ZombieFireGrenade(edict_t* self, vec3_t st)
{
	PF_sound(self, CHAN_WEAPON, "zombie/z_shot1.wav", 1, ATTN_NORM);

	auto missile = PF_Spawn();
	missile->v.owner = self;
	missile->v.movetype = MOVETYPE_BOUNCE;
	missile->v.solid = SOLID_BBOX;

	// calc org
	auto org = AsVector(self->v.origin) + st[0] * AsVector(pr_global_struct->v_forward)
		+ st[1] * AsVector(pr_global_struct->v_right)
		+ (st[2] - 24) * AsVector(pr_global_struct->v_up);

	// set missile speed	

	PF_makevectors(self->v.angles);

	PF_normalize(AsVector(self->v.enemy->v.origin) - AsVector(org), missile->v.velocity);
	AsVector(missile->v.velocity) = AsVector(missile->v.velocity) * 600;
	missile->v.velocity[2] = 200;

	AsVector(missile->v.avelocity) = Vector3D{3000, 1000, 2000};

	missile->v.touch = ZombieGrenadeTouch;

	// set missile duration
	missile->v.nextthink = pr_global_struct->time + 2.5;
	missile->v.think = SUB_Remove;

	PF_setmodel(missile, "progs/zom_gib.mdl");
	PF_setsize(missile, Vector3D{0, 0, 0}, Vector3D{0, 0, 0});
	PF_setorigin(missile, org);
}

void zombie_missile(edict_t* self)
{
	const float r = PF_random();

	if (r < 0.3)
		zombie_atta1(self);
	else if (r < 0.6)
		zombie_attb1(self);
	else
		zombie_attc1(self);
}

LINK_FUNCTION_TO_NAME(zombie_missile);

/*
=============================================================================

PAIN

=============================================================================
*/
void zombie_die(edict_t* self)
{
	PF_sound(self, CHAN_VOICE, "zombie/z_gib.wav", 1, ATTN_NORM);
	ThrowHead(self, "progs/h_zombie.mdl", self->v.health);
	ThrowGib(self, "progs/gib1.mdl", self->v.health);
	ThrowGib(self, "progs/gib2.mdl", self->v.health);
	ThrowGib(self, "progs/gib3.mdl", self->v.health);
}

LINK_FUNCTION_TO_NAME(zombie_die);

/*
=================
zombie_pain

Zombies can only be killed (gibbed) by doing 60 hit points of damage
in a single frame (rockets, grenades, quad shotgun, quad nailgun).

A hit of 25 points or more (super shotgun, quad nailgun) will allways put it
down to the ground.

A hit of from 10 to 40 points in one frame will cause it to go down if it
has been twice in two seconds, otherwise it goes into one of the four
fast pain frames.

A hit of less than 10 points of damage (winged by a shotgun) will be ignored.

FIXME: don't use pain_finished because of nightmare hack
=================
*/
void zombie_pain(edict_t* self, edict_t* attacker, float take)
{
	self->v.health = 60;		// allways reset health

	if (take < 9)
		return;				// totally ignore

	if (self->v.inpain == 2)
		return;			// down on ground, so don't reset any counters

// go down immediately if a big enough hit
	if (take >= 25)
	{
		self->v.inpain = 2;
		zombie_paine1(self);
		return;
	}

	if (self->v.inpain)
	{
		// if hit again in next gre seconds while not in pain frames, definately drop
		self->v.pain_finished = pr_global_struct->time + 3;
		return;			// currently going through an animation, don't change
	}

	if (self->v.pain_finished > pr_global_struct->time)
	{
		// hit again, so drop down
		self->v.inpain = 2;
		zombie_paine1(self);
		return;
	}

	// gp into one of the fast pain animations	
	self->v.inpain = 1;

	const float r = PF_random();
	if (r < 0.25)
		zombie_paina1(self);
	else if (r < 0.5)
		zombie_painb1(self);
	else if (r < 0.75)
		zombie_painc1(self);
	else
		zombie_paind1(self);
}

LINK_FUNCTION_TO_NAME(zombie_pain);

//============================================================================

/*QUAKED monster_zombie (1 0 0) (-16 -16 -24) (16 16 32) Crucified ambush

If crucified, stick the bounding box 12 pixels back into a wall to look right.
*/
void monster_zombie(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}

	PF_precache_model("progs/zombie.mdl");
	PF_precache_model("progs/h_zombie.mdl");
	PF_precache_model("progs/zom_gib.mdl");

	PF_precache_sound("zombie/z_idle.wav");
	PF_precache_sound("zombie/z_idle1.wav");
	PF_precache_sound("zombie/z_shot1.wav");
	PF_precache_sound("zombie/z_gib.wav");
	PF_precache_sound("zombie/z_pain.wav");
	PF_precache_sound("zombie/z_pain1.wav");
	PF_precache_sound("zombie/z_fall.wav");
	PF_precache_sound("zombie/z_miss.wav");
	PF_precache_sound("zombie/z_hit.wav");
	PF_precache_sound("zombie/idle_w2.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel(self, "progs/zombie.mdl");

	PF_setsize(self, Vector3D{-16, -16, -24}, Vector3D{16, 16, 40});
	self->v.health = 60;

	self->v.th_stand = zombie_stand1;
	self->v.th_walk = zombie_walk1;
	self->v.th_run = zombie_run1;
	self->v.th_pain = zombie_pain;
	self->v.th_die = zombie_die;
	self->v.th_missile = zombie_missile;
	self->v.animations_get = &zombie_animations_get;

	if (self->v.spawnflags & SPAWN_CRUCIFIED)
	{
		self->v.movetype = MOVETYPE_NONE;
		zombie_cruc1(self);
	}
	else
		walkmonster_start(self);
}

LINK_ENTITY_TO_CLASS(monster_zombie);
