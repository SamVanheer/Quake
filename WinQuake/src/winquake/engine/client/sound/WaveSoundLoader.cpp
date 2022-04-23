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
#include "WaveSoundLoader.h"

//TODO: this can be improved further.

class WaveSoundLoader
{
public:
	WaveSoundLoader(byte* wav, int wavlength)
		: iff_data(wav)
		, iff_end(wav + wavlength)
	{
	}

	short GetLittleShort();
	int GetLittleLong();

	bool FindNextChunk(const char* name);
	bool FindChunk(const char* name);
	void DumpChunks();

	byte* iff_data;
	byte* iff_end;

	byte* data_p = nullptr;
	byte* last_chunk = nullptr;
};

short WaveSoundLoader::GetLittleShort()
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p + 1) << 8);
	data_p += 2;
	return val;
}

int WaveSoundLoader::GetLittleLong()
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p + 1) << 8);
	val = val + (*(data_p + 2) << 16);
	val = val + (*(data_p + 3) << 24);
	data_p += 4;
	return val;
}

bool WaveSoundLoader::FindNextChunk(const char* name)
{
	while (1)
	{
		data_p = last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return false;
		}

		data_p += 4;
		const int iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return false;
		}
		//		if (iff_chunk_len > 1024*1024)
		//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ((iff_chunk_len + 1) & ~1);
		if (!Q_strncmp(reinterpret_cast<char*>(data_p), name, 4))
			break;
	}

	return true;
}

bool WaveSoundLoader::FindChunk(const char* name)
{
	last_chunk = iff_data;
	return FindNextChunk(name);
}

void WaveSoundLoader::DumpChunks()
{
	char str[5];

	str[4] = 0;
	data_p = iff_data;
	do
	{
		memcpy(str, data_p, 4);
		data_p += 4;
		const int iff_chunk_len = GetLittleLong();
		Con_Printf("0x%x : %s (%tu)\n", (std::ptrdiff_t)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}

wavinfo_t GetWavinfo(const char* name, byte* wav, int wavlength)
{
	wavinfo_t info;
	memset(&info, 0, sizeof(info));

	if (!wav)
		return info;

	WaveSoundLoader loader{wav, wavlength};

	// find "RIFF" chunk
	if (!(loader.FindChunk("RIFF") && !Q_strncmp(reinterpret_cast<char*>(loader.data_p + 8), "WAVE", 4)))
	{
		Con_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

	// get "fmt " chunk
	loader.iff_data = loader.data_p + 12;
	// DumpChunks ();

	if (!loader.FindChunk("fmt "))
	{
		Con_Printf("Missing fmt chunk\n");
		return info;
	}
	loader.data_p += 8;
	const int format = loader.GetLittleShort();
	if (format != 1)
	{
		Con_Printf("Microsoft PCM format only\n");
		return info;
	}

	info.channels = loader.GetLittleShort();
	info.rate = loader.GetLittleLong();
	loader.data_p += 4 + 2;
	info.width = loader.GetLittleShort() / 8;

	// get cue chunk
	if (loader.FindChunk("cue "))
	{
		loader.data_p += 32;
		info.loopstart = loader.GetLittleLong();
		//		Con_Printf("loopstart=%d\n", sfx->loopstart);

			// if the next chunk is a LIST chunk, look for a cue length marker
		if (loader.FindNextChunk("LIST"))
		{
			if (!strncmp(reinterpret_cast<char*>(loader.data_p + 28), "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				loader.data_p += 24;
				const int i = loader.GetLittleLong();	// samples in loop
				info.samples = info.loopstart + i;
				//				Con_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

	// find data chunk
	if (!loader.FindChunk("data"))
	{
		Con_Printf("Missing data chunk\n");
		return info;
	}

	loader.data_p += 4;
	const int samples = loader.GetLittleLong() / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Sys_Error("Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = loader.data_p - wav;

	return info;
}

