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
#include "items.h"
#include "monsters.h"
#include "player.h"
#include "subs.h"
#include "weapons.h"

/*
==============================================================================

OGRE

==============================================================================
*/
void chainsaw(edict_t* self, float side);
void ogre_run1(edict_t* self);
void OgreFireGrenade(edict_t* self);

void ogre_stand_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 4)
	{
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "ogre/ogidle.wav", 1, ATTN_IDLE);
	}

	ai_stand(self);
}

void ogre_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	default:
		ai_walk(self, 3);
		break;

	case 1:
	case 2:
	case 3:
	case 4:
	case 7:
	case 10:
		ai_walk(self, 2);
		break;

	case 5:
		ai_walk(self, 5);
		break;

	case 9:
		ai_walk(self, 1);
		break;

	case 15:
		ai_walk(self, 4);
		break;
	}

	switch (frame)
	{
	case 3:
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "ogre/ogidle.wav", 1, ATTN_IDLE);
		break;

	case 5:
		if (random() < 0.1)
			PF_sound(self, CHAN_VOICE, "ogre/ogdrag.wav", 1, ATTN_IDLE);
		break;
	}
}

void ogre_run_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		ai_run(self, 9);
		if (random() < 0.2)
			PF_sound(self, CHAN_VOICE, "ogre/ogidle2.wav", 1, ATTN_IDLE);
		break;

	case 1:
		ai_run(self, 12);
		break;

	case 2:
		ai_run(self, 8);
		break;

	case 3:
		ai_run(self, 22);
		break;

	case 4:
		ai_run(self, 16);
		break;

	case 5:
		ai_run(self, 4);
		break;

	case 6:
		ai_run(self, 13);
		break;

	case 7:
		ai_run(self, 24);
		break;
	}
}

void ogre_swing_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		ai_charge(self, 11);
		PF_sound(self, CHAN_WEAPON, "ogre/ogsawatk.wav", 1, ATTN_NORM);
		break;

	case 1:
		ai_charge(self, 1);
		break;

	case 2:
		ai_charge(self, 4);
		break;

	case 3:
		ai_charge(self, 13);
		break;

	case 4:
		ai_charge(self, 9);
		chainsaw(self, 0);
		self->v.angles[1] = self->v.angles[1] + random() * 25;
		break;

	case 5:
		chainsaw(self, 200);
		self->v.angles[1] = self->v.angles[1] + random() * 25;
		break;

	case 6:
	case 7:
	case 8:
	case 10:
		chainsaw(self, 0);
		self->v.angles[1] = self->v.angles[1] + random() * 25;
		break;

	case 9:
		chainsaw(self, -200);
		self->v.angles[1] = self->v.angles[1] + random() * 25;
		break;

	case 11:
		ai_charge(self, 3);
		break;

	case 12:
		ai_charge(self, 8);
		break;

	case 13:
		ai_charge(self, 9);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = ogre_run1;
	}
}

void ogre_smash_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		ai_charge(self, 6);
		PF_sound(self, CHAN_WEAPON, "ogre/ogsawatk.wav", 1, ATTN_NORM);
		break;

	case 1:
	case 2:
	case 11:
		ai_charge(self, 0);
		break;

	case 3:
		ai_charge(self, 1);
		break;

	case 4:
	case 12:
		ai_charge(self, 4);
		break;

	case 5:
	case 6:
		ai_charge(self, 4);
		chainsaw(self, 0);
		break;

	case 7:
		ai_charge(self, 10);
		chainsaw(self, 0);
		break;

	case 8:
		ai_charge(self, 13);
		chainsaw(self, 0);
		break;

	case 9:
		chainsaw(self, 1);
		break;

	case 10:
		ai_charge(self, 2);
		chainsaw(self, 0);
		// slight variation
		self->v.nextthink = self->v.nextthink + random() * 0.2;
		break;

	case 13:
		ai_charge(self, 12);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = ogre_run1;
	}
}

void ogre_shoot_frame(edict_t* self, const Animation* animation, int frame)
{
	ai_face(self);

	if (frame == 2)
	{
		OgreFireGrenade(self);
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = ogre_run1;
	}
}

void ogre_pain_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = ogre_run1;
	}
}

void ogre_pain_alt_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 1:
		ai_pain(self, 10);
		break;

	case 2:
		ai_pain(self, 9);
		break;

	case 3:
		ai_pain(self, 4);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = ogre_run1;
	}
}

void ogre_death_frame(edict_t* self, const Animation* animation, int frame)
{
	if (frame == 2)
	{
		self->v.solid = SOLID_NOT;
		self->v.ammo_rockets = 2;
		DropBackpack(self);
	}
}

