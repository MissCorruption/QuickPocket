Scriptname QuickPocketMCM extends SKI_ConfigBase

import QuickPocketNative
import JsonUtil

bool Property QP_Enabled = true Auto Hidden
int Property QP_LogLevel = 0 Auto Hidden
int Property QP_KeybindingPlantModeToggle = -1 Auto Hidden
bool Property QP_PlantAllowPoisons = true Auto Hidden
bool Property QP_PlantAllowJewelry = true Auto Hidden
bool Property QP_PlantAllowOtherItems = false Auto Hidden
bool Property QP_PlantAllowEquipped = false Auto Hidden
bool Property QP_CloseOnDetection = false Auto Hidden

string Property ConfigPath = "../QuickPocket/DefaultConfig.json" Auto Hidden

Event OnConfigInit()
	Pages = new string[3]
	Pages[0] = "$qp_GeneralPage"
	Pages[1] = "$qp_DisplayPage"
	Pages[2] = "$qp_ControlsPage"
EndEvent

Event OnPageReset(string page)
	If page == "$qp_GeneralPage"
		BuildGeneralPage()
		return
	EndIf

	If page == "$qp_DisplayPage"
		BuildDisplayPage()
		return
	EndIf

	If page == "$qp_ControlsPage"
		BuildControlPage()
		return
	EndIf
EndEvent

Function BuildGeneralPage()
	SetCursorFillMode(TOP_TO_BOTTOM)
	SetCursorPosition(0)
	AddHeaderOption("$qp_BehaviorHeader")
	AddTextOptionST("state_Enabled", "$qp_Enabled", Status(QP_Enabled))
	AddEmptyOption()
	AddHeaderOption("$qp_PlantFiltersHeader")
	AddTextOptionST("state_PlantAllowPoisons", "$qp_PlantAllowPoisons", Status(QP_PlantAllowPoisons))
	AddTextOptionST("state_PlantAllowJewelry", "$qp_PlantAllowJewelry", Status(QP_PlantAllowJewelry))
	AddTextOptionST("state_PlantAllowOtherItems", "$qp_PlantAllowOtherItems", Status(QP_PlantAllowOtherItems))
	AddTextOptionST("state_PlantAllowEquipped", "$qp_PlantAllowEquipped", Status(QP_PlantAllowEquipped))
	AddTextOptionST("state_CloseOnDetection", "$qp_CloseOnDetection", Status(QP_CloseOnDetection))

	SetCursorPosition(1)
	AddHeaderOption("$qp_ModInformationHeader")
	AddTextOption("", "$qp_ModName")
	AddTextOption("$qp_ModVersion", ((self as Quest) as QuickPocketMaintenance).CurrentVersionString)
	AddTextOption("$qp_DllVersion", QuickPocketNative.GetDllVersion())
EndFunction

Function BuildDisplayPage()
	SetCursorFillMode(TOP_TO_BOTTOM)
	SetCursorPosition(0)
	AddHeaderOption("$qp_DisplayHeader")
	AddTextOptionST("state_LogLevel", "$qp_LogLevel", LogLevelName(QP_LogLevel))
EndFunction

Function BuildControlPage()
	SetCursorFillMode(TOP_TO_BOTTOM)
	SetCursorPosition(0)
	AddHeaderOption("$qp_ControlsHeader")
	AddKeyMapOptionST("state_PlantToggleKey", "$qp_PlantToggleKey", QP_KeybindingPlantModeToggle, OPTION_FLAG_WITH_UNMAP)
	AddEmptyOption()
	AddHeaderOption("$qp_ManageHeader")
	AddTextOptionST("state_Export", "$qp_Export", "")
	AddTextOptionST("state_Import", "$qp_Import", "")
EndFunction

string Function Status(bool value)
	If value
		return "$Enabled"
	EndIf
	return "$Disabled"
EndFunction

string Function LogLevelName(int value)
	If value == 2
		return "trace"
	ElseIf value == 1
		return "debug"
	EndIf
	return "info"
EndFunction

state state_Enabled
	Event OnSelectST()
		QP_Enabled = !QP_Enabled
		QuickPocketNative.SetEnabled(QP_Enabled)
		SetTextOptionValueST(Status(QP_Enabled))
	EndEvent
endstate

state state_LogLevel
	Event OnSelectST()
		QP_LogLevel += 1
		If QP_LogLevel > 2
			QP_LogLevel = 0
		EndIf
		QuickPocketNative.SetLogLevel(LogLevelName(QP_LogLevel))
		SetTextOptionValueST(LogLevelName(QP_LogLevel))
	EndEvent
endstate

state state_PlantToggleKey
	Event OnKeyMapChangeST(int keyCode, string conflictControl, string conflictName)
		QP_KeybindingPlantModeToggle = keyCode
		QuickPocketNative.SetPlantModeToggleKey(QP_KeybindingPlantModeToggle)
		SetKeyMapOptionValueST(keyCode)
	EndEvent

	Event OnDefaultST()
		QP_KeybindingPlantModeToggle = -1
		QuickPocketNative.SetPlantModeToggleKey(QP_KeybindingPlantModeToggle)
		SetKeyMapOptionValueST(QP_KeybindingPlantModeToggle)
	EndEvent
