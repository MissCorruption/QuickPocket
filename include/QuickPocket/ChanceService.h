#pragma once

#include <string>

#include "RE/Skyrim.h"

namespace QuickPocket
{
	class ChanceService
	{
	public:
		static int ComputeChance(RE::Actor* victim, RE::InventoryEntryData* entry);
		static std::string FormatChance(int chancePercent);
	};
}
