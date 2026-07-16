Scriptname QuickPocketLoad extends SKI_PlayerLoadGameAlias

QuickPocketMaintenance Property MaintenanceScript Auto

Event OnPlayerLoadGame()
	If MaintenanceScript
		MaintenanceScript.OnGameLoaded()
	EndIf
EndEvent
