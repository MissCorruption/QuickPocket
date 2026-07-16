#include <algorithm>
#include <string_view>

#include "QuickPocket/EventHandlers.h"

#include "QuickPocket/ChanceService.h"
#include "QuickPocket/Config.h"
#include "QuickPocket/FormUtil.h"
#include "QuickPocket/HoverController.h"
#include "QuickPocket/PickpocketRules.h"
#include "QuickPocket/PlantModeController.h"
#include "QuickPocket/QuickLootBridge.h"

#include "SKSE/API.h"

#include "RE/A/AlchemyItem.h"
#include "RE/B/BGSPerk.h"
#include "RE/M/MagicSystem.h"

namespace
{
	std::atomic<int> g_selectedChance{ -1 };

	// QuickLoot pickpocket UI treats the NPC as the viewed container for extra-list splitting.
	inline constexpr bool kIsViewingPickpocketTarget = true;

	enum class PickpocketInventoryKind : std::uint8_t
	{
		kPlant,
		kSteal,
	};

	RE::ExtraDataList* GetInventoryEntryExtraListForRemoval(
		RE::InventoryEntryData* entry,
		std::int32_t count,
		bool isViewingContainer)
	{
		if (REL::Module::IsVR()) {
			if (!entry || count <= 0 || !entry->extraLists || entry->extraLists->empty()) {
				return nullptr;
			}

			auto remaining = count;
			for (auto& xList : *entry->extraLists) {
				if (!xList) {
					continue;
				}

				const auto xCount = std::clamp<std::int32_t>(xList->GetCount(), 1, remaining);
				remaining -= xCount;
				if (remaining <= 0) {
					return xList;
				}
			}

			return nullptr;
		}

		using func_t = RE::ExtraDataList* (*)(RE::InventoryEntryData*, int, bool);
		static REL::Relocation<func_t> func{ RELOCATION_ID(50948, 51825) };
		return func(entry, static_cast<int>(count), isViewingContainer);
	}

	const char* GetEntryDisplayName(const RE::InventoryEntryData* entry)
	{
		if (!entry || !entry->object) {
			return "(unknown item)";
		}
		const char* name = entry->object->GetName();
		return (name && name[0] != '\0') ? name : "(unnamed item)";
	}

	const char* GetObjectDisplayName(RE::TESBoundObject* object)
	{
		if (!object) {
			return "item";
		}

		const char* name = object->GetName();
		return (name && name[0] != '\0') ? name : "item";
	}

	std::int32_t ClampTakeCount(
		const RE::InventoryEntryData* liveEntry,
		const RE::InventoryEntryData* displayedEntry)
	{
		return std::max(1, std::min(liveEntry->countDelta, displayedEntry ? displayedEntry->countDelta : 1));
	}

	// ContainerMenu poison-plant natives are unavailable; cast planted poison on next task when Poisoned perk is present.
	inline constexpr RE::FormID kPoisonedPickpocketPerkID = 0x000105F28;

	RE::InventoryEntryData* FindTransferEntry(
		PickpocketInventoryKind kind,
		RE::PlayerCharacter* player,
		RE::Actor* victim,
		RE::TESBoundObject* object,
		std::int32_t count)
	{
		if (!player || !victim || !object) {
			return nullptr;
		}

		if (kind == PickpocketInventoryKind::kPlant) {
			if (const auto changes = player->GetInventoryChanges(false); changes && changes->entryList) {
				for (const auto entry : *changes->entryList) {
					if (entry && entry->object == object &&
						QuickPocket::PickpocketRules::ShouldDisplayPlantEntry(entry) &&
						entry->countDelta >= count) {
						return entry;
					}
				}
			}
		} else {
			if (const auto changes = victim->GetInventoryChanges(false); changes && changes->entryList) {
				for (const auto entry : *changes->entryList) {
					if (entry && entry->object == object &&
						QuickPocket::PickpocketRules::ShouldDisplayNpcEntry(entry, player) &&
						entry->countDelta >= count) {
						return entry;
					}
				}
			}
		}

		return nullptr;
	}

