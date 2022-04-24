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

#define AL_ALEXT_PROTOTYPES
#include <AL/alext.h>

#include "CDAudio.h"
#include "OggSoundLoader.h"
#include "sound_internal.h"

struct ContextSwitcher
{
	ALCcontext* const Previous;

	const bool Success;

	ContextSwitcher(ALCcontext* context)
		: Previous(alcGetCurrentContext())
		, Success(ALC_FALSE != alcMakeContextCurrent(context))
	{
		if (!Success)
		{
			Con_SafePrintf("Couldn't make OpenAL context current\n");
		}
	}

	~ContextSwitcher()
	{
		alcMakeContextCurrent(Previous);
	}

	operator bool() const { return Success; }
};

std::unique_ptr<ICDAudio> g_CDAudio;

constexpr std::array MusicFileNames
{
	"", //Track 0 is not used.
	"", //Track 1 is not used.
	"music/track02.ogg",
	"music/track03.ogg",
	"music/track04.ogg",
	"music/track05.ogg",
	"music/track06.ogg",
	"music/track07.ogg",
	"music/track08.ogg",
	"music/track09.ogg",
	"music/track10.ogg",
	"music/track11.ogg"
};

CDAudio::~CDAudio()
{
	//Stop worker thread and wait until it finishes.
	m_Quit = true;

	if (m_Thread.joinable())
	{
		m_Thread.join();
	}

	//Since we switch to this context on-demand we can't rely on the default destructor to clean up.
	if (const ContextSwitcher switcher{m_Context.get()}; switcher)
	{
		m_Source.Delete();

		for (auto& buffer : m_Buffers)
		{
			buffer.Delete();
		}
	}
}

bool CDAudio::Create(SoundSystem& soundSystem)
{
	if (ALC_FALSE == alcIsExtensionPresent(soundSystem.GetDevice(), "ALC_EXT_thread_local_context"))
	{
		Con_SafePrintf("OpenAL does not provide extension \"ALC_EXT_thread_local_context\", required for music playback\n");
		return false;
	}

	m_Context.reset(alcCreateContext(soundSystem.GetDevice(), nullptr));

	if (!m_Context)
	{
		Con_SafePrintf("Couldn't create OpenAL context\n");
		return false;
	}

	const ContextSwitcher switcher{m_Context.get()};

	if (!switcher)
	{
		return false;
	}

	for (auto& buffer : m_Buffers)
	{
		buffer = OpenALBuffer::Create();

		if (!buffer)
		{
			Con_SafePrintf("Couldn't create OpenAL buffer\n");
			return false;
		}
	}

	m_Source = OpenALSource::Create();

	if (!m_Source)
	{
		Con_SafePrintf("Couldn't create OpenAL source\n");
		return false;
	}

	//Start the worker thread.
	m_Thread = std::thread{&CDAudio::Run, this};

	return true;
}

void CDAudio::Play(byte track, bool looping)
{
	//Do this first to allow for console output.
	if (track < 2 || track > MusicFileNames.size())
	{
		Con_DPrintf("CDAudio: Bad track number %u.\n", track);
		return;
	}

	FILE* file;

	if (COM_FOpenFile(MusicFileNames[track], &file) < 0)
	{
		Con_DPrintf("CDAudio: Track %u (\"%s\") does not exist.\n", track, MusicFileNames[track]);
		return;
	}

	if (RunOnWorkerThread(&CDAudio::Play, track, looping))
	{
		fclose(file);
		return;
	}

	if (!m_Enabled)
		return;

	//TODO: don't print to console from worker thread.

	if (m_Playing || m_Paused)
	{
		if (m_Track == track)
			return;
		Stop();
	}

	m_Loader = OggSoundLoader::TryOpenFile(file);

	if (!m_Loader)
	{
		//Can happen if the file was deleted or otherwise rendered inaccessible since queueing this.
		return;
	}

	//Fill all buffers with data.
	byte dataBuffer[BufferSize];

	for (auto& buffer : m_Buffers)
	{
		const long bytesRead = m_Loader->Read(dataBuffer, sizeof(dataBuffer));

		if (bytesRead <= 0)
			break;

		alBufferData(buffer.Id, m_Loader->GetFormat(), dataBuffer, bytesRead, m_Loader->GetRate());
	}

	//Attach all buffers and play.
	ALuint buffers[NumBuffers]{};

	for (std::size_t i = 0; i < m_Buffers.size(); ++i)
	{
		buffers[i] = m_Buffers[i].Id;
	}

	alSourceQueueBuffers(m_Source.Id, m_Buffers.size(), buffers);

	alSourcePlay(m_Source.Id);

	m_Looping = looping;
	m_Track = track;
	m_Playing = true;

	if (m_Volume == 0.0)
		Pause();
}

void CDAudio::Stop()
{
	if (RunOnWorkerThread(&CDAudio::Stop))
	{
		return;
	}

	if (!m_Enabled)
		return;

	if (!m_Playing && !m_Paused)
		return;

	alSourceStop(m_Source.Id);

	//Unqueue all buffers as well to free them up.
	alSourcei(m_Source.Id, AL_BUFFER, NullBuffer);

	m_Loader.reset();

	m_Paused = false;
	m_Playing = false;
}

