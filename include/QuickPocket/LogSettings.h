#pragma once

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace QuickPocket::LogSettings
{
	inline std::optional<spdlog::level::level_enum> ParseLevel(std::string_view s)
	{
		while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
			s.remove_prefix(1);
		}
		while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
			s.remove_suffix(1);
		}
		if (s.empty()) {
			return std::nullopt;
		}

		std::string normalized;
		normalized.reserve(s.size());
		for (char ch : s) {
			normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
		}
		s = normalized;

		if (s == "trace") {
			return spdlog::level::trace;
		}
		if (s == "debug") {
			return spdlog::level::debug;
		}
		if (s == "info") {
			return spdlog::level::info;
		}
		if (s == "warn" || s == "warning") {
			return spdlog::level::warn;
		}
		if (s == "err" || s == "error") {
			return spdlog::level::err;
		}
		if (s == "critical") {
			return spdlog::level::critical;
		}
		if (s == "off") {
			return spdlog::level::off;
		}
		return std::nullopt;
	}

	inline spdlog::level::level_enum ResolveStartupLevel(const std::filesystem::path& skseLogDirectory)
	{
#if defined(_DEBUG)
		(void)skseLogDirectory;
		return spdlog::level::debug;
#else
		const auto filePath = skseLogDirectory / "QuickPocket.loglevel";
		if (std::ifstream ifs{ filePath }) {
			std::string line;
			if (std::getline(ifs, line)) {
				if (const auto lv = ParseLevel(line)) {
					return *lv;
				}
			}
		}
#if defined(_MSC_VER)
		char* envBuf = nullptr;
		std::size_t envLen = 0;
		if (_dupenv_s(&envBuf, &envLen, "QP_LOG_LEVEL") == 0 && envBuf) {
			struct EnvBufFreer {
				char* p;
				~EnvBufFreer() { std::free(p); }
			} freer{ envBuf };
			if (const auto lv = ParseLevel(std::string_view{ envBuf })) {
				return *lv;
			}
		}
#else
		if (const char* env = std::getenv("QP_LOG_LEVEL")) {
			if (const auto lv = ParseLevel(env)) {
				return *lv;
			}
		}
#endif
		return spdlog::level::info;
#endif
	}

	inline bool ApplyLevel(std::string_view s)
	{
		const auto lv = ParseLevel(s);
		if (!lv) {
			return false;
		}
		auto lg = spdlog::default_logger();
		if (!lg) {
			return false;
		}
		lg->set_level(*lv);
		lg->flush_on(*lv);
		return true;
	}
}
