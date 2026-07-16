#include "QuickPocket/PickpocketRules.h"

#include "QuickPocket/Config.h"
#include "QuickPocket/HoverController.h"

namespace QuickPocket::PickpocketRules
{
	namespace
	{
		inline constexpr RE::FormID kGoldFormID = 0x0000000F;

		bool SharesExtraDataList(const RE::InventoryEntryData* a, const RE::InventoryEntryData* b)
		{
			if (!a || !b) {
				return false;
			}

			if (!a->extraLists && !b->extraLists) {
				return true;
			}

			if (!a->extraLists || !b->extraLists || a->extraLists->empty() || b->extraLists->empty()) {
				return false;
			}

			for (const auto* xListA : *a->extraLists) {
				if (!xListA) {
					continue;
				}

				for (const auto* xListB : *b->extraLists) {
					if (xListA == xListB) {
						return true;
					}
				}
			}

			return false;
		}

		bool HasDisplayablePositiveCount(const RE::InventoryEntryData* entry)
		{
			return entry && entry->object && entry->countDelta > 0;
		}

		bool IsPlayerGold(const RE::TESBoundObject& object)
		{
			return object.GetFormID() == kGoldFormID;
		}

		bool IsJewelryObject(const RE::TESBoundObject& object)
		{
			if (const auto keywordForm = skyrim_cast<const RE::BGSKeywordForm*>(&object); keywordForm && keywordForm->HasKeywordString("VendorItemJewelry")) {
				return true;
			}

			const auto biped = skyrim_cast<const RE::BGSBipedObjectForm*>(&object);
			if (!biped) {
				return false;
			}

			return biped->HasPartOf(RE::BGSBipedObjectForm::BipedObjectSlot::kRing) ||
			       biped->HasPartOf(RE::BGSBipedObjectForm::BipedObjectSlot::kAmulet);
		}

		RE::BGSEntryPointPerkEntry::EntryPoint GetEquippedPickpocketEntryPoint()
		{
			return static_cast<RE::BGSEntryPointPerkEntry::EntryPoint>(
				static_cast<std::uint8_t>(RE::BGSEntryPoint::ENTRY_POINT::kCanPickpocketEquippedItem));
		}
	}

	bool IsActorContainer(RE::ObjectRefHandle container)
	{
		const auto ref = container.get();
		return ref && ref->As<RE::Actor>();
	}

	bool IsPickpocketMenuContainer(RE::ObjectRefHandle container)
	{
		if (!IsActorContainer(container)) {
			return false;
		}

		const auto ref = container.get();
		const auto actor = ref ? ref->As<RE::Actor>() : nullptr;
		return actor && !actor->IsDead(false);
	}

	bool IsActivePickpocketTarget(RE::ObjectRefHandle container)
	{
		if (!IsPickpocketMenuContainer(container)) {
			return false;
		}

		return HoverController::AllowsTakeFromContainer(container) ||
		       HoverController::IsForcedPickpocketContainer(container);
	}

	bool IsDisplayableObject(const RE::TESBoundObject& object)
	{
		if (!object.GetPlayable()) {
			return false;
		}

		const auto name = object.GetName();
		if (!name || name[0] == '\0') {
			return false;
		}

		switch (object.GetFormType()) {
		case RE::FormType::Scroll:
		case RE::FormType::Armor:
		case RE::FormType::Book:
		case RE::FormType::Ingredient:
		case RE::FormType::Misc:
		case RE::FormType::Weapon:
		case RE::FormType::KeyMaster:
		case RE::FormType::AlchemyItem:
		case RE::FormType::Note:
		case RE::FormType::SoulGem:
			return true;
		case RE::FormType::Ammo:
			{
				const auto keywordForm = skyrim_cast<const RE::BGSKeywordForm*>(&object);
				return !keywordForm || !keywordForm->HasKeywordString("VendorItemBoundArrow");
			}
		case RE::FormType::Light:
			return skyrim_cast<const RE::TESObjectLIGH*>(&object)->CanBeCarried();
		default:
			return false;
		}
	}

	bool CanStealWornItems(const RE::Actor* actor)
	{
		return actor && actor->HasPerkEntries(GetEquippedPickpocketEntryPoint());
	}

	bool ShouldDisplayNpcEntry(const RE::InventoryEntryData* entry, const RE::Actor* player)
	{
		if (!HasDisplayablePositiveCount(entry)) {
			return false;
		}

		if (!IsDisplayableObject(*entry->object)) {
			return false;
		}

		if (entry->IsWorn() && !CanStealWornItems(player)) {
			return false;
		}

		return true;
	}

	bool ShouldDisplayPlantEntry(const RE::InventoryEntryData* entry)
	{
		if (!HasDisplayablePositiveCount(entry)) {
			return false;
		}

		if (!IsDisplayableObject(*entry->object)) {
			return false;
		}

		if (IsPlayerGold(*entry->object)) {
			return false;
		}

		const auto cfg = QuickPocket::Config::Get();
		if (entry->IsWorn() && !cfg.plantAllowEquipped) {
			return false;
		}

		if (const auto alchemy = skyrim_cast<RE::AlchemyItem*>(entry->object); alchemy && alchemy->IsPoison()) {
			return cfg.plantAllowPoisons;
		}

		if (IsJewelryObject(*entry->object)) {
			return cfg.plantAllowJewelry;
		}

		return cfg.plantAllowOtherItems;
	}

	RE::InventoryEntryData* FindPlayerEntryForPlanting(const RE::InventoryEntryData* displayedEntry)
	{
		if (!displayedEntry || !displayedEntry->object) {
			return nullptr;
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto changes = player ? player->GetInventoryChanges(false) : nullptr;
		if (!changes || !changes->entryList) {
			return nullptr;
		}

		RE::InventoryEntryData* fallback = nullptr;
		for (const auto entry : *changes->entryList) {
			if (!entry || entry->object != displayedEntry->object || !ShouldDisplayPlantEntry(entry)) {
				continue;
			}

			if (SharesExtraDataList(displayedEntry, entry)) {
				return entry;
			}

			if (!fallback) {
				fallback = entry;
			}
		}

		return fallback;
	}

	RE::InventoryEntryData* FindVictimEntryForSteal(RE::Actor* victim, const RE::InventoryEntryData* displayedEntry)
	{
		if (!victim || !displayedEntry || !displayedEntry->object) {
			return nullptr;
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto changes = victim->GetInventoryChanges(false);
		if (!changes || !changes->entryList) {
			return nullptr;
		}

		RE::InventoryEntryData* fallback = nullptr;
		for (const auto entry : *changes->entryList) {
			if (!entry || entry->object != displayedEntry->object || !ShouldDisplayNpcEntry(entry, player)) {
				continue;
			}

			if (SharesExtraDataList(displayedEntry, entry)) {
				return entry;
			}

			if (!fallback) {
				fallback = entry;
			}
		}

		return fallback;
	}
}
