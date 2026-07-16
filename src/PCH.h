#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#define NOMINMAX

#pragma warning(push)
#pragma warning(disable: 4200)
#pragma warning(disable: 4201)
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#pragma warning(pop)

using namespace std::literals;

namespace logger = SKSE::log;
