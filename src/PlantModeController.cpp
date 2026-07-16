#include "QuickPocket/PlantModeController.h"

#include "QuickPocket/Config.h"
#include "QuickPocket/FormUtil.h"
#include "QuickPocket/HoverController.h"
#include "QuickPocket/PickpocketRules.h"
#include "QuickPocket/QuickLootBridge.h"

#ifdef ENABLE_SKYRIM_VR
#	include "RE/B/BSOpenVRControllerDevice.h"
#endif

namespace QuickPocket
{
	namespace
	{
		inline constexpr auto LOOT_MENU_NAME = "LootMenu";
		bool ShouldConsumeToggleInput();

		RE::Actor* GetTargetActor(RE::ObjectRefHandle handle)
		{
			const auto ref = handle.get();
			return ref ? ref->As<RE::Actor>() : nullptr;
		}

		bool IsLootMenuOpen()
		{
			const auto ui = RE::UI::GetSingleton();
			return ui && ui->IsMenuOpen(LOOT_MENU_NAME);
		}

		bool IsPickpocketMenuActive()
		{
			return IsLootMenuOpen() || HoverController::IsPickpocketSessionActive();
		}

#ifdef ENABLE_SKYRIM_VR
		bool IsVrWandDevice(RE::INPUT_DEVICE device)
		{
			return RE::BSOpenVRControllerDevice::IsPrimaryController(device) ||
			       RE::BSOpenVRControllerDevice::IsSecondaryController(device);
		}

		bool IsVrBButton(const RE::ButtonEvent* buttonEvent)
		{
			return buttonEvent && RE::BSOpenVRControllerDevice::IsBButton(buttonEvent->GetIDCode());
		}

		bool TryToggleFromVrButton(const RE::ButtonEvent* buttonEvent)
		{
			if (!REL::Module::IsVR() || !buttonEvent || !IsPickpocketMenuActive()) {
				return false;
			}

			if (!IsVrWandDevice(buttonEvent->GetDevice()) || !IsVrBButton(buttonEvent)) {
				return false;
			}

			static bool s_vrBTogglePending = false;

			if (buttonEvent->IsDown() || (buttonEvent->IsPressed() && !buttonEvent->IsHeld())) {
				s_vrBTogglePending = true;
				return true;
			}

			if (buttonEvent->IsUp()) {
				const auto container = HoverController::GetActivePickpocketContainer();
				logger::info(
					"[QP] VR B/Y release (device={}, key={}) on container {:08X}",
					static_cast<std::uint32_t>(buttonEvent->GetDevice()),
					buttonEvent->GetIDCode(),
					FormUtil::GetFormID(container));
				PlantModeController::ToggleOnce(container);
				s_vrBTogglePending = false;
				return true;
			}

			if (buttonEvent->IsHeld() || buttonEvent->IsPressed()) {
				return s_vrBTogglePending;
			}

			return false;
		}
#endif

		class ToggleInputSink final : public RE::BSTEventSink<RE::InputEvent*>
		{
		public:
			static ToggleInputSink* GetSingleton()
			{
				static ToggleInputSink instance;
				return &instance;
			}

			RE::BSEventNotifyControl ProcessEvent(
				RE::InputEvent* const* event,
				RE::BSTEventSource<RE::InputEvent*>*) override
			{
				if (!event || !*event) {
					return RE::BSEventNotifyControl::kContinue;
				}

				for (auto current = *event; current; current = current->next) {
#ifdef ENABLE_SKYRIM_VR
					if (TryToggleFromVrButton(current->AsButtonEvent())) {
						return RE::BSEventNotifyControl::kStop;
					}
#endif

					const auto keyCode = Config::Get().plantModeToggleKey;
					if (keyCode < 0) {
						continue;
					}
					const auto buttonEvent = current->AsButtonEvent();
					if (!buttonEvent ||
						buttonEvent->GetDevice() != RE::INPUT_DEVICE::kKeyboard ||
						!buttonEvent->IsDown() ||
						buttonEvent->GetIDCode() != static_cast<std::uint32_t>(keyCode)) {
						continue;
					}

					const auto toggled = PlantModeController::ToggleOnce(HoverController::GetActivePickpocketContainer());
					if (toggled || ShouldConsumeToggleInput()) {
						return RE::BSEventNotifyControl::kStop;
					}
				}

				return RE::BSEventNotifyControl::kContinue;
			}
		};

		bool ShouldConsumeToggleInput()
		{
			if (!IsPickpocketMenuActive()) {
				return false;
			}

			return GetTargetActor(HoverController::GetActivePickpocketContainer()) != nullptr;
		}
	}

	void PlantModeController::Install()
	{
		const auto inputManager = RE::BSInputDeviceManager::GetSingleton();
		if (!inputManager) {
			return;
		}

		inputManager->AddEventSink(ToggleInputSink::GetSingleton());
		logger::info("[QP] Plant mode input listener installed");
	}

	bool PlantModeController::IsEnabled()
	{
		return _enabled.load();
	}

	bool PlantModeController::Toggle()
	{
		return Toggle(HoverController::GetActivePickpocketContainer());
	}

	bool PlantModeController::Toggle(RE::ObjectRefHandle container)
	{
		if (!CanToggle(container)) {
			logger::info(
				"[QP] Plant mode toggle rejected for container {:08X} (menu open={}, session={})",
				FormUtil::GetFormID(container),
				IsLootMenuOpen(),
				HoverController::IsPickpocketSessionActive());
			return false;
		}

		SetEnabled(!IsEnabled(), true);
		return true;
	}

	bool PlantModeController::ToggleOnce(RE::ObjectRefHandle container)
	{
		static std::uint32_t s_lastToggleMs = 0;
		const auto nowMs = static_cast<std::uint32_t>(GetTickCount64());
		if (nowMs - s_lastToggleMs < 100) {
			return false;
		}

		if (!Toggle(container)) {
			return false;
		}

		s_lastToggleMs = nowMs;
		return true;
	}

	void PlantModeController::Reset(bool refreshMenu)
	{
		if (!IsEnabled()) {
			return;
		}

		SetEnabled(false, refreshMenu);
	}

	void PlantModeController::EnableForEmptyStealList(bool refreshMenu)
	{
		SetEnabled(true, refreshMenu);
	}

	bool PlantModeController::CanToggle()
	{
		return CanToggle(HoverController::GetActivePickpocketContainer());
	}

	bool PlantModeController::CanToggle(RE::ObjectRefHandle container)
	{
		if (!IsPickpocketMenuActive()) {
			return false;
		}

		if (!container) {
			container = HoverController::GetActivePickpocketContainer();
		}

		return PickpocketRules::IsPickpocketMenuContainer(container);
	}

	void PlantModeController::SetEnabled(bool enabled, bool refreshMenu)
	{
		const auto old = _enabled.exchange(enabled);
		if (old == enabled) {
			return;
		}

		logger::info("[QP] Plant mode {}", enabled ? "enabled" : "disabled");
		if (refreshMenu && IsPickpocketMenuActive()) {
			QuickLootBridge::RefreshLootMenu();
		}
	}
}
