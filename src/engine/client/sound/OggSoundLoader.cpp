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

#include "quakedef.h"
#include "OggSoundLoader.h"

std::unique_ptr<OggSoundLoader> OggSoundLoader::TryOpenFile(FILE* file)
{
	if (!file)
	{
		return {};
	}

	OggVorbis_File vorbisFile;

	//NOTE: if vorbisfile is not using the same runtime as winquake this will need to use ov_open_callbacks instead on Windows.
	//See https://xiph.org/vorbis/doc/vorbisfile/ov_open.html
	if (ov_open(file, &vorbisFile, nullptr, 0))
	{
		return {};
	}

	return std::make_unique<OggSoundLoader>(vorbisFile);
}
