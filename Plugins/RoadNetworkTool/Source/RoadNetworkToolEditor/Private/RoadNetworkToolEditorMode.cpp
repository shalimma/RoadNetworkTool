// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadNetworkToolEditorMode.h"
#include "RoadNetworkToolEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "RoadNetworkToolEditorModeCommands.h"
#include "Modules/ModuleManager.h"
#include "RoadNetworkTool/Public/RoadHelper.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/RoadNetworkToolSimpleTool.h"
#include "Tools/RoadNetworkToolInteractiveTool.h"
#include "Tools/RoadNetworkToolLineTool.h"

// step 2: register a ToolBuilder in FRoadNetworkToolEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "RoadNetworkToolEditorMode"

const FEditorModeID URoadNetworkToolEditorMode::EM_RoadNetworkToolEditorModeId = TEXT("EM_RoadNetworkToolEditorMode");

FString URoadNetworkToolEditorMode::LineToolName = TEXT("RoadNetworkTool_RoadCreationTool");
FString URoadNetworkToolEditorMode::SimpleToolName = TEXT("RoadNetworkTool_ActorInfoTool");
FString URoadNetworkToolEditorMode::InteractiveToolName = TEXT("RoadNetworkTool_MeasureDistanceTool");


URoadNetworkToolEditorMode::URoadNetworkToolEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(URoadNetworkToolEditorMode::EM_RoadNetworkToolEditorModeId,
		LOCTEXT("ModeName", "RoadNetworkTool"),
		FSlateIcon(),
		true);
}


URoadNetworkToolEditorMode::~URoadNetworkToolEditorMode()
{
}


void URoadNetworkToolEditorMode::ActorSelectionChangeNotify()
{
}

void URoadNetworkToolEditorMode::Enter()
{
    UEdMode::Enter();

    ////////////////////////////////////////////////////
    // AddYourTool Step 2 - register the ToolBuilder for your tools here.
    // The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
    ////////////////////////////////////////////////////
    const FRoadNetworkToolEditorModeCommands& SampleToolCommands = FRoadNetworkToolEditorModeCommands::Get();

    RegisterTool(SampleToolCommands.LineTool, LineToolName, NewObject<URoadNetworkToolLineToolBuilder>(this));
    RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<URoadNetworkToolSimpleToolBuilder>(this));
    RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<URoadNetworkToolInteractiveToolBuilder>(this));

    // active tool type is not relevant here, we just set to default
    GetToolManager()->SelectActiveToolType(EToolSide::Left, LineToolName);
    GetToolManager()->ActivateTool(EToolSide::Left);

    ARoadActor::bIsInRoadNetworkMode = true;
}

void URoadNetworkToolEditorMode::Exit()
{
    UEdMode::Exit();

    ARoadActor::bIsInRoadNetworkMode = false;
}

void URoadNetworkToolEditorMode::CreateToolkit()
{
    Toolkit = MakeShareable(new FRoadNetworkToolEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> URoadNetworkToolEditorMode::GetModeCommands() const
{
	return FRoadNetworkToolEditorModeCommands::Get().GetCommands();
}

bool URoadNetworkToolEditorMode::IsSelectionAllowed(AActor* InActor, bool bInSelected) const
{
	return FRoadHelper::IsRoadActor(InActor);
}

#undef LOCTEXT_NAMESPACE
