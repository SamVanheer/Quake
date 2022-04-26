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

#include <array>
#include <atomic>
#include <cstdio>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "ICDAudio.h"
#include "OggSoundLoader.h"
#include "SoundSystem.h"

class CDAudio final : public ICDAudio
{
private:
	//This is tuned to work pretty well with the original Quake soundtrack, but needs testing to make sure it's good for any input.
	static constexpr std::size_t NumBuffers = 8;
	static constexpr std::size_t BufferSize = 1024 * 4;

	//Wrapper around FILE* that implements copy behavior as move behavior.
	//This is needed because std::function is copyable, and std::unique_ptr captured by lambda disables the lambda's copy constructor.
	//C++23 has std::move_only_function which solves this problem, but it's not available yet.
	struct FileWrapper
	{
		mutable FILE* File = nullptr;

		FileWrapper(FILE* file) noexcept
			: File(file)
		{
		}

		FileWrapper(FileWrapper&& other) noexcept
			: File(other.File)
		{
			other.File = nullptr;
		}

		FileWrapper& operator=(FileWrapper&& other) noexcept
		{
			if (this != &other)
			{
				Close();
				File = other.File;
				other.File = nullptr;
			}

			return *this;
		}

		FileWrapper(const FileWrapper& other) noexcept
			: File(other.File)
		{
			other.File = nullptr;
		}

		FileWrapper& operator=(const FileWrapper& other) noexcept
		{
			if (this != &other)
			{
				Close();
				File = other.File;
				other.File = nullptr;
			}

			return *this;
		}

		~FileWrapper() noexcept
		{
			Close();
		}

		FILE* get() noexcept
		{
			return File;
		}

		void release() noexcept
		{
			File = nullptr;
		}

	private:
		void Close()
		{
			if (File)
			{
				fclose(File);
				File = nullptr;
			}
		}
	};

public:
	~CDAudio() override;

	bool Create(SoundSystem& soundSystem);

	void Play(byte track, bool looping) override;
	void Stop() override;
	void Pause() override;
	void Resume() override;
	void CD_Command() override;

private:
	/**
	*	@brief If the current thread is not the worker thread, queues the given function and arguments for execution on the worker thread.
	*	@return @c true if the function was queued, false otherwise.
	*/
	template<typename Func, typename... Args>
	bool RunOnWorkerThread(Func func, Args&&... args);

	void StartPlaying(byte track, bool looping, FileWrapper file);

	void Run();
	void Update();

private:
	std::atomic<bool> m_Enabled = true;
	std::atomic<bool> m_Playing = false;
	std::atomic<bool> m_Paused = false;
	std::atomic<bool> m_Looping = false;

	std::atomic<byte> m_Track = 0;
	std::atomic<float> m_Volume = -1; //Set to -1 to force initialization.

	std::unique_ptr<ALCcontext, DeleterWrapper<alcDestroyContext>> m_Context;

	std::array<OpenALBuffer, NumBuffers> m_Buffers;
	OpenALSource m_Source;

	std::unique_ptr<OggSoundLoader> m_Loader;

	std::thread m_Thread;

	std::atomic<bool> m_Quit;

	std::mutex m_JobMutex;

	std::vector<std::function<void()>> m_Jobs;
	std::vector<std::function<void()>> m_JobsToExecute;
};
