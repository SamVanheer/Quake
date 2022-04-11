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
// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"
#include <commctrl.h>

#include <thread>

#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_version.h>
#include <SDL_video.h>

#define MAX_MODE_LIST	30
#define VID_ROW_SIZE	3
#define WARP_WIDTH		320
#define WARP_HEIGHT		200
#define MAXWIDTH		10000
#define MAXHEIGHT		10000
#define BASEWIDTH		320
#define BASEHEIGHT		200

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define MODE_FULLSCREEN_DEFAULT	(MODE_WINDOWED + 1)

typedef struct {
	modestate_t	type;
	int			width;
	int			height;
	int			modenum;
	int			dib;
	int			fullscreen;
	int			bpp;
	int			halfscreen;
	char		modedesc[17];
} vmode_t;

typedef struct {
	int			width;
	int			height;
} lmode_t;

lmode_t	lowresmodes[] = {
	{320, 200},
	{320, 240},
	{400, 300},
	{512, 384},
};

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

qboolean		DDActive;
qboolean		scr_skipupdate;

static vmode_t	modelist[MAX_MODE_LIST];
static int		nummodes;
static vmode_t	*pcurrentmode;
static vmode_t	badmode;

static qboolean	vid_initialized = false;
static qboolean	windowed, leavecurrentmode;
static qboolean vid_canalttab = false;
static qboolean vid_wassuspended = false;
static int		windowed_mouse;
extern qboolean	mouseactive;  // from in_win.c
//TODO: replace
//static HICON	hIcon;

int			DIBWidth, DIBHeight;
RECT		WindowRect;

SDL_Window*	mainwindow, * dibwindow;

int			vid_modenum = NO_MODE;
int			vid_realmode;
int			vid_default = MODE_WINDOWED;
static int	windowed_default;
unsigned char	vid_curpal[256*3];
static qboolean fullsbardraw = false;

static float vid_gamma = 1.0;

SDL_GLContext GLContext = nullptr;

glvert_t glv;

cvar_t	gl_ztrick = {"gl_ztrick","1"};

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];
unsigned char d_15to8table[65536];

float		gldepthmin, gldepthmax;

modestate_t	modestate = MS_UNINIT;

void VID_MenuDraw (void);
void VID_MenuKey (int key);

void AppActivate(bool fActive, bool minimize);
char *VID_GetModeDescription (int mode);
void ClearAllStates (void);
void VID_UpdateWindowStatus (void);
void GL_Init (void);

void* glArrayElementEXT;
void* glColorPointerEXT;
void* glTexCoordPointerEXT;
void* glVertexPointerEXT;

qboolean is8bit = false;
qboolean isPermedia = false;
qboolean gl_mtexable = false;

//====================================

cvar_t		vid_mode = {"vid_mode","0", false};
// Note that 0 is MODE_WINDOWED
cvar_t		_vid_default_mode = {"_vid_default_mode","0", true};
// Note that 3 is MODE_FULLSCREEN_DEFAULT
cvar_t		_vid_default_mode_win = {"_vid_default_mode_win","3", true};
cvar_t		vid_wait = {"vid_wait","0"};
cvar_t		vid_nopageflip = {"vid_nopageflip","0", true};
cvar_t		_vid_wait_override = {"_vid_wait_override", "0", true};
cvar_t		vid_config_x = {"vid_config_x","800", true};
cvar_t		vid_config_y = {"vid_config_y","600", true};
cvar_t		vid_stretch_by_2 = {"vid_stretch_by_2","1", true};
cvar_t		_windowed_mouse = {"_windowed_mouse","1", true};

int			window_center_x, window_center_y, window_x, window_y, window_width, window_height;
RECT		window_rect;

HWND VID_GetWindowHandle()
{
	SDL_SysWMinfo wmInfo{};
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(mainwindow, &wmInfo);

	return wmInfo.info.win.window;
}

// direct draw software compatability stuff

void VID_HandlePause (qboolean pause)
{
}

void VID_ForceLockState (int lk)
{
}

void VID_LockBuffer (void)
{
}

void VID_UnlockBuffer (void)
{
}

int VID_ForceUnlockedAndReturnState (void)
{
	return 0;
}

void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}


