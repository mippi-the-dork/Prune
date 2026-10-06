// Copyright Mippithedork 2026, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Prune : ModuleRules
{
    public Prune(ReadOnlyTargetRules Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Engine",
            "Slate",
            "SlateCore",
            "PropertyEditor",
            "Projects",
            "Settings",
            "DetailCustomizations",
            "UnrealEd"
        });
    }
}
