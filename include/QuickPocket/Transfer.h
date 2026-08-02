#pragma once

#include "QuickPocket/QuickLootAPI.h"

namespace QuickPocket
{
	class Transfer
	{
	public:
		static void OnTakingItem(QuickLoot::API::TakingItemEvent* e);
	};
}
