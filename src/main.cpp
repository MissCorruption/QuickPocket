#include <format>
#include <string_view>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "SKSE/SKSE.h"

#include "LogSettings.h"
#include "EventHandlers.h"
#include "Papyrus.h"
#include "PlantInput.h"
#include "QuickLootBridge.h"
#include "QuickLootSettings.h"
#include "Targeting.h"

namespace
{
	// Same pattern as QuickLootIE/src/XSEPlugin.cpp: own the default logger, then
	// SKSE::Init(..., false) so CommonLib does not replace it with log::init().
	void InitializeLog(spdlog::level::level_enum level = spdlog::level::info)
	{
		auto path = SKSE::log::log_directory();
		if (!path) {
			SKSE::stl::report_and_fail("Unable to lookup SKSE log directory"sv);
		}

		std::string_view logName = "QuickPocket"sv;
		if (const auto* plugin = SKSE::PluginVersionData::GetSingleton()) {
			const auto name = plugin->GetPluginName();
			if (!name.empty()) {
				logName = name;
			}
		}

		const auto skseLogDir = *path;
		*path /= std::format("{}.log", logName);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

		const auto effectiveLevel = (level == spdlog::level::info) ?
			                            QuickPocket::LogSettings::ResolveStartupLevel(skseLogDir) :
			                            level;
		log->set_level(effectiveLevel);
		log->flush_on(effectiveLevel);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S.%e] [%l] [%t] %v");
	}

	void OnSKSEMessage(SKSE::MessagingInterface::Message* message)
	{
		if (!message) {
			return;
		}

		if (message->type == SKSE::MessagingInterface::kDataLoaded) {
			if (!QuickPocket::QuickLootBridge::Initialize()) {
				SKSE::log::warn("[QP] QuickLootIE API not available. QuickPocket is disabled.");
				return;
			}

			QuickPocket::QuickLootSettings::Initialize();
			SKSE::Translation::ParseTranslation("QuickPocket");
			QuickPocket::EventHandlers::Install();
			QuickPocket::Targeting::Start();
			QuickPocket::PlantInput::Install();
			SKSE::log::info("[QP] QuickPocket initialized.");
			return;
		}

		if (message->type == SKSE::MessagingInterface::kNewGame ||
		    message->type == SKSE::MessagingInterface::kPostLoadGame) {
			if (QuickPocket::QuickLootBridge::IsReady()) {
				QuickPocket::QuickLootSettings::Refresh();
			}
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	InitializeLog();

	SKSE::Init(skse, false);

	SKSE::log::info("[QP] Loading QuickPocket");

	if (!SKSE::GetPapyrusInterface()->Register(QuickPocket::Papyrus::Register)) {
		SKSE::log::warn("[QP] Failed to register Papyrus bindings.");
	}
	return SKSE::GetMessagingInterface()->RegisterListener(OnSKSEMessage);
}