void CDAudio::Pause()
{
	if (RunOnWorkerThread(&CDAudio::Pause))
	{
		return;
	}

	if (!m_Enabled)
		return;

	if (!m_Playing)
		return;

	alSourcePause(m_Source.Id);

	m_Paused = m_Playing.load();
	m_Playing = false;
}

void CDAudio::Resume()
{
	if (RunOnWorkerThread(&CDAudio::Resume))
	{
		return;
	}

	if (!m_Enabled)
		return;

	if (!m_Paused)
		return;

	alSourcePlay(m_Source.Id);

	m_Playing = true;
}

void CDAudio::CD_Command()
{
	if (Cmd_Argc() < 2)
		return;

	auto command = Cmd_Argv(1);

	if (Q_strcasecmp(command, "on") == 0)
	{
		m_Enabled = true;
	}
	else if (Q_strcasecmp(command, "off") == 0)
	{
		Stop();
		m_Enabled = false;
	}
	else if (Q_strcasecmp(command, "reset") == 0)
	{
		m_Enabled = true;
		Stop();
	}
	else if (Q_strcasecmp(command, "remap") == 0)
	{
		Con_Printf("CD remapping not supported\n");
	}
	else if (Q_strcasecmp(command, "play") == 0)
	{
		Play((byte)Q_atoi(Cmd_Argv(2)), false);
	}
	else if (Q_strcasecmp(command, "loop") == 0)
	{
		Play((byte)Q_atoi(Cmd_Argv(2)), true);
	}
	else if (Q_strcasecmp(command, "stop") == 0)
	{
		Stop();
	}
	else if (Q_strcasecmp(command, "pause") == 0)
	{
		Pause();
	}
	else if (Q_strcasecmp(command, "resume") == 0)
	{
		Resume();
	}
	else if (Q_strcasecmp(command, "info") == 0)
	{
		Con_Printf("%u tracks\n", MusicFileNames.size());
		if (m_Playing)
			Con_Printf("Currently %s track %u\n", m_Looping ? "looping" : "playing", m_Track.load());
		else if (m_Paused)
			Con_Printf("Paused %s track %u\n", m_Looping ? "looping" : "playing", m_Track.load());
		Con_Printf("Volume is %f\n", m_Volume.load());
	}
}

template<typename Func, typename... Args>
bool CDAudio::RunOnWorkerThread(Func func, Args&&... args)
{
	if (std::this_thread::get_id() == m_Thread.get_id())
	{
		return false;
	}

	//Queue up for execution on the worker thread.
	const std::lock_guard guard{m_JobMutex};

	m_Jobs.emplace_back([=]() { (this->*func)(args...); });

	return true;
}

void CDAudio::Run()
{
	//Use our context on our own thread only.
	alcSetThreadContext(m_Context.get());

	while (!m_Quit)
	{
		//Run pending jobs, if any.
		{
			const std::lock_guard guard{m_JobMutex};

			for (auto& job : m_Jobs)
			{
				job();
			}

			m_Jobs.clear();
		}

		Update();
	}

	alcSetThreadContext(nullptr);
}

void CDAudio::Update()
{
	if (!m_Enabled)
		return;

	if (m_Playing)
	{
		//Fill processed buffers with new data if needed.
		ALint processed;
		alGetSourcei(m_Source.Id, AL_BUFFERS_PROCESSED, &processed);

		ALuint bufferId;

		byte dataBuffer[BufferSize];

		while (processed-- > 0)
		{
			alSourceUnqueueBuffers(m_Source.Id, 1, &bufferId);

			long bytesRead = m_Loader->Read(dataBuffer, sizeof(dataBuffer));

			if (bytesRead <= 0)
			{
				if (!m_Looping)
				{
					break;
				}

				m_Loader->Reset();

				bytesRead = m_Loader->Read(dataBuffer, sizeof(dataBuffer));
			}

			if (bytesRead <= 0)
			{
				break;
			}

			alBufferData(bufferId, m_Loader->GetFormat(), dataBuffer, bytesRead, m_Loader->GetRate());

			alSourceQueueBuffers(m_Source.Id, 1, &bufferId);
		}
	}

	//TODO: value can potentially change while being read.
	if (float setting = bgmvolume.value; setting != m_Volume)
	{
		//Force value to be either 0 or 1.
		if (setting != 0)
		{
			setting = 1;
		}

		alSourcei(m_Source.Id, AL_GAIN, setting);

		if (m_Volume)
		{
			Pause();
		}
		else
		{
			Resume();
		}

		m_Volume = setting;
	}
}

void CD_f()
{
	g_CDAudio->CD_Command();
}

bool CDAudio_Init()
{
	if (cls.state == ca_dedicated)
		return false;

	if (COM_CheckParm("-nocdaudio"))
		return false;

	g_CDAudio = g_SoundSystem->CreateCDAudio();

	if (!g_CDAudio)
	{
		Con_Printf("Error Initializing CD Audio\n");
		return false;
	}

	Cmd_AddCommand("cd", CD_f);

	Con_Printf("CD Audio Initialized\n");

	return true;
}

void CDAudio_Shutdown()
{
	g_CDAudio.reset();
}
