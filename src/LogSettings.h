#pragma once

#include <filesystem>
#include <spdlog/spdlog.h>
#include <string_view>

namespace QuickPocket::LogSettings
{
	spdlog::level::level_enum ResolveStartupLevel(const std::filesystem::path& skseLogDirectory);
	bool ApplyLevel(std::string_view s);
}