endstate

state state_PlantAllowPoisons
	Event OnSelectST()
		QP_PlantAllowPoisons = !QP_PlantAllowPoisons
		QuickPocketNative.SetPlantAllowPoisons(QP_PlantAllowPoisons)
		SetTextOptionValueST(Status(QP_PlantAllowPoisons))
	EndEvent
endstate

state state_PlantAllowJewelry
	Event OnSelectST()
		QP_PlantAllowJewelry = !QP_PlantAllowJewelry
		QuickPocketNative.SetPlantAllowJewelry(QP_PlantAllowJewelry)
		SetTextOptionValueST(Status(QP_PlantAllowJewelry))
	EndEvent
endstate

state state_PlantAllowOtherItems
	Event OnSelectST()
		QP_PlantAllowOtherItems = !QP_PlantAllowOtherItems
		QuickPocketNative.SetPlantAllowOtherItems(QP_PlantAllowOtherItems)
		SetTextOptionValueST(Status(QP_PlantAllowOtherItems))
	EndEvent
endstate

state state_PlantAllowEquipped
	Event OnSelectST()
		QP_PlantAllowEquipped = !QP_PlantAllowEquipped
		QuickPocketNative.SetPlantAllowEquipped(QP_PlantAllowEquipped)
		SetTextOptionValueST(Status(QP_PlantAllowEquipped))
	EndEvent
endstate

state state_CloseOnDetection
	Event OnSelectST()
		QP_CloseOnDetection = !QP_CloseOnDetection
		QuickPocketNative.SetCloseOnDetection(QP_CloseOnDetection)
		SetTextOptionValueST(Status(QP_CloseOnDetection))
	EndEvent
endstate

state state_Export
	Event OnSelectST()
		JsonUtil.SetPathIntValue(ConfigPath, "KeybindingNewFormat", 1)
		JsonUtil.SetPathIntValue(ConfigPath, "Enabled", QP_Enabled as int)
		JsonUtil.SetPathIntValue(ConfigPath, "LogLevel", QP_LogLevel)
		JsonUtil.SetPathIntValue(ConfigPath, "KeybindingPlantModeToggle", QP_KeybindingPlantModeToggle)
		JsonUtil.SetPathIntValue(ConfigPath, "PlantAllowPoisons", QP_PlantAllowPoisons as int)
		JsonUtil.SetPathIntValue(ConfigPath, "PlantAllowJewelry", QP_PlantAllowJewelry as int)
		JsonUtil.SetPathIntValue(ConfigPath, "PlantAllowOtherItems", QP_PlantAllowOtherItems as int)
		JsonUtil.SetPathIntValue(ConfigPath, "PlantAllowEquipped", QP_PlantAllowEquipped as int)
		JsonUtil.SetPathIntValue(ConfigPath, "CloseOnDetection", QP_CloseOnDetection as int)
		JsonUtil.Save(ConfigPath)
	EndEvent
endstate

state state_Import
	Event OnSelectST()
		If JsonUtil.JsonExists(ConfigPath)
			QP_Enabled = JsonUtil.GetPathIntValue(ConfigPath, "Enabled", QP_Enabled as int)
			QP_LogLevel = JsonUtil.GetPathIntValue(ConfigPath, "LogLevel", QP_LogLevel)
			QP_KeybindingPlantModeToggle = JsonUtil.GetPathIntValue(ConfigPath, "KeybindingPlantModeToggle", QP_KeybindingPlantModeToggle)
			QP_PlantAllowPoisons = JsonUtil.GetPathIntValue(ConfigPath, "PlantAllowPoisons", QP_PlantAllowPoisons as int) as bool
			QP_PlantAllowJewelry = JsonUtil.GetPathIntValue(ConfigPath, "PlantAllowJewelry", QP_PlantAllowJewelry as int) as bool
			QP_PlantAllowOtherItems = JsonUtil.GetPathIntValue(ConfigPath, "PlantAllowOtherItems", QP_PlantAllowOtherItems as int) as bool
			QP_PlantAllowEquipped = JsonUtil.GetPathIntValue(ConfigPath, "PlantAllowEquipped", QP_PlantAllowEquipped as int) as bool
			QP_CloseOnDetection = JsonUtil.GetPathIntValue(ConfigPath, "CloseOnDetection", QP_CloseOnDetection as int) as bool
			JsonUtil.Unload(ConfigPath, false)
			QuickPocketNative.SetEnabled(QP_Enabled)
			QuickPocketNative.SetLogLevel(LogLevelName(QP_LogLevel))
			QuickPocketNative.SetPlantModeToggleKey(QP_KeybindingPlantModeToggle)
			QuickPocketNative.SetPlantAllowPoisons(QP_PlantAllowPoisons)
			QuickPocketNative.SetPlantAllowJewelry(QP_PlantAllowJewelry)
			QuickPocketNative.SetPlantAllowOtherItems(QP_PlantAllowOtherItems)
			QuickPocketNative.SetPlantAllowEquipped(QP_PlantAllowEquipped)
			QuickPocketNative.SetCloseOnDetection(QP_CloseOnDetection)
		EndIf
	EndEvent
endstate
