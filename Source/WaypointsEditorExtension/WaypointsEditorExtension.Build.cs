// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WaypointsEditorExtension : ModuleRules
{
    public WaypointsEditorExtension(ReadOnlyTargetRules Target) : base(Target)
    {
        IWYUSupport = IWYUSupport.None;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
			}
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "Slate",
                "SlateCore",
                "LevelEditor",
                "Waypoints",
                "PluginUtils",
                "Projects"
			}
        );
    }
}
