#pragma once

namespace QuickPocket::FormUtil
{
	inline RE::FormID GetFormID(RE::TESObjectREFR* ref)
	{
		return ref ? ref->GetFormID() : 0;
	}

	inline RE::FormID GetFormID(RE::ObjectRefHandle handle)
	{
		const auto ref = handle.get();
		return GetFormID(ref.get());
	}
}
