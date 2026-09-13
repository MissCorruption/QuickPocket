#include "PickpocketSession.h"

#include "FormUtil.h"
#include "PickpocketRules.h"
#include "QuickLootBridge.h"

using namespace std::chrono_literals;

namespace QuickPocket
{
	namespace
	{
		inline constexpr auto kLiveChanceRefreshInterval = 100ms;
		inline constexpr std::uint32_t kToggleDebounceMs = 250;

		bool IsLootMenuOpen()
		{
			const auto ui = RE::UI::GetSingleton();
			return ui && ui->IsMenuOpen("LootMenu");
		}
	}

	bool PickpocketSession::IsActive()
	{
		return _active.load();
	}

	bool PickpocketSession::IsPlantMode()
	{
		return _plantMode.load();
	}

	bool PickpocketSession::CanAct(RE::ObjectRefHandle container)
	{
		return PickpocketRules::CanAct(container);
	}

	bool PickpocketSession::CanTogglePlant(RE::ObjectRefHandle container)
	{
		return PickpocketRules::CanTogglePlant(container);
	}

	RE::ObjectRefHandle PickpocketSession::GetVictim()
	{
		std::scoped_lock lock(_mutex);
		return _victim ? _victim : _currentTarget;
	}

	RE::ObjectRefHandle PickpocketSession::GetCurrentTarget()
	{
		std::scoped_lock lock(_mutex);
		return _currentTarget;
	}

	void PickpocketSession::SetCurrentTarget(RE::ObjectRefHandle target)
	{
		std::scoped_lock lock(_mutex);
		_currentTarget = std::move(target);
	}

	void PickpocketSession::ClearCurrentTarget()
	{
		std::scoped_lock lock(_mutex);
		_currentTarget.reset();
	}

	void PickpocketSession::Begin(RE::ObjectRefHandle victim)
	{
		std::scoped_lock lock(_mutex);
		if (!victim) {
			return;
		}

		const auto targetChanged = !_victim || _victim != victim;
		const auto wasActive = _active.load();

		QuickLootBridge::ForceContainer(victim);
		QuickLootBridge::RefreshLootMenu();

		_victim = victim;
		_active.store(true);
		_nextLiveRefresh = std::chrono::steady_clock::now() + kLiveChanceRefreshInterval;

		if (!wasActive || targetChanged) {
			logger::debug("[QP] Session begin for {:08X}", FormUtil::GetFormID(victim));
		}
	}

	void PickpocketSession::End(bool closeMenu)
	{
		std::scoped_lock lock(_mutex);

		const auto wasActive = _active.exchange(false);
		_victim.reset();
		_nextLiveRefresh = {};
		ResetPlantMode(false);

		if (wasActive) {
			QuickLootBridge::ClearForcedContainer();
			logger::debug("[QP] Session end");
		}

		if (closeMenu && wasActive) {
			QuickLootBridge::CloseLootMenu();
		}
	}

	void PickpocketSession::RefreshIfNeeded()
	{
		std::scoped_lock lock(_mutex);
		if (!_active.load() || !_victim) {
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		if (now >= _nextLiveRefresh) {
			QuickLootBridge::RefreshLootMenu();
			_nextLiveRefresh = now + kLiveChanceRefreshInterval;
		}
	}

	bool PickpocketSession::TogglePlant()
	{
		const auto nowMs = static_cast<std::uint32_t>(GetTickCount64());
		if (nowMs - _lastToggleMs < kToggleDebounceMs) {
			return false;
		}

		const auto container = GetVictim();
		if (!CanTogglePlant(container)) {
			logger::info(
				"[QP] Plant mode toggle rejected for container {:08X} (menu open={}, session={})",
				FormUtil::GetFormID(container),
				IsLootMenuOpen(),
				IsActive());
			return false;
		}

		SetPlantMode(!IsPlantMode(), true);
		_lastToggleMs = nowMs;
		return true;
	}

	void PickpocketSession::SetPlantMode(bool enabled, bool refreshMenu)
	{
		const auto old = _plantMode.exchange(enabled);
		if (old == enabled) {
			return;
		}

		logger::info("[QP] Plant mode {}", enabled ? "enabled" : "disabled");
		if (refreshMenu && (IsActive() || IsLootMenuOpen())) {
			QuickLootBridge::RefreshLootMenu();
		}
	}

	void PickpocketSession::ResetPlantMode(bool refreshMenu)
	{
		if (!IsPlantMode()) {
			return;
		}

		SetPlantMode(false, refreshMenu);
	}
}
