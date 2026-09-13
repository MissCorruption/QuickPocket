#include "Targeting.h"

#include "Config.h"
#include "FormUtil.h"
#include "PickpocketRules.h"
#include "PickpocketSession.h"
#include "QuickLootBridge.h"

#include <chrono>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

namespace QuickPocket
{
	namespace
	{
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
			const auto vrData = player ? player->GetVRPlayerRuntimeData() : nullptr;
			const auto rightHanded = vrData && vrData->isRightHandMainHand;
			const auto hand = rightHanded ? RE::VR_DEVICE::kRightController : RE::VR_DEVICE::kLeftController;

			if (const auto targetRef = getSlotTarget(hand)) {
				return targetRef;
			}

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

	void Targeting::Start()
	{
		if (_worker.joinable()) {
			return;
		}

		if (!REL::Module::IsVR()) {
			SKSE::GetCrosshairRefEventSource()->AddEventSink(CrosshairRefSink::GetSingleton());
			logger::info("[QP] Installed crosshair event sink (non-VR).");
		}

		logger::info("[QP] Targeting started.");
		_worker = std::jthread([](std::stop_token stopToken) {
			Run(stopToken);
		});
	}

	void Targeting::Stop()
	{
		if (_worker.joinable()) {
			logger::info("[QP] Targeting stopping.");
			_worker.request_stop();
			_worker.join();
		}
	}

	void Targeting::Run(std::stop_token stopToken)
	{
		while (!stopToken.stop_requested()) {
			if (const auto* tasks = SKSE::GetTaskInterface()) {
				bool expected = false;
				if (_tickQueued.compare_exchange_strong(expected, true)) {
					tasks->AddTask([] {
						_tickQueued.store(false, std::memory_order_relaxed);
						Tick();
					});
				}
			}
			std::this_thread::sleep_for(33ms);
		}
	}

	void Targeting::Tick()
	{
		const auto config = Config::Get();
		if (!config.enabled || !QuickLootBridge::IsReady()) {
			PickpocketSession::ClearCurrentTarget();
			PickpocketSession::End(false);
			return;
		}

		const auto pickData = RE::CrosshairPickData::GetSingleton();
		const auto targetHandle = GetCurrentCrosshairTarget(pickData);
		auto* target = targetHandle.get().get();
		if (!target) {
			const auto wasActive = PickpocketSession::IsActive();
			PickpocketSession::ClearCurrentTarget();
			PickpocketSession::End(false);
			if (wasActive) {
				logger::debug("[QP] Lost crosshair target, ending session");
			}
			return;
		}

		const auto current = PickpocketSession::GetCurrentTarget();
		if (!current || current.get().get() != target) {
			logger::debug("[QP] Crosshair target changed to {:08X}", FormUtil::GetFormID(target));
			PickpocketSession::SetCurrentTarget(target->CreateRefHandle());
		}

		if (!PickpocketRules::CanOpen(target)) {
			const auto wasActive = PickpocketSession::IsActive();
			PickpocketSession::End(wasActive);
			if (wasActive) {
				logger::debug("[QP] Target {:08X} is no longer eligible", FormUtil::GetFormID(target));
			}
			return;
		}

		const auto victim = PickpocketSession::GetCurrentTarget();
		const auto wasActive = PickpocketSession::IsActive();
		const auto sessionVictim = PickpocketSession::GetVictim();
		const auto targetChanged = !sessionVictim || sessionVictim != victim;

		if (!wasActive || targetChanged) {
			if (!wasActive) {
				logger::debug("[QP] Target {:08X} became eligible", FormUtil::GetFormID(target));
			}
			PickpocketSession::Begin(victim);
			return;
		}

		PickpocketSession::RefreshIfNeeded();
	}
}
