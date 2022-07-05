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

#pragma once

#include <cstdio>
#include <memory>

#include <AL/al.h>

#include <vorbis/vorbisfile.h>

class OggSoundLoader final
{
public:
	OggSoundLoader(OggVorbis_File file)
		: m_File(file)
	{
		const auto sampleCount = ov_pcm_total(&m_File, -1);
		const auto info = ov_info(&m_File, -1);

		m_SampleSizeInBytes = info->channels * 2;
		m_Size = sampleCount * m_SampleSizeInBytes;
		m_Format = info->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
		m_Rate = info->rate;
	}

	~OggSoundLoader()
	{
		ov_clear(&m_File);
	}

	static std::unique_ptr<OggSoundLoader> TryOpenFile(FILE* file);

	std::size_t GetSampleSizeInBytes() const { return m_SampleSizeInBytes; }

	std::size_t GetSize() const { return m_Size; }

	ALenum GetFormat() const { return m_Format; }

	long GetRate() const { return m_Rate; }

	long Read(byte* buffer, int bufferLength)
	{
		int bitStream = 0;
		return ov_read(&m_File, reinterpret_cast<char*>(buffer), bufferLength, 0, 2, 1, &bitStream);
	}

	void Reset()
	{
		ov_raw_seek(&m_File, 0);
	}

private:
	OggVorbis_File m_File;
	std::size_t m_SampleSizeInBytes;
	std::size_t m_Size;
	ALenum m_Format;
	long m_Rate;
};

