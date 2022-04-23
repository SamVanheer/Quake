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
// snd_mem.cpp: sound caching

#include "quakedef.h"
#include "WaveSoundLoader.h"

bool S_LoadSound(sfx_t* s)
{
// see if still in memory
	if (s->buffer)
		return true;

	//Con_Printf ("S_LoadSound: %x\n", (int)stackbuf);
	// load it in
	char namebuffer[256];
	Q_strcpy(namebuffer, "sound/");
	Q_strcat(namebuffer, s->name);

	//	Con_Printf ("loading %s\n",namebuffer);

	byte stackbuf[1 * 1024]; // avoid dirtying the cache heap
	auto data = COM_LoadStackFile(namebuffer, stackbuf, sizeof(stackbuf));

	if (!data)
	{
		Con_Printf("Couldn't load %s\n", namebuffer);
		return false;
	}

	const wavinfo_t info = GetWavinfo(s->name, data, com_filesize);

	if (info.channels != 1)
	{
		Con_Printf("%s is a stereo sample\n", s->name);
		return false;
	}

	const ALenum format = [&]()
	{
		switch (info.channels)
		{
		case 1:
			switch (info.width)
			{
			case 1: return AL_FORMAT_MONO8;
			case 2: return AL_FORMAT_MONO16;
			}
			break;
		case 2:
			switch (info.width)
			{
			case 1: return AL_FORMAT_STEREO8;
			case 2: return AL_FORMAT_STEREO16;
			}
			break;
		}

		Con_Printf("%s has invalid format (channels: %d, sample bits: %d)\n", s->name, info.channels, info.width * 8);
		return 0;
	}();

	if (format == 0)
	{
		return false;
	}

	s->buffer = OpenALBuffer::Create();

	if (!s->buffer)
		return false;

	if (info.loopstart >= 0)
	{
		s->loopingBuffer = OpenALBuffer::Create();

		if (!s->loopingBuffer)
		{
			s->buffer.Delete();
			return false;
		}
	}

	byte* samples = data + info.dataofs;

	if (info.width == 2)
	{
		for (int i = 0; i < info.samples; i++)
		{
			((short*)samples)[i] = LittleShort(((short*)samples)[i]);
		}
	}

	if (info.loopstart >= 0)
	{
		alBufferData(s->buffer.Id, format, samples, info.loopstart * info.width * info.channels, info.rate);
		alBufferData(s->loopingBuffer.Id, format, samples + info.loopstart, (info.samples - info.loopstart) * info.width * info.channels, info.rate);
	}
	else
	{
		alBufferData(s->buffer.Id, format, samples, info.samples * info.width * info.channels, info.rate);
	}

	return true;
}
