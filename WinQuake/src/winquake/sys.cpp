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

#ifdef WIN32
#include "winquake.h"
#include "conproc.h"
#include <VersionHelpers.h>
#else
#include <sys/stat.h>
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

qboolean	ActiveApp, Minimized;

static double starttime = 0.0;
static double curtime = 0.0;
static double lastcurtime = 0.0;

qboolean			isDedicated;
static qboolean		sc_return_on_enter = false;

static char			*tracking_tag = "Clams & Mooses";

#ifdef WIN32
static HANDLE hinput, houtput;
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
int filelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE	*f;
	int		i, retval;

	i = findhandle ();

	f = fopen(path, "rb");

	if (!f)
	{
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*hndl = i;
		retval = filelength(f);
	}

	return retval;
}

int Sys_FileOpenWrite (char *path)
{
	FILE	*f;
	int		i;
	
	i = findhandle ();

	f = fopen(path, "wb");
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
	int		x;

	x = fread (dest, 1, count, sys_handles[handle]);
	return x;
}

int Sys_FileWrite (int handle, const void *data, int count)
{
	int		x;

	x = fwrite (data, 1, count, sys_handles[handle]);
	return x;
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	int		retval;
	
	f = fopen(path, "rb");

	if (f)
	{
		fclose(f);
		retval = 1;
	}
	else
	{
		retval = -1;
	}

	return retval;
}

void Sys_mkdir (char *path)
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


void Sys_Error (char *error, ...)
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

		const double starttime = Sys_FloatTime ();
		sc_return_on_enter = true;	// so Enter will get us out of here

		while (!Sys_ConsoleInput () &&
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

void Sys_Printf (char *fmt, ...)
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

#ifdef WIN32
char *Sys_ConsoleInput (void)
{
	static char	text[256];
	static int		len;
	INPUT_RECORD	recs;
	int		ch;
	DWORD dummy;
	DWORD numread, numevents;

	if (!isDedicated)
		return NULL;


	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, &recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs.EventType == KEY_EVENT)
		{
			if (!recs.Event.KeyEvent.bKeyDown)
			{
				ch = recs.Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);	

						if (len)
						{
							text[len] = 0;
							len = 0;
							return text;
						}
						else if (sc_return_on_enter)
						{
						// special case to allow exiting from the error handler on Enter
							text[0] = '\r';
							len = 0;
							return text;
						}

						break;

					case '\b':
						WriteFile(houtput, "\b \b", 3, &dummy, NULL);	
						if (len)
						{
							len--;
						}
						break;

					default:
						if (ch >= ' ')
						{
							WriteFile(houtput, &ch, 1, &dummy, NULL);	
							text[len] = ch;
							len = (len + 1) & 0xff;
						}

						break;

				}
			}
		}
	}

	return NULL;
}
#endif

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
		scr_skipupdate = 0;

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

		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);

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

// because sound is off until we become active
	S_BlockSound ();

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

			while (time < sys_ticrate.value )
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
				scr_skipupdate = 1;		// no point in bothering to draw
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
#endif
