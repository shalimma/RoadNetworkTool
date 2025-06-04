// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadNetworkToolEditorModeCommands.h"
#include "RoadNetworkToolEditorMode.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "RoadNetworkToolEditorModeCommands"

FRoadNetworkToolEditorModeCommands::FRoadNetworkToolEditorModeCommands()
	: TCommands<FRoadNetworkToolEditorModeCommands>("RoadNetworkToolEditorMode",
		NSLOCTEXT("RoadNetworkToolEditorMode", "RoadNetworkToolEditorModeCommands", "RoadNetworkTool Editor Mode"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FRoadNetworkToolEditorModeCommands::RegisterCommands()
{
	TArray <TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);

	UI_COMMAND(LineTool, "Linker", "Click to set the origin, then click to extend the line", EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(LineTool);

	UI_COMMAND(SimpleTool, "Show Actor Info", "Opens message box with info about a clicked actor", EUserInterfaceActionType::Button, FInputChord());
	ToolCommands.Add(SimpleTool);

	UI_COMMAND(InteractiveTool, "Measure Distance", "Measures distance between 2 points (click to set origin, shift-click to set end point)", EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(InteractiveTool);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FRoadNetworkToolEditorModeCommands::GetCommands()
{
	return FRoadNetworkToolEditorModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE
