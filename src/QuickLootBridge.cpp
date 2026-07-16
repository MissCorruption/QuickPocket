#include "QuickPocket/QuickLootBridge.h"

#include "QuickPocket/FormUtil.h"

#include "SKSE/API.h"

namespace QuickPocket
{
	bool QuickLootBridge::Initialize()
	{
		const auto ready = QuickLoot::API::QuickLootAPI::Init(PluginName, QuickLoot::API::ApiVersion::kV21);
		logger::info("[QP] QuickLoot bridge initialized: interface={}", ready ? "ready" : "null");
		return ready;
	}

	bool QuickLootBridge::IsReady()
	{
		return QuickLoot::API::QuickLootAPI::IsReady(QuickLoot::API::ApiVersion::kV20);
	}

	bool QuickLootBridge::IsCompatible()
	{
		return QuickLoot::API::QuickLootAPI::IsReady(QuickLoot::API::ApiVersion::kV21);
	}

	bool QuickLootBridge::SupportsInputActionHook()
	{
		return IsCompatible();
	}

	void QuickLootBridge::RegisterHandlers(
		QuickLoot::API::OpeningLootMenuHandler openingHandler,
		QuickLoot::API::SelectItemHandler selectHandler,
		QuickLoot::API::PopulateInfoBarHandler infoBarHandler,
		QuickLoot::API::ModifyButtonBarHandler modifyButtonBarHandler,
		QuickLoot::API::InputActionHandler inputActionHandler,
		QuickLoot::API::ModifyInventoryHandler modifyInventoryHandler,
		QuickLoot::API::TakingItemHandler takingItemHandler,
		QuickLoot::API::TakeItemHandler takeItemHandler)
	{
		if (!IsCompatible()) {
			logger::warn("[QP] RegisterHandlers skipped: interface not ready");
			return;
		}

		logger::debug("[QP] Registering QuickLoot API handlers");
		QuickLoot::API::QuickLootAPI::RegisterOpeningLootMenuHandler(openingHandler);
		QuickLoot::API::QuickLootAPI::RegisterSelectItemHandler(selectHandler);
		QuickLoot::API::QuickLootAPI::RegisterPopulateInfoBarHandler(infoBarHandler);
		QuickLoot::API::QuickLootAPI::RegisterModifyButtonBarHandler(modifyButtonBarHandler);
		if (inputActionHandler) {
			QuickLoot::API::QuickLootAPI::RegisterInputActionHandler(inputActionHandler);
		}
		QuickLoot::API::QuickLootAPI::RegisterModifyInventoryHandler(modifyInventoryHandler);
		QuickLoot::API::QuickLootAPI::RegisterTakingItemHandler(takingItemHandler);
		QuickLoot::API::QuickLootAPI::RegisterTakeItemHandler(takeItemHandler);
	}

	void QuickLootBridge::ForceContainer(RE::ObjectRefHandle container)
	{
		if (!IsReady()) {
			return;
		}

		const auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return;
		}

		tasks->AddTask([container = std::move(container)]() mutable {
			if (!IsReady()) {
				return;
			}

			logger::debug("[QP] ForceContainer {:08X}", QuickPocket::FormUtil::GetFormID(container));
			QuickLoot::API::QuickLootAPI::ForceCurrentContainer(std::move(container));
		});
	}

	void QuickLootBridge::ClearForcedContainer()
	{
		if (!IsReady()) {
			return;
		}

		const auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return;
		}

		tasks->AddTask([] {
			if (!IsReady()) {
				return;
			}
			logger::debug("[QP] ClearForcedContainer");
			QuickLoot::API::QuickLootAPI::ClearForcedContainer();
		});
	}

	void QuickLootBridge::CloseLootMenu()
	{
		if (!IsReady()) {
			return;
		}

		const auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return;
		}

		tasks->AddTask([] {
			if (!IsReady()) {
				return;
			}
			logger::debug("[QP] CloseLootMenu");
			QuickLoot::API::QuickLootAPI::CloseLootMenu();
		});
	}

	void QuickLootBridge::RefreshLootMenu()
	{
		if (!IsReady()) {
			return;
		}

		const auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return;
		}

		tasks->AddTask([] {
			if (!IsReady()) {
				return;
			}
			logger::debug("[QP] RefreshLootMenu");
			QuickLoot::API::QuickLootAPI::RefreshLootMenu();
		});
	}
}
