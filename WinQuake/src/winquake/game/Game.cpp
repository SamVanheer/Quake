/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
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

bool Game::SpawnEntity(edict_t* entity, const char* classname)
{
	//TODO
	return false;
}

void Game::EntityThink(edict_t* entity, edict_t* other)
{
	//TODO
}

void Game::EntityTouch(edict_t* entity, edict_t* other)
{
	//TODO
}

void Game::EntityBlocked(edict_t* entity, edict_t* other)
{
	//TODO
}

void Game::StartFrame(edict_t* entities)
{
	//TODO
}
