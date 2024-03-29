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
// sys.h -- non-portable functions

//
// file IO
//

// returns the file size
// return -1 if file is not present
// the file should be in BINARY mode for stupid OSs that care
long Sys_FileOpenRead(const char* path, FILE** hndl);

FILE* Sys_FileOpenWrite(const char* path);
time_t Sys_FileTime(const char* path);
void Sys_mkdir(const char* path);

//
// system IO
//
void Sys_DebugLog(const char* file, const char* fmt, ...);

[[ noreturn ]] void Sys_Error(const char* error, ...);
// an error will cause the entire program to exit

void Sys_Printf(const char* fmt, ...);
// send text to the console

void Sys_Quit(void);

double Sys_FloatTime(void);

char* Sys_ConsoleInput(bool acceptEmptyInput = false);

void Sys_Sleep(void);
// called to yield for a little bit so as
// not to hog cpu when paused or debugging

void Sys_SendKeyEvents(void);
// Perform Key_Event () callbacks until the input que is empty