	void LogMissingTransferEntry(
		PickpocketInventoryKind kind,
		RE::TESBoundObject* object,
		RE::FormID objectFormID,
		std::int32_t count)
	{
		if (kind == PickpocketInventoryKind::kPlant) {
			logger::warn(
				"[QP] Pickpocket transfer (plant): '{}' (form {:08X}) no longer in thief inventory (need {}).",
				GetObjectDisplayName(object),
				objectFormID,
				count);
			return;
		}

		logger::warn(
			"[QP] Pickpocket transfer (steal): '{}' (form {:08X}) is no longer on the NPC (need {}).",
			GetObjectDisplayName(object),
			objectFormID,
			count);
	}

	void ApplyPlantedPoisonEffectsIfNeeded(
		RE::PlayerCharacter* thief,
		RE::Actor* victim,
		RE::TESBoundObject* planted,
		std::int32_t plantCount)
	{
		const auto poison = skyrim_cast<RE::AlchemyItem*>(planted);
		if (!poison || !poison->IsPoison()) {
			return;
		}

		auto* perk = RE::TESForm::LookupByID<RE::BGSPerk>(kPoisonedPickpocketPerkID);
		if (!perk || !thief->HasPerk(perk)) {
			return;
		}

		auto* caster = thief->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!caster) {
			caster = thief->GetMagicCaster(RE::MagicSystem::CastingSource::kOther);
		}
		if (!caster) {
			logger::warn("[QP] Pickpocket plant poison: no magic caster; planted poison has no spell effect.");
			return;
		}

		const std::int32_t doses = std::max(1, plantCount);
		for (std::int32_t i = 0; i < doses; ++i) {
			caster->CastSpellImmediate(
				poison,
				false,
				victim,
				1.0f,
				false,
				0.0f,
				thief);
		}

		RE::InventoryEntryData* victimEntry =
			FindTransferEntry(PickpocketInventoryKind::kSteal, thief, victim, planted, 1);

		const std::int32_t removeCount = victimEntry ? std::min(doses, victimEntry->countDelta) : 0;
		if (victimEntry && removeCount > 0) {
			const auto extraList = GetInventoryEntryExtraListForRemoval(victimEntry, removeCount, kIsViewingPickpocketTarget);
			const auto dropRef = victim->RemoveItem(
				planted,
				removeCount,
				RE::ITEM_REMOVE_REASON::kRemove,
				extraList,
				nullptr);
			planted->HandleRemoveItemFromContainer(victim);
			if (dropRef) {
				logger::warn(
					"[QP] Pickpocket plant poison: engine spawned world drop (ref {:08X}) while consuming poison from target.",
					QuickPocket::FormUtil::GetFormID(dropRef));
			}
		} else if (doses > 0) {
			const char* rn = planted->GetName();
			const char* miss = (rn && rn[0] != '\0') ? rn : "item";
			logger::warn(
				"[QP] Pickpocket plant poison: could not find '{}' (form {:08X}) on target {:08X} to consume after applying effects.",
				miss,
				planted->GetFormID(),
				victim->GetFormID());
		}

