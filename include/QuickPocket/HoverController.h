#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "RE/Skyrim.h"
#include "QuickPocket/Config.h"

namespace QuickPocket
{
	class HoverController
	{
	public:
		static void Start();
		static void Stop();
		static bool AllowsTakeFromContainer(RE::ObjectRefHandle container);
		static bool IsForcedPickpocketContainer(RE::ObjectRefHandle container);
		static bool IsPickpocketSessionActive();
		static RE::ObjectRefHandle GetActivePickpocketContainer();
		static RE::ObjectRefHandle GetCurrentTarget();
		static RE::Actor* GetCurrentVictim();

	private:
		static void Run(std::stop_token stopToken);
		static void Tick();
		static bool IsTargetEligible(RE::TESObjectREFR* target);
		static void ClearForcedContainerIfSet();
		static bool IsCurrentTargetLocked(RE::ObjectRefHandle container);

		static inline std::jthread _worker{};
		static inline std::recursive_mutex _stateMutex{};
		static inline std::atomic<bool> _eligible{ false };
		static inline RE::ObjectRefHandle _currentTarget{};
		static inline RE::ObjectRefHandle _lastForcedTarget{};
		static inline std::atomic<bool> _forcedContainerActive{ false };
		static inline std::chrono::steady_clock::time_point _nextLiveRefresh{};
	};
}
