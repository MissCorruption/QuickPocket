#pragma once

#include "RE/Skyrim.h"

namespace QuickPocket::PickpocketRules
{
	bool IsActorContainer(RE::ObjectRefHandle container);
	bool IsPickpocketMenuContainer(RE::ObjectRefHandle container);
	bool IsActivePickpocketTarget(RE::ObjectRefHandle container);
	bool IsDisplayableObject(const RE::TESBoundObject& object);
	bool CanStealWornItems(const RE::Actor* actor);
	bool ShouldDisplayNpcEntry(const RE::InventoryEntryData* entry, const RE::Actor* player);
	bool ShouldDisplayPlantEntry(const RE::InventoryEntryData* entry);
	RE::InventoryEntryData* FindPlayerEntryForPlanting(const RE::InventoryEntryData* displayedEntry);
	RE::InventoryEntryData* FindVictimEntryForSteal(RE::Actor* victim, const RE::InventoryEntryData* displayedEntry);
}