void ogre_bdeath_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 1:
		ai_forward(self, 5);
		break;

	case 2:
		self->v.solid = SOLID_NOT;
		self->v.ammo_rockets = 2;
		DropBackpack(self);
		break;

	case 3:
		ai_forward(self, 1);
		break;

	case 4:
		ai_forward(self, 3);
		break;

	case 5:
		ai_forward(self, 7);
		break;

	case 6:
		ai_forward(self, 25);
		break;
	}
}

const Animations OgreAnimations = MakeAnimations(
	{
		{"stand", 9, &ogre_stand_frame, AnimationFlag::Looping},
		{"walk", 16, &ogre_walk_frame, AnimationFlag::Looping},
		{"run", 8, &ogre_run_frame, AnimationFlag::Looping},
		{"swing", 14, &ogre_swing_frame},
		{"smash", 14, &ogre_smash_frame},
		{"shoot", 6, &ogre_shoot_frame},
		{"pain", 5, &ogre_pain_frame},
		{"painb", 3, &ogre_pain_frame},
		{"painc", 6, &ogre_pain_frame},
		{"paind", 16, &ogre_pain_alt_frame},
		{"paine", 15, &ogre_pain_alt_frame},
		{"death", 14, &ogre_death_frame},
		{"bdeath", 10, &ogre_bdeath_frame},
		{"pull", 11} //Never used
	});

//=============================================================================


void OgreGrenadeExplode(edict_t* self)
{
	T_RadiusDamage (self, self, self->v.owner, 40, pr_global_struct->world);
	PF_sound (self, CHAN_VOICE, "weapons/r_exp3.wav", 1, ATTN_NORM);

	PF_WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte (MSG_BROADCAST, TE_EXPLOSION);
	PF_WriteCoord (MSG_BROADCAST, self->v.origin[0]);
	PF_WriteCoord (MSG_BROADCAST, self->v.origin[1]);
	PF_WriteCoord (MSG_BROADCAST, self->v.origin[2]);

	AsVector(self->v.velocity) = AsVector(vec3_origin);
	self->v.touch = SUB_NullTouch;
	PF_setmodel (self, "progs/s_explod.spr");
	self->v.solid = SOLID_NOT;
	s_explode1 (self);
}

void OgreGrenadeTouch(edict_t* self, edict_t* other)
{
	if (other == self->v.owner)
		return;		// don't explode on owner
	if (other->v.takedamage == DAMAGE_AIM)
	{
		OgreGrenadeExplode(self);
		return;
	}
	PF_sound (self, CHAN_VOICE, "weapons/bounce.wav", 1, ATTN_NORM);	// bounce sound
	if (AsVector(self->v.velocity) == AsVector(vec3_origin))
		AsVector(self->v.avelocity) = AsVector(vec3_origin);
}

/*
================
OgreFireGrenade
================
*/
void OgreFireGrenade(edict_t* self)
{
	self->v.effects = self->v.effects | EF_MUZZLEFLASH;

	PF_sound (self, CHAN_WEAPON, "weapons/grenade.wav", 1, ATTN_NORM);

	auto missile = PF_Spawn ();
	missile->v.owner = self;
	missile->v.movetype = MOVETYPE_BOUNCE;
	missile->v.solid = SOLID_BBOX;
		
// set missile speed	

	PF_makevectors (self->v.angles);

	PF_normalize(AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin), missile->v.velocity);
	AsVector(missile->v.velocity) = AsVector(missile->v.velocity) * 600;
	missile->v.velocity[2] = 200;

	AsVector(missile->v.avelocity) = Vector3D{300, 300, 300};

	PF_vectoangles(missile->v.velocity, missile->v.angles);
	
	missile->v.touch = OgreGrenadeTouch;
	
// set missile duration
	missile->v.nextthink = pr_global_struct->time + 2.5;
	missile->v.think = OgreGrenadeExplode;

	PF_setmodel (missile, "progs/grenade.mdl");
	PF_setsize (missile, vec3_origin, vec3_origin);
	PF_setorigin (missile, self->v.origin);
}


//=============================================================================

/*
================
chainsaw

FIXME
================
*/
void chainsaw(edict_t* self, float side)
{
	if (!self->v.enemy)
		return;
	if (!CanDamage (self, self->v.enemy, self))
		return;

	ai_charge(self, 10);

	auto delta = AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin);

	if (PF_vlen(delta) > 100)
		return;
		
	const float ldmg = (random() + random() + random()) * 4;
	T_Damage (self, self->v.enemy, self, self, ldmg);
	
	if (side)
	{
		PF_makevectors (self->v.angles);
		if (side == 1)
			SpawnMeatSpray (self, AsVector(self->v.origin) + AsVector(pr_global_struct->v_forward)*16, crandom() * 100 * AsVector(pr_global_struct->v_right));
		else
			SpawnMeatSpray (self, AsVector(self->v.origin) + AsVector(pr_global_struct->v_forward)*16, side * AsVector(pr_global_struct->v_right));
	}
}

