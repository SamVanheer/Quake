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

#include <cstdio>
#include <vector>

#include "IDedicatedConsole.h"
#include "IDedicatedLauncher.h"
#include "interface.h"

static void Sys_Error(const char* msg)
{
	printf("%s", msg);
}

static int LaunchGame(std::size_t argc, const char* const* argv)
{
	DynamicLibrary engine{"engine"};

	if (!engine)
	{
		Sys_Error("Could not load library \"engine\"\n");
		return EXIT_FAILURE;
	}

	auto accessor = engine.GetInterfaceAccessor();

	if (!accessor)
	{
		Sys_Error("Could not get interface accessor for engine\n");
		return EXIT_FAILURE;
	}

	auto launcher = reinterpret_cast<IDedicatedLauncher*>(accessor(DEDICATED_LAUNCHER_VERSION));

	if (!launcher)
	{
		Sys_Error("Could not get launcher interface\n");
		return EXIT_FAILURE;
	}

	return launcher->Run(argc, argv, GetLocalQueryInterface());
}

int main(int argc, char* argv[])
{
	const int result = LaunchGame(argc, argv);

	return result;
}

struct CDedicatedConsole final : public IDedicatedConsole
{
	void Printf(const char* format, ...) override
	{
		va_list list;
		va_start(list, format);
		vfprintf(stdout, format, list);
		va_end(list);
		fflush(stdout);
	}

	void VPrintf(const char* format, va_list list) override
	{
		vfprintf(stdout, format, list);
		fflush(stdout);
	}

	bool GetTextInput(char* buffer, std::size_t bufferSize) override
	{
		//TODO: blocking input, freezes server.
		return fgets(buffer, bufferSize, stdin) != nullptr;
	}
};

REGISTER_INTERFACE_SINGLETON(DEDICATED_CONSOLE_VERSION, IDedicatedConsole, CDedicatedConsole);
