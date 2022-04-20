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
#include "doors.h"
#include "fight.h"
#include "Game.h"
#include "subs.h"
#include "weapons.h"

/*
==============================================================================

BOSS-ONE

==============================================================================
*/
void boss_missile1(edict_t* self);
void boss_missile(edict_t* self, vec3_t p);
void boss_face(edict_t* self);
void boss_death1(edict_t* self);

void boss_rise_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		PF_sound(self, CHAN_WEAPON, "boss1/out1.wav", 1, ATTN_NORM);
		break;

	case 1:
		PF_sound(self, CHAN_VOICE, "boss1/sight1.wav", 1, ATTN_NORM);
		break;
	}

	if (animation->ReachedEnd(frame))
	{
		self->v.think = boss_missile1;
	}
}

void boss_walk_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		// look for other players
		break;

	default:
		boss_face(self);
		break;
	}
}

void boss_attack_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	default:
		boss_face(self);
		break;

	case 8:
		boss_missile(self, Vector3D{100, 100, 200});
		break;

	case 19:
		boss_missile(self, Vector3D{100, -100, 200});
		break;
	}
}

void boss_shocka_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = boss_missile1;
	}
}

void boss_shockb_postframes(edict_t* self)
{
	if (self->v.frame >= 9)
	{
		self->v.think = boss_missile1;
	}
	else
	{
		++self->v.frame;
	}
}

void boss_shockb_frame(edict_t* self, const Animation* animation, int frame)
{
	//shockb only has 6 frames but needs to run for 10.
	if (animation->ReachedEnd(frame))
	{
		self->v.think = boss_shockb_postframes;
	}
}

void boss_shockc_frame(edict_t* self, const Animation* animation, int frame)
{
	if (animation->ReachedEnd(frame))
	{
		self->v.think = boss_death1;
	}
}

void boss_death_final(edict_t* self)
{
	pr_global_struct->killed_monsters = pr_global_struct->killed_monsters + 1;
	PF_WriteByte(MSG_ALL, SVC_KILLEDMONSTER);	// FIXME: reliable broadcast
	SUB_UseTargets(self);
	PF_Remove(self);
}

void boss_death_frame(edict_t* self, const Animation* animation, int frame)
{
	switch (frame)
	{
	case 0:
		PF_sound(self, CHAN_VOICE, "boss1/death.wav", 1, ATTN_NORM);
		break;

	case 8:
		PF_sound(self, CHAN_BODY, "boss1/out1.wav", 1, ATTN_NORM);
		PF_WriteByte(MSG_BROADCAST, SVC_TEMPENTITY);
		PF_WriteByte(MSG_BROADCAST, TE_LAVASPLASH);
		PF_WriteCoord(MSG_BROADCAST, self->v.origin[0]);
		PF_WriteCoord(MSG_BROADCAST, self->v.origin[1]);
		PF_WriteCoord(MSG_BROADCAST, self->v.origin[2]);

		//The last frame is played twice, so we need to delay this by a frame.
		self->v.think = &boss_death_final;
		break;
	}
}

const Animations BossAnimations = MakeAnimations(
	{
		{"rise", 17, &boss_rise_frame},
		{"walk", 31, &boss_walk_frame, AnimationFlag::Looping},
		{"death", 9, &boss_death_frame},
		{"attack", 23, &boss_attack_frame, AnimationFlag::Looping},
		{"shocka", 10, &boss_shocka_frame},
		{"shockb", 6, &boss_shockb_frame},
		{"shockc", 10, &boss_shockc_frame}
	});

void boss_face(edict_t* self)
{
	
// go for another player if multi player
	if (self->v.enemy->v.health <= 0 || random() < 0.02)
	{
		self->v.enemy = PF_Find(self->v.enemy, "classname", "player");
		if (self->v.enemy == pr_global_struct->world)
			self->v.enemy = PF_Find(self->v.enemy, "classname", "player");
	}
	ai_face(self);
}

void boss_rise1(edict_t* self)
{
	animation_set(self, "rise");
}

void boss_idle1(edict_t* self)
{
	animation_set(self, "walk");
}

void boss_missile1(edict_t* self)
{
	animation_set(self, "attack");
}

void boss_shocka1(edict_t* self)
{
	animation_set(self, "shocka");
}

void boss_shockb1(edict_t* self)
{
	animation_set(self, "shockb");
}

void boss_shockc1(edict_t* self)
{
	animation_set(self, "shockc");
}

void boss_death1(edict_t* self)
{
	animation_set(self, "death");
}

void boss_missile(edict_t* self, vec3_t p)
{
	Vector3D offang;
	Vector3D vec, d;

	PF_vectoangles (AsVector(self->v.enemy->v.origin) - AsVector(self->v.origin), offang);
	PF_makevectors (offang);

	auto org = AsVector(self->v.origin) + p[0]* AsVector(pr_global_struct->v_forward) + p[1]* AsVector(pr_global_struct->v_right) + p[2] * Vector3D{0, 0, 1};
	
// lead the player on hard mode
	if (game_skill > 1)
	{
		const float t = PF_vlen(AsVector(self->v.enemy->v.origin) - org) / 300;
		vec = AsVector(self->v.enemy->v.velocity);
		vec[2] = 0;
		d = AsVector(self->v.enemy->v.origin) + t * vec;
	}
	else
	{
		d = AsVector(self->v.enemy->v.origin);
	}
	
	PF_normalize (d - org, vec);

	auto newmis = launch_spike (self, org, vec);
	PF_setmodel (newmis, "progs/lavaball.mdl");
	AsVector(newmis->v.avelocity) = Vector3D{200, 100, 300};
	PF_setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);		
	AsVector(newmis->v.velocity) = vec*300;
	newmis->v.touch = T_MissileTouch; // rocket explosion
	PF_sound (self, CHAN_WEAPON, "boss1/throw.wav", 1, ATTN_NORM);

