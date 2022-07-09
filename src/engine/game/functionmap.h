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
#include <cstring>
#include <utility>
#include <vector>

/**
*	@brief Map of function name<->function.
*/
class FunctionMap final
{
public:
	using Function = void (*)();

	void Add(const char* name, Function address);

	Function FindAddress(const char* name) const;

	const char* FindName(Function address) const;

private:
	std::vector<std::pair<const char*, Function>> m_Map;
};

inline FunctionMap& GetFunctionMap()
{
	static FunctionMap map;
	return map;
}

inline void FunctionMap::Add(const char* name, Function address)
{
	if (!name || !address)
	{
		return;
	}

	if (FindAddress(name) || FindName(address))
	{
		Sys_Error("Duplicate function map definition");
	}

	m_Map.emplace_back(name, address);
}

inline FunctionMap::Function FunctionMap::FindAddress(const char* name) const
{
	if (auto it = std::find_if(m_Map.begin(), m_Map.end(), [&](const auto& candidate)
		{
			return !strcmp(candidate.first, name);
		}); it != m_Map.end())
	{
		return it->second;
	}

		return nullptr;
}

inline const char* FunctionMap::FindName(Function address) const
{
	if (auto it = std::find_if(m_Map.begin(), m_Map.end(), [&](const auto& candidate)
		{
			return candidate.second == address;
		}); it != m_Map.end())
	{
		return it->first;
	}

		return {};
}

struct FunctionDefinition final
{
	FunctionDefinition(const char* name, FunctionMap::Function address)
	{
		GetFunctionMap().Add(name, address);
	}
};

/**
*	@brief Macro to simplify linking names to functions.
*/
#define LINK_FUNCTION_TO_NAME(name) \
static const FunctionDefinition g_##name##FunctionDefinition(#name, reinterpret_cast<FunctionMap::Function>(&name))
