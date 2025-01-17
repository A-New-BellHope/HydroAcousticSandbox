// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class netcdfunreal : ModuleRules
{
	public netcdfunreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableExceptions = true;

        PublicIncludePaths.AddRange(
			new string[] {
                "../../../Plugins/netcdfunreal/Source/ThirdParty/netcdfunrealLibrary/include"
				// ... add public include paths required here ...
			}
            );
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                "../../../Plugins/netcdfunreal/Source/ThirdParty",
                "../../../Plugins/netcdfunreal/Source/ThirdParty/netcdfunrealLibrary/include"
				// ... add other private include paths required here ...
			}
            );
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
                "Engine",
				"Core",
				"netcdfunrealLibrary",
				"Projects"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add private dependencies that you statically link with here ...	
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
