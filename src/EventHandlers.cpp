#include "QuickPocket/EventHandlers.h"

#include "QuickPocket/QuickLootBridge.h"
#include "QuickPocket/Transfer.h"
#include "QuickPocket/UiHandlers.h"

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