void CenterWindow(SDL_Window* hWndCenter)
{
	SDL_SetWindowPosition(hWndCenter, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

static void ClearWindowToBlack(SDL_Window* window)
{
	// because we have set the background brush for the window to NULL
	// (to avoid flickering when re-sizing the window on the desktop),
	// we clear the window to black when created, otherwise it will be
	// empty while Quake starts up.
	auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

	if (!renderer)
	{
		Sys_Error("Couldn't create renderer to clear window: %s", SDL_GetError());
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	SDL_DestroyRenderer(renderer);
}

qboolean VID_CreateWindow(int modenum, bool windowed)
{
	modestate = windowed ? MS_WINDOWED : MS_FULLDIB;

	WindowRect.top = WindowRect.left = 0;

	WindowRect.right = modelist[modenum].width;
	WindowRect.bottom = modelist[modenum].height;

	DIBWidth = modelist[modenum].width;
	DIBHeight = modelist[modenum].height;

	RECT rect = WindowRect;

	const int width = rect.right - rect.left;
	const int height = rect.bottom - rect.top;

	// Create the DIB window
	Uint32 flags = SDL_WINDOW_OPENGL;

	if (!windowed)
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	dibwindow = SDL_CreateWindow("GLQuake", rect.left, rect.top, width, height, flags);

	if (!dibwindow)
		Sys_Error("Couldn't create DIB window");

	if (windowed)
	{
		CenterWindow(dibwindow);
	}

	SDL_ShowWindow(dibwindow);

	ClearWindowToBlack(dibwindow);

	if (vid.conheight > static_cast<unsigned int>(modelist[modenum].height))
		vid.conheight = modelist[modenum].height;
	if (vid.conwidth > static_cast<unsigned int>(modelist[modenum].width))
		vid.conwidth = modelist[modenum].width;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.numpages = 2;

	if (!windowed)
	{
		// needed because we're not getting WM_MOVE messages fullscreen on NT
		window_x = 0;
		window_y = 0;
	}

	mainwindow = dibwindow;

	//TODO: load icon from disk, set on window
	/*
	SendMessage (mainwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
	SendMessage (mainwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);
	*/

	return true;
}

int VID_SetMode (int modenum, unsigned char *palette)
{
	int				original_mode, temp;
	qboolean		stat;

	if ((windowed && (modenum != 0)) ||
		(!windowed && (modenum < 1)) ||
		(!windowed && (modenum >= nummodes)))
	{
		Sys_Error ("Bad video mode\n");
	}

// so Con_Printfs don't mess us up by forcing vid and snd updates
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	CDAudio_Pause ();

	if (vid_modenum == NO_MODE)
		original_mode = windowed_default;
	else
		original_mode = vid_modenum;

	// Set either the fullscreen or windowed mode
	if (modelist[modenum].type == MS_WINDOWED)
	{
		if (_windowed_mouse.value && key_dest == key_game)
		{
			stat = VID_CreateWindow(modenum, true);
			IN_ActivateMouse ();
			IN_HideMouse ();
		}
		else
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
			stat = VID_CreateWindow(modenum, true);
		}
	}
	else if (modelist[modenum].type == MS_FULLDIB)
	{
		stat = VID_CreateWindow(modenum, false);
		IN_ActivateMouse ();
		IN_HideMouse ();
	}
	else
	{
		Sys_Error ("VID_SetMode: Bad mode type in modelist");
	}

	window_width = DIBWidth;
	window_height = DIBHeight;
	VID_UpdateWindowStatus ();

	CDAudio_Resume ();
	scr_disabled_for_loading = temp;

	if (!stat)
	{
		Sys_Error ("Couldn't set video mode");
	}

// now we try to make sure we get the focus on the mode switch, because
// sometimes in some systems we don't.  We grab the foreground, then
// finish setting up, pump all our messages, and sleep for a little while
// to let messages finish bouncing around the system, then we put
// ourselves at the top of the z order, then grab the foreground again,
// Who knows if it helps, but it probably doesn't hurt
	SDL_RaiseWindow (mainwindow);
	VID_SetPalette (palette);
	vid_modenum = modenum;
	Cvar_SetValue ("vid_mode", (float)vid_modenum);

	Sys_SendKeyEvents();

	std::this_thread::sleep_for(std::chrono::milliseconds{100});

	//SDL_SetWindowPosition(mainwindow, 0, 0);

	SDL_RaiseWindow(mainwindow);

// fix the leftover Alt from any Alt-Tab or the like that switched us away
	ClearAllStates ();

	if (!msg_suppress_1)
		Con_SafePrintf ("Video mode %s initialized.\n", VID_GetModeDescription (vid_modenum));

	VID_SetPalette (palette);

	vid.recalc_refdef = 1;

	return true;
}


/*
================
VID_UpdateWindowStatus
================
*/
void VID_UpdateWindowStatus (void)
{

	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	IN_UpdateClipCursor ();
}


//====================================

BINDTEXFUNCPTR bindTexFunc;

#define TEXTURE_EXT_STRING "GL_EXT_texture_object"


void CheckTextureExtensions (void)
{
	char		*tmp;
	HINSTANCE	hInstGL;

	bool texture_ext = false;
	/* check for texture extension */
	tmp = (char *)glGetString(GL_EXTENSIONS);
	while (*tmp)
	{
		if (strncmp((const char*)tmp, TEXTURE_EXT_STRING, strlen(TEXTURE_EXT_STRING)) == 0)
			texture_ext = true;
		tmp++;
	}

	if (!texture_ext || COM_CheckParm ("-gl11") )
	{
		hInstGL = LoadLibrary("opengl32.dll");
		
		if (hInstGL == NULL)
			Sys_Error ("Couldn't load opengl32.dll\n");

		bindTexFunc = reinterpret_cast<decltype( bindTexFunc )>( GetProcAddress(hInstGL,"glBindTexture") );

		if (!bindTexFunc)
			Sys_Error ("No texture objects!");
		return;
	}

/* load library and get procedure adresses for texture extension API */
	if ((bindTexFunc = (BINDTEXFUNCPTR)
		SDL_GL_GetProcAddress((LPCSTR) "glBindTextureEXT")) == NULL)
	{
		Sys_Error ("GetProcAddress for BindTextureEXT failed");
		return;
	}
}

void CheckArrayExtensions (void)
{
	char		*tmp;

	/* check for texture extension */
	tmp = (char *)glGetString(GL_EXTENSIONS);
	while (*tmp)
	{
		if (strncmp((const char*)tmp, "GL_EXT_vertex_array", strlen("GL_EXT_vertex_array")) == 0)
		{
			if (
((glArrayElementEXT = SDL_GL_GetProcAddress("glArrayElementEXT")) == NULL) ||
((glColorPointerEXT = SDL_GL_GetProcAddress("glColorPointerEXT")) == NULL) ||
((glTexCoordPointerEXT = SDL_GL_GetProcAddress("glTexCoordPointerEXT")) == NULL) ||
((glVertexPointerEXT = SDL_GL_GetProcAddress("glVertexPointerEXT")) == NULL) )
			{
				Sys_Error ("GetProcAddress for vertex extension failed");
				return;
			}
			return;
		}
		tmp++;
	}

	Sys_Error ("Vertex array extension not present");
}

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

#ifdef _WIN32
void CheckMultiTextureExtensions(void) 
{
	if (strstr(gl_extensions, "GL_SGIS_multitexture ") && !COM_CheckParm("-nomtex")) {
		Con_Printf("Multitexture extensions found.\n");
		qglMTexCoord2fSGIS = reinterpret_cast<decltype( qglMTexCoord2fSGIS )>(SDL_GL_GetProcAddress("glMTexCoord2fSGIS") );
		qglSelectTextureSGIS = reinterpret_cast<decltype( qglSelectTextureSGIS )>(SDL_GL_GetProcAddress("glSelectTextureSGIS") );
		gl_mtexable = true;
	}
}
#else
void CheckMultiTextureExtensions(void) 
{
		gl_mtexable = true;
}
#endif

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	gl_vendor = reinterpret_cast<const char*>( glGetString (GL_VENDOR) );
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = reinterpret_cast<const char*>( glGetString (GL_RENDERER) );
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = reinterpret_cast<const char*>( glGetString (GL_VERSION) );
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = reinterpret_cast<const char*>( glGetString (GL_EXTENSIONS) );
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

//	Con_Printf ("%s %s\n", gl_renderer, gl_version);

    if (strnicmp(gl_renderer,"PowerVR",7)==0)
         fullsbardraw = true;

    if (strnicmp(gl_renderer,"Permedia",8)==0)
         isPermedia = true;

	CheckTextureExtensions ();
	CheckMultiTextureExtensions ();

	glClearColor (1,0,0,0);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666f);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

#if 0
	CheckArrayExtensions ();

	glEnable (GL_VERTEX_ARRAY_EXT);
	glEnable (GL_TEXTURE_COORD_ARRAY_EXT);
	glVertexPointerEXT (3, GL_FLOAT, 0, 0, &glv.x);
	glTexCoordPointerEXT (2, GL_FLOAT, 0, 0, &glv.s);
	glColorPointerEXT (3, GL_FLOAT, 0, 0, &glv.r);
#endif
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	extern cvar_t gl_clear;

	*x = *y = 0;
	*width = WindowRect.right - WindowRect.left;
	*height = WindowRect.bottom - WindowRect.top;

//    if (!SDL_GL_MakeCurrent( mainwindow, GLContext ))
//		Sys_Error ("SDL_GL_MakeCurrent failed");

//	glViewport (*x, *y, *width, *height);
}


