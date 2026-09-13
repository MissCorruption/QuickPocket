#include "LogSettings.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>

namespace QuickPocket::LogSettings
{
	namespace
	{
		std::optional<spdlog::level::level_enum> ParseLevel(std::string_view s)
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

			if (normalized == "warn") {
				normalized = "warning";
			} else if (normalized == "err") {
				normalized = "error";
			}

			const auto lv = spdlog::level::from_str(normalized);
			if (lv == spdlog::level::off && normalized != "off") {
				return std::nullopt;
			}
			return lv;
		}
	}

	spdlog::level::level_enum ResolveStartupLevel(const std::filesystem::path& skseLogDirectory)
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

	bool ApplyLevel(std::string_view s)
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
