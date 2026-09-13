#include "PlantInput.h"

#include "Config.h"
#include "PickpocketSession.h"

namespace QuickPocket
{
	namespace
	{
		// VR plant toggle is not handled here. QuickLootIE already maps main-hand B →
		// kTakeAll on IsDown (after NormalizeDeviceKey); UiHandlers::OnInputAction
		// routes that into PickpocketSession::TogglePlant.
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

				const auto keyCode = Config::Get().plantModeToggleKey;
				if (keyCode < 0) {
					return RE::BSEventNotifyControl::kContinue;
				}

				for (auto current = *event; current; current = current->next) {
					const auto buttonEvent = current->AsButtonEvent();
					if (!buttonEvent ||
						buttonEvent->GetDevice() != RE::INPUT_DEVICE::kKeyboard ||
						!buttonEvent->IsDown() ||
						buttonEvent->GetIDCode() != static_cast<std::uint32_t>(keyCode)) {
						continue;
					}

					if (PickpocketSession::TogglePlant() || PickpocketSession::CanTogglePlant()) {
						return RE::BSEventNotifyControl::kStop;
					}
				}

				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	void PlantInput::Install()
	{
		const auto inputManager = RE::BSInputDeviceManager::GetSingleton();
		if (!inputManager) {
			return;
		}

		inputManager->AddEventSink(ToggleInputSink::GetSingleton());
		logger::info("[QP] Plant mode input listener installed");
	}
}