void GL_EndRendering (void)
{
	if (!scr_skipupdate || block_drawing)
		SDL_GL_SwapWindow(mainwindow);

// handle the mouse state when windowed if that's changed
	if (modestate == MS_WINDOWED)
	{
		if (!_windowed_mouse.value) {
			if (windowed_mouse)	{
				IN_DeactivateMouse ();
				IN_ShowMouse ();
				windowed_mouse = false;
			}
		} else {
			windowed_mouse = true;
			if (key_dest == key_game && !mouseactive && ActiveApp) {
				IN_ActivateMouse ();
				IN_HideMouse ();
			} else if (mouseactive && key_dest != key_game) {
				IN_DeactivateMouse ();
				IN_ShowMouse ();
			}
		}
	}
	if (fullsbardraw)
		Sbar_Changed();
}

void	VID_SetPalette (unsigned char *palette)
{
	byte	*pal;
	unsigned r,g,b;
	unsigned v;
	int     r1,g1,b1;
	int		j,k,l;
	unsigned short i;
	unsigned	*table;

//
// 8 8 8 encoding
//
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		
//		v = (255<<24) + (r<<16) + (g<<8) + (b<<0);
//		v = (255<<0) + (r<<8) + (g<<16) + (b<<24);
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
	}
	d_8to24table[255] &= 0xffffff;	// 255 is transparent

	// JACK: 3D distance calcs - k is last closest, l is the distance.
	// FIXME: Precalculate this and cache to disk.
	for (i=0; i < (1<<15); i++) {
		/* Maps
			000000000000000
			000000000011111 = Red  = 0x1F
			000001111100000 = Blue = 0x03E0
			111110000000000 = Grn  = 0x7C00
		*/
		r = ((i & 0x1F) << 3)+4;
		g = ((i & 0x03E0) >> 2)+4;
		b = ((i & 0x7C00) >> 7)+4;
		pal = (unsigned char *)d_8to24table;
		for (v=0,k=0,l=10000*10000; v<256; v++,pal+=4) {
			r1 = r-pal[0];
			g1 = g-pal[1];
			b1 = b-pal[2];
			j = (r1*r1)+(g1*g1)+(b1*b1);
			if (j<l) {
				k=v;
				l=j;
			}
		}
		d_15to8table[i]=k;
	}
}

