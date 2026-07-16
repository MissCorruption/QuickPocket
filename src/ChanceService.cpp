#include "QuickPocket/ChanceService.h"

namespace
{
	inline constexpr int kUnknownChance = -1;
	inline constexpr std::int32_t kDetectionFullyDetectedThreshold = 2;
}

int QuickPocket::ChanceService::ComputeChance(RE::Actor* victim, RE::InventoryEntryData* entry)
{
	if (!victim || !entry || !entry->object) {
		return kUnknownChance;
	}

	const auto player = RE::PlayerCharacter::GetSingleton();
	if (!player) {
		return kUnknownChance;
	}

	const auto count = std::max(1, entry->countDelta);
	const auto value = victim->GetStealValue(entry, count, true);
	const auto weight = entry->object->GetWeight();
	const auto detectionLevel = victim->RequestDetectionLevel(player);
	const auto detected = detectionLevel >= kDetectionFullyDetectedThreshold;
	const auto playerSkill = player->AsActorValueOwner()->GetClampedActorValue(RE::ActorValue::kPickpocket);
	const auto victimSkill = victim->AsActorValueOwner()->GetActorValue(RE::ActorValue::kPickpocket);

	return RE::AIFormulas::ComputePickpocketSuccess(playerSkill, victimSkill, value, weight, player, victim, detected, entry->object);
}

std::string QuickPocket::ChanceService::FormatChance(int chancePercent)
{
	if (chancePercent < 0) {
		return "Unknown";
	}

	const auto clamped = std::clamp(chancePercent, 0, 100);
	return std::format("{}%", clamped);
}
