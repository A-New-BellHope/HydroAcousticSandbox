// Copyright Epic Games, Inc. All Rights Reserved.

#include "netcdfunreal.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FnetcdfunrealModule"

void FnetcdfunrealModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Get the base directory of this plugin
	FString BaseDir = IPluginManager::Get().FindPlugin("netcdfunreal")->GetBaseDir();

	// Add on the relative location of the third party dll and load it
	FString LibraryPath;
#if PLATFORM_WINDOWS
	//Get the external dll's for netcdf. - Order matters.
	FPlatformProcess::GetDllHandle(*FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/netcdfunrealLibrary/Win64/zlib1.dll")));
	FPlatformProcess::GetDllHandle(*FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/netcdfunrealLibrary/Win64/szip.dll")));
	FPlatformProcess::GetDllHandle(*FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/netcdfunrealLibrary/Win64/libcurl.dll")));
	FPlatformProcess::GetDllHandle(*FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/netcdfunrealLibrary/Win64/hdf5.dll")));
	FPlatformProcess::GetDllHandle(*FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/netcdfunrealLibrary/Win64/hdf5_hl.dll")));
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/netcdfunrealLibrary/Win64/netcdf.dll"));
#elif PLATFORM_MAC
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/netcdfunrealLibrary/Mac/Release/libExampleLibrary.dylib"));
#elif PLATFORM_LINUX
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/netcdfunrealLibrary/Linux/x86_64-unknown-linux-gnu/libExampleLibrary.so"));
#endif // PLATFORM_WINDOWS

	NetCDFLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;

	if (NetCDFLibraryHandle)
	{
		// Call the test function in the third party library that opens a message box
		//ExampleLibraryFunction();
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "Failed to load example third party library"));
	}
}

void FnetcdfunrealModule::ShutdownModule()
{
	UnloadBathymetryFile();

	// Free the dll handle
	FPlatformProcess::FreeDllHandle(NetCDFLibraryHandle);
	NetCDFLibraryHandle = nullptr;
}

/// <summary>
/// Open up the global bathymetry file.
/// Assumes it's compatible with GEBCO standard.
/// </summary>
/// <param name="Filename"></param>
/// <returns></returns>
bool FnetcdfunrealModule::LoadBathymetryFile(const FString& Filename)
{
	UnloadBathymetryFile();
	std::string const fil = TCHAR_TO_UTF8(*Filename);
	DataFile.open(fil, netCDF::NcFile::read);
	return CheckNcFile();
}

/// <summary>
/// Close the file (if it's open)
/// Safe to call even if no file is open.
/// </summary>
/// <returns>success</returns>
void FnetcdfunrealModule::UnloadBathymetryFile()
{
	DataFile.close();
}

/// <summary>
/// Get the array data stored in the variable.
/// Assumes a double is read and a float array returned (hack).
/// Not much checking.
/// </summary>
/// <param name="Variable">name of the variable</param>
/// <param name="Value">modified</param>
/// <returns>success</returns>
bool FnetcdfunrealModule::GetArray(
	const FString& Variable, TArray<double>& Value) const
{
	std::string const VarName = TCHAR_TO_UTF8(*Variable);

	netCDF::NcDim dim = DataFile.getDim(VarName);
	if (dim.isNull()) {
		std::cout << "Error: could not find the variable " << 
			VarName << 
			" flagging failure and returning.\n" << 
			std::flush;
		return false;
	}

	//TODO: extra copying we shouldn't need. Time to read probably swamps the copy time.
	TSharedPtr<double> buffer(new double[dim.getSize()]);
	netCDF::NcVar lat = DataFile.getVar(VarName);
	lat.getVar(buffer.Get());

	Value.SetNumUninitialized(dim.getSize());
	for (int i = 0; i < dim.getSize(); ++i) {
		Value[i] = buffer.Get()[i];
	}

	return true;
}

/// <summary>
/// Get subset data from an array. Allows getting just part of the data.
/// netCDF is optimized to do this and has constant time index lookup.
/// </summary>
/// <param name="Variable">var name, elevation in gebco</param>
/// <param name="start">start index</param>
/// <param name="count">count</param>
/// <param name="Value">modified, the data read in netcdf style. Dims incremented right to left.</param>
/// <returns>success</returns>
bool FnetcdfunrealModule::GetArraySubset(const FString& Variable,
	const std::vector<size_t>& start, const std::vector<size_t>& count,
	TArray<double>& Value) const
{
	std::string const VarName = TCHAR_TO_UTF8(*Variable);

	size_t Entries = 1;
	for (auto& i : count) { Entries *= i; }

	if (Entries <= 0) {
		std::string err = "Error: size is invalid " +
			VarName + " size " + std::to_string(Entries) +
			" flagging failure and returning.";
		FString Message = err.c_str();
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		return false;
	}

	auto allDims = DataFile.getDims();
	for (auto itr = allDims.begin(); itr != allDims.end(); ++itr) {
		std::string msg = itr->first;
		UE_LOG(LogTemp, Warning, TEXT("%hs"), msg.c_str());
	}
	netCDF::NcVar var = DataFile.getVar(VarName);
	if (var.isNull()) {
		std::string err = "Error: could not find the variable " +
			VarName +
			" flagging failure and returning.\n";
		UE_LOG(LogTemp, Warning, TEXT("%hs"), err.c_str());
		return false;
	}

	//TODO: extra copying we shouldn't need. Time to read probably swamps the copy time.
	TSharedPtr<int16_t> buffer(new int16_t[Entries]);
	netCDF::NcVar values = DataFile.getVar(VarName);
	values.getVar(start, count, buffer.Get());

	Value.SetNumUninitialized(Entries);
	for (int i = 0; i < Entries; ++i) {
		Value[i] = buffer.Get()[i];
	}

	return true;
}


/// <summary>
/// Check the compatibility.
/// Must have float arrays lat, lon, and elevation
/// </summary>
/// <returns>status of the file</returns>
bool FnetcdfunrealModule::CheckNcFile() const
{
	return !DataFile.isNull();
}

/// <summary>
/// Load the data from the specified dataset.
/// Appends the appropriate command to restrict the sizes.
/// </summary>
/// <param name="DatasetURL">Location of the dataset. Likely a url.
/// e.g. https://tds.hycom.org/thredds/dodsC/GLBy0.08/expt_93.0/ts3z</param>
/// <param name="Depth"></param>
bool FnetcdfunrealModule::
LoadHYCOMDepth(const FString& DatasetURL, TArray<double>& Depth)
{
	bool ret = true;
	if (IsDepthDirty) {
		auto start = std::chrono::high_resolution_clock::now();
		std::string url = TCHAR_TO_UTF8(*DatasetURL);
		ret = ReadVariable(url, "depth", "[0:1:39]", DepthCache);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		UE_LOG(LogTemp, Warning, TEXT("Finished loading HYCOM depth. Elapsed time %g seconds."),
			elapsed_seconds.count());
		IsDepthDirty = false;
	}

	Depth.Insert(DepthCache, 0);
	return ret;
}
bool FnetcdfunrealModule::
LoadHYCOMLatitude(const FString& DatasetURL, TArray<double>& Latitude)
{
	bool ret = true;
	if (IsLatitudeDirty) {
		IsLatitudeDirty = false;
		auto start = std::chrono::high_resolution_clock::now();
		std::string url = TCHAR_TO_UTF8(*DatasetURL);
		ret = ReadVariable(url, "lat", "[0:1:4250]", LatitudeCache);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		UE_LOG(LogTemp, Warning, TEXT("Finished loading HYCOM latitude. Elapsed time %g seconds."),
			elapsed_seconds.count());
		IsLatitudeDirty = false;
	}

	Latitude.Insert(LatitudeCache, 0);
	return ret;
}
bool FnetcdfunrealModule::
LoadHYCOMLongitude(const FString& DatasetURL, TArray<double>& Longitude)
{
	bool ret = true;
	if (IsLongitudeDirty) {
		auto start = std::chrono::high_resolution_clock::now();
		std::string url = TCHAR_TO_UTF8(*DatasetURL);
		ret = ReadVariable(url, "lon", "[0:1:4499]", LongitudeCache);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		UE_LOG(LogTemp, Warning, TEXT("Finished loading HYCOM longitude. Elapsed time %g seconds."),
			elapsed_seconds.count());
		IsLongitudeDirty = false;
	}

	Longitude.Insert(LongitudeCache, 0);
	return ret;
}
bool FnetcdfunrealModule::
LoadHYCOMTime(const FString& DatasetURL, TArray<double>& InTime)
{
	bool ret = true;
	if (IsTimeDirty) {
		auto start = std::chrono::high_resolution_clock::now();
		std::string url = TCHAR_TO_UTF8(*DatasetURL);
		ret = ReadVariable(url, "time", "[0:1:1983]", TimeCache);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		UE_LOG(LogTemp, Warning, TEXT("Finished loading HYCOM time. Elapsed time %g seconds."),
			elapsed_seconds.count());
		IsTimeDirty = false;
	}

	InTime.Insert(TimeCache, 0);
	return ret;
}

/// <summary>
/// Get the SoundSpeed profile.
/// Indices are not checked.
/// </summary>
/// <param name="DatasetURL">url for hycom</param>
/// <param name="TimeIndexLow"></param>
/// <param name="TimeIndexHigh"></param>
/// <param name="DepthIndexLow"></param>
/// <param name="DepthIndexHigh"></param>
/// <param name="LatitudeIndexLow"></param>
/// <param name="LatitudeIndexHigh"></param>
/// <param name="LongitudeIndexLow"></param>
/// <param name="LongitudeIndexHigh"></param>
/// <param name="SoundSpeed">order is time, depth, lat, lon</param>
/// <returns>success</returns>
bool FnetcdfunrealModule::LoadHYCOMSoundSpeed(const FString& DatasetURL,
	const int& TimeIndexLow, const int& TimeIndexHigh,
	const int& DepthIndexLow, const int& DepthIndexHigh,
	const int& LatitudeIndexLow, const int& LatitudeIndexHigh,
	const int& LongitudeIndexLow, const int& LongitudeIndexHigh,
	TArray<double>& SoundSpeed)
{
	std::string url = TCHAR_TO_UTF8(*DatasetURL);
	std::string water_column = url + "?depth" +
		"[" + std::to_string(DepthIndexLow) + ":1:" + std::to_string(DepthIndexHigh) + "]" +
		",lat"
		"[" + std::to_string(LatitudeIndexLow) + ":1:" + std::to_string(LatitudeIndexHigh) + "]" +
		",lon"
		"[" + std::to_string(LongitudeIndexLow) + ":1:" + std::to_string(LongitudeIndexHigh) + "]" +
		",water_temp" +
		"[" + std::to_string(TimeIndexLow) + ":1:" + std::to_string(TimeIndexHigh) + "]" +
		"[" + std::to_string(DepthIndexLow) + ":1:" + std::to_string(DepthIndexHigh) + "]" +
		"[" + std::to_string(LatitudeIndexLow) + ":1:" + std::to_string(LatitudeIndexHigh) + "]" +
		"[" + std::to_string(LongitudeIndexLow) + ":1:" + std::to_string(LongitudeIndexHigh) + "]" +
		",salinity" +
		"[" + std::to_string(TimeIndexLow) + ":1:" + std::to_string(TimeIndexHigh) + "]" +
		"[" + std::to_string(DepthIndexLow) + ":1:" + std::to_string(DepthIndexHigh) + "]" +
		"[" + std::to_string(LatitudeIndexLow) + ":1:" + std::to_string(LatitudeIndexHigh) + "]" +
		"[" + std::to_string(LongitudeIndexLow) + ":1:" + std::to_string(LongitudeIndexHigh) + "]";

	/*if (LastURL == water_column) {
		SoundSpeed.Insert(SoundSpeedCache, 0);
		return true;
	}*/

	if (USaveHycom* LoadedGame = Cast<USaveHycom>(UGameplayStatics::LoadGameFromSlot("Test", 0)))
	{
		// The operation was successful, so LoadedGame now contains the data we saved earlier.
		UE_LOG(LogTemp, Warning, TEXT("LOADED: %s"), *LoadedGame->SaveSlotName);
		SoundSpeed.Insert(LoadedGame->SaveData.SoundSpeedCache, 0);
		return true;
	}

	UE_LOGFMT(LogTemp, Warning, "Loading net file {0}", water_column.c_str());
	auto start = std::chrono::high_resolution_clock::now();

	netCDF::NcFile remoteFile(water_column, netCDF::NcFile::read);
	netCDF::NcVar varDepth = remoteFile.getVar("depth");
	netCDF::NcVar varLatitude = remoteFile.getVar("lat");
	netCDF::NcVar varLongitude = remoteFile.getVar("lon");
	netCDF::NcVar varSalinity = remoteFile.getVar("salinity");

	int NumberSamples = (TimeIndexHigh - TimeIndexLow + 1) *
		(DepthIndexHigh - DepthIndexLow + 1) *
		(LatitudeIndexHigh - LatitudeIndexLow + 1) *
		(LongitudeIndexHigh - LongitudeIndexLow + 1);

	TArray<double> depths;
	depths.SetNumUninitialized(DepthIndexHigh - DepthIndexLow + 1);
	varDepth.getVar(depths.GetData());

	TArray<double> latitudes;
	latitudes.SetNumUninitialized(LatitudeIndexHigh - LatitudeIndexLow + 1);
	varLatitude.getVar(latitudes.GetData());

	TArray<double> longitudes;
	longitudes.SetNumUninitialized(LongitudeIndexHigh - LongitudeIndexLow + 1);
	varLongitude.getVar(longitudes.GetData());

	TArray<double> waterTemperatures;
	auto waterTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [&]
		{
			UE_LOG(LogTemp, Warning, TEXT("Starting water download"));
			netCDF::NcFile remoteFile2(water_column, netCDF::NcFile::read);
			netCDF::NcVar varWaterTemperature = remoteFile2.getVar("water_temp");
			HYCOMShortToDoubleWithScale(
				varWaterTemperature,
				NumberSamples,
				waterTemperatures);
			UE_LOG(LogTemp, Warning, TEXT("Done water download"));
		}
	);

	TArray<double> salinities;
	auto salinityTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [&]
		{
			UE_LOG(LogTemp, Warning, TEXT("Starting salinity download"));
			HYCOMShortToDoubleWithScale(varSalinity, NumberSamples, salinities);
			UE_LOG(LogTemp, Warning, TEXT("Done salinity download"));
		}
	);

	UE::Tasks::Wait(TArray{ waterTask, salinityTask });

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	UE_LOG(LogTemp, Warning, TEXT("Finished loading HYCOM. Elapsed time %g seconds."),
		elapsed_seconds.count());

	SoundSpeedCache.SetNumUninitialized(NumberSamples);
	TeosSea seaUtilities;

	const double DefaultSoundSpeed = 1500.0;
	int i = 0;
	for (int t = 0; t < TimeIndexHigh - TimeIndexLow + 1; ++t) {
		for (int d = 0; d < DepthIndexHigh - DepthIndexLow + 1; ++d) {
			for (int lat = 0; lat < LatitudeIndexHigh - LatitudeIndexLow + 1; ++lat) {
				for (int lon = 0; lon < LongitudeIndexHigh - LongitudeIndexLow + 1; ++lon) {
					if (waterTemperatures[i] > 0 && salinities[i] > 0) {
						double p = seaUtilities.gsw_p_from_z(-1.0 * depths[d],
							latitudes[lat]);
						double sa = seaUtilities.gsw_sa_from_sp(
							salinities[i],
							p,
							longitudes[lon],
							latitudes[lat]
						);
						SoundSpeedCache[i] =
							seaUtilities.gsw_sound_speed_t_exact(sa, waterTemperatures[i], p);
					}
					else {
						SoundSpeedCache[i] = DefaultSoundSpeed;
					}
					++i;
				}
			}
		}
	}

	SoundSpeed.Insert(SoundSpeedCache, 0);
	if (USaveHycom* SaveGameInstance = Cast<USaveHycom>(UGameplayStatics::CreateSaveGameObject(USaveHycom::StaticClass())))
	{
		SaveGameInstance->SaveData.HycomURL.append(water_column);
		SaveGameInstance->SaveData.SoundSpeedCache.Append(SoundSpeedCache);

		if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->SaveSlotName, SaveGameInstance->UserIndex))
		{
			// Save succeeded.
			UE_LOG(LogTemp, Warning, TEXT("Save was successful!"));
		}
	}
	LastURL = water_column;

	return true;
}

