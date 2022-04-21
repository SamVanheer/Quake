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
#include "animation.h"

const Animations* animations_get(edict_t* self)
{
	if (!self->v.animations_get)
	{
		PF_objerror("Cannot advance animations without animations_get");
	}

	auto animations = self->v.animations_get(self);

	if (!animations)
	{
		PF_objerror("Cannot advance animations without animations set");
	}

	return animations;
}

void animation_set_frame(edict_t* self, const Animation* animation, float frame)
{
	self->v.frame = frame;

	if (self->v.frame < 0)
	{
		self->v.frame = 0;
	}
	else if (self->v.frame >= animation->GetEnd())
	{
		if (animation->Flags & AnimationFlag::Looping)
		{
			self->v.frame = animation->StartIndex;
		}
		else
		{
			self->v.frame = animation->GetEnd() - 1;
		}
	}

	if (animation->FrameFunction)
	{
		animation->FrameFunction(self, animation, static_cast<int>(self->v.frame - animation->StartIndex));
	}
}

void animation_set(edict_t* self, const char* name, int frame)
{
	auto animations = animations_get(self);

	self->v.animation = name;

	if (!self->v.animation)
	{
		PF_objerror("Cannot set animation without animation name");
	}

	//Set before calling animation think so it can override.
	self->v.nextthink = pr_global_struct->time + 0.1f;
	self->v.think = &animation_advance;

	const auto animation = animations->FindAnimation(self->v.animation);

	if (!animation)
	{
		PF_objerror("Animation does not exist");
	}

	animation_set_frame(self, animation, animation->StartIndex + frame);
}

void animation_advance(edict_t* self)
{
	auto animations = animations_get(self);

	if (!self->v.animation)
	{
		PF_objerror("Cannot advance animation without animation name");
	}

	//Set before calling animation think so it can override.
	self->v.nextthink = pr_global_struct->time + 0.1f;

	const auto animation = animations->FindAnimation(self->v.animation);

	animation_set_frame(self, animation, self->v.frame + 1);
}
