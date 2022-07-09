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
// sys.cpp -- system interface code

#include <chrono>
#include <filesystem>
#include <thread>

#include "quakedef.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "IClientLauncher.h"
#include "IDedicatedConsole.h"
#include "IDedicatedLauncher.h"
#include "interface.h"

#ifdef WIN32
#include "winquake.h"
#else
#include <signal.h>
#endif

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_main.h>
#include <SDL_messagebox.h>

#define MINIMUM_WIN_MEMORY		0x0880000
#define MAXIMUM_WIN_MEMORY		0x1000000

#define CONSOLE_ERROR_TIMEOUT	60.0	// # of seconds to wait on Sys_Error running
										//  dedicated before exiting
#define PAUSE_SLEEP		50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP	20				// sleep time when not focus

bool ActiveApp, Minimized;

static double starttime = 0.0;
static double curtime = 0.0;
static double lastcurtime = 0.0;

bool isDedicated;

static const char* tracking_tag = "Clams & Mooses";

IDedicatedConsole* g_DedicatedConsole = nullptr;

void Sys_InitFloatTime(void);

volatile int					sys_checksum;

/*
===============================================================================

FILE IO

===============================================================================
*/

/*
================
filelength
================
*/
long filelength(FILE* f)
{
	const long pos = ftell(f);
	fseek(f, 0, SEEK_END);
	const long end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}

long Sys_FileOpenRead(const char* path, FILE** hndl)
{
	if (auto f = fopen(path, "rb"); f)
	{
		*hndl = f;
		return filelength(f);
	}

	*hndl = nullptr;
	return -1;
}

FILE* Sys_FileOpenWrite(const char* path)
{
	auto f = fopen(path, "wb");
	if (!f)
		Sys_Error("Error opening %s: %s", path, strerror(errno));
	return f;
}

time_t Sys_FileTime(const char* path)
{
	struct stat buf;

	if (stat(path, &buf) == -1)
	{
		return -1;
	}

	return buf.st_mtime;
}

void Sys_mkdir(const char* path)
{
#ifdef WIN32
	_mkdir(path);
#else
	mkdir(path, 0777);
#endif
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

/*
================
Sys_Init
================
*/
void Sys_Init(void)
{
	Sys_InitFloatTime();
}


void Sys_Error(const char* error, ...)
{
	static bool in_sys_error0 = false;
	static bool in_sys_error1 = false;
	static bool in_sys_error3 = false;

	if (!in_sys_error3)
	{
		in_sys_error3 = true;
	}

	va_list argptr;
	va_start(argptr, error);
	char text[1024];
	vsprintf(text, error, argptr);
	va_end(argptr);

	if (isDedicated)
	{
		//Can be null if we're erroring before it's acquired.
		if (g_DedicatedConsole)
		{
			g_DedicatedConsole->Printf("\n");
			g_DedicatedConsole->Printf("***********************************\n");
			g_DedicatedConsole->Printf("ERROR: %s\n", text);
			g_DedicatedConsole->Printf("***********************************\n");
		}
		else
		{
			//So something is logged somewhere.
			printf("ERROR: %s\n", text);
		}
	}
	else
	{
		// switch to windowed so the message box is visible, unless we already
		// tried that and failed
		if (!in_sys_error0)
		{
			in_sys_error0 = true;
			VID_SetDefaultMode();
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Quake Error", text, nullptr);
		}
		else
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Double Quake Error", text, nullptr);
		}
	}

	if (!in_sys_error1)
	{
		in_sys_error1 = true;
		Host_Shutdown();
	}

	exit(1);
}

void Sys_Printf(const char* fmt, ...)
{
	if (isDedicated)
	{
		va_list argptr;
		va_start(argptr, fmt);
		g_DedicatedConsole->VPrintf(fmt, argptr);
		va_end(argptr);
	}
}

void Sys_Quit(void)
{
	Host_Shutdown();

	fflush(stdout);
	exit(0);
}


/*
================
Sys_FloatTime
================
*/
double Sys_FloatTime(void)
{
	static bool first = true;

	static const std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

	if (first)
	{
		first = false;
		curtime = starttime;
	}
	else
	{
		const auto currentTime = std::chrono::high_resolution_clock::now();

		const auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0;

		curtime = starttime + timePassed;

		static int sametimecount = 0;

		if (curtime == lastcurtime)
		{
			sametimecount++;

			if (sametimecount > 100000)
			{
				curtime += 1.0;
				sametimecount = 0;
			}
		}
		else
		{
			sametimecount = 0;
		}
	}

	lastcurtime = curtime;

	return curtime;
}


/*
================
Sys_InitFloatTime
================
*/
void Sys_InitFloatTime(void)
{
	if (const int j = COM_CheckParm("-starttime"); j)
	{
		starttime = (double)(Q_atof(com_argv[j + 1]));
	}
	else
	{
		starttime = 0.0;
	}

	Sys_FloatTime();

	lastcurtime = curtime;
}

