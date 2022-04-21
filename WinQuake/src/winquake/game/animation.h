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

#include <algorithm>
#include <string_view>
#include <vector>

struct Animation;

/**
*	@file
*
*	Quake animation facilities.
*/

using animation_frame_func = void (*)(edict_t* self, const Animation* animation, int frame);

namespace AnimationFlag
{
enum
{
	None = 0,
	Looping = 1 << 0,
};
}

struct Animation
{
	const std::string_view Name;
	const int StartIndex;
	const int NumFrames;
	const animation_frame_func FrameFunction;
	const int Flags;

	Animation(std::string_view name, int startIndex, int numFrames, animation_frame_func frameFunction, int flags)
		: Name(name)
		, StartIndex(startIndex)
		, NumFrames(numFrames)
		, FrameFunction(frameFunction)
		, Flags(flags)
	{
	}

	constexpr int GetEnd() const { return StartIndex + NumFrames; }

	constexpr bool ReachedEnd(int frame) const { return (frame + 1) >= NumFrames; }
};

/**
*	@brief Immutable collection of animations.
*/
struct Animations
{
	//TODO: it might be possible to use a std::array here if variadic templates work with specific types.
	//C++20 allows that, but seems to have trouble understanding those kinds of template constructs.
	const std::vector<Animation> Animations;

	const Animation* FindAnimation(std::string_view name) const
	{
		if (auto it = std::find_if(Animations.begin(), Animations.end(), [&](const auto& animation)
			{
				return animation.Name == name;
			}); it != Animations.end())
		{
			return &(*it);
		}

		return nullptr;
	}

	int FindAnimationStart(std::string_view name) const
	{
		if (auto animation = FindAnimation(name); animation)
		{
			return animation->StartIndex;
		}

		//Safe default value. The first animation will be used.
		return 0;
	}

	int FindAnimationEnd(std::string_view name) const
	{
		if (auto animation = FindAnimation(name); animation)
		{
			return animation->GetEnd();
		}

		//Safe default value. The first animation will be used.
		return 0;
	}
};

struct AnimationDescriptor
{
	const std::string_view Name;
	const int NumFrames;

	/**
	*	@brief If provided, this function will be called every frame with the current animation and frame number (starting at 0).
	*/
	const animation_frame_func FrameFunction = nullptr;
	const int Flags = AnimationFlag::None;
};

/**
*	@brief To match QuakeC this should be used to generate frame indices for a set of animations as defined in a QuakeC source file.
*	All of the frames defined in a single QuakeC file are part of the same set.
*/
inline Animations MakeAnimations(std::initializer_list<AnimationDescriptor> anims)
{
	std::vector<Animation> result;

	result.reserve(anims.size());

	int startIndex = 0;

	for (const auto& source : anims)
	{
		result.emplace_back(source.Name, startIndex, source.NumFrames, source.FrameFunction, source.Flags);
		startIndex += source.NumFrames;
	}

	return Animations{std::move(result)};
}

const Animations* animations_get(edict_t* self);
void animation_set(edict_t* self, const char* name, int frame = 0);
void animation_advance(edict_t* self);
