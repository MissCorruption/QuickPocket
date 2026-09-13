#pragma once

#include "QuickLootAPI.h"

namespace QuickPocket
{
	class Transfer
	{
	public:
		static void OnTakingItem(QuickLoot::API::TakingItemEvent* e);
	};
}