char* Sys_ConsoleInput(bool acceptEmptyInput)
{
	if (!isDedicated)
		return nullptr;

	static char	text[256]{};
	static size_t len = 0;

	{
		char* buffer = text;
		size_t count = sizeof(text);

		if (len > 0)
		{
			//Previous unterminated input still in the buffer, try to read additional text.
			buffer += len;
			count -= len;
		}

		if (!g_DedicatedConsole->GetTextInput(buffer, count))
		{
			//No input was read, or some error occurred.
			return nullptr;
		}
	}

	len = strlen(text);

	if (const char end = text[len - 1]; end == '\n' || end == '\r')
	{
		//It's a complete line. Return command and clear buffer for next input.
		text[len - 1] = '\0';
		len = 0;

		if (text[0] != '\0' || acceptEmptyInput)
		{
			return text;
		}

		return nullptr;
	}

	if (len + 1 >= sizeof(text))
	{
		//Input too large, wipe contents and start over. (matches original behavior on Windows)
		memset(text, 0, sizeof(text));
		len = 0;
	}

	//Incomplete line.
	return nullptr;
}

void Sys_Sleep(void)
{
	std::this_thread::sleep_for(std::chrono::milliseconds{1});
}


void Sys_SendKeyEvents(void)
{
	SDL_PumpEvents();

	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		// we always update if there are any event, even if we're paused
		scr_skipupdate = false;

		VID_ProcessEvent(event);
	}
}


/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/


/*
==================
WinMain
==================
*/
void SleepUntilInput(int time)
{
	SDL_WaitEventTimeout(nullptr, time);
}


/*
==================
WinMain
==================
*/

#ifndef WIN32
void signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	Sys_Quit();
	exit(0);
}

void InitSig()
{
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}
#endif

int EngineMain(int argc, const char* const* argv, InterfaceAccessor launcherInterfaceAccessor, bool dedicated)
{
	if (!launcherInterfaceAccessor)
	{
		Sys_Error("Launcher interface accessor is null");
	}

#ifndef WIN32
	InitSig();
#endif

	//TODO: need to rework this so SDL2's setup code is used instead.
	SDL_SetMainReady();

	std::string cwd = std::filesystem::current_path().string();

	if (cwd.empty())
		Sys_Error("Couldn't determine current directory");

	if (cwd.back() == '/')
	{
		cwd.resize(cwd.size() - 1);
	}

	quakeparms_t parms{};
	parms.basedir = cwd.c_str();
	parms.cachedir = NULL;

	COM_InitArgv(argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	isDedicated = dedicated;

	if (isDedicated)
	{
		g_DedicatedConsole = reinterpret_cast<IDedicatedConsole*>(launcherInterfaceAccessor(DEDICATED_CONSOLE_VERSION));

		if (!g_DedicatedConsole)
		{
			Sys_Error("Couldn't get dedicated server console");
		}
	}

	// take 16 Mb, unless they explicitly
	// request otherwise
	parms.memsize = MAXIMUM_WIN_MEMORY;

	if (int t = COM_CheckParm("-heapsize"); t != 0)
	{
		t = COM_CheckParm("-heapsize") + 1;

		if (t < com_argc)
			parms.memsize = Q_atoi(com_argv[t]) * 1024;
	}

	parms.membase = malloc(parms.memsize);

	if (!parms.membase)
		Sys_Error("Not enough memory free; check disk space\n");

	Sys_Init();

	Sys_Printf("Host_Init\n");
	Host_Init(&parms);

	double oldtime = Sys_FloatTime();

	double time, newtime;

	/* main window message loop */
	while (1)
	{
		if (isDedicated)
		{
			newtime = Sys_FloatTime();
			time = newtime - oldtime;

			while (time < sys_ticrate.value)
			{
				Sys_Sleep();
				newtime = Sys_FloatTime();
				time = newtime - oldtime;
			}
		}
		else
		{
			// yield the CPU for a little while when paused, minimized, or not the focus
			if ((cl.paused && !ActiveApp) || Minimized || block_drawing)
			{
				SleepUntilInput(PAUSE_SLEEP);
				scr_skipupdate = true;		// no point in bothering to draw
			}
			else if (!ActiveApp)
			{
				SleepUntilInput(NOT_FOCUS_SLEEP);
			}

			newtime = Sys_FloatTime();
			time = newtime - oldtime;
		}

		Host_Frame(time);
		oldtime = newtime;
	}

	SDL_Quit();

	/* return success of application */
	return 0;
}

struct CClientLauncher final : public IClientLauncher
{
	int Run(std::size_t argc, const char* const* argv, InterfaceAccessor launcherInterfaceAccessor) override
	{
		return EngineMain(argc, argv, launcherInterfaceAccessor, false);
	}
};

REGISTER_INTERFACE_SINGLETON(CLIENT_LAUNCHER_VERSION, IClientLauncher, CClientLauncher);

struct CDedicatedLauncher final : public IDedicatedLauncher
{
	int Run(std::size_t argc, const char* const* argv, InterfaceAccessor launcherInterfaceAccessor) override
	{
		return EngineMain(argc, argv, launcherInterfaceAccessor, true);
	}
};

REGISTER_INTERFACE_SINGLETON(DEDICATED_LAUNCHER_VERSION, IDedicatedLauncher, CDedicatedLauncher);
