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
#include "entitymap.h"
#include "game.h"
#include "IGame.h"

Game g_GameImplementation;

IGame* g_Game = &g_GameImplementation;

void Game::Initialize()
{
}

void Game::Shutdown()
{
}

void Game::NewMapStarted()
{
}

void* Game::FindFunctionAddress(const char* name)
{
	return GetFunctionMap().FindAddress(name);
}

const char* Game::FindFunctionName(void* address)
{
	return GetFunctionMap().FindName(address);
}

bool Game::SpawnEntity(edict_t* entity, const char* classname)
{
	return GetEntityMap().TrySpawn(classname, entity);
}

void Game::EntityThink(edict_t* entity, edict_t* other)
{
	//Should always be true if we get here since nextthink has to be non-zero.
	if (entity->v.think)
	{
		entity->v.think(entity);
	}
}

void Game::EntityTouch(edict_t* entity, edict_t* other)
{
	if (entity->v.touch)
	{
		entity->v.touch(entity, other);
	}
}

void Game::EntityBlocked(edict_t* entity, edict_t* other)
{
	if (entity->v.blocked)
	{
		entity->v.blocked(entity, other);
	}
}

void Game::StartFrame(edict_t* entities)
{
	pr_global_struct->teamplay = PF_cvar("teamplay");
	pr_global_struct->game_skill = PF_cvar("skill");
	++pr_global_struct->framecount;
}
