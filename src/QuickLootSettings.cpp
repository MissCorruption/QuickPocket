#include "QuickLootSettings.h"

#include <fstream>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace QuickPocket::QuickLootSettings
{
	namespace
	{
		using json = nlohmann::json;

		inline constexpr auto kConfigPath = "Data\\SKSE\\Plugins\\QuickLootIE.json";
		inline constexpr auto kPluginName = "QuickLootIE.esp";
		inline constexpr RE::FormID kMcmQuestLocalID = 0x000001;
		inline constexpr auto kMcmScriptName = "QuickLootIEMCM";
		inline constexpr auto kLootMenuName = "LootMenu";
		inline constexpr auto kExcludeKeyword = "QuickLootIE_Exclude";

		struct Snapshot
		{
			std::vector<std::string> menuWhitelist{};
			std::set<RE::FormID> containerBlacklist{};
			bool showInCombat{ true };
			bool showInThirdPerson{ true };
			bool showWhenMounted{ false };
		};

		std::shared_mutex _lock{};
		Snapshot _snapshot{};
		bool _sinkInstalled{ false };

		RE::FormID ParseFormID(const std::string& identifier)
		{
			std::istringstream ss{ identifier };
			std::string plugin;
			std::string id;
			std::getline(ss, plugin, '|');
			std::getline(ss, id);

			RE::FormID localFormID = 0;
			std::istringstream{ id } >> std::hex >> localFormID;
			const auto dataHandler = RE::TESDataHandler::GetSingleton();
			return dataHandler ? dataHandler->LookupFormID(localFormID, plugin) : 0;
		}

		std::vector<std::string> LoadStringArray(const json& config, const char* key)
		{
			std::vector<std::string> result{};
			if (!config.contains(key) || !config.at(key).is_array()) {
				return result;
			}

			for (const auto& element : config.at(key)) {
				if (element.is_string()) {
					result.push_back(element.get<std::string>());
				}
			}
			return result;
		}

		bool ReadMcmBool(const char* propertyName, bool fallback)
		{
			const auto dataHandler = RE::TESDataHandler::GetSingleton();
			const auto quest = dataHandler ? dataHandler->LookupForm<RE::TESQuest>(kMcmQuestLocalID, kPluginName) : nullptr;
			if (!quest) {
				return fallback;
			}

			const auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			if (!vm || !vm->handlePolicy) {
				return fallback;
			}

			const auto typeID = static_cast<RE::VMTypeID>(quest->GetFormType());
			const auto handle = vm->handlePolicy->GetHandleForObject(typeID, quest);
			RE::BSTSmartPointer<RE::BSScript::Object> object{};
			vm->FindBoundObject(handle, kMcmScriptName, object);
			if (!object) {
				return fallback;
			}

			if (const auto* prop = object->GetProperty(propertyName)) {
				return prop->GetBool();
			}
			return fallback;
		}

		void LoadJson(Snapshot& snapshot)
		{
			try {
				std::ifstream ifs{ kConfigPath };
				if (!ifs.is_open()) {
					logger::debug("[QP] QuickLootIE.json not present; using empty whitelist/blacklist");
					return;
				}

				const auto config = json::parse(ifs, nullptr, true, true);
				snapshot.menuWhitelist = LoadStringArray(config, "menuWhitelist");
				for (const auto& identifier : LoadStringArray(config, "containerBlacklist")) {
					if (const auto formID = ParseFormID(identifier)) {
						snapshot.containerBlacklist.insert(formID);
					}
				}
				logger::debug(
					"[QP] Inherited QuickLootIE.json ({} whitelist, {} blacklist)",
					snapshot.menuWhitelist.size(),
					snapshot.containerBlacklist.size());
			} catch (const std::exception& ex) {
				logger::warn("[QP] Failed to parse QuickLootIE.json: {}", ex.what());
			}
		}

		void LoadMcm(Snapshot& snapshot)
		{
			snapshot.showInCombat = ReadMcmBool("QLIE_ShowInCombat", true);
			snapshot.showInThirdPerson = ReadMcmBool("QLIE_ShowInThirdPerson", true);
			snapshot.showWhenMounted = ReadMcmBool("QLIE_ShowWhenMounted", false);
		}

		bool IsValidCameraState(RE::CameraState state, const Snapshot& snapshot)
		{
			switch (state) {
			case RE::CameraState::kFirstPerson:
				return true;
			case RE::CameraState::kThirdPerson:
				return snapshot.showInThirdPerson;
			case RE::CameraState::kMount:
				return snapshot.showWhenMounted;
			default:
				return false;
			}
		}

		bool IsContainerBlacklisted(const RE::TESObjectREFR& target, const Snapshot& snapshot)
		{
			if (snapshot.containerBlacklist.contains(target.formID)) {
				return true;
			}
			if (const auto baseObj = target.GetBaseObject()) {
				return snapshot.containerBlacklist.contains(baseObj->formID);
			}
			return false;
		}

		bool IsBlockedByOpenMenu(const Snapshot& snapshot)
		{
			const auto ui = RE::UI::GetSingleton();
			if (!ui) {
				return false;
			}

			std::set<RE::IMenu*> whitelistedMenus{};
			if (const auto lootMenu = ui->GetMenu(kLootMenuName)) {
				whitelistedMenus.emplace(lootMenu.get());
			}
			if (const auto cursorMenu = ui->GetMenu(RE::CursorMenu::MENU_NAME)) {
				whitelistedMenus.emplace(cursorMenu.get());
			}
			for (const auto& menuName : snapshot.menuWhitelist) {
				if (const auto whitelistedMenu = ui->GetMenu(menuName)) {
					whitelistedMenus.emplace(whitelistedMenu.get());
				}
			}

			for (const auto& menu : ui->menuStack) {
				if (!menu || (menu->menuFlags & RE::UI_MENU_FLAGS::kAlwaysOpen)) {
					continue;
				}
				if (!whitelistedMenus.contains(menu.get())) {
					return true;
				}
			}
			return false;
		}

		class JournalSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			static JournalSink* GetSingleton()
			{
				static JournalSink instance;
				return &instance;
			}

			RE::BSEventNotifyControl ProcessEvent(
				const RE::MenuOpenCloseEvent* event,
				RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				if (event && !event->opening && event->menuName == RE::JournalMenu::MENU_NAME) {
					Refresh();
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	void Refresh()
	{
		Snapshot next{};
		LoadJson(next);
		LoadMcm(next);

		std::unique_lock guard(_lock);
		_snapshot = std::move(next);
	}

	void Initialize()
	{
		Refresh();

		if (_sinkInstalled) {
			return;
		}

		if (const auto ui = RE::UI::GetSingleton()) {
			ui->AddEventSink(JournalSink::GetSingleton());
			_sinkInstalled = true;
		}
	}

	bool CanOpenEnvironment(RE::TESObjectREFR* target)
	{
		if (!target) {
			return false;
		}

		Snapshot snapshot{};
		{
			std::shared_lock guard(_lock);
			snapshot = _snapshot;
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return false;
		}

		if (target->IsLocked() || target->IsActivationBlocked()) {
			return false;
		}

		if (!snapshot.showInCombat && player->IsInCombat()) {
			return false;
		}

		if (player->IsGrabbing() || player->HasActorDoingCommand()) {
			return false;
		}

		if (const auto menuControls = RE::MenuControls::GetSingleton(); menuControls && menuControls->InBeastForm()) {
			return false;
		}

		if (const auto camera = RE::PlayerCamera::GetSingleton(); camera && camera->currentState) {
			if (!IsValidCameraState(camera->currentState->id, snapshot)) {
				return false;
			}
		}

		if (const auto ui = RE::UI::GetSingleton(); ui && ui->GameIsPaused()) {
			return false;
		}

		if (IsContainerBlacklisted(*target, snapshot)) {
			return false;
		}

		if (target->HasKeywordByEditorID(kExcludeKeyword)) {
			return false;
		}

		return !IsBlockedByOpenMenu(snapshot);
	}
}
