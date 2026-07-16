#pragma once

#include <atomic>

#include "RE/Skyrim.h"

namespace QuickPocket
{
	class PlantModeController
	{
	public:
		static void Install();
		static bool IsEnabled();
		static bool Toggle();
		static bool Toggle(RE::ObjectRefHandle container);
		static bool ToggleOnce(RE::ObjectRefHandle container);
		static void Reset(bool refreshMenu = true);
		static void EnableForEmptyStealList(bool refreshMenu);

	private:
		static bool CanToggle();
		static bool CanToggle(RE::ObjectRefHandle container);
		static void SetEnabled(bool enabled, bool refreshMenu);

		static inline std::atomic<bool> _enabled{ false };
	};
}
