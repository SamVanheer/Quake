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
// snd_mem.c: sound caching

#include "quakedef.h"

wavinfo_t GetWavinfo(const char* name, byte* wav, int wavlength);

/*
================
ConvertPCMData
================
*/
void ConvertPCMData(sfx_t* sfx, const wavinfo_t& info, byte* data)
{
	if (info.width == 2)
	{
		for (int i = 0; i < info.samples; i++)
		{
			((short*)data)[i] = LittleShort(((short*)data)[i]);
		}
	}
}

//=============================================================================

/*
==============
S_LoadSound
==============
*/
sfx_t* S_LoadSound(sfx_t* s)
{
// see if still in memory
	if (s->buffer)
		return s;

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
		return NULL;
	}

	const wavinfo_t info = GetWavinfo(s->name, data, com_filesize);

	if (info.channels != 1)
	{
		Con_Printf("%s is a stereo sample\n", s->name);
		return NULL;
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
		return NULL;
	}

	s->buffer = OpenALBuffer::Create();

	if (!s->buffer)
		return NULL;

	if (info.loopstart >= 0)
	{
		s->loopingBuffer = OpenALBuffer::Create();

		if (!s->loopingBuffer)
		{
			s->buffer.Delete();
			return NULL;
		}
	}

	byte* samples = data + info.dataofs;

	ConvertPCMData(s, info, samples);

	if (info.loopstart >= 0)
	{
		alBufferData(s->buffer.Id, format, samples, info.loopstart * info.width * info.channels, info.rate);
		alBufferData(s->loopingBuffer.Id, format, samples + info.loopstart, (info.samples - info.loopstart) * info.width * info.channels, info.rate);
	}
	else
	{
		alBufferData(s->buffer.Id, format, samples, info.samples * info.width * info.channels, info.rate);
	}

	return s;
}

/*
===============================================================================

WAV loading

===============================================================================
*/
byte* data_p;
byte* iff_end;
byte* last_chunk;
byte* iff_data;
int 	iff_chunk_len;

short GetLittleShort()
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p + 1) << 8);
	data_p += 2;
	return val;
}

int GetLittleLong()
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p + 1) << 8);
	val = val + (*(data_p + 2) << 16);
	val = val + (*(data_p + 3) << 24);
	data_p += 4;
	return val;
}

void FindNextChunk(const char* name)
{
	while (1)
	{
		data_p = last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
		//		if (iff_chunk_len > 1024*1024)
		//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ((iff_chunk_len + 1) & ~1);
		if (!Q_strncmp(reinterpret_cast<char*>(data_p), name, 4))
			return;
	}
}

void FindChunk(const char* name)
{
	last_chunk = iff_data;
	FindNextChunk(name);
}

void DumpChunks()
{
	char	str[5];

	str[4] = 0;
	data_p = iff_data;
	do
	{
		memcpy(str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Con_Printf("0x%x : %s (%tu)\n", (std::ptrdiff_t)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo(const char* name, byte* wav, int wavlength)
{
	wavinfo_t info;
	memset(&info, 0, sizeof(info));

	if (!wav)
		return info;

	iff_data = wav;
	iff_end = wav + wavlength;

	// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !Q_strncmp(reinterpret_cast<char*>(data_p + 8), "WAVE", 4)))
	{
		Con_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;
	// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		Con_Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	const int format = GetLittleShort();
	if (format != 1)
	{
		Con_Printf("Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4 + 2;
	info.width = GetLittleShort() / 8;

	// get cue chunk
	FindChunk("cue ");
	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong();
		//		Con_Printf("loopstart=%d\n", sfx->loopstart);

			// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk("LIST");
		if (data_p)
		{
			if (!strncmp(reinterpret_cast<char*>(data_p + 28), "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				const int i = GetLittleLong();	// samples in loop
				info.samples = info.loopstart + i;
				//				Con_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

	// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		Con_Printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	const int samples = GetLittleLong() / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Sys_Error("Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;

	return info;
}
