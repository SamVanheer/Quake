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

#pragma once

#include <cassert>
#include <string_view>

#include "common/Platform.h"

using InterfaceFactory = void* (*)();

struct CInterfaceRegistry final
{
	CInterfaceRegistry(std::string_view name, InterfaceFactory factory)
		: Next(Head)
		, Name(name)
		, Factory(factory)
	{
		assert(!name.empty());
		assert(factory);

		Head = this;
	}

	static inline CInterfaceRegistry* Head = nullptr;

	CInterfaceRegistry* const Next;
	const std::string_view Name;
	const InterfaceFactory Factory;
};

#define REGISTER_INTERFACE_SINGLETON(name, interfaceName, className)																\
static className g_##className;																										\
static const CInterfaceRegistry g_##className##Reg(name, [] { return static_cast<void*>(static_cast<interfaceName*>(&g_##className)); })

extern "C" DLLEXPORT void* QueryInterface(const char* name);

using InterfaceAccessor = decltype(QueryInterface)*;

constexpr char QueryInterfaceName[]{"QueryInterface"};

InterfaceAccessor GetLocalQueryInterface();

/**
*	@brief Manages a dynamic library automatically.
*/
class DynamicLibrary final
{
private:
	using Library = void*;

public:
	DynamicLibrary() = default;

	/**
	*	@brief Loads the dynamic library with the given name.
	*/
	DynamicLibrary(const char* libraryName);
	~DynamicLibrary();

	DynamicLibrary(const DynamicLibrary&) = delete;
	DynamicLibrary& operator=(const DynamicLibrary&) = delete;

	DynamicLibrary(DynamicLibrary&& other) noexcept;
	DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

	/**
	*	@brief Returns whether this library is loaded.
	*/
	operator bool() const { return m_Library != nullptr; }

	/**
	*	@brief Closes the library if it is open.
	*	@details
	*	All references to memory acquired from this library should be freed and cleared before closing the library.
	*/
	void Close();

	/**
	*	@brief Gets the QueryInterface function if this library has one.
	*/
	InterfaceAccessor GetInterfaceAccessor() const;

private:
	static Library LoadDynamicLibrary(const char* fileName);
	static void FreeDynamicLibrary(Library library);

private:
	Library m_Library = nullptr;
};
