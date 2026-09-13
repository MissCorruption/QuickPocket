#pragma once

#include <atomic>
#include <thread>

namespace QuickPocket
{
	class Targeting
	{
	public:
		static void Start();
		static void Stop();

	private:
		static void Run(std::stop_token stopToken);
		static void Tick();

		static inline std::jthread _worker{};
		static inline std::atomic<bool> _tickQueued{ false };
	};
}
