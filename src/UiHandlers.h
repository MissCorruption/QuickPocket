#pragma once

#include "QuickLootAPI.h"

namespace QuickPocket
{
	class UiHandlers
	{
	public:
		static void OnOpeningMenu(QuickLoot::API::OpeningLootMenuEvent* e);
		static void OnModifyInventory(QuickLoot::API::ModifyInventoryEvent* e);
		static void OnSelectItem(QuickLoot::API::SelectItemEvent* e);
		static void OnPopulateInfoBar(QuickLoot::API::PopulateInfoBarEvent* e);
		static void OnModifyButtonBar(QuickLoot::API::ModifyButtonBarEvent* e);
		static void OnInputAction(QuickLoot::API::InputActionEvent* e);
		static void OnTakeItem(QuickLoot::API::TakeItemEvent* e);
	};
}