		const char* n = planted->GetName();
		const char* label = (n && n[0] != '\0') ? n : "item";
		logger::debug(
			"[QP] Pickpocket plant poison: applied '{}' x{} on target {:08X} (Poisoned perk); consumed {} from target.",
			label,
			doses,
			victim->GetFormID(),
			removeCount);
	}

	void QueuePlantedPoisonEffectTask(
		RE::ObjectRefHandle victimHandle,
		RE::FormID boundFormID,
		std::int32_t plantCount)
	{
		const auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return;
		}

		tasks->AddTask([victimHandle, boundFormID, plantCount]() {
			const auto thief = RE::PlayerCharacter::GetSingleton();
			const auto ref = victimHandle.get();
			const auto victim = ref ? ref->As<RE::Actor>() : nullptr;
			if (!thief || !victim) {
				return;
			}

			auto* obj = RE::TESForm::LookupByID<RE::TESBoundObject>(boundFormID);
			if (!obj) {
				return;
			}

			ApplyPlantedPoisonEffectsIfNeeded(thief, victim, obj, plantCount);
		});
	}

	void ExecutePickpocketTransfer(
		PickpocketInventoryKind kind,
		RE::PlayerCharacter* player,
		RE::Actor* victim,
		RE::FormID objectFormID,
		std::int32_t count)
	{
		if (!player || !victim) {
			logger::warn("[QP] Pickpocket transfer: thief or target actor invalid; refreshing menu.");
			QuickPocket::QuickLootBridge::RefreshLootMenu();
			return;
		}

		auto* obj = RE::TESForm::LookupByID<RE::TESBoundObject>(objectFormID);
		if (!obj) {
			logger::warn(
				"[QP] Pickpocket transfer: item form {:08X} is not loaded; skipping move.",
				objectFormID);
			QuickPocket::QuickLootBridge::RefreshLootMenu();
			return;
		}

		RE::InventoryEntryData* liveEntry = FindTransferEntry(kind, player, victim, obj, count);

		if (!liveEntry) {
			LogMissingTransferEntry(kind, obj, objectFormID, count);
			QuickPocket::QuickLootBridge::RefreshLootMenu();
			return;
		}

		switch (kind) {
		case PickpocketInventoryKind::kPlant: {
			const auto detection = victim->RequestDetectionLevel(player);
			logger::debug(
				"[QP] Pickpocket transfer (plant): inventory move (same frame as roll); target detection level {}.",
				detection);

			const auto extraList = GetInventoryEntryExtraListForRemoval(liveEntry, count, kIsViewingPickpocketTarget);
			// Match QuickLootIE ItemStack::Take when not stealing (kStoreInContainer).
			const auto dropRef = player->RemoveItem(
				obj,
				count,
				RE::ITEM_REMOVE_REASON::kStoreInContainer,
				extraList,
				victim);
			obj->HandleRemoveItemFromContainer(player);
			if (dropRef) {
				logger::warn(
					"[QP] Pickpocket transfer (plant): engine spawned a world drop (ref {:08X}) instead of merging stacks.",
					QuickPocket::FormUtil::GetFormID(dropRef));
			}
			QueuePlantedPoisonEffectTask(victim->GetHandle(), objectFormID, count);
			break;
		}
		case PickpocketInventoryKind::kSteal: {
			const auto owner = victim->GetOwner();
			const auto stealValue = victim->GetStealValue(
				liveEntry,
				static_cast<std::uint32_t>(count),
				true);

			player->AddPlayerAddItemEvent(obj, owner, victim, RE::AQUIRE_TYPE::kSteal);
			player->PlayPickUpSound(obj, true, false);

			const auto extraList = GetInventoryEntryExtraListForRemoval(liveEntry, count, kIsViewingPickpocketTarget);
			const auto dropRef = victim->RemoveItem(
				obj,
				count,
				RE::ITEM_REMOVE_REASON::kSteal,
				extraList,
				player);
			obj->HandleRemoveItemFromContainer(victim);

			if (obj->IsAmmo()) {
				victim->ClearExtraArrows();
			}

			player->StealAlarm(
				victim,
				obj,
				count,
				static_cast<std::int32_t>(stealValue),
				owner,
				true);

			if (dropRef) {
				logger::warn(
					"[QP] Pickpocket transfer (steal): engine spawned a world drop (ref {:08X}).",
					QuickPocket::FormUtil::GetFormID(dropRef));
			}
			break;
		}
		}

		QuickPocket::QuickLootBridge::RefreshLootMenu();
	}

	void QueuePickpocketTransferTask(
		PickpocketInventoryKind kind,
		RE::ObjectRefHandle victimHandle,
		RE::FormID objectFormID,
		std::int32_t count)
	{
		const auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			logger::warn("[QP] Pickpocket transfer: SKSE task queue unavailable; refreshing menu.");
			QuickPocket::QuickLootBridge::RefreshLootMenu();
			return;
		}

		tasks->AddTask([kind, victimHandle, objectFormID, count]() {
			const auto player = RE::PlayerCharacter::GetSingleton();
			const auto victimPtr = victimHandle.get();
			const auto victim = victimPtr ? victimPtr->As<RE::Actor>() : nullptr;
			ExecutePickpocketTransfer(kind, player, victim, objectFormID, count);
		});
	}

	void QueuePickpocketTransferIfValid(
		RE::Actor* victim,
		RE::FormID objectFormID,
		std::int32_t count,
		PickpocketInventoryKind kind)
	{
		if (!objectFormID) {
			QuickPocket::QuickLootBridge::RefreshLootMenu();
			return;
		}

		// Plant transfer runs on the same frame as AttemptPickpocket; poison apply is deferred to QueuePlantedPoisonEffectTask.
		if (kind == PickpocketInventoryKind::kPlant) {
			const auto player = RE::PlayerCharacter::GetSingleton();
			ExecutePickpocketTransfer(kind, player, victim, objectFormID, count);
			return;
		}

		QueuePickpocketTransferTask(kind, victim->GetHandle(), objectFormID, count);
	}

	void LogPickpocketAttemptContext(RE::Actor* victim, RE::PlayerCharacter* player, const RE::InventoryEntryData* entry)
	{
		if (!player || !victim) {
			return;
		}

		const auto sneaking = player->AsActorState()->IsSneaking();
		const auto detection = victim->RequestDetectionLevel(player);
		const auto objId = entry && entry->object ? entry->object->GetFormID() : 0;

		logger::debug(
			"[QP] Pickpocket context: target {:08X}, item {:08X} '{}', thief posture {}, target detection level {}",
			victim->GetFormID(),
			objId,
			GetEntryDisplayName(entry),
			sneaking ? "sneaking" : "upright",
			detection);
	}

	void LogPickpocketRoll(const char* actionLabel, bool success, std::int32_t count)
	{
		logger::debug(
			"[QP] Pickpocket native roll: {} x{} — {}",
			actionLabel,
			count,
			success ? "success" : "failed");
	}

	void LogPickpocketFailure(
		const char* modeLabel,
		RE::Actor* victim,
		RE::PlayerCharacter* player,
		const RE::InventoryEntryData* entry,
		std::int32_t count)
	{
		const auto form = entry && entry->object ? entry->object->GetFormID() : 0;
		const auto det = victim->RequestDetectionLevel(player);
		logger::info(
			"[QP] Pickpocket failed (target detected attempt): {} — '{}' x{} (form {:08X}) on target {:08X}, thief sneaking {}, target detection level {}",
			modeLabel,
			GetEntryDisplayName(entry),
			count,
			form,
			victim->GetFormID(),
			player->AsActorState()->IsSneaking() ? "yes" : "no",
			det);
	}

	void ClearItemStacks(RE::BSTArray<QuickLoot::API::ItemStack>& inventory)
	{
		for (auto& stack : inventory) {
			delete stack.entry;
		}
		inventory.clear();
	}

	bool PlayerHasPlantableItem()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto changes = player ? player->GetInventoryChanges(false) : nullptr;
		if (!changes || !changes->entryList) {
			return false;
		}

		for (const auto playerEntry : *changes->entryList) {
			if (QuickPocket::PickpocketRules::ShouldDisplayPlantEntry(playerEntry)) {
				return true;
			}
		}

		return false;
	}

	// Active hover pickpocket only; corpses fall through to vanilla QuickLoot.
	bool IsQuickPocketPickpocketContainer(RE::ObjectRefHandle container)
	{
		return QuickPocket::PickpocketRules::IsActivePickpocketTarget(container);
	}

	void PopulatePlantInventory(QuickLoot::API::ModifyInventoryEvent* e)
	{
		ClearItemStacks(e->inventory);

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto changes = player ? player->GetInventoryChanges(false) : nullptr;
		if (!changes || !changes->entryList) {
			return;
		}

		for (const auto playerEntry : *changes->entryList) {
			if (!QuickPocket::PickpocketRules::ShouldDisplayPlantEntry(playerEntry)) {
				continue;
			}

			e->inventory.push_back(QuickLoot::API::ItemStack{
				.entry = new RE::InventoryEntryData(*playerEntry),
				.dropRef = {}
			});
		}
	}

	void OnOpeningMenu(QuickLoot::API::OpeningLootMenuEvent* e)
	{
		QuickPocket::PlantModeController::Reset(false);

		if (!IsQuickPocketPickpocketContainer(e->container)) {
			return;
		}

		const auto currentTarget = QuickPocket::HoverController::GetCurrentTarget();
		if (!currentTarget || currentTarget != e->container) {
			logger::debug(
				"[QP] Opening menu target mismatch (container {:08X}, current target {:08X})",
				QuickPocket::FormUtil::GetFormID(e->container),
				QuickPocket::FormUtil::GetFormID(currentTarget));
		}
	}

	void OnModifyInventory(QuickLoot::API::ModifyInventoryEvent* e)
	{
		if (!IsQuickPocketPickpocketContainer(e->container)) {
			return;
		}

		if (QuickPocket::PlantModeController::IsEnabled()) {
			PopulatePlantInventory(e);
			return;
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto victim = e->container.get() ? e->container.get()->As<RE::Actor>() : nullptr;
		for (int i = 0; i < std::ssize(e->inventory); ++i) {
			const auto entry = e->inventory[i].entry;
			if (!QuickPocket::PickpocketRules::ShouldDisplayNpcEntry(entry, player) ||
				!QuickPocket::PickpocketRules::FindVictimEntryForSteal(victim, entry)) {
				delete entry;
				e->inventory.erase(&e->inventory[i]);
				--i;
			}
		}

		if (e->inventory.empty() && PlayerHasPlantableItem()) {
			logger::debug(
				"[QP] Steal list empty for target {:08X} (nothing to take); plantable items available in plant mode.",
				QuickPocket::FormUtil::GetFormID(e->container));
		}
	}

	void OnSelectItem(QuickLoot::API::SelectItemEvent* e)
	{
		if (!IsQuickPocketPickpocketContainer(e->container)) {
			g_selectedChance.store(-1);
			return;
		}

		const auto victim = e->container.get() ? e->container.get()->As<RE::Actor>() : nullptr;
		const auto displayedEntry = e->stack ? e->stack->entry : nullptr;
		const auto chanceEntry = QuickPocket::PlantModeController::IsEnabled() ?
			                         QuickPocket::PickpocketRules::FindPlayerEntryForPlanting(displayedEntry) :
			                         displayedEntry;
		g_selectedChance.store(QuickPocket::ChanceService::ComputeChance(victim, chanceEntry));
		logger::debug(
			"[QP] Pickpocket: selection on target {:08X}, chance {}%",
			QuickPocket::FormUtil::GetFormID(e->container),
			g_selectedChance.load());
	}

	void OnPopulateInfoBar(QuickLoot::API::PopulateInfoBarEvent* e)
	{
		if (!IsQuickPocketPickpocketContainer(e->container)) {
			return;
		}

		const auto plantMode = QuickPocket::PlantModeController::IsEnabled();
		const auto text = QuickPocket::ChanceService::FormatChance(g_selectedChance.load());
		e->result.push_back(std::format("Mode: {}", plantMode ? "Plant" : "Steal").c_str());
		e->result.push_back(std::format("{} chance: {}", plantMode ? "Plant" : "Steal", text).c_str());
	}

	void OnModifyButtonBar(QuickLoot::API::ModifyButtonBarEvent* e)
	{
		if (!e || !IsQuickPocketPickpocketContainer(e->container)) {
			return;
		}

		const auto plantMode = QuickPocket::PlantModeController::IsEnabled();

		for (int i = 0; i < std::ssize(e->buttons); ++i) {
			auto& button = e->buttons[i];
			button.stealing = true;

			if (button.action == QuickLoot::API::QuickLootAction::kTakeAll &&
				QuickPocket::QuickLootBridge::SupportsInputActionHook()) {
				button.label = "$qp_ToggleMode";
				continue;
			}

			// Search is not useful on a live pickpocket target.
			if (button.action == QuickLoot::API::QuickLootAction::kTransfer) {
				e->buttons.erase(&e->buttons[i]);
				--i;
				continue;
			}

			if (!plantMode) {
				continue;
			}

			switch (button.action) {
			case QuickLoot::API::QuickLootAction::kTake:
				button.label = "$Give";
				break;

			case QuickLoot::API::QuickLootAction::kTakeAll:
				button.label = "$GiveAll";
				break;

			default:
				break;
			}
		}
	}

	void OnInputAction(QuickLoot::API::InputActionEvent* e)
	{
		if (!e || e->action != QuickLoot::API::QuickLootAction::kTakeAll) {
			return;
		}

		if (!QuickPocket::PickpocketRules::IsPickpocketMenuContainer(e->container)) {
			return;
		}

		e->result = QuickLoot::API::HandleResult::kStop;
		logger::info("[QP] TakeAll intercepted for container {:08X}", QuickPocket::FormUtil::GetFormID(e->container));
		QuickPocket::PlantModeController::ToggleOnce(e->container);
	}

	void OnTakingItem(QuickLoot::API::TakingItemEvent* e)
	{
		if (!QuickPocket::PickpocketRules::IsActorContainer(e->container)) {
			return;
		}

		const auto victim = e->container.get() ? e->container.get()->As<RE::Actor>() : nullptr;
		if (!victim || victim->IsDead(false)) {
			return;
		}

		if (!QuickPocket::HoverController::AllowsTakeFromContainer(e->container)) {
			e->result = QuickLoot::API::HandleResult::kStop;
			logger::info(
				"[QP] Pickpocket: blocked take — loot target changed (container {:08X}, current crosshair {:08X}).",
				QuickPocket::FormUtil::GetFormID(e->container),
				QuickPocket::FormUtil::GetFormID(QuickPocket::HoverController::GetCurrentTarget()));
			return;
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			logger::warn(
				"[QP] Pickpocket: aborted — missing thief (container {:08X}, event actor {:08X}).",
				QuickPocket::FormUtil::GetFormID(e->container),
				e->actor ? e->actor->GetFormID() : 0);
			return;
		}

		const auto displayedEntry = e->stack ? e->stack->entry : nullptr;
		const bool plantMode = QuickPocket::PlantModeController::IsEnabled();
		e->result = QuickLoot::API::HandleResult::kStop;

		RE::InventoryEntryData* liveEntry = nullptr;
		if (plantMode) {
			liveEntry = QuickPocket::PickpocketRules::FindPlayerEntryForPlanting(displayedEntry);
			if (!liveEntry) {
				logger::info(
					"[QP] Plant: listed stack has no matching thief inventory entry (form {:08X}, count {}). Refreshing list.",
					displayedEntry && displayedEntry->object ? displayedEntry->object->GetFormID() : 0,
					displayedEntry ? displayedEntry->countDelta : 0);
				QuickPocket::QuickLootBridge::RefreshLootMenu();
				return;
			}
		} else {
			liveEntry = QuickPocket::PickpocketRules::FindVictimEntryForSteal(victim, displayedEntry);
			if (!liveEntry) {
				logger::info(
					"[QP] Steal: listed stack has no matching target inventory entry (form {:08X}, count {}). Refreshing list.",
					displayedEntry && displayedEntry->object ? displayedEntry->object->GetFormID() : 0,
					displayedEntry ? displayedEntry->countDelta : 0);
				QuickPocket::QuickLootBridge::RefreshLootMenu();
				return;
			}
		}

		const auto count = plantMode ? 1 : ClampTakeCount(liveEntry, displayedEntry);
		const auto kind = plantMode ? PickpocketInventoryKind::kPlant : PickpocketInventoryKind::kSteal;
		const char* actionLabel = plantMode ? "plant on target" : "steal from target";
		const char* failModeLabel = plantMode ? "Plant" : "Steal";

		LogPickpocketAttemptContext(victim, player, liveEntry);
		logger::debug(
			"[QP] Pickpocket: rolling {} — '{}' x{}",
			actionLabel,
			GetEntryDisplayName(liveEntry),
			count);

		const auto ok = player->AttemptPickpocket(victim, liveEntry, count, !plantMode);
		LogPickpocketRoll(actionLabel, ok, count);

		if (!ok) {
			LogPickpocketFailure(failModeLabel, victim, player, liveEntry, count);
			QuickPocket::QuickLootBridge::CloseLootMenu();
		} else {
			const auto objectFormID = liveEntry->object ? liveEntry->object->GetFormID() : 0;
			QueuePickpocketTransferIfValid(victim, objectFormID, count, kind);
		}
	}

	void OnTakeItem(QuickLoot::API::TakeItemEvent* e)
	{
		if (!e || !IsQuickPocketPickpocketContainer(e->container)) {
			return;
		}

		logger::debug(
			"[QP] Take item event: actor={:08X}, container={:08X}, plant mode={}",
			e->actor ? e->actor->GetFormID() : 0,
			QuickPocket::FormUtil::GetFormID(e->container),
			QuickPocket::PlantModeController::IsEnabled());

		// Intentionally empty: QLIE handles close-on-failed-pickpocket natively.
	}
}

void QuickPocket::EventHandlers::Install()
{
	logger::info("[QP] Installing QuickLoot API handlers");
	QuickLootBridge::RegisterHandlers(
		OnOpeningMenu,
		OnSelectItem,
		OnPopulateInfoBar,
		OnModifyButtonBar,
		OnInputAction,
		OnModifyInventory,
		OnTakingItem,
		OnTakeItem);
}
