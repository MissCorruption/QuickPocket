#include "QuickPocket/Config.h"

namespace QuickPocket
{
	RuntimeConfig Config::_runtimeConfig{};
	std::shared_mutex Config::_runtimeConfigLock{};

	RuntimeConfig Config::Get()
	{
		std::shared_lock guard(_runtimeConfigLock);
		return _runtimeConfig;
	}

	void Config::Set(const RuntimeConfig& config)
	{
		std::unique_lock guard(_runtimeConfigLock);
		_runtimeConfig = config;
	}

	void Config::SetEnabled(bool value)
	{
		std::unique_lock guard(_runtimeConfigLock);
		_runtimeConfig.enabled = value;
	}

	void Config::SetPlantModeToggleKey(std::int32_t value)
	{
		std::unique_lock guard(_runtimeConfigLock);
		_runtimeConfig.plantModeToggleKey = value;
	}

	void Config::SetPlantAllowPoisons(bool value)
	{
		std::unique_lock guard(_runtimeConfigLock);
		_runtimeConfig.plantAllowPoisons = value;
	}

	void Config::SetPlantAllowJewelry(bool value)
	{
		std::unique_lock guard(_runtimeConfigLock);
		_runtimeConfig.plantAllowJewelry = value;
	}

	void Config::SetPlantAllowOtherItems(bool value)
	{
		std::unique_lock guard(_runtimeConfigLock);
		_runtimeConfig.plantAllowOtherItems = value;
	}

	void Config::SetPlantAllowEquipped(bool value)
	{
		std::unique_lock guard(_runtimeConfigLock);
		_runtimeConfig.plantAllowEquipped = value;
	}

	void Config::SetCloseOnDetection(bool value)
	{
		std::unique_lock guard(_runtimeConfigLock);
		_runtimeConfig.closeOnDetection = value;
	}
}
