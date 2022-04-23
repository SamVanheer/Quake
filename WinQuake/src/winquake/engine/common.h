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
// comndef.h  -- general definitions

#include <cstddef>

#if !defined BYTE_DEFINED
typedef unsigned char 		byte;
#define BYTE_DEFINED 1
#endif

//============================================================================

typedef struct sizebuf_s
{
	bool	allowoverflow;	// if false, do a Sys_Error
	bool	overflowed;		// set to true if the buffer size failed
	byte* data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

void SZ_Alloc(sizebuf_t* buf, int startsize);
void SZ_Free(sizebuf_t* buf);
void SZ_Clear(sizebuf_t* buf);
void* SZ_GetSpace(sizebuf_t* buf, int length);
void SZ_Write(sizebuf_t* buf, const void* data, int length);
void SZ_Print(sizebuf_t* buf, const char* data);	// strcats onto the sizebuf

//============================================================================

typedef struct link_s
{
	struct link_s* prev, * next;
} link_t;


void ClearLink(link_t* l);
void RemoveLink(link_t* l);
void InsertLinkBefore(link_t* l, link_t* before);
void InsertLinkAfter(link_t* l, link_t* after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - offsetof(t, m)))

//============================================================================

#ifndef NULL
#define NULL ((void *)0)
#endif

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT	((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT 	((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================

extern	bool		bigendien;

extern	short	(*BigShort) (short l);
extern	short	(*LittleShort) (short l);
extern	int	(*BigLong) (int l);
extern	int	(*LittleLong) (int l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

//============================================================================

void MSG_WriteChar(sizebuf_t* sb, int c);
void MSG_WriteByte(sizebuf_t* sb, int c);
void MSG_WriteShort(sizebuf_t* sb, int c);
void MSG_WriteLong(sizebuf_t* sb, int c);
void MSG_WriteFloat(sizebuf_t* sb, float f);
void MSG_WriteString(sizebuf_t* sb, const char* s);
void MSG_WriteCoord(sizebuf_t* sb, float f);
void MSG_WriteAngle(sizebuf_t* sb, float f);

extern	int			msg_readcount;
extern	bool		msg_badread;		// set if a read goes beyond end of message

void MSG_BeginReading(void);
int MSG_ReadChar(void);
int MSG_ReadByte(void);
int MSG_ReadShort(void);
int MSG_ReadLong(void);
float MSG_ReadFloat(void);
char* MSG_ReadString(void);

float MSG_ReadCoord(void);
float MSG_ReadAngle(void);

//============================================================================

void Q_memset(void* dest, int fill, int count);
void Q_memcpy(void* dest, const void* src, int count);
int Q_memcmp(const void* m1, const void* m2, int count);
void Q_strcpy(char* dest, const char* src);
void Q_strncpy(char* dest, const char* src, int count);
int Q_strlen(const char* str);
const char* Q_strrchr(const char* s, char c);
char* Q_strrchr(char* s, char c);
void Q_strcat(char* dest, const char* src);
int Q_strcmp(const char* s1, const char* s2);
int Q_strncmp(const char* s1, const char* s2, int count);
int Q_strcasecmp(const char* s1, const char* s2);
int Q_strncasecmp(const char* s1, const char* s2, int n);
int	Q_atoi(const char* str);
float Q_atof(const char* str);

//============================================================================

extern	char		com_token[1024];

char* COM_Parse(char* data);


extern	int		com_argc;
extern	const char** com_argv;

int COM_CheckParm(const char* parm);
void COM_Init(const char* path);
void COM_InitArgv(int argc, const char* const* argv);

const char* COM_SkipPath(const char* pathname);
void COM_StripExtension(const char* in, char* out);
void COM_FileBase(const char* in, char* out);
void COM_DefaultExtension(char* path, const char* extension);

char* va(const char* format, ...);
// does a varargs printf into a temp buffer


//============================================================================

extern int com_filesize;
struct cache_user_s;

extern	char	com_gamedir[MAX_OSPATH];

void COM_WriteFile(const char* filename, const void* data, int len);
int COM_OpenFile(const char* filename, int* hndl);
int COM_FOpenFile(const char* filename, FILE** file);
void COM_CloseFile(int h);

byte* COM_LoadStackFile(const char* path, void* buffer, int bufsize);
byte* COM_LoadTempFile(const char* path);
byte* COM_LoadHunkFile(const char* path);
void COM_LoadCacheFile(const char* path, struct cache_user_s* cu);


extern	struct cvar_s	registered;

extern bool		standard_quake, rogue, hipnotic;