// check for dead enemy
	if (self->v.enemy->v.health <= 0)
		boss_idle1 (self);
}


void boss_awake(edict_t* self, edict_t* other)
{
	self->v.solid = SOLID_SLIDEBOX;
	self->v.movetype = MOVETYPE_STEP;
	self->v.takedamage = DAMAGE_NO;
	
	PF_setmodel (self, "progs/boss.mdl");
	PF_setsize (self, Vector3D{-128, -128, -24}, Vector3D{128, 128, 256});
	
	if (game_skill == 0)
		self->v.health = 1;
	else
		self->v.health = 3;

	self->v.enemy = activator;

	PF_WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
	PF_WriteByte (MSG_BROADCAST, TE_LAVASPLASH);
	PF_WriteCoord (MSG_BROADCAST, self->v.origin[0]);
	PF_WriteCoord (MSG_BROADCAST, self->v.origin[1]);
	PF_WriteCoord (MSG_BROADCAST, self->v.origin[2]);

	self->v.yaw_speed = 20;
	boss_rise1 (self);
}


/*QUAKED monster_boss (1 0 0) (-128 -128 -24) (128 128 256)
*/
void monster_boss(edict_t* self)
{
	if (pr_global_struct->deathmatch)
	{
		PF_Remove(self);
		return;
	}
	PF_precache_model ("progs/boss.mdl");
	PF_precache_model("progs/lavaball.mdl");

	PF_precache_sound ("weapons/rocket1i.wav");
	PF_precache_sound ("boss1/out1.wav");
	PF_precache_sound ("boss1/sight1.wav");
	PF_precache_sound ("misc/power.wav");
	PF_precache_sound ("boss1/throw.wav");
	PF_precache_sound ("boss1/pain.wav");
	PF_precache_sound ("boss1/death.wav");

	pr_global_struct->total_monsters = pr_global_struct->total_monsters + 1;

	self->v.use = boss_awake;
	self->v.animations_get = [](edict_t* self) { return &BossAnimations; };
}

LINK_ENTITY_TO_CLASS(monster_boss);

//===========================================================================

edict_t*	le1, * le2;
float	lightning_end;

void lightning_fire(edict_t* self)
{
	if (pr_global_struct->time >= lightning_end)
	{	// done here, put the terminals back up
		door_go_down (le1);
		door_go_down (le2);
		return;
	}
	
	auto p1 = (AsVector(le1->v.mins) + AsVector(le1->v.maxs)) * 0.5f;
	p1[2] = le1->v.absmin[2] - 16;
	
	auto p2 = (AsVector(le2->v.mins) + AsVector(le2->v.maxs)) * 0.5f;
	p2[2] = le2->v.absmin[2] - 16;
	
	// compensate for length of bolt
	Vector3D offset;
	PF_normalize(p2 - p1, offset);

	p2 = p2 - offset*100;

	self->v.nextthink = pr_global_struct->time + 0.1;
	self->v.think = lightning_fire;

	PF_WriteByte (MSG_ALL, SVC_TEMPENTITY);
	PF_WriteByte (MSG_ALL, TE_LIGHTNING3);
	PF_WriteEntity (MSG_ALL, pr_global_struct->world);
	PF_WriteCoord (MSG_ALL, p1[0]);
	PF_WriteCoord (MSG_ALL, p1[1]);
	PF_WriteCoord (MSG_ALL, p1[2]);
	PF_WriteCoord (MSG_ALL, p2[0]);
	PF_WriteCoord (MSG_ALL, p2[1]);
	PF_WriteCoord (MSG_ALL, p2[2]);
}

void lightning_use(edict_t* self, edict_t* other)
{
	if (lightning_end >= pr_global_struct->time + 1)
		return;

	le1 = PF_Find(pr_global_struct->world, "target", "lightning");
	le2 = PF_Find( le1, "target", "lightning");
	if (le1 == pr_global_struct->world || le2 == pr_global_struct->world)
	{
		dprint ("missing lightning targets\n");
		return;
	}
	
	if (
	 (le1->v.state != STATE_TOP && le1->v.state != STATE_BOTTOM)
	|| (le2->v.state != STATE_TOP && le2->v.state != STATE_BOTTOM)
	|| (le1->v.state != le2->v.state) )
	{
//		dprint ("not aligned\n");
		return;
	}

// don't let the electrodes go back up until the bolt is done
	le1->v.nextthink = -1;
	le2->v.nextthink = -1;
	lightning_end = pr_global_struct->time + 1;

	PF_sound (self, CHAN_VOICE, "misc/power.wav", 1, ATTN_NORM);
	lightning_fire (self);		

// advance the boss pain if down
	self = PF_Find(pr_global_struct->world, "classname", "monster_boss");
	if (self == pr_global_struct->world)
		return;
	self->v.enemy = activator;
	if (le1->v.state == STATE_TOP && self->v.health > 0)
	{
		PF_sound (self, CHAN_VOICE, "boss1/pain.wav", 1, ATTN_NORM);
		self->v.health = self->v.health - 1;
		if (self->v.health >= 2)
			boss_shocka1(self);
		else if (self->v.health == 1)
			boss_shockb1(self);
		else if (self->v.health == 0)
			boss_shockc1(self);
	}	
}


/*QUAKED event_lightning (0 1 1) (-16 -16 -16) (16 16 16)
Just for boss level.
*/
void event_lightning(edict_t* self)
{
	self->v.use = lightning_use;
}

LINK_ENTITY_TO_CLASS(event_lightning);
