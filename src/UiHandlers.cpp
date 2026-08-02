#include "QuickPocket/UiHandlers.h"

#include "QuickPocket/ChanceService.h"
#include "QuickPocket/FormUtil.h"
#include "QuickPocket/PickpocketRules.h"
#include "QuickPocket/PickpocketSession.h"
#include "QuickPocket/QuickLootBridge.h"

namespace
{
	std::atomic<int> g_selectedChance{ -1 };

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
}

void QuickPocket::UiHandlers::OnOpeningMenu(QuickLoot::API::OpeningLootMenuEvent* e)
{
	PickpocketSession::ResetPlantMode(false);

	if (!PickpocketSession::CanAct(e->container)) {
		return;
	}

	const auto currentTarget = PickpocketSession::GetCurrentTarget();
	if (!currentTarget || currentTarget != e->container) {
		logger::debug(
			"[QP] Opening menu target mismatch (container {:08X}, current target {:08X})",
			FormUtil::GetFormID(e->container),
			FormUtil::GetFormID(currentTarget));
	}
}

void QuickPocket::UiHandlers::OnModifyInventory(QuickLoot::API::ModifyInventoryEvent* e)
{
	if (!PickpocketSession::CanAct(e->container)) {
		return;
	}

	if (PickpocketSession::IsPlantMode()) {
		PopulatePlantInventory(e);
		return;
	}

	const auto player = RE::PlayerCharacter::GetSingleton();
	const auto victim = e->container.get() ? e->container.get()->As<RE::Actor>() : nullptr;
	for (int i = 0; i < std::ssize(e->inventory); ++i) {
		const auto entry = e->inventory[i].entry;
		if (!PickpocketRules::ShouldDisplayStealEntry(entry, player) ||
			!PickpocketRules::FindVictimEntryForSteal(victim, entry)) {
			delete entry;
			e->inventory.erase(&e->inventory[i]);
			--i;
		}
	}

	if (e->inventory.empty() && PlayerHasPlantableItem()) {
		logger::debug(
			"[QP] Steal list empty for target {:08X} (nothing to take); plantable items available in plant mode.",
			FormUtil::GetFormID(e->container));
	}
}

void QuickPocket::UiHandlers::OnSelectItem(QuickLoot::API::SelectItemEvent* e)
{
	if (!PickpocketSession::CanAct(e->container)) {
		g_selectedChance.store(-1);
		return;
	}

	const auto victim = e->container.get() ? e->container.get()->As<RE::Actor>() : nullptr;
	const auto displayedEntry = e->stack ? e->stack->entry : nullptr;
	const auto chanceEntry = PickpocketSession::IsPlantMode() ?
		                         PickpocketRules::FindPlayerEntryForPlanting(displayedEntry) :
		                         displayedEntry;
	g_selectedChance.store(ChanceService::ComputeChance(victim, chanceEntry));
	logger::debug(
		"[QP] Pickpocket: selection on target {:08X}, chance {}%",
		FormUtil::GetFormID(e->container),
		g_selectedChance.load());
}

void QuickPocket::UiHandlers::OnPopulateInfoBar(QuickLoot::API::PopulateInfoBarEvent* e)
{
	if (!PickpocketSession::CanAct(e->container)) {
		return;
	}

	const auto plantMode = PickpocketSession::IsPlantMode();
	const auto text = ChanceService::FormatChance(g_selectedChance.load());
	e->result.push_back(std::format("Mode: {}", plantMode ? "Plant" : "Steal").c_str());
	e->result.push_back(std::format("{} chance: {}", plantMode ? "Plant" : "Steal", text).c_str());
}

void QuickPocket::UiHandlers::OnModifyButtonBar(QuickLoot::API::ModifyButtonBarEvent* e)
{
	if (!e || !PickpocketSession::CanAct(e->container)) {
		return;
	}

	const auto plantMode = PickpocketSession::IsPlantMode();

	for (int i = 0; i < std::ssize(e->buttons); ++i) {
		auto& button = e->buttons[i];
		button.stealing = true;

		if (button.action == QuickLoot::API::QuickLootAction::kTakeAll &&
			QuickLootBridge::SupportsInputActionHook()) {
			button.label = "$qp_ToggleMode";
			continue;
		}

		if (button.action == QuickLoot::API::QuickLootAction::kTransfer) {
			e->buttons.erase(&e->buttons[i]);
			--i;
			continue;
		}

		if (!plantMode) {
			continue;
		}

		if (button.action == QuickLoot::API::QuickLootAction::kTake) {
			button.label = "$qp_Plant";
		}
	}
}

void QuickPocket::UiHandlers::OnInputAction(QuickLoot::API::InputActionEvent* e)
{
	if (!e || e->action != QuickLoot::API::QuickLootAction::kTakeAll) {
		return;
	}

	if (!PickpocketSession::CanTogglePlant(e->container)) {
		return;
	}

	e->result = QuickLoot::API::HandleResult::kStop;
	logger::info("[QP] TakeAll intercepted for container {:08X}", FormUtil::GetFormID(e->container));
	PickpocketSession::TogglePlant();
}

void QuickPocket::UiHandlers::OnTakeItem(QuickLoot::API::TakeItemEvent* e)
{
	if (!e || !PickpocketSession::CanAct(e->container)) {
		return;
	}

	logger::debug(
		"[QP] Take item event: actor={:08X}, container={:08X}, plant mode={}",
		e->actor ? e->actor->GetFormID() : 0,
		FormUtil::GetFormID(e->container),
		PickpocketSession::IsPlantMode());
}
