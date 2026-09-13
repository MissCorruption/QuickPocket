Scriptname QuickPocketMaintenance extends Quest

import QuickPocketNative

String Property CurrentVersionString = "0.1.0" Auto
QuickPocketMCM Property MCMConfig Auto

string Function LogLevelName(int value)
	If value == 2
		return "trace"
	ElseIf value == 1
		return "debug"
	EndIf
	return "info"
EndFunction

Function OnGameLoaded()
	If !MCMConfig
		return
	EndIf

	QuickPocketNative.SetEnabled(MCMConfig.QP_Enabled)
	QuickPocketNative.SetPlantModeToggleKey(MCMConfig.QP_KeybindingPlantModeToggle)
	QuickPocketNative.SetPlantAllowPoisons(MCMConfig.QP_PlantAllowPoisons)
	QuickPocketNative.SetPlantAllowJewelry(MCMConfig.QP_PlantAllowJewelry)
	QuickPocketNative.SetPlantAllowOtherItems(MCMConfig.QP_PlantAllowOtherItems)
	QuickPocketNative.SetPlantAllowEquipped(MCMConfig.QP_PlantAllowEquipped)
	QuickPocketNative.SetCloseOnDetection(MCMConfig.QP_CloseOnDetection)
	QuickPocketNative.SetLogLevel(LogLevelName(MCMConfig.QP_LogLevel))
EndFunction
