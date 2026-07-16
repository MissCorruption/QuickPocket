#include "QuickPocket/Papyrus.h"

#include "QuickPocket/Config.h"
#include "QuickPocket/LogSettings.h"

namespace
{
	inline constexpr std::string_view kPapyrusClassName = "QuickPocketNative";
	inline constexpr std::string_view kDllVersion = "0.1.0";

	std::string GetDllVersionImpl(RE::StaticFunctionTag*)
	{
		return std::string{ kDllVersion };
	}

	bool SetEnabledImpl(RE::StaticFunctionTag*, bool value)
	{
		QuickPocket::Config::SetEnabled(value);
		return true;
	}

	bool SetPlantModeToggleKeyImpl(RE::StaticFunctionTag*, std::int32_t keyCode)
	{
		QuickPocket::Config::SetPlantModeToggleKey(keyCode);
		return true;
	}

	bool SetPlantAllowPoisonsImpl(RE::StaticFunctionTag*, bool value)
	{
		QuickPocket::Config::SetPlantAllowPoisons(value);
		return true;
	}

	bool SetPlantAllowJewelryImpl(RE::StaticFunctionTag*, bool value)
	{
		QuickPocket::Config::SetPlantAllowJewelry(value);
		return true;
	}

	bool SetPlantAllowOtherItemsImpl(RE::StaticFunctionTag*, bool value)
	{
		QuickPocket::Config::SetPlantAllowOtherItems(value);
		return true;
	}

	bool SetPlantAllowEquippedImpl(RE::StaticFunctionTag*, bool value)
	{
		QuickPocket::Config::SetPlantAllowEquipped(value);
		return true;
	}

	bool SetCloseOnDetectionImpl(RE::StaticFunctionTag*, bool value)
	{
		QuickPocket::Config::SetCloseOnDetection(value);
		return true;
	}

	bool SetLogLevelImpl(RE::StaticFunctionTag*, RE::BSFixedString level)
	{
		const char* c = level.c_str();
		const std::string_view sv = c ? std::string_view{ c } : std::string_view{};
		return QuickPocket::LogSettings::ApplyLevel(sv);
	}
}

bool QuickPocket::Papyrus::Register(RE::BSScript::IVirtualMachine* vm)
{
	if (!vm) {
		return false;
	}

	vm->RegisterFunction("SetEnabled", kPapyrusClassName.data(), SetEnabledImpl);
	vm->RegisterFunction("SetPlantModeToggleKey", kPapyrusClassName.data(), SetPlantModeToggleKeyImpl);
	vm->RegisterFunction("SetPlantAllowPoisons", kPapyrusClassName.data(), SetPlantAllowPoisonsImpl);
	vm->RegisterFunction("SetPlantAllowJewelry", kPapyrusClassName.data(), SetPlantAllowJewelryImpl);
	vm->RegisterFunction("SetPlantAllowOtherItems", kPapyrusClassName.data(), SetPlantAllowOtherItemsImpl);
	vm->RegisterFunction("SetPlantAllowEquipped", kPapyrusClassName.data(), SetPlantAllowEquippedImpl);
	vm->RegisterFunction("SetCloseOnDetection", kPapyrusClassName.data(), SetCloseOnDetectionImpl);
	vm->RegisterFunction("SetLogLevel", kPapyrusClassName.data(), SetLogLevelImpl);
	vm->RegisterFunction("GetDllVersion", kPapyrusClassName.data(), GetDllVersionImpl);
	logger::info("[QP] Papyrus functions registered for class {}", kPapyrusClassName);
	return true;
}
