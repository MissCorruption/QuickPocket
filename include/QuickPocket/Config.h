#pragma once

#include <shared_mutex>

namespace QuickPocket
{
	struct RuntimeConfig
	{
		bool enabled{ true };
		std::int32_t plantModeToggleKey{ -1 };
		bool plantAllowPoisons{ true };
		bool plantAllowJewelry{ true };
		bool plantAllowOtherItems{ false };
		bool plantAllowEquipped{ false };
		bool closeOnDetection{ false };
	};

	class Config
	{
	public:
		static RuntimeConfig Get();
		static void Set(const RuntimeConfig& config);
		static void SetEnabled(bool value);
		static void SetPlantModeToggleKey(std::int32_t value);
		static void SetPlantAllowPoisons(bool value);
		static void SetPlantAllowJewelry(bool value);
		static void SetPlantAllowOtherItems(bool value);
		static void SetPlantAllowEquipped(bool value);
		static void SetCloseOnDetection(bool value);

	private:
		static RuntimeConfig _runtimeConfig;
		static std::shared_mutex _runtimeConfigLock;
	};
}
