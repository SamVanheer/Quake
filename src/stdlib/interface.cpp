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

#include "interface.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

static void* LocalQueryInterface(const char* name)
{
	for (auto reg = CInterfaceRegistry::Head; reg; reg = reg->Next)
	{
		if (reg->Name == name)
		{
			return reg->Factory();
		}
	}

	return nullptr;
}

void* QueryInterface(const char* name)
{
	return LocalQueryInterface(name);
}

InterfaceAccessor GetLocalQueryInterface()
{
	return &LocalQueryInterface;
}

DynamicLibrary::DynamicLibrary(const char* libraryName)
	: m_Library(LoadDynamicLibrary(libraryName))
{
}

DynamicLibrary::~DynamicLibrary()
{
	Close();
}

DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
	: m_Library(other.m_Library)
{
	other.m_Library = nullptr;
}

DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept
{
	if (this != &other)
	{
		m_Library = other.m_Library;
		other.m_Library = nullptr;
	}

	return *this;
}

void DynamicLibrary::Close()
{
	if (m_Library)
	{
		FreeDynamicLibrary(m_Library);
		m_Library = nullptr;
	}
}

InterfaceAccessor DynamicLibrary::GetInterfaceAccessor() const
{
	if (*this)
	{
		return reinterpret_cast<InterfaceAccessor>([&]()
		{
#ifdef WIN32
			return GetProcAddress(reinterpret_cast<HMODULE>(m_Library), QueryInterfaceName);
#else
			return dlsym(m_Library, QueryInterfaceName);
#endif
		}());
	}

	return nullptr;
}

DynamicLibrary::Library DynamicLibrary::LoadDynamicLibrary(const char* fileName)
{
#ifdef WIN32
	return reinterpret_cast<Library>(LoadLibraryA(fileName));
#else
	return dlopen(fileName, RTLD_NOW);
#endif
}

void DynamicLibrary::FreeDynamicLibrary(Library library)
{
	if (library)
	{
#ifdef WIN32
		::FreeLibrary(reinterpret_cast<HMODULE>(library));
#else
		dlclose(library);
#endif
	}
}