void ogre_stand1(edict_t* self)
{
	animation_set(self, "stand");
}

void ogre_walk1(edict_t* self)
{
	animation_set(self, "walk");
}

void ogre_run1(edict_t* self)
{
	animation_set(self, "run");
}

void ogre_swing1(edict_t* self)
{
	animation_set(self, "swing");
}

void ogre_smash1(edict_t* self)
{
	animation_set(self, "smash");
}

void ogre_nail1(edict_t* self)
{
	animation_set(self, "shoot");
}

void ogre_pain1(edict_t* self)
{
	animation_set(self, "pain");
}

void ogre_painb1(edict_t* self)
{
	animation_set(self, "painb");
}

void ogre_painc1(edict_t* self)
{
	animation_set(self, "painc");
}

void ogre_paind1(edict_t* self)
{
	animation_set(self, "paind");
}

void ogre_paine1(edict_t* self)
{
	animation_set(self, "paine");
}

void ogre_die1(edict_t* self)
{
	animation_set(self, "death");
}

void ogre_bdie1(edict_t* self)
{
	animation_set(self, "bdeath");
}

void ogre_pain(edict_t* self, edict_t* attacker, float damage)
{
// don't make multiple pain sounds right after each other
	if (self->v.pain_finished > pr_global_struct->time)
		return;

	PF_sound (self, CHAN_VOICE, "ogre/ogpain1.wav", 1, ATTN_NORM);		

	const float r = random();
	
	if (r < 0.25)
	{
		ogre_pain1 (self);
		self->v.pain_finished = pr_global_struct->time + 1;
	}
	else if (r < 0.5)
	{
		ogre_painb1 (self);
		self->v.pain_finished = pr_global_struct->time + 1;
	}
	else if (r < 0.75)
	{
		ogre_painc1 (self);
		self->v.pain_finished = pr_global_struct->time + 1;
	}
	else if (r < 0.88)
	{
		ogre_paind1 (self);
		self->v.pain_finished = pr_global_struct->time + 2;
	}
	else
	{
		ogre_paine1 (self);
		self->v.pain_finished = pr_global_struct->time + 2;
	}
}

void ogre_die(edict_t* self)
{
// check for gib
	if (self->v.health < -80)
	{
		PF_sound (self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead (self, "progs/h_ogre.mdl", self->v.health);
		ThrowGib (self, "progs/gib3.mdl", self->v.health);
		ThrowGib (self, "progs/gib3.mdl", self->v.health);
		ThrowGib (self, "progs/gib3.mdl", self->v.health);
		return;
	}

	PF_sound (self, CHAN_VOICE, "ogre/ogdth.wav", 1, ATTN_NORM);
	
	if (random() < 0.5)
		ogre_die1 (self);
	else
		ogre_bdie1 (self);
}

void ogre_melee(edict_t* self)
{
	if (random() > 0.5)
		ogre_smash1 (self);
	else
		ogre_swing1 (self);
}

/*QUAKED monster_ogre (1 0 0) (-32 -32 -24) (32 32 64) Ambush

*/
void monster_ogre(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model ("progs/ogre.mdl");
	PF_precache_model("progs/h_ogre.mdl");
	PF_precache_model("progs/grenade.mdl");

	PF_precache_sound ("ogre/ogdrag.wav");
	PF_precache_sound ("ogre/ogdth.wav");
	PF_precache_sound ("ogre/ogidle.wav");
	PF_precache_sound ("ogre/ogidle2.wav");
	PF_precache_sound ("ogre/ogpain1.wav");
	PF_precache_sound ("ogre/ogsawatk.wav");
	PF_precache_sound ("ogre/ogwake.wav");

	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;

	PF_setmodel (self, "progs/ogre.mdl");

	PF_setsize (self, VEC_HULL2_MIN, VEC_HULL2_MAX);
	self->v.health = 200;

	self->v.th_stand = ogre_stand1;
	self->v.th_walk = ogre_walk1;
	self->v.th_run = ogre_run1;
	self->v.th_die = ogre_die;
	self->v.th_melee = ogre_melee;
	self->v.th_missile = ogre_nail1;
	self->v.th_pain = ogre_pain;
	self->v.animations_get = [](edict_t* self) { return &OgreAnimations; };
	
	walkmonster_start(self);
}

void monster_ogre_marksman(edict_t* self)
{
	monster_ogre (self);
}

LINK_ENTITY_TO_CLASS(monster_ogre);
LINK_ENTITY_TO_CLASS(monster_ogre_marksman);
