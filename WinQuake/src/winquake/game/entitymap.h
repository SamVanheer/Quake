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

#include <string_view>
#include <unordered_map>

class EntityDefinition;

/**
*	@brief Map of entity name->spawn function.
*/
class EntityMap final
{
public:
	void Add(const EntityDefinition* definition);

	bool TrySpawn(std::string_view name, edict_t* self);

private:
	std::unordered_map<std::string_view, const EntityDefinition*> m_Map;
};

inline EntityMap& GetEntityMap()
{
	static EntityMap map;
	return map;
}

class EntityDefinition final
{
public:
	using SpawnFunction = void (*)(edict_t* self);

	EntityDefinition(std::string_view name, SpawnFunction spawnFunction)
		: m_Name(name)
		, m_SpawnFunction(spawnFunction)
	{
		GetEntityMap().Add(this);
	}

	~EntityDefinition() = default;

public:
	constexpr std::string_view GetName() const { return m_Name; }

	void Spawn(edict_t* self) const
	{
		m_SpawnFunction(self);
	}

private:
	const std::string_view m_Name;
	const SpawnFunction m_SpawnFunction;
};

inline void EntityMap::Add(const EntityDefinition* definition)
{
	if (!definition)
	{
		return;
	}

	if (m_Map.find(definition->GetName()) != m_Map.end())
	{
		Sys_Error("Duplicate entity definition");
	}

	m_Map.emplace(definition->GetName(), definition);
}

inline bool EntityMap::TrySpawn(std::string_view name, edict_t* self)
{
	if (auto it = m_Map.find(name); it != m_Map.end())
	{
		it->second->Spawn(self);
		return true;
	}

	return false;
}

/**
*	@brief Macro to simplify linking entities to spawn functions.
*/
#define LINK_ENTITY_TO_CLASS(classname) \
static const EntityDefinition g_##classname##Definition(#classname, classname)
