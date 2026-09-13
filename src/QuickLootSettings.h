#pragma once

namespace QuickPocket::QuickLootSettings
{
	void Initialize();
	void Refresh();
	bool CanOpenEnvironment(RE::TESObjectREFR* target);
}
