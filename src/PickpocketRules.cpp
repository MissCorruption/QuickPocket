#include "QuickPocket/PickpocketRules.h"

#include "QuickPocket/Config.h"
#include "QuickPocket/PickpocketSession.h"

#include <array>
#include <set>

namespace QuickPocket::PickpocketRules
{
	namespace
	{
		inline constexpr RE::FormID kGoldFormID = 0x0000000F;
		inline constexpr std::int32_t kSpottedDetectionThreshold = 0;
		inline constexpr auto kLootMenuName = "LootMenu";
		inline constexpr std::array kQuickLootCompatMenuWhitelist{
			"Durability Menu",
			"UIHS_WidgetMenu",
			"WidgetMenu",
			"CombatAlertOverlayMenu"
		};

		bool IsBlockedByOpenMenu()
		{
			const auto ui = RE::UI::GetSingleton();
			if (!ui) {
				return false;
			}

			std::set<RE::IMenu*> whitelistedMenus{};
			if (const auto lootMenu = ui->GetMenu(kLootMenuName)) {
				whitelistedMenus.emplace(lootMenu.get());
			}

			if (const auto cursorMenu = ui->GetMenu(RE::CursorMenu::MENU_NAME)) {
				whitelistedMenus.emplace(cursorMenu.get());
			}

			for (const auto menuName : kQuickLootCompatMenuWhitelist) {
				if (const auto whitelistedMenu = ui->GetMenu(menuName)) {
					whitelistedMenus.emplace(whitelistedMenu.get());
				}
			}

			for (const auto& menu : ui->menuStack) {
				if (!menu) {
					continue;
				}

				if (menu->menuFlags & RE::UI_MENU_FLAGS::kAlwaysOpen) {
					continue;
				}

				if (whitelistedMenus.contains(menu.get())) {
					continue;
				}

				return true;
			}

			return false;
		}

		bool IsLootMenuOpen()
		{
			const auto ui = RE::UI::GetSingleton();
			return ui && ui->IsMenuOpen(kLootMenuName);
		}

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

	bool CanOpen(RE::TESObjectREFR* target)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !target) {
			return false;
		}

		if (IsBlockedByOpenMenu()) {
			return false;
		}

		if (!player->AsActorState()->IsSneaking()) {
			return false;
		}

		const auto actor = target->As<RE::Actor>();
		if (!actor || actor == player || actor->IsDead(false)) {
			return false;
		}

		if (player->IsInCombat() || actor->IsInCombat()) {
			return false;
		}

		const auto config = Config::Get();
		if (config.closeOnDetection && actor->RequestDetectionLevel(player) >= kSpottedDetectionThreshold) {
			return false;
		}

		if (!actor->HasKeywordString("ActorTypeNPC")) {
			return false;
		}

		return actor->GetActorBase() && actor->GetActorBase()->GetRace();
	}

	bool CanAct(RE::ObjectRefHandle container)
	{
		if (!IsLivingActorContainer(container)) {
			return false;
		}

		const auto victim = PickpocketSession::GetVictim();
		const auto current = PickpocketSession::GetCurrentTarget();
		return (victim && victim == container) || (current && current == container);
	}

	bool CanTogglePlant(RE::ObjectRefHandle container)
	{
		if (!IsLootMenuOpen() && !PickpocketSession::IsActive()) {
			return false;
		}

		if (!container) {
			container = PickpocketSession::GetVictim();
		}

		return IsLivingActorContainer(container);
	}

	bool IsActorContainer(RE::ObjectRefHandle container)
	{
		const auto ref = container.get();
		return ref && ref->As<RE::Actor>();
	}

	bool IsLivingActorContainer(RE::ObjectRefHandle container)
	{
		if (!IsActorContainer(container)) {
			return false;
		}

		const auto ref = container.get();
		const auto actor = ref ? ref->As<RE::Actor>() : nullptr;
		return actor && !actor->IsDead(false);
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

	bool ShouldDisplayStealEntry(const RE::InventoryEntryData* entry, const RE::Actor* player)
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
			if (!entry || entry->object != displayedEntry->object || !ShouldDisplayStealEntry(entry, player)) {
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
