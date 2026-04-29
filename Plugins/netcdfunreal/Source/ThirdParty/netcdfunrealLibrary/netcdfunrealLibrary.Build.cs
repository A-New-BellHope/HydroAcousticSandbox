// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class netcdfunrealLibrary : ModuleRules
{
	public netcdfunrealLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "bin", "hdf5.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "bin", "hdf5_hl.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "bin", "libcurl.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "bin", "netcdf.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "bin", "netcdf-cxx4.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "bin", "pkgconf.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "bin", "szip.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "bin", "zlib.lib"));

			// Delay-load the DLL, so we can load it from the right place first
			PublicDelayLoadDLLs.Add("hdf5.dll");
			PublicDelayLoadDLLs.Add("hdf5_hl.dll");
			PublicDelayLoadDLLs.Add("libcurl.dll");
			PublicDelayLoadDLLs.Add("netcdf.dll");
			//PublicDelayLoadDLLs.Add("netcdf-cxx4.dll");
			PublicDelayLoadDLLs.Add("pkgconf.dll");
			PublicDelayLoadDLLs.Add("szip.dll");
			PublicDelayLoadDLLs.Add("zlib.dll");

			// Ensure that the DLL is staged along with the executable
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/netcdfunrealLibrary/Win64/hdf5.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/netcdfunrealLibrary/Win64/hdf5_hl.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/netcdfunrealLibrary/Win64/libcurl.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/netcdfunrealLibrary/Win64/netcdf.dll");
			//RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/netcdfunrealLibrary/Win64/netcdf-cxx4.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/netcdfunrealLibrary/Win64/pkgconf-4.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/netcdfunrealLibrary/Win64/szip.dll");
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/netcdfunrealLibrary/Win64/zlib1.dll");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
		}
	}
}
