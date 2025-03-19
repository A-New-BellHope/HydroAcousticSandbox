// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <chrono>

#include "Modules/ModuleManager.h"
#include "Logging/StructuredLog.h"

#include "netcdfunrealLibrary/include/netcdf"
#include "TeosSea.h"

class FnetcdfunrealModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	//interface

	bool LoadBathymetryFile(const FString& Filename);
	void UnloadBathymetryFile();

	bool GetArray(const FString& Variable, TArray<double>& Value) const;
	bool GetArraySubset(const FString& Variable,
		const std::vector<size_t>& start, const std::vector<size_t>& count,
		TArray<double>& Value) const;

	bool CheckNcFile() const;

	bool LoadHYCOMDepth(const FString& DatasetURL, TArray<double>& Depth);
	bool LoadHYCOMLatitude(const FString& DatasetURL, TArray<double>& Latitude);
	bool LoadHYCOMLongitude(const FString& DatasetURL, TArray<double>& Longitude);
	bool LoadHYCOMTime(const FString& DatasetURL, TArray<double>& InTime);
	bool LoadHYCOMSoundSpeed(const FString& DatasetURL,
		const int& TimeIndexLow, const int& TimeIndexHigh,
		const int& DepthIndexLow, const int& DepthIndexHigh,
		const int& LatitudeIndexLow, const int& LatitudeIndexHigh,
		const int& LongitudeIndexLow, const int& LongitudeIndexHigh,
		TArray<double>& SoundSpeed);

	void MarkAllHYCOMDirty();

private:
	//helpers

	bool ReadVariable(std::string DatasetURL, std::string Variable, 
		std::string Range, TArray<double>& Values) const;

	bool HYCOMShortToDoubleWithScale(
		const netCDF::NcVar& Source,
		const size_t NumberSamples,
		TArray<double>& Converted) const;

private:

	void*	NetCDFLibraryHandle;
	netCDF::NcFile DataFile;

	bool IsDepthDirty = true;
	bool IsLongitudeDirty = true;
	bool IsLatitudeDirty = true;
	bool IsTimeDirty = true;

	std::string LastURL = "";

	TArray<double> DepthCache;
	TArray<double> LongitudeCache;
	TArray<double> LatitudeCache;
	TArray<double> TimeCache;
	TArray<double> SoundSpeedCache;

};
