// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class DynamicAdvertPlugin : ModuleRules
	{
		public DynamicAdvertPlugin(ReadOnlyTargetRules Target):base(Target)
		{
            bEnableExceptions = true;
            

			PublicIncludePaths.AddRange(
				new string[] {
					"MediaAssets/Public"
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"HTTP", "MediaAssets", "Runtime/Projects/Public","Engine"
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "Sockets","Json", "MediaAssets", "UMG","Engine"
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"HTTP",
                    "Core",
                    "Projects",
                    "Engine"
                }
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}