bool	gammaworks;

void	VID_ShiftPalette (unsigned char *palette)
{
	extern	byte ramps[3][256];
	
//	VID_SetPalette (palette);

//	gammaworks = SetDeviceGammaRamp (maindc, ramps) != FALSE;
}


void VID_SetDefaultMode (void)
{
	IN_DeactivateMouse ();
}


void	VID_Shutdown (void)
{
	if (vid_initialized)
	{
		vid_canalttab = false;

		SDL_GL_MakeCurrent(nullptr, nullptr);

		if (GLContext)
		{
			SDL_GL_DeleteContext(GLContext);
			GLContext = nullptr;
		}

		AppActivate(false, false);
	}
}


//==========================================================================

/*
=======
MapKey

Map from windows to quake keynums
=======
*/
int MapKey (SDL_Keycode key)
{
	if (key >= SDLK_a && key <= SDLK_z)
	{
		return 'a' + (key - SDLK_a);
	}

	if (key >= SDLK_F1 && key <= SDLK_F12)
	{
		return K_F1 + (key - SDLK_F1);
	}

	switch (key)
	{
	case SDLK_UNKNOWN: return 0;
	case SDL_KeyCode::SDLK_ESCAPE: return 27;

	case SDLK_1: return '1';
	case SDLK_2: return '2';
	case SDLK_3: return '3';
	case SDLK_4: return '4';
	case SDLK_5: return '5';
	case SDLK_6: return '6';
	case SDLK_7: return '7';
	case SDLK_8: return '8';
	case SDLK_9: return '9';
	case SDLK_0: return '0';

	case SDLK_MINUS: return '-';
	case SDLK_EQUALS: return '=';
	case SDLK_BACKSPACE: return K_BACKSPACE;

		//case SDLK_: return 9; //No horizontal tab key in SDL2.

	case SDLK_RETURN: return '\r';

	case SDLK_LCTRL: return K_CTRL;
	case SDLK_RCTRL: return K_CTRL;

	case SDLK_LSHIFT: return K_SHIFT;
	case SDLK_RSHIFT: return K_SHIFT;

	case SDLK_LALT: return K_ALT;
	case SDLK_RALT: return K_ALT;

	case SDLK_SEMICOLON: return ';';
	case SDLK_COMMA: return ',';
	case SDLK_PERIOD: return '.';

	case SDLK_QUOTE: return '\'';
	case SDLK_BACKQUOTE: return '`';
	case SDLK_BACKSLASH: return '\\';
	case SDLK_SLASH: return '/';

	case SDLK_KP_MULTIPLY: return '*';
	case SDLK_PLUS: return '+';

	case SDLK_SPACE: return ' ';

	case SDLK_INSERT: return K_INS;
	case SDLK_DELETE: return K_DEL;

	case SDLK_HOME: return K_HOME;
	case SDLK_END: return K_END;

	case SDLK_PAGEUP: return K_PGUP;
	case SDLK_PAGEDOWN: return K_PGDN;

	case SDLK_PAUSE: return K_PAUSE;

	case SDLK_UP: return K_UPARROW;
	case SDLK_DOWN: return K_DOWNARROW;
	case SDLK_RIGHT: return K_RIGHTARROW;
	case SDLK_LEFT: return K_LEFTARROW;

	default:
	{
		Con_DPrintf("key 0x%02x has no translation\n", key);
		return 0;
	}
	}
}

/*
===================================================================

MAIN WINDOW

===================================================================
*/

/*
================
ClearAllStates
================
*/
void ClearAllStates (void)
{
	int		i;
	
// send an up event for each key, to make sure the server clears them all
	for (i=0 ; i<256 ; i++)
	{
		Key_Event (i, false);
	}

	Key_ClearStates ();
	IN_ClearStates ();
}

void AppActivate(bool fActive, bool minimize)
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
{
	static bool	sound_active;

	ActiveApp = fActive;
	Minimized = minimize;

// enable/disable sound on focus gain/loss
	if (!ActiveApp && sound_active)
	{
		S_BlockSound ();
		sound_active = false;
	}
	else if (ActiveApp && !sound_active)
	{
		S_UnblockSound ();
		sound_active = true;
	}

	if (fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			IN_ActivateMouse ();
			IN_HideMouse ();
			if (vid_canalttab && vid_wassuspended) {
				vid_wassuspended = false;
				SDL_ShowWindow(mainwindow);
			}
		}
		else if ((modestate == MS_WINDOWED) && _windowed_mouse.value && key_dest == key_game)
		{
			IN_ActivateMouse ();
			IN_HideMouse ();
		}
	}

	if (!fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
			if (vid_canalttab) { 
				vid_wassuspended = true;
			}
		}
		else if ((modestate == MS_WINDOWED) && _windowed_mouse.value)
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
		}
	}
}

void VID_ProcessEvent(SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_WINDOWEVENT:
	{
		switch (event.window.event)
		{
		case SDL_WINDOWEVENT_MOVED:
		{
			window_x = event.window.data1;
			window_y = event.window.data2;
			VID_UpdateWindowStatus();
			break;
		}

		case SDL_WINDOWEVENT_RESIZED:
			break;

		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
		{
			const bool fActive = event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED;
			const bool fMinimized = false;// (BOOL)HIWORD(wParam); //TODO: figure out if this is even needed.

			if (!fActive)
				SDL_MinimizeWindow(mainwindow);

			AppActivate(fActive, fMinimized);

			// fix the leftover Alt from any Alt-Tab or the like that switched us away
			ClearAllStates();

			break;
		}

		case SDL_WINDOWEVENT_CLOSE:
		{
			constexpr int YesId = 1;
			constexpr int NoId = 0;

			const SDL_MessageBoxButtonData buttons[2] =
			{
				{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, NoId, "No"},
				{0, YesId, "Yes"}
			};

			const SDL_MessageBoxData data
			{
				SDL_MESSAGEBOX_INFORMATION,
				mainwindow,
				"Confirm Exit",
				"Are you sure you want to quit?",
				sizeof(buttons) / sizeof(buttons[0]),
				buttons,
				nullptr
			};

			int buttonId;

			if (SDL_ShowMessageBox(&data, &buttonId) < 0)
			{
				Sys_Error("Error displaying message box: %s", SDL_GetError());
			}

			if (buttonId == YesId)
			{
				Sys_Quit();
			}

			break;
		}
		}
	}

	case SDL_KEYDOWN:
		Key_Event(MapKey(event.key.keysym.sym), true);
		break;

	case SDL_KEYUP:
		Key_Event(MapKey(event.key.keysym.sym), false);
		break;

		// this is complicated because Win32 seems to pack multiple mouse events into
		// one update sometimes, so we always check all states and look for events
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEMOTION:
	{
		int temp = 0;

		if (event.button.button == SDL_BUTTON_LEFT)
			temp |= 1;

		if (event.button.button == SDL_BUTTON_RIGHT)
			temp |= 2;

		if (event.button.button == SDL_BUTTON_MIDDLE)
			temp |= 4;

		IN_MouseEvent(temp);

		break;
	}

		// JACK: This is the mouse wheel with the Intellimouse
		// Its delta is either positive or neg, and we generate the proper
		// Event.
	case SDL_MOUSEWHEEL:
		if (event.wheel.y > 0)
		{
			Key_Event(K_MWHEELUP, true);
			Key_Event(K_MWHEELUP, false);
		}
		else
		{
			Key_Event(K_MWHEELDOWN, true);
			Key_Event(K_MWHEELDOWN, false);
		}
		break;

	//TODO
	/*
	case MM_MCINOTIFY:
		lRet = CDAudio_MessageHandler(hWnd, uMsg, wParam, lParam);
		break;
		*/
	}
}


/*
=================
VID_NumModes
=================
*/
int VID_NumModes (void)
{
	return nummodes;
}

	
/*
=================
VID_GetModePtr
=================
*/
vmode_t *VID_GetModePtr (int modenum)
{

	if ((modenum >= 0) && (modenum < nummodes))
		return &modelist[modenum];
	else
		return &badmode;
}


/*
=================
VID_GetModeDescription
=================
*/
char *VID_GetModeDescription (int mode)
{
	char		*pinfo;
	vmode_t		*pv;
	static char	temp[100];

	if ((mode < 0) || (mode >= nummodes))
		return NULL;

	if (!leavecurrentmode)
	{
		pv = VID_GetModePtr (mode);
		pinfo = pv->modedesc;
	}
	else
	{
		sprintf (temp, "Desktop resolution (%dx%d)",
				 modelist[MODE_FULLSCREEN_DEFAULT].width,
				 modelist[MODE_FULLSCREEN_DEFAULT].height);
		pinfo = temp;
	}

	return pinfo;
}


// KJB: Added this to return the mode driver name in description for console

char *VID_GetExtModeDescription (int mode)
{
	static char	pinfo[40];
	vmode_t		*pv;

	if ((mode < 0) || (mode >= nummodes))
		return NULL;

	pv = VID_GetModePtr (mode);
	if (modelist[mode].type == MS_FULLDIB)
	{
		if (!leavecurrentmode)
		{
			sprintf(pinfo,"%s fullscreen", pv->modedesc);
		}
		else
		{
			sprintf (pinfo, "Desktop resolution (%dx%d)",
					 modelist[MODE_FULLSCREEN_DEFAULT].width,
					 modelist[MODE_FULLSCREEN_DEFAULT].height);
		}
	}
	else
	{
		if (modestate == MS_WINDOWED)
			sprintf(pinfo, "%s windowed", pv->modedesc);
		else
			sprintf(pinfo, "windowed");
	}

	return pinfo;
}


/*
=================
VID_DescribeCurrentMode_f
=================
*/
void VID_DescribeCurrentMode_f (void)
{
	Con_Printf ("%s\n", VID_GetExtModeDescription (vid_modenum));
}


/*
=================
VID_NumModes_f
=================
*/
void VID_NumModes_f (void)
{

	if (nummodes == 1)
		Con_Printf ("%d video mode is available\n", nummodes);
	else
		Con_Printf ("%d video modes are available\n", nummodes);
}


/*
=================
VID_DescribeMode_f
=================
*/
void VID_DescribeMode_f (void)
{
	int		t, modenum;
	
	modenum = Q_atoi (Cmd_Argv(1));

	t = leavecurrentmode;
	leavecurrentmode = 0;

	Con_Printf ("%s\n", VID_GetExtModeDescription (modenum));

	leavecurrentmode = t;
}


/*
=================
VID_DescribeModes_f
=================
*/
void VID_DescribeModes_f (void)
{
	int			i, lnummodes, t;
	char		*pinfo;
	vmode_t		*pv;

	lnummodes = VID_NumModes ();

	t = leavecurrentmode;
	leavecurrentmode = 0;

	for (i=1 ; i<lnummodes ; i++)
	{
		pv = VID_GetModePtr (i);
		pinfo = VID_GetExtModeDescription (i);
		Con_Printf ("%2d: %s\n", i, pinfo);
	}

	leavecurrentmode = t;
}


void VID_InitDIB (HINSTANCE hInstance)
{
	modelist[0].type = MS_WINDOWED;

	if (COM_CheckParm("-width"))
		modelist[0].width = Q_atoi(com_argv[COM_CheckParm("-width")+1]);
	else
		modelist[0].width = 640 * 2; //Double the windowed mode size

	if (modelist[0].width < 320)
		modelist[0].width = 320;

	if (COM_CheckParm("-height"))
		modelist[0].height= Q_atoi(com_argv[COM_CheckParm("-height")+1]);
	else
		modelist[0].height = modelist[0].width * 240/320;

	if (modelist[0].height < 240)
		modelist[0].height = 240;

	sprintf (modelist[0].modedesc, "%dx%d",
			 modelist[0].width, modelist[0].height);

	modelist[0].modenum = MODE_WINDOWED;
	modelist[0].dib = 1;
	modelist[0].fullscreen = 0;
	modelist[0].halfscreen = 0;
	modelist[0].bpp = 0;

	nummodes = 1;

	//Configure OpenGL parameters.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}


/*
=================
VID_InitFullDIB
=================
*/
void VID_InitFullDIB (HINSTANCE hInstance)
{
// enumerate >8 bpp modes
	const int originalnummodes = nummodes;

	//TODO: support multiple displays.
	const int candidateCount = SDL_GetNumDisplayModes(0);

	for (int modenum = 0; modenum < candidateCount; ++modenum)
	{
		SDL_DisplayMode devmode;

		if (SDL_GetDisplayMode(0, modenum, &devmode) < 0)
		{
			Sys_Error("Error getting display mode: %s", SDL_GetError());
		}

		const int bpp = SDL_BITSPERPIXEL(devmode.format);

		if ((bpp >= 15) &&
			(devmode.w <= MAXWIDTH) &&
			(devmode.h <= MAXHEIGHT) &&
			(nummodes < MAX_MODE_LIST))
		{
			modelist[nummodes].type = MS_FULLDIB;
			modelist[nummodes].width = devmode.w;
			modelist[nummodes].height = devmode.h;
			modelist[nummodes].modenum = 0;
			modelist[nummodes].halfscreen = 0;
			modelist[nummodes].dib = 1;
			modelist[nummodes].fullscreen = 1;
			modelist[nummodes].bpp = bpp;
			sprintf(modelist[nummodes].modedesc, "%dx%dx%d", devmode.w, devmode.h, bpp);

			// if the width is more than twice the height, reduce it by half because this
			// is probably a dual-screen monitor
			if (!COM_CheckParm("-noadjustaspect"))
			{
				if (modelist[nummodes].width > (modelist[nummodes].height << 1))
				{
					modelist[nummodes].width >>= 1;
					modelist[nummodes].halfscreen = 1;
					sprintf(modelist[nummodes].modedesc, "%dx%dx%d",
						modelist[nummodes].width,
						modelist[nummodes].height,
						modelist[nummodes].bpp);
				}
			}

			bool existingmode = false;

			for (int i = originalnummodes; i < nummodes; i++)
			{
				if ((modelist[nummodes].width == modelist[i].width) &&
					(modelist[nummodes].height == modelist[i].height) &&
					(modelist[nummodes].bpp == modelist[i].bpp))
				{
					existingmode = true;
					break;
				}
			}

			if (!existingmode)
			{
				nummodes++;
			}
		}
	}

	//Low-res mode search removed because SDL2 does not provide APIs to test for them.

	if (nummodes == originalnummodes)
		Con_SafePrintf ("No fullscreen DIB modes found\n");
}

qboolean VID_Is8bit() {
	return is8bit;
}

static void Check_Gamma (unsigned char *pal)
{
	float	f, inf;
	unsigned char	palette[768];
	int		i;

	if ((i = COM_CheckParm("-gamma")) == 0) {
		if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
			(gl_vendor && strstr(gl_vendor, "3Dfx")))
			vid_gamma = 1;
		else
			vid_gamma = 0.7f; // default to 0.7 on non-3dfx hardware
	} else
		vid_gamma = Q_atof(com_argv[i+1]);

	for (i=0 ; i<768 ; i++)
	{
		f = pow ( (pal[i]+1)/256.0 , vid_gamma );
		inf = f*255 + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		palette[i] = inf;
	}

	memcpy (pal, palette, sizeof(palette));
}

/*
===================
VID_Init
===================
*/
void	VID_Init (unsigned char *palette)
{
	int		i;
	int		basenummodes, width, height, bpp, findbpp;
	char	gldir[MAX_OSPATH];

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
	{
		Sys_Error("Couldn't initialize video subsystem: %s", SDL_GetError());
	}

	Cvar_RegisterVariable (&vid_mode);
	Cvar_RegisterVariable (&vid_wait);
	Cvar_RegisterVariable (&vid_nopageflip);
	Cvar_RegisterVariable (&_vid_wait_override);
	Cvar_RegisterVariable (&_vid_default_mode);
	Cvar_RegisterVariable (&_vid_default_mode_win);
	Cvar_RegisterVariable (&vid_config_x);
	Cvar_RegisterVariable (&vid_config_y);
	Cvar_RegisterVariable (&vid_stretch_by_2);
	Cvar_RegisterVariable (&_windowed_mouse);
	Cvar_RegisterVariable (&gl_ztrick);

	Cmd_AddCommand ("vid_nummodes", VID_NumModes_f);
	Cmd_AddCommand ("vid_describecurrentmode", VID_DescribeCurrentMode_f);
	Cmd_AddCommand ("vid_describemode", VID_DescribeMode_f);
	Cmd_AddCommand ("vid_describemodes", VID_DescribeModes_f);

	//TODO: replace
	//hIcon = LoadIcon (global_hInstance, MAKEINTRESOURCE (IDI_ICON2));

	VID_InitDIB (global_hInstance);
	basenummodes = nummodes = 1;

	VID_InitFullDIB (global_hInstance);

	if (COM_CheckParm("-window"))
	{
		windowed = true;

		vid_default = MODE_WINDOWED;
	}
	else
	{
		if (nummodes == 1)
			Sys_Error ("No RGB fullscreen modes available");

		windowed = false;

		if (COM_CheckParm("-mode"))
		{
			vid_default = Q_atoi(com_argv[COM_CheckParm("-mode")+1]);
		}
		else
		{
			if (COM_CheckParm("-current"))
			{
				SDL_Rect rect;

				if (SDL_GetDisplayBounds(0, &rect) < 0)
				{
					Sys_Error("Couldn't get display bounds: %s", SDL_GetError());
				}

				modelist[MODE_FULLSCREEN_DEFAULT].width = rect.w - rect.x;
				modelist[MODE_FULLSCREEN_DEFAULT].height = rect.h - rect.y;
				vid_default = MODE_FULLSCREEN_DEFAULT;

				leavecurrentmode = 1;
			}
			else
			{
				if (COM_CheckParm("-width"))
				{
					width = Q_atoi(com_argv[COM_CheckParm("-width")+1]);
				}
				else
				{
					width = 640;
				}

				if (COM_CheckParm("-bpp"))
				{
					bpp = Q_atoi(com_argv[COM_CheckParm("-bpp")+1]);
					findbpp = 0;
				}
				else
				{
					bpp = 15;
					findbpp = 1;
				}

				if (COM_CheckParm("-height"))
				{
					height = Q_atoi(com_argv[COM_CheckParm("-height") + 1]);
				}
				else
				{
					height = 480;
				}

			// if they want to force it, add the specified mode to the list
				if (COM_CheckParm("-force") && (nummodes < MAX_MODE_LIST))
				{
					modelist[nummodes].type = MS_FULLDIB;
					modelist[nummodes].width = width;
					modelist[nummodes].height = height;
					modelist[nummodes].modenum = 0;
					modelist[nummodes].halfscreen = 0;
					modelist[nummodes].dib = 1;
					modelist[nummodes].fullscreen = 1;
					modelist[nummodes].bpp = bpp;
					//TODO: the devmode variable was always zero initialized so the parameters are also all 0.
					sprintf (modelist[nummodes].modedesc, "%dx%dx%d", 0, 0, 0);

					bool existingmode = false;

					for (i=nummodes; i<nummodes ; i++)
					{
						if ((modelist[nummodes].width == modelist[i].width)   &&
							(modelist[nummodes].height == modelist[i].height) &&
							(modelist[nummodes].bpp == modelist[i].bpp))
						{
							existingmode = true;
							break;
						}
					}

					if (!existingmode)
					{
						nummodes++;
					}
				}

				bool done = false;

				do
				{
					if (COM_CheckParm("-height"))
					{
						height = Q_atoi(com_argv[COM_CheckParm("-height")+1]);

						for (i=1, vid_default=0 ; i<nummodes ; i++)
						{
							if ((modelist[i].width == width) &&
								(modelist[i].height == height) &&
								(modelist[i].bpp == bpp))
							{
								vid_default = i;
								done = true;
								break;
							}
						}
					}
					else
					{
						for (i=1, vid_default=0 ; i<nummodes ; i++)
						{
							if ((modelist[i].width == width) && (modelist[i].bpp == bpp))
							{
								vid_default = i;
								done = true;
								break;
							}
						}
					}

					if (!done)
					{
						if (findbpp)
						{
							switch (bpp)
							{
							case 15:
								bpp = 16;
								break;
							case 16:
								bpp = 32;
								break;
							case 32:
								bpp = 24;
								break;
							case 24:
								done = true;
								break;
							}
						}
						else
						{
							done = true;
						}
					}
				} while (!done);

				if (!vid_default)
				{
					Sys_Error ("Specified video mode not available");
				}
			}
		}
	}

	vid_initialized = true;

	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = Q_atoi(com_argv[i+1]);
	else
		vid.conwidth = 640;

	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth*3 / 4;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.conheight = Q_atoi(com_argv[i+1]);
	if (vid.conheight < 200)
		vid.conheight = 200;

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	Check_Gamma(palette);
	VID_SetPalette (palette);

	VID_SetMode (vid_default, palette);

	GLContext = SDL_GL_CreateContext(mainwindow);

	if (!GLContext)
		Sys_Error (
			"Could not initialize GL (SDL_GL_CreateContext failed).\n\nMake sure you in are 16 bit color mode, and try running -window.\n%s",
			SDL_GetError());

    if (SDL_GL_MakeCurrent(mainwindow, GLContext) < 0)
		Sys_Error ("SDL_GL_MakeCurrent failed: %s", SDL_GetError());

	GL_Init ();

	sprintf (gldir, "%s/glquake", com_gamedir);
	Sys_mkdir (gldir);

	vid_realmode = vid_modenum;

	vid_menudrawfn = VID_MenuDraw;
	vid_menukeyfn = VID_MenuKey;

	strcpy (badmode.modedesc, "Bad mode");
	vid_canalttab = true;

	if (COM_CheckParm("-fullsbar"))
		fullsbardraw = true;
}


//========================================================
// Video menu stuff
//========================================================

extern void M_Menu_Options_f (void);
extern void M_Print (int cx, int cy, char *str);
extern void M_PrintWhite (int cx, int cy, char *str);
extern void M_DrawCharacter (int cx, int line, int num);
extern void M_DrawTransPic (int x, int y, qpic_t *pic);
extern void M_DrawPic (int x, int y, qpic_t *pic);

static int	vid_line, vid_wmodes;

typedef struct
{
	int		modenum;
	char	*desc;
	int		iscur;
} modedesc_t;

#define MAX_COLUMN_SIZE		9
#define MODE_AREA_HEIGHT	(MAX_COLUMN_SIZE + 2)
#define MAX_MODEDESCS		(MAX_COLUMN_SIZE*3)

static modedesc_t	modedescs[MAX_MODEDESCS];

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	qpic_t		*p;
	char		*ptr;
	int			lnummodes, i, k, column, row;
	vmode_t		*pv;

	p = Draw_CachePic ("gfx/vidmodes.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	vid_wmodes = 0;
	lnummodes = VID_NumModes ();
	
	for (i=1 ; (i<lnummodes) && (vid_wmodes < MAX_MODEDESCS) ; i++)
	{
		ptr = VID_GetModeDescription (i);
		pv = VID_GetModePtr (i);

		k = vid_wmodes;

		modedescs[k].modenum = i;
		modedescs[k].desc = ptr;
		modedescs[k].iscur = 0;

		if (i == vid_modenum)
			modedescs[k].iscur = 1;

		vid_wmodes++;

	}

	if (vid_wmodes > 0)
	{
		M_Print (2*8, 36+0*8, "Fullscreen Modes (WIDTHxHEIGHTxBPP)");

		column = 8;
		row = 36+2*8;

		for (i=0 ; i<vid_wmodes ; i++)
		{
			if (modedescs[i].iscur)
				M_PrintWhite (column, row, modedescs[i].desc);
			else
				M_Print (column, row, modedescs[i].desc);

			column += 13*8;

			if ((i % VID_ROW_SIZE) == (VID_ROW_SIZE - 1))
			{
				column = 8;
				row += 8;
			}
		}
	}

	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*2,
			 "Video modes must be set from the");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*3,
			 "command line with -width <width>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*4,
			 "and -bpp <bits-per-pixel>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*6,
			 "Select windowed mode with -window");
}


/*
================
VID_MenuKey
================
*/
void VID_MenuKey (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		S_LocalSound ("misc/menu1.wav");
		M_Menu_Options_f ();
		break;

	default:
		break;
	}
}
