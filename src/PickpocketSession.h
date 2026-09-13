#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

#include "RE/Skyrim.h"

namespace QuickPocket
{
	class PickpocketSession
	{
	public:
		static bool IsActive();
		static bool IsPlantMode();
		static bool CanAct(RE::ObjectRefHandle container);
		static bool CanTogglePlant(RE::ObjectRefHandle container = {});

		static RE::ObjectRefHandle GetVictim();
		static RE::ObjectRefHandle GetCurrentTarget();

		static void SetCurrentTarget(RE::ObjectRefHandle target);
		static void ClearCurrentTarget();

		static void Begin(RE::ObjectRefHandle victim);
		static void End(bool closeMenu = false);
		static void RefreshIfNeeded();

		static bool TogglePlant();
		static void SetPlantMode(bool enabled, bool refreshMenu = true);
		static void ResetPlantMode(bool refreshMenu = true);

	private:
		static inline std::recursive_mutex _mutex{};
		static inline RE::ObjectRefHandle _currentTarget{};
		static inline RE::ObjectRefHandle _victim{};
		static inline std::atomic<bool> _active{ false };
		static inline std::atomic<bool> _plantMode{ false };
		static inline std::uint32_t _lastToggleMs{ 0 };
		static inline std::chrono::steady_clock::time_point _nextLiveRefresh{};
	};
}
