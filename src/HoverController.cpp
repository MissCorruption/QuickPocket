#include "QuickPocket/HoverController.h"

#include "QuickPocket/Config.h"
#include "QuickPocket/FormUtil.h"
#include "QuickPocket/PlantModeController.h"
#include "QuickPocket/QuickLootBridge.h"

using namespace std::chrono_literals;

namespace QuickPocket
{
	namespace
	{
		inline constexpr auto kLiveChanceRefreshInterval = 100ms;
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

		class CrosshairRefSink : public RE::BSTEventSink<SKSE::CrosshairRefEvent>
		{
		public:
			static CrosshairRefSink* GetSingleton()
			{
				static CrosshairRefSink instance;
				return &instance;
			}

			RE::BSEventNotifyControl ProcessEvent(
				const SKSE::CrosshairRefEvent* event,
				RE::BSTEventSource<SKSE::CrosshairRefEvent>*) override
			{
				std::scoped_lock lock(_lock);
				if (event && event->crosshairRef) {
					_handle = event->crosshairRef->CreateRefHandle();
				} else {
					_handle.reset();
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			RE::ObjectRefHandle GetCurrentHandle()
			{
				std::scoped_lock lock(_lock);
				return _handle;
			}

		private:
			std::mutex _lock{};
			RE::ObjectRefHandle _handle{};
		};

		RE::ObjectRefHandle GetCurrentCrosshairTarget(RE::CrosshairPickData* pickData)
		{
			if (!REL::Module::IsVR()) {
				return CrosshairRefSink::GetSingleton()->GetCurrentHandle();
			}

			if (!pickData) {
				return {};
			}

			auto getSlotTarget = [pickData](RE::VR_DEVICE slot) {
				auto targetRef = pickData->grabPickRef[slot];
				if (!targetRef) {
					targetRef = pickData->targetActor[slot];
				}
				if (!targetRef) {
					targetRef = pickData->target[slot];
				}
				return targetRef;
			};

			const auto player = RE::PlayerCharacter::GetSingleton();
			const auto rightHanded = player && player->GetVRPlayerRuntimeData().isRightHandMainHand;
			const auto hand = rightHanded ? RE::VR_DEVICE::kRightController : RE::VR_DEVICE::kLeftController;

			if (const auto targetRef = getSlotTarget(hand)) {
				return targetRef;
			}

			// Fallback to any other slot in case hand preference has no target yet.
			for (const auto slot : { RE::VR_DEVICE::kLeftController, RE::VR_DEVICE::kRightController, RE::VR_DEVICE::kHeadset }) {
				if (slot == hand) {
					continue;
				}
				if (const auto targetRef = getSlotTarget(slot)) {
					return targetRef;
				}
			}

			return {};
		}
	}

	void HoverController::Start()
	{
		if (_worker.joinable()) {
			return;
		}

		if (!REL::Module::IsVR()) {
			SKSE::GetCrosshairRefEventSource()->AddEventSink(CrosshairRefSink::GetSingleton());
			logger::info("[QP] Installed crosshair event sink (non-VR).");
		}

		logger::info("[QP] Hover controller started.");
		_worker = std::jthread([](std::stop_token stopToken) {
			Run(stopToken);
		});
	}

	void HoverController::Stop()
	{
		if (_worker.joinable()) {
			logger::info("[QP] Hover controller stopping.");
			_worker.request_stop();
			_worker.join();
		}
	}

	bool HoverController::AllowsTakeFromContainer(RE::ObjectRefHandle container)
	{
		if (!container) {
			return false;
		}

		std::scoped_lock lock(_stateMutex);
		return IsCurrentTargetLocked(container) || (_lastForcedTarget && _lastForcedTarget == container);
	}

	bool HoverController::IsCurrentTargetLocked(RE::ObjectRefHandle container)
	{
		return _currentTarget && _currentTarget == container;
	}

	bool HoverController::IsForcedPickpocketContainer(RE::ObjectRefHandle container)
	{
		if (!container) {
			return false;
		}

		std::scoped_lock lock(_stateMutex);
		return _forcedContainerActive.load() && _lastForcedTarget && _lastForcedTarget == container;
	}

	bool HoverController::IsPickpocketSessionActive()
	{
		return _forcedContainerActive.load();
	}

	RE::ObjectRefHandle HoverController::GetActivePickpocketContainer()
	{
		std::scoped_lock lock(_stateMutex);
		if (_lastForcedTarget) {
			return _lastForcedTarget;
		}

		return _currentTarget;
	}

	RE::ObjectRefHandle HoverController::GetCurrentTarget()
	{
		std::scoped_lock lock(_stateMutex);
		return _currentTarget;
	}

	RE::Actor* HoverController::GetCurrentVictim()
	{
		std::scoped_lock lock(_stateMutex);
		const auto target = _currentTarget.get();
		return target ? target->As<RE::Actor>() : nullptr;
	}

	void HoverController::Run(std::stop_token stopToken)
	{
		while (!stopToken.stop_requested()) {
			Tick();
			std::this_thread::sleep_for(33ms);
		}
	}

	void HoverController::ClearForcedContainerIfSet()
	{
		if (!_forcedContainerActive.exchange(false)) {
			return;
		}
		_nextLiveRefresh = {};
		QuickLootBridge::ClearForcedContainer();
	}

	void HoverController::Tick()
	{
		std::scoped_lock lock(_stateMutex);

		const auto wasEligible = _eligible.load();
		const auto config = Config::Get();
		if (!config.enabled || !QuickLootBridge::IsReady()) {
			_eligible.store(false);
			_currentTarget.reset();
			_lastForcedTarget.reset();
			_nextLiveRefresh = {};
			ClearForcedContainerIfSet();
			PlantModeController::Reset(false);
			return;
		}

		const auto pickData = RE::CrosshairPickData::GetSingleton();
		const auto targetHandle = GetCurrentCrosshairTarget(pickData);
		auto target = targetHandle.get().get();
		if (!target) {
			_eligible.store(false);
			_currentTarget.reset();
			_lastForcedTarget.reset();
			_nextLiveRefresh = {};
			ClearForcedContainerIfSet();
			PlantModeController::Reset(false);
			if (wasEligible) {
				logger::debug("[QP] Lost crosshair target, clearing forced container");
			}
			return;
		}

		if (!_currentTarget || _currentTarget.get().get() != target) {
			logger::debug("[QP] Crosshair target changed to {:08X}", QuickPocket::FormUtil::GetFormID(target));
			_currentTarget = target->CreateRefHandle();
		}

		if (!IsTargetEligible(target)) {
			_eligible.store(false);
			_lastForcedTarget.reset();
			_nextLiveRefresh = {};
			ClearForcedContainerIfSet();
			PlantModeController::Reset(false);
			if (wasEligible) {
				QuickLootBridge::CloseLootMenu();
			}
			if (wasEligible) {
				logger::debug("[QP] Target {:08X} is no longer eligible", QuickPocket::FormUtil::GetFormID(target));
			}
			return;
		}

		_eligible.store(true);
		if (!wasEligible) {
			logger::debug("[QP] Target {:08X} became eligible", QuickPocket::FormUtil::GetFormID(target));
		}

		const auto becameEligible = !wasEligible;
		const auto targetChanged = !_lastForcedTarget || _lastForcedTarget != _currentTarget;
		if (becameEligible || targetChanged) {
			QuickLootBridge::ForceContainer(_currentTarget);
			QuickLootBridge::RefreshLootMenu();
			_lastForcedTarget = _currentTarget;
			_forcedContainerActive.store(true);
			_nextLiveRefresh = std::chrono::steady_clock::now() + kLiveChanceRefreshInterval;
			return;
		}

		if (_forcedContainerActive.load() && _lastForcedTarget == _currentTarget) {
			const auto now = std::chrono::steady_clock::now();
			if (now >= _nextLiveRefresh) {
				QuickLootBridge::RefreshLootMenu();
				_nextLiveRefresh = now + kLiveChanceRefreshInterval;
			}
		}
	}

	bool HoverController::IsTargetEligible(RE::TESObjectREFR* target)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
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
}