/// <summary>
/// Mark all the HYCOM variables for recalculation.
/// </summary>
void FnetcdfunrealModule::MarkAllHYCOMDirty()
{
	IsDepthDirty = true;
	IsLongitudeDirty = true;
	IsLatitudeDirty = true;
	LastURL = "";
}

/// <summary>
/// Helper to make the call to HYCOM. 1D only
/// No checking that the variable name exists.
/// </summary>
/// <param name="DatasetURL">url for hycom</param>
/// <param name="Variable">variable name</param>
/// <param name="Range">e.g. [1:1:10]</param>
/// <param name="Values">values</param>
/// <returns>success</returns>
bool FnetcdfunrealModule::ReadVariable(
	std::string DatasetURL, 
	std::string Variable, 
	std::string Range, 
	TArray<double>& Values) const
{
	DatasetURL += "?";
	DatasetURL += Variable;
	DatasetURL += Range;
	netCDF::NcFile remoteFile(DatasetURL, netCDF::NcFile::read);
	netCDF::NcDim dim = remoteFile.getDim(Variable);
	netCDF::NcVar var = remoteFile.getVar(Variable);

	TSharedPtr<double> buffer(new double[dim.getSize()]);

	var.getVar(buffer.Get());

	Values.SetNumUninitialized(dim.getSize());
	for (int i = 0; i < dim.getSize(); ++i) {
		Values[i] = buffer.Get()[i];
	}

	remoteFile.close();
	return true;
}

/// <summary>
/// Convert shorts to doubles, including scale and offset.
/// Assumes HYCOM format.
/// TODO: values hacked in.
/// </summary>
/// <param name="Source">TODO: must have attributes add_offset and scale</param>
/// <param name="NumberSamples"></param>
/// <param name="Converted">modified</param>
/// <returns>success</returns>
bool FnetcdfunrealModule::HYCOMShortToDoubleWithScale(
	const netCDF::NcVar& Source, const size_t NumberSamples,
	TArray<double>& Converted) const {

	const double DefaultSoundSpeed = 1500.0;
	TArray<int16_t> buffer;
	buffer.SetNumUninitialized(NumberSamples);

	Source.getVar(buffer.GetData());

	double Scale = 0.001;
	double AddOffset = 20.0;
	//TODO: should read these
	//Source.getAtt("scale").getValues(&Scale);
	//Source.getAtt("add_offset").getValues(&AddOffset);

	Converted.SetNumUninitialized(NumberSamples);
	for (int i = 0; i < NumberSamples; ++i) {
		Converted[i] = buffer[i] * Scale + AddOffset;
	}

	return true;
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FnetcdfunrealModule, netcdfunreal)
