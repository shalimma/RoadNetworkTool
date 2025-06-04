// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadNetworkToolModule.h"
#include "RoadNetworkToolEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "RoadNetworkToolModule"

void FRoadNetworkToolModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FRoadNetworkToolEditorModeCommands::Register();
}

void FRoadNetworkToolModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FRoadNetworkToolEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRoadNetworkToolModule, RoadNetworkTool)