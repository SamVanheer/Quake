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

// this file is shared by quake and qcc

#include <cstdint>

typedef intptr_t func_t;

typedef enum : unsigned short {ev_void, ev_string, ev_float, ev_vector, ev_entity, ev_field, ev_function, ev_pointer, ev_int} etype_t;

constexpr int etypes_count = ev_int + 1;

#define	DEF_SAVEGLOBAL	(1<<15)
