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

#include <vector>

#define SDL_MAIN_HANDLED
#include <SDL_messagebox.h>

#include "IClientLauncher.h"
#include "interface.h"

#ifdef WIN32
#include <Windows.h>
#endif

static void Sys_Error(const char* msg)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Quake Error", msg, nullptr);
}

static int LaunchGame(std::size_t argc, const char** argv)
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

	auto launcher = reinterpret_cast<IClientLauncher*>(accessor(CLIENT_LAUNCHER_VERSION));

	if (!launcher)
	{
		Sys_Error("Could not get launcher interface\n");
		return EXIT_FAILURE;
	}

	return launcher->Run(argc, argv, GetLocalQueryInterface());
}

#ifdef WIN32
std::vector<const char*> argv;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	HANDLE mutex = CreateMutexA(nullptr, TRUE, "Quake1Mutex");

	if (mutex == nullptr || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		Sys_Error("Could not launch game.\nOnly one instance of this game can be run at a time.");
		return 1;
	}

	argv.push_back("");

	while (*lpCmdLine)
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv.push_back(lpCmdLine);

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}

	const int result = LaunchGame(argv.size(), argv.data());

	ReleaseMutex(mutex);
	mutex = nullptr;

	//Translate regular main return values to WinMain return values.
	return result != 0 ? 0 : 1;
}
#else
int main(int argc, char* argv[])
{
	const int result = LaunchGame(argc, argv);

	return result;
}
#endif
