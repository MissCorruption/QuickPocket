#include "EventHandlers.h"

#include "QuickLootBridge.h"
#include "Transfer.h"
#include "UiHandlers.h"

void QuickPocket::EventHandlers::Install()
{
	logger::info("[QP] Installing QuickLoot API handlers");
	QuickLootBridge::RegisterHandlers(
		UiHandlers::OnOpeningMenu,
		UiHandlers::OnSelectItem,
		UiHandlers::OnPopulateInfoBar,
		UiHandlers::OnModifyButtonBar,
		UiHandlers::OnInputAction,
		UiHandlers::OnModifyInventory,
		Transfer::OnTakingItem,
		UiHandlers::OnTakeItem);
}
