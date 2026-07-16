#pragma once

#include "QuickPocket/QuickLootAPI.h"

namespace QuickPocket
{
	class QuickLootBridge
	{
	public:
		static bool Initialize();
		static bool IsReady();
		static bool IsCompatible();
		static bool SupportsInputActionHook();
		static void RegisterHandlers(
			QuickLoot::API::OpeningLootMenuHandler openingHandler,
			QuickLoot::API::SelectItemHandler selectHandler,
			QuickLoot::API::PopulateInfoBarHandler infoBarHandler,
			QuickLoot::API::ModifyButtonBarHandler modifyButtonBarHandler,
			QuickLoot::API::InputActionHandler inputActionHandler,
			QuickLoot::API::ModifyInventoryHandler modifyInventoryHandler,
			QuickLoot::API::TakingItemHandler takingItemHandler,
			QuickLoot::API::TakeItemHandler takeItemHandler);

		static void ForceContainer(RE::ObjectRefHandle container);
		static void ClearForcedContainer();
		static void CloseLootMenu();
		static void RefreshLootMenu();

	private:
		static inline constexpr auto PluginName = "QuickPocket";
	};
}
