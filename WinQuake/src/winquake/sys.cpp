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

#ifdef WIN32
#include "winquake.h"
#include "server/conproc.h"
#include <VersionHelpers.h>
#else
#include <signal.h>
#endif

#define SDL_MAIN_HANDLED
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

static const char			*tracking_tag = "Clams & Mooses";

#ifdef WIN32
static HANDLE hFile;
static HANDLE heventParent;
static HANDLE heventChild;
#endif

void Sys_InitFloatTime (void);

volatile int					sys_checksum;

/*
===============================================================================

FILE IO

===============================================================================
*/

#define	MAX_HANDLES		10
FILE	*sys_handles[MAX_HANDLES];

int		findhandle (void)
{
	int		i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
}

/*
================
filelength
================
*/
long filelength (FILE *f)
{
	const long pos = ftell (f);
	fseek (f, 0, SEEK_END);
	const long end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

long Sys_FileOpenRead (const char *path, int *hndl)
{
	const int i = findhandle ();

	if (auto f = fopen(path, "rb"); f)
	{
		sys_handles[i] = f;
		*hndl = i;
		return filelength(f);
	}

	*hndl = -1;
	return -1;
}

int Sys_FileOpenWrite (const char *path)
{
	const int i = findhandle ();

	auto f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, const void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

time_t Sys_FileTime (const char *path)
{
	struct stat buf;

	if (stat(path, &buf) == -1)
	{
		return -1;
	}

	return buf.st_mtime;
}

void Sys_mkdir (const char *path)
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
void Sys_Init (void)
{
	Sys_InitFloatTime ();

#ifdef WIN32
	//Raised OS requirement to Windows 7 or newer because GetVersionEx is deprecated,
	//and the XP toolset support is deprecated in VS2019 and removed entirely in current redistributables
	//Steam dropped support for Windows Vista and this is being developed using the Steam version of Quake
	//so this check avoids problems with older versions of the game
	if (!IsWindows7OrGreater())
	{
		Sys_Error("WinQuake requires at least Windows 7");
	}
#endif
}


void Sys_Error (const char *error, ...)
{
	static bool in_sys_error0 = false;
	static bool in_sys_error1 = false;
	static bool in_sys_error2 = false;
	static bool in_sys_error3 = false;

	if (!in_sys_error3)
	{
		in_sys_error3 = true;
	}

	va_list argptr;
	va_start (argptr, error);
	char text[1024];
	vsprintf (text, error, argptr);
	va_end (argptr);

	if (isDedicated)
	{
		const char* text3 = "Press Enter to exit\n";
		const char* text4 = "***********************************\n";
		const char* text5 = "\n";

		char text2[1024];
		sprintf (text2, "ERROR: %s\n", text);

		fwrite(text5, sizeof(char), strlen (text5), stdout);
		fwrite(text4, sizeof(char), strlen (text4), stdout);
		fwrite(text2, sizeof(char), strlen (text2), stdout);
		fwrite(text3, sizeof(char), strlen (text3), stdout);
		fwrite(text4, sizeof(char), strlen (text4), stdout);

		fflush(stdout);

		const double starttime = Sys_FloatTime ();

		// accept empty input so Enter will get us out of here
		while (!Sys_ConsoleInput (true) &&
				((Sys_FloatTime () - starttime) < CONSOLE_ERROR_TIMEOUT))
		{
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
		Host_Shutdown ();
	}

#ifdef WIN32
// shut down QHOST hooks if necessary
	if (!in_sys_error2)
	{
		in_sys_error2 = true;
		DeinitConProc ();
	}
#endif

	exit(1);
}

void Sys_Printf (const char *fmt, ...)
{
	if (isDedicated)
	{
		va_list argptr;
		va_start (argptr,fmt);
		vfprintf(stdout, fmt, argptr);
		va_end (argptr);
	}
}

void Sys_Quit (void)
{
	Host_Shutdown();

#ifdef WIN32
	if (isDedicated)
		FreeConsole();

// shut down QHOST hooks if necessary
	DeinitConProc ();
#endif

	fflush(stdout);
	exit (0);
}


/*
================
Sys_FloatTime
================
*/
double Sys_FloatTime (void)
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
void Sys_InitFloatTime (void)
{
	if (const int j = COM_CheckParm("-starttime"); j)
	{
		starttime = (double)(Q_atof(com_argv[j + 1]));
	}
	else
	{
		starttime = 0.0;
	}

	Sys_FloatTime ();

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

		if (!fgets(buffer, count, stdin))
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

void Sys_Sleep (void)
{
	std::this_thread::sleep_for(std::chrono::milliseconds{1});
}


void Sys_SendKeyEvents (void)
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
void SleepUntilInput (int time)
{
	SDL_WaitEventTimeout(nullptr, time);
}


/*
==================
WinMain
==================
*/

int EngineMain (int argc, char* argv[])
{
	//TODO: need to rework this so SDL2's setup code is used instead.
	SDL_SetMainReady();

	std::string cwd = std::filesystem::current_path().u8string();

	if (cwd.empty())
		Sys_Error ("Couldn't determine current directory");

	if (cwd.back() == '/')
	{
		cwd.resize(cwd.size() - 1);
	}

	quakeparms_t parms{};
	parms.basedir = cwd.c_str();
	parms.cachedir = NULL;

	COM_InitArgv (argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	isDedicated = (COM_CheckParm ("-dedicated") != 0);

// take 16 Mb, unless they explicitly
// request otherwise
	parms.memsize = MAXIMUM_WIN_MEMORY;

	if (int t = COM_CheckParm ("-heapsize"); t != 0)
	{
		t = COM_CheckParm("-heapsize") + 1;

		if (t < com_argc)
			parms.memsize = Q_atoi (com_argv[t]) * 1024;
	}

	parms.membase = malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error ("Not enough memory free; check disk space\n");

	if (isDedicated)
	{
#ifdef WIN32
		if (!AllocConsole ())
		{
			Sys_Error ("Couldn't create dedicated server console");
		}

		//Since we're allocating the console ourselves in a GUI program
		//we need to re-open these streams so we can use the handles.
		{
			FILE* stream;
			freopen_s(&stream, "CONOUT$", "w", stdout);
			freopen_s(&stream, "CONIN$", "r", stdin);
		}

	// give QHOST a chance to hook into the console
		if (int t = COM_CheckParm("-HFILE"); t > 0)
		{
			if (t < com_argc)
				hFile = (HANDLE)Q_atoi (com_argv[t+1]);
		}
			
		if (int t = COM_CheckParm ("-HPARENT"); t > 0)
		{
			if (t < com_argc)
				heventParent = (HANDLE)Q_atoi (com_argv[t+1]);
		}
			
		if (int t = COM_CheckParm ("-HCHILD"); t > 0)
		{
			if (t < com_argc)
				heventChild = (HANDLE)Q_atoi (com_argv[t+1]);
		}

		InitConProc (hFile, heventParent, heventChild);
#endif
	}

	Sys_Init ();

	Sys_Printf ("Host_Init\n");
	Host_Init (&parms);

	double oldtime = Sys_FloatTime ();

	double time, newtime;

    /* main window message loop */
	while (1)
	{
		if (isDedicated)
		{
			newtime = Sys_FloatTime ();
			time = newtime - oldtime;

			extern int vcrFile;
			extern bool recording;

			while (time < sys_ticrate.value && (vcrFile == -1 || recording))
			{
				Sys_Sleep();
				newtime = Sys_FloatTime ();
				time = newtime - oldtime;
			}
		}
		else
		{
		// yield the CPU for a little while when paused, minimized, or not the focus
			if ((cl.paused && !ActiveApp) || Minimized || block_drawing)
			{
				SleepUntilInput (PAUSE_SLEEP);
				scr_skipupdate = true;		// no point in bothering to draw
			}
			else if (!ActiveApp)
			{
				SleepUntilInput (NOT_FOCUS_SLEEP);
			}

			newtime = Sys_FloatTime ();
			time = newtime - oldtime;
		}

		Host_Frame (time);
		oldtime = newtime;
	}

	SDL_Quit();

    /* return success of application */
    return 0;
}

#ifdef WIN32
char* argv[MAX_NUM_ARGVS];
static char* empty_string = "";

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	HANDLE mutex = CreateMutexA(nullptr, TRUE, "Quake1Mutex");

	if (mutex == nullptr || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		Sys_Error("Could not launch game.\nOnly one instance of this game can be run at a time.");
	}

	int argc = 1;
	argv[0] = empty_string;

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}

	const int result = EngineMain(argc, argv);

	ReleaseMutex(mutex);
	mutex = nullptr;

	//Translate regular main return values to WinMain return values.
	return result != 0 ? 0 : 1;
}
#else
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

int main(int argc, char* argv[])
{
	InitSig();

	const int result = EngineMain(argc, argv);

	return result;
}
#endif
