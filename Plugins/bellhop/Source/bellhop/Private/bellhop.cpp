// Copyright Epic Games, Inc. All Rights Reserved.

#include "bellhop.h"

#define LOCTEXT_NAMESPACE "FbellhopModule"

/// <summary>
/// Intended for callback from bellhop.
/// TODO: global function may be improved.
/// </summary>
/// <param name="message"></param>
void LogBellhop(const char* Message)
{
	FString FMessage = FString(Message);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *FMessage);
}

void LogBellhopPRT(const char* Message)
{
	std::string MarkedMessage = "PRT: ";
	MarkedMessage += Message;
	LogBellhop(MarkedMessage.c_str());
}
void LogBellhopOutput(const char* Message)
{
	std::string MarkedMessage = "Output: ";
	MarkedMessage += Message;
	LogBellhop(MarkedMessage.c_str());
}

FbellhopModule::FbellhopModule(): BellhopLibraryHandle(nullptr),
recalculateRays(true),
working(false),
MaxArrivalTime(-1.0),
lastFileName("")
{
	ResetBellhopInitialization();
}

void FbellhopModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Get the base directory of this plugin
	FString BaseDir = IPluginManager::Get().FindPlugin("bellhop")->GetBaseDir();

	// Add on the relative location of the third party dll and load it
	FString LibraryPath;
#if PLATFORM_WINDOWS
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/bellhopLibrary/Win64/bellhopcxxlib.dll"));
#elif PLATFORM_MAC
	//not supported - LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/bellhopLibrary/Mac/Release/libExampleLibrary.dylib"));
#elif PLATFORM_LINUX
	//not supported - LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/bellhopLibrary/Linux/x86_64-unknown-linux-gnu/libExampleLibrary.so"));
#endif // PLATFORM_WINDOWS

	BellhopLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;
	UE_LOG(LogTemp, Warning, TEXT("found bellhop dll"));

	if (IsBellhopReady())
	{
		LogBellhop("Imported bellhop library");
	}
	else
	{
		LogBellhop("Failed to load bellhop library");
	}
}

void FbellhopModule::ShutdownModule()
{
	if (IsBellhopSetup()) {
		std::visit([](auto& x)
			{
				bhc::finalize(x.first, x.second);
			}, params);
		IsSetup = false;
	}

	FPlatformProcess::FreeDllHandle(BellhopLibraryHandle);
	LogBellhop("Released Bellhop library.");
	BellhopLibraryHandle = nullptr;
}

/// <summary>
/// Check if the library loaded ok.
/// </summary>
/// <returns>success</returns>
bool FbellhopModule::IsBellhopReady()
{
	return (bool)BellhopLibraryHandle;
}

/// <summary>
/// Check if bellhop's setup has been called.
/// If it's not setup, then it's been finalized and there may
///   not be any memory allocated.
/// </summary>
/// <returns>success</returns>
bool FbellhopModule::IsBellhopSetup()
{
	return IsSetup;
}

void FbellhopModule::MarkBellhopRun(const bool& State)
{ 
	bBellhopRun = State;

	if (IsRayMode()) {
		bRaysUpdated = State;
	}

	if (IsTransmissionLossMode()) {
		bTransmissionLossUpdated = State;
	}

}

/// <summary>
/// Write out the environment data for external use.
/// </summary>
/// <param name="FileRoot">Fully qualified name, without any extension</param>
/// <returns></returns>
bool FbellhopModule::WriteEnvironment(FString FileRoot)
{
	bool res = false;
	if (IsBellhopSetup()) {
		std::visit([&](auto& x)
			{
				auto fname = StringCast<ANSICHAR>(*FileRoot).Get();
				res = bhc::writeenv(x.first, fname);
			}, params);
	}
	return res;
}

TArray<FVector> FbellhopModule::GetBoundaryPoints()
{
	TArray<FVector> ret;

	if (std::holds_alternative<RunType2D>(params))
	{
		auto const& x = std::get<RunType2D>(params).first;
		for(int i = 0; i < x.bdinfo->bot.NPts; ++i)
		{
			ret.Add(FVector(x.bdinfo->bot.bd[i].x.x, 0, -x.bdinfo->bot.bd[i].x.y));
		}
	}
	else if (std::holds_alternative<RunTypeNx2D>(params))
	{
		auto const& x = std::get<RunTypeNx2D>(params).first;
		for (int iy = 0; iy < x.bdinfo->bot.NPts.y; ++iy)
		{
			for (int ix = 0; ix < x.bdinfo->bot.NPts.x; ++ix)
			{
				const auto& b = x.bdinfo->bot.bd[ix * x.bdinfo->bot.NPts.y + iy];
				ret.Add(FVector(b.x.x, b.x.y, -b.x.z));
			}
		}
	}
	else if (std::holds_alternative<RunType3D>(params))
	{
		auto const& x = std::get<RunType3D>(params).first;
		for (int iy = 0; iy < x.bdinfo->bot.NPts.y; ++iy)
		{
			for (int ix = 0; ix < x.bdinfo->bot.NPts.x; ++ix)
			{
				const auto& b = x.bdinfo->bot.bd[ix * x.bdinfo->bot.NPts.y + iy];
				ret.Add(FVector(b.x.x, b.x.y, -b.x.z));
			}
		}
	}

	return ret;
}

/// <summary>
/// Get and set for the Title.
/// At most 80 characters hardcoded in bellhop.
/// </summary>
/// <returns></returns>
void FbellhopModule::SetTitle(const FString& Title)
{
	std::visit([&](auto& x) mutable
		{
			auto FNameConvert = StringCast<ANSICHAR>(*Title).Get();
			size_t n = std::min( (size_t)80, strlen(FNameConvert));
			strncpy(x.first.Title, FNameConvert, n);
		}, params);
}
FString FbellhopModule::GetTitle() const
{
	std::visit([](auto& x)
		{
			size_t n = strnlen(x.first.Title, 80);
			return FString(n, x.first.Title);
		}, params);
	return FString("--Bad Title--");
}

/// <summary>
/// Set the bottom and top.
/// TODO: the top is only flat.
/// </summary>
/// <param name="Depths">Access as Depths[i + j*NumberY]</param>
/// <param name="XGrid"></param>
/// <param name="YGrid"></param>
/// <param name="O3D"></param>
void FbellhopModule::SetBottom(const TArray<double>& Depths,
	const TArray<double>& XGrid, const TArray<double>& YGrid,
	bool O3D)
{
	if (std::holds_alternative<RunType2D>(params))
	{
		LogBellhop("FbellhopModule::GetBoundarySize: TODO ... 2D not supported yet.");
	}
	else if (std::holds_alternative<RunTypeNx2D>(params) )
	{
		LogBellhop("FbellhopModule::GetBoundarySize: TODO ... Nx2D not supported yet.");
	}
	else if (std::holds_alternative<RunType3D>(params))
	{
		//Not intended to be called too often, so reallocation is safest.
		bhc::extsetup_altimetry(std::get<RunType3D>(params).first, { 2,2 });
		bhc::IORI2<true> Size = { XGrid.Num(), YGrid.Num() };
		bhc::extsetup_bathymetry(std::get<RunType3D>(params).first, Size);

		UpdateBoundary3D(std::get<RunType3D>(params).first.bdinfo->top,
			{ 0, 0, 0, 0 },
			{ XGrid[0], XGrid.Last() },
			{ YGrid[0], YGrid.Last() });
		UpdateBoundary3D(std::get<RunType3D>(params).first.bdinfo->bot,
			Depths, XGrid, YGrid);
	}
}

/// <summary>
/// Update the boundary data and set flags.
/// No checking for size.
/// </summary>
/// <param name="Boundary">values modified. Must have room for data.</param>
/// <param name="Depth">meters</param>
/// <param name="GridX">meters</param>
/// <param name="GridY">meters</param>
void FbellhopModule::UpdateBoundary3D(bhc::BdryInfoTopBot<true>& Boundary,
	const TArray<double>& Depth,
	const TArray<double>& GridX, const TArray<double>& GridY) const
{
	Boundary.dirty = true;
	Boundary.rangeInKm = false;
	Boundary.NPts[0] = GridX.Num();
	Boundary.NPts[1] = GridY.Num();

	for (int iy = 0; iy < GridY.Num(); ++iy) {
		for (int ix = 0; ix < GridX.Num(); ++ix) {
			Boundary.bd[ix * GridY.Num() + iy].x.x = GridX[ix];
			Boundary.bd[ix * GridY.Num() + iy].x.y = GridY[iy];
			Boundary.bd[ix * GridY.Num() + iy].x.z = Depth[ix * GridY.Num() + iy];
		}
	}

}


/// <summary>
/// The number of points in the x direction.
/// May return 0 if nothing has been calculated.
/// </summary>
/// <returns>points in the x (or only) direction</returns>
int FbellhopModule::GetBoundarySizeX()
{
	int ret = 0;
	if (std::holds_alternative<RunType2D>(params))
	{
		ret = std::get<RunType2D>(params).first.bdinfo->bot.NPts;
	}
	else if (std::holds_alternative<RunTypeNx2D>(params))
	{
		//TODO: not supported
		LogBellhop("FbellhopModule::GetBoundarySize: Nx2D not supported ... continuing with blank bottom.");
	}
	else if (std::holds_alternative<RunType3D>(params))
	{
		ret = std::get<RunType3D>(params).first.bdinfo->bot.NPts.x;
	}

	return ret;
}

/// <summary>
/// The number of points in the x direction.
/// May return 0 if nothing has been calculated.
/// </summary>
/// <returns>points in the x (or only) direction</returns>
int FbellhopModule::GetBoundarySizeY()
{
	int ret = 0;
	//nothing to do for 2D
	if (std::holds_alternative<RunTypeNx2D>(params))
	{
		//TODO: not supported
		LogBellhop("FbellhopModule::GetBoundarySize: Nx2D not supported ... continuing with blank bottom.");
	}
	else if (std::holds_alternative<RunType3D>(params))
	{
		ret = std::get<RunType3D>(params).first.bdinfo->bot.NPts.y;
	}

	return ret;
}

FString FbellhopModule::GetBellhopDirectory()
{
	return IPluginManager::Get().FindPlugin("bellhop")->GetBaseDir();
}

/// <summary>
/// The default directory with bellhop env files.
/// </summary>
/// <returns>plugin directory</returns>
FString FbellhopModule::GetBellhopEnvironmentDirectory()
{
	return IPluginManager::Get().FindPlugin("bellhop")->GetBaseDir() +
		"/Source/ThirdParty/bellhopLibrary/EnvironmentFiles";
}

/// <summary>
/// Sets up and runs Bellhop on the given file.
/// Specify 2D, Nx2D, or 3D as
/// 2D - O3D=false, R3D=false
/// Nx2D - O3D=true, R3D=false
/// 3D - O3D=true, R3D=true
/// </summary>
/// <param name="fname">Bellhop env filename. no checking</param>
/// <param name="O3D">3D ocean environment</param>
/// <param name="R3D">3D rays</param>
/// <returns>1 for success, -1 for failure</returns>
int FbellhopModule::RunBellhopFile(FString fname, bool O3D, bool R3D) {
	FString BaseDir = IPluginManager::Get().FindPlugin("bellhop")->GetBaseDir();
	UE_LOG(LogTemp, Warning, TEXT("Bellhop root directory: %s"), *BaseDir);

	if (working) {
		//not safe to call multiple times
		UE_LOG(LogTemp, Warning, TEXT("multiple calls to bellhop ... ignoring"));
		return -1;
	}

	if ((recalculateRays || lastFileName != fname) && !lastFileName.IsEmpty()) {
		//need to cleanup for a new read
		std::visit([] (auto x)
			{
				bhc::finalize(x.first, x.second);
			}, params);
		recalculateRays = true;
		IsSetup = false;
		UE_LOG(LogTemp, Warning, TEXT("Finalized bellhop."));
	}

	working = true;
	MarkBellhopRun(false);
	int ret = 1;
	lastFileName = fname;

	if (recalculateRays) {
		recalculateRays = false;
		UE_LOG(LogTemp, Warning, TEXT("accessed bellhop"));

		if (!O3D && !R3D)
		{
			params.emplace<RunType2D>();
		}
		else if (O3D && !R3D)
		{
			params.emplace<RunTypeNx2D>();
			//TODO: support nx2d
			LogBellhop("Warning: Nx2D not implemented. Unknown results to follow.");
		}
		else if (O3D && R3D)
		{
			params.emplace<RunType3D>();
		}
		else
		{
			LogBellhop("Warning: 3D rays in a 2D ocean not allowed. Check O3D and R3D...unknown behavior");
		}

		BellhopInitializaiton.FileRoot = StringCast<ANSICHAR>(*fname).Get();

		std::visit([&](auto& x)
			{
				auto FNameConvert = StringCast<ANSICHAR>(*fname);
				BellhopInitializaiton.FileRoot = FNameConvert.Get();
				bhc::setup(BellhopInitializaiton, x.first, x.second);
				IsSetup = true;
			}, params);
		std::visit([](auto& x)
			{
				LogBellhop("Started bhc::run");
				bhc::run(x.first, x.second);
				LogBellhop("Finished bhc::run");
			}, params);
		UpdateAllRays();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("bellhop already calculated"));
	}

	working = false;
	MarkBellhopRun(true);
	return ret;
}

/// <summary>
/// Create a minimal default environment.
/// </summary>
/// <param name="O3D">3D ocean</param>
/// <param name="R3D">2D ocean</param>
void FbellhopModule::SetupDefaults(const bool& O3D, const bool& R3D)
{
	//TODO: should standardize the cleanup
	if (working) {
		UE_LOG(LogTemp, Warning, 
			TEXT("Warning: trying to run FbellhopModule::SetupDefaults "
			"while already working ... ignoring"));
		return;
	}

	if (IsBellhopSetup()) {
		std::visit([](auto& x)
			{
				bhc::finalize(x.first, x.second);
			}, params);
		IsSetup = false;
	}

	MarkBellhopRun(false);
	if (!O3D && !R3D) {
		params.emplace<RunType2D>();
	}
	else if (O3D && !R3D) {
		params.emplace<RunTypeNx2D>();
	}
	else if (O3D && R3D) {
		params.emplace<RunType3D>();
	}
	else {
		LogBellhop("Warning: 3D rays in a 2D ocean not allowed. Check O3D and R3D...unknown behavior");
	}

	ResetBellhopInitialization();

	std::visit([&](auto& x)
		{
			bhc::setup(BellhopInitializaiton, x.first, x.second);
			IsSetup = true;
		}, params);
}


/// <summary>
/// Run bellhop with current parameters.
/// Call after modifying params in Unreal.
/// </summary>
void FbellhopModule::RunBellhop()
{
	if (working) {
		//not safe to call multiple times
		UE_LOG(LogTemp, Warning, TEXT("multiple calls to bellhop ... ignoring"));
		return;
	}

	MarkBellhopRun(false);

	std::visit([&](auto& x)
		{
			bhc::echo(x.first);
			if (!bhc::run(x.first, x.second)) {
				LogBellhop("Error in bhc::run. Check the log. ... continuing");
			}
			UpdateAllRays();
		}, params);

	working = false;
	MarkBellhopRun(true);
}

/// <summary>
/// Get the number of rays. Use this to loop.
/// Always safe to call. 0 indicates no data.
/// </summary>
/// <returns>Total rays calculated.</returns>
int FbellhopModule::GetNRays() const {
	int ret = 0;
	std::visit([&](auto& x)
		{
			if (x.second.rayinfo) {
				ret = x.second.rayinfo->NRays;
			}
		}, params);
	return ret;
}

/// <summary>
/// Get the max number of points in each ray.
/// Always safe to call. May be 0.
/// </summary>
/// <returns>max points on each ray.</returns>
int FbellhopModule::GetMaxPoints() {
	int ret = 0;
	std::visit([&](auto& x)
		{
			if (x.second.rayinfo) {
				ret = x.second.rayinfo->MaxPointsPerRay;
			}
		}, params);
	return ret;
}

/// <summary>
/// Altitude of a single ray in degrees.
/// Display an error and ignore if the ray doesn't exist.
/// </summary>
/// <param name="rayID">should exist</param>
/// <returns>degrees from horizontal, negetive up</returns>
float FbellhopModule::GetSingleRayAltitude(const int& rayID) const
{
	float ret = 1.0e10f;
	if (rayID < 0 || rayID >= GetNRays()) {
		std::string msg = "Warning in GetSingleRayAltitude: ray " + 
			std::to_string(rayID) + " out of range [0, " + 
			std::to_string(GetNRays()) + ") ... ignoring.";
		LogBellhop(msg.c_str());
		return ret;
	}
	
	ret = std::visit([=](auto& x)
		{
			return FMath::RadiansToDegrees(
				x.first.Angles->alpha.angles[rayID]);
		}, params);

	return ret;
}

/// <summary>
/// Set the altitude of a single ray in degrees.
/// Display an error and ignore if the ray doesn't exist.
/// </summary>
/// <param name="altitude">degrees from horizontal, negetive up</param>
/// <param name="rayID">should exist</param>
void FbellhopModule::SetSingleRayAltitude(const float& altitude, const int& rayID)
{
	if (rayID < 0 || rayID >= GetNRays()) {
		std::string msg = "Warning in SetSingleRayAltitude: ray " +
			std::to_string(rayID) + " out of range [0, " +
			std::to_string(GetNRays()) + ") ... ignoring.";
		LogBellhop(msg.c_str());
	}

	std::visit([=](auto& x)
		{
			x.first.Angles->alpha.angles[rayID] =
				FMath::DegreesToRadians(altitude);
		}, params);
}

/// <summary>
/// Altitude of a single ray in degrees.
/// Display an error and ignore if the ray doesn't exist.
/// </summary>
/// <param name="rayID">should exist</param>
/// <returns>degrees from horizontal, negetive up</returns>
float FbellhopModule::GetSingleRayAzimuth(const int& rayID) const
{
	float ret = 1.0e10f;
	if (rayID < 0 || rayID >= GetNRays()) {
		std::string msg = "Warning in GetSingleRayAzimuth: ray " +
			std::to_string(rayID) + " out of range [0, " +
			std::to_string(GetNRays()) + ") ... ignoring.";
		LogBellhop(msg.c_str());
		return 1.0e10f;
	}

	ret = std::visit([=](auto& x)
		{
			return FMath::RadiansToDegrees(
				(float)x.first.Angles->beta.angles[rayID]);
		}, params);

	return ret;
}

/// <summary>
/// Set the azimuth of a single ray in degrees.
/// Display an error and ignore if the ray doesn't exist.
/// </summary>
/// <param name="altitude">degrees from horizontal, negetive up</param>
/// <param name="rayID">should exist</param>
void FbellhopModule::SetSingleRayAzimuth(const float& azimuth, const int& rayID)
{
	if (rayID < 0 || rayID >= GetNRays()) {
		std::string msg = "Warning in SetSingleRayAzimuth: ray " +
			std::to_string(rayID) + " out of range [0, " +
			std::to_string(GetNRays()) + ") ... ignoring.";
		LogBellhop(msg.c_str());
	}

	std::visit([=](auto& x)
		{
			x.first.Angles->beta.angles[rayID] =
				FMath::DegreesToRadians(azimuth);
		}, params);
}

/// <summary>
/// Altitude spread of the test rays.
/// Negative is up. Angles are in degrees.
/// </summary>
/// <param name="low">modified to lower bound</param>
/// <param name="high">modified to upper bound</param>
/// <param name="n">modified to number of rays</param>
void FbellhopModule::GetRayAltitudes(float& low, float& high, int& n) const
{
	std::visit([&](auto& x)
		{
			n = x.first.Angles->alpha.n;
			if (n < 2)
			{
				LogBellhop("Error (GetRayAltitudes): not enough angle data ... continuing, but this is bad");
				return;
			}
			low = FMath::RadiansToDegrees(x.first.Angles->alpha.angles[0]);
			high = FMath::RadiansToDegrees(x.first.Angles->alpha.angles[n-1]);
		}, params);
}

/// <summary>
/// Set the altitude spread of the test rays.
/// Inclusive range from low to high, e.g.,
///    if you want [1,2,3,4] set low=1, high=4, steps=4
/// Negative is up. Angles are in degrees.
/// <param name="low">start angle (degrees)</param>
/// <param name="high">stop angle (degrees)</param>
/// <param name="n">total number of angles</param>
/// </summary>
void FbellhopModule::
SetRayAltitudes(const float& low, const float& high, const int& n)
{
	std::visit([=](auto& x)
		{
			//avoid realoccation if possible
			if (x.first.Angles->alpha.n != n) {
				bhc::extsetup_rayelevations(x.first, n);
			}
			x.first.Angles->alpha.inDegrees = true;
			x.first.Angles->alpha.d = (n==1) ? 1 : (high - low) / float(n - 1);
			for (int i = 0; i < n; ++i) {
				x.first.Angles->alpha.angles[i] = low + float(i) * x.first.Angles->alpha.d;
			}
		}, params);
}


/// <summary>
/// Azimuth  (horizontal) spread of the test rays.
/// Angles are in degrees.
/// </summary>
/// <param name="low">modified to lower bound</param>
/// <param name="high">modified to upper bound</param>
/// <param name="n">modified to number of rays</param>
void FbellhopModule::GetRayAzimuths(float& low, float& high, int& n) const
{
	std::visit([&](auto& x)
		{
			n = x.first.Angles->beta.n;
			if (n < 2)
			{
				LogBellhop("Error (GetRayAzimuth): not enough angle data ... continuing, but this is bad or you may be runnnig in 2D mode.");
				return;
			}
			low = FMath::RadiansToDegrees(x.first.Angles->beta.angles[0]);
			high = FMath::RadiansToDegrees(x.first.Angles->beta.angles[n - 1]);
		}, params);
}

/// <summary>
/// Set the azimuth (bearings) spread of the test rays.
/// Inclusive range from low to high, e.g.,
///    if you want [1,2,3,4] set low=1, high=4, steps=4
/// Negative is up. Angles are in degrees.
/// <param name="low">start angle (degrees)</param>
/// <param name="high">stop angle (degrees)</param>
/// <param name="n">total number of angles</param>
/// </summary>
void FbellhopModule::
SetRayAzimuths(const float& low, const float& high, const int& n)
{
	std::visit([=](auto& x)
		{
			//avoid realoccation if possible
			if (x.first.Angles->beta.n != n) {
				bhc::extsetup_raybearings(x.first, n);
			}
			x.first.Angles->beta.inDegrees = true;
			x.first.Angles->beta.d = (n == 1) ? 1 : (high - low) / float(n - 1);
			for (int i = 0; i < n; ++i) {
				x.first.Angles->beta.angles[i] = low + float(i) * x.first.Angles->beta.d;
			}
		}, params);
}

/// <summary>
/// Set the number of points in the altitude.
/// Does not change the low and high.
/// </summary>
/// <param name="n">new count, must be positive</param>
void FbellhopModule::SetAltitudeCount(const int& n)
{
	float low, high;
	int oldN;
	GetRayAltitudes(low, high, oldN);
	SetRayAltitudes(low, high, n);
}

/// <summary>
/// Set the number of points in azimuth.
/// Does not change the low and high.
/// </summary>
/// <param name="n">new count, must be positive</param>
void FbellhopModule::SetAzimuthCount(const int& n)
{
	float low, high;
	int oldN;
	GetRayAzimuths(low, high, oldN);
	SetRayAzimuths(low, high, n);
}

/// <summary>
/// Get the receiver locations - range, depth, and bearing.
/// In 2D there's only 1 bearing at 0 degrees.
/// In 3D Bearing 0 is east, bearing 90 is south (Bellhop convention).
/// </summary>
/// <returns>range (m), depth (m), and bearing (degrees)</returns>
TArray<float> FbellhopModule::GetReceiverRanges() const
{
	TArray<float> ret;
	std::visit([&](auto& x)
		{
			for (int i = 0; i < x.first.Pos->NRr; ++i)
			{
				ret.Add(x.first.Pos->Rr[i]);
			}
		}, params);
	return ret;
}
TArray<float> FbellhopModule::GetReceiverDepths() const
{
	TArray<float> ret;
	std::visit([&](auto& x)
		{
			for (int i = 0; i < x.first.Pos->NRz; ++i)
			{
				ret.Add(x.first.Pos->Rz[i]);
			}
		}, params);
	return ret;
}
TArray<float> FbellhopModule::GetReceiverBearings() const
{
	TArray<float> ret;
	std::visit([&](auto& x)
		{
			for (int i = 0; i < x.first.Pos->Ntheta; ++i)
			{
				ret.Add(x.first.Pos->theta[i]);
			}
		}, params);
	return ret;
}

/// <summary>
/// Get the receiver locations - range, depth, and bearing.
/// In 2D there's only 1 bearing at 0 degrees.
/// In 3D Bearing 0 is east, bearing 90 is south (Bellhop convention).
/// </summary>
/// <returns>All receiver locations in cylindrical coordinates</returns>
TArray<FVector> FbellhopModule::GetReceivers() const
{
	TArray<FVector> ret;
	for (const auto& r : GetReceiverRanges()) {
		for (const auto& d: GetReceiverDepths()) {
			for (const auto& b: GetReceiverBearings()) {
				ret.Add(FVector(r, d, b));
			}
		}
	}
	return ret;
}

/// <summary>
/// Changes the location of the receiver at the given location.
/// Use to move single receiveres. Only works if the receiver exists.
/// If you want to add recievers, use the individual functions below.
/// </summary>
/// <param name="rangeID">index of range to update</param>
/// <param name="range">in meteres</param>
/// <param name="depthID">index</param>
/// <param name="depth">depth in meters</param>
/// <param name="bearingID">not needed in 2d</param>
/// <param name="bearing">degrees, not needed in 2d</param>
void FbellhopModule::SetReceiver(
	const int& rangeID, const float& range,
	const int& depthID, const float& depth,
	const int& bearingID, const float& bearing)
{
	std::visit([=](auto& x)
		{
			if (rangeID < 0 || rangeID >= x.first.Pos->NRr) {
				LogBellhop("Warning in SetReceiver: bad rangeID ... ignoring.");
				return;
			}
			if (depthID < 0 || depthID >= x.first.Pos->NRz) {
				LogBellhop("Warning in SetReceiver: bad depthID ... ignoring.");
				return;
			}
			if (bearingID < 0 || bearingID >= x.first.Pos->Ntheta) {
				LogBellhop("Warning in SetReceiver: bad bearingID ... ignoring.");
				return;
			}

			x.first.Pos->Rr[rangeID] = range;
			x.first.Pos->Rz[depthID] = depth;
			x.first.Pos->theta[bearingID] = bearing;
		}, params);
}

/// <summary>
/// Update the receiver locations. Creates room for the receivers.
/// inclusive counting, e.g. to create [1,2,3,4] low=1, high=4, steps=4
/// </summary>
/// <param name="low">start in meters</param>
/// <param name="high">stop in meters</param>
/// <param name="n">total number</param>
void FbellhopModule::SetReceiverRanges(const float& low, const float& high, const int& n)
{
	std::visit([=](auto& x)
		{
			//avoid realoccation if possible
			if (x.first.Pos->NRr != n) {
				bhc::extsetup_rcvrranges(x.first, n);
			}
			float stepSize = (high - low) / float(n - 1);
			for (int i = 0; i < n; ++i) {
				x.first.Pos->Rr[i] = low + float(i) * stepSize;
			}
		}, params);
}
void FbellhopModule::SetReceiverDepths(const float& low, const float& high, const int& n)
{
	std::visit([=](auto& x)
		{
			//avoid realoccation if possible
			if (x.first.Pos->NRz != n) {
				bhc::extsetup_rcvrdepths(x.first, n);
			}
			float stepSize = (high - low) / float(n - 1);
			for (int i = 0; i < n; ++i) {
				x.first.Pos->Rz[i] = low + float(i) * stepSize;
			}
		}, params);
}
void FbellhopModule::SetReceiverBearings(const float& low, const float& high, const int& n)
{
	std::visit([=](auto& x)
		{
			//avoid realoccation if possible
			if (x.first.Pos->Ntheta != n) {
				bhc::extsetup_rcvrbearings(x.first, n);
			}
			float stepSize = (high - low) / float(n - 1);
			for (int i = 0; i < n; ++i) {
				x.first.Pos->theta[i] = low + float(i) * stepSize;
			}
		}, params);
}


/// <summary>
/// Get points and arrival time for each point.
/// Arrival times are integrated locally.
/// </summary>
/// <param name="ray">[0, GetNRays())</param>
/// <returns>ray position-arrival data for Unreal</returns>
std::map<double, FVector> FbellhopModule::GetAllRayPointArrival(const int& ray) {
	TArray<FVector4> points = GetAllRayPoints(ray);
	TArray<double> arrivals = CalculateArrival(points);
	std::map<double, FVector> ret;
	for (int i = 0; i < points.Num(); ++i) {
		ret[arrivals[i]] = FVector(points[i].X, points[i].Y, points[i].Z);
	}
	return ret;
}

/// <summary>
/// Update the local storage of all the rays or transmission loss data.
/// </summary>
void FbellhopModule::UpdateAllRays() {
	if (IsRayMode()) {
		AllRays.Empty();
		AllRayArrivals.Empty();
		MaxArrivalTime = -1.0;
		int count = GetNRays();
		for (int i = 0; i < count; ++i) {
			AllRays.Add(GetAllRayPoints(i));
			AllRayArrivals.Add(GetAllRayPointArrival(i));
		}
	}
	else if (IsTransmissionLossMode()) {
		GetTransmissionLoss(_TransmissionLoss, _Width, _Height, _Bearings);
		bTransmissionLossUpdated = true;
	}
}

/// <summary>
/// Get the ray point with arrival time information.
/// Must call UpdateAllRays before trying to use this.
/// </summary>
/// <param name="ray">ray id</param>
/// <param name="sample">sample number</param>
/// <returns>x,y,z,t</returns>
FVector4 FbellhopModule::GetRayArrival(const int& ray, const int& sample) {
	if (IsBellhopRun() && ray < AllRays.Num() && sample < AllRays[ray].Num()) {
		return AllRays[ray][sample];
	}
	return FVector4(0, 0, 0, 0);
}

/// <summary>
/// Overload function
/// Get the ray with arrival time
/// Finds ray point that is closest to the lower bound of the sample time
/// </summary>
/// <param name="ray">ray id</param>
/// <param name="time">sample time</param>
/// <returns></returns>
FVector4 FbellhopModule::GetRayArrival(const int& ray, const float& time) {
	if (IsBellhopRun() && ray < AllRayArrivals.Num() && time < MaxArrivalTime) {
		auto iter = AllRayArrivals[ray].lower_bound(time);
		if (iter != AllRayArrivals[ray].end()) {
			return FVector4(iter->second.X, iter->second.Y, iter->second.Z, iter->first);
		}
	}
	auto iter = AllRayArrivals[ray].rbegin();
	return FVector4(iter->second.X, iter->second.Y, iter->second.Z, iter->first);
}

/// <summary>
/// Largest arrival time on the current set of rays.
/// Updated after a call to GetAllRayPointArrival.
/// Value is -1 if it's not updated.
/// </summary>
/// <returns>return precalculated value, no checking</returns>
double FbellhopModule::GetMaxArrivalTime() {
	return MaxArrivalTime;
}

/// <summary>
/// The number of points in the ray structure (for looping)
/// </summary>
/// <param name="ray">ray id</param>
/// <returns>size of the ray arrival</returns>
int FbellhopModule::GetNPoints(const int& ray) {
	return AllRays[ray].Num();
}

/// <summary>
/// Calculate the arrival times by integrating the SSP.
/// Trapezoid integration.
/// </summary>
/// <param name="ray">Ray vertices and speed (x,y,z,c), probably from 
/// FbellhopModule::GetAllRayPoints</param>
/// <returns>ray position-arrival data for Unreal</returns>
TArray<double> FbellhopModule::CalculateArrival(const TArray<FVector4>& ray) {
	TArray<double> ret;
	ret.Add(0);
	double t = 0.0;
	for (int i = 1; i < ray.Num(); ++i) {
		t += 2.0 / (ray[i].W + ray[i-1].W) * (ray[i] - ray[i - 1]).Size3();
		ret.Add(t);
	}

	MaxArrivalTime = std::max(MaxArrivalTime, t);

	return ret;
}

/// <summary>
/// Get all the points in a ray.
/// Actually copies them, so it might be slow.
/// Only reports data with nonzero amplitude.
/// </summary>
/// <param name="ray">[0, GetNRays())</param>
/// <returns>ray position data for Unreal</returns>
TArray<FVector4> FbellhopModule::GetAllRayPoints(const int& ray) {
	TArray<FVector4> ret;
	if (std::holds_alternative<RunType2D>(params))
	{
		auto const& x = std::get<RunType2D>(params).second;
		auto const& RayData = x.rayinfo->results[ray];
		for (int i = 0; i < RayData.Nsteps; ++i) {
			if (RayData.ray[i].Amp > 1.e-6) {
				ret.Add(FVector4(RayData.ray[i].x.x, 0, -RayData.ray[i].x.y, RayData.ray[i].c));
			}
		}
	}
	else if (std::holds_alternative<RunTypeNx2D>(params))
	{
		auto const& x = std::get<RunTypeNx2D>(params).second;
		auto const& RayData = x.rayinfo->results[ray];
		for (int i = 0; i < RayData.Nsteps; ++i) {
			if (RayData.ray[i].Amp > 1.e-6) {
				//TODO: how do we recover the bearing?
				//ret.Add(FVector(RayData.ray[i].x.x, 0, -RayData.ray[i].x.y));
				ret.Add(FVector4(0, 0, 0, RayData.ray[i].c));
			}
		}
	}
	else if (std::holds_alternative<RunType3D>(params))
	{
		auto const& x = std::get<RunType3D>(params).second;
		auto const& RayData = x.rayinfo->results[ray];
		for (int i = 0; i < RayData.Nsteps; ++i) {
			if (RayData.ray[i].Amp > 1.e-6) {
				ret.Add(FVector4(RayData.ray[i].x.x, RayData.ray[i].x.y, -RayData.ray[i].x.z, RayData.ray[i].c));
			}
		}
	}
	return ret;
}

/// <summary>
/// Returns the position of the nth point on the ray.
/// Mostly for testing.
/// </summary>
/// <param name="ray">[0, GetNRays())</param>
/// <param name="sample">[0, GetMaxPoints())</param>
/// <returns>position oriented for unreal, or 0 if the Amp is too small</returns>
FVector FbellhopModule::GetNextPoint(const int& ray, const int& sample) {
	FVector ret(0, 0, 0);

	if (IsBellhopRun())
	{
		//TODO: hacked in MaxN for now, should be in params struct
		int MaxN = GetMaxPoints();
		int maxRays = GetNRays();
		int i = ray * MaxN + sample;
		if (0 <= i && i < MaxN * maxRays)
		{
			if (std::holds_alternative<RunType2D>(params))
			{
				auto const& x = std::get<RunType2D>(params).second;
				if (x.rayinfo->results->ray->Amp > 1.0e-10)
				{
					ret.X = x.rayinfo->results->ray[i].x.x;
					ret.Z = -x.rayinfo->results->ray[i].x.y;
				}
			}
			else if (std::holds_alternative<RunTypeNx2D>(params))
			{
				auto const& x = std::get<RunTypeNx2D>(params).second;
				if (x.rayinfo->results->ray->Amp > 1.0e-10)
				{
					ret.X = x.rayinfo->results->ray[i].x.x;
					ret.Z = -x.rayinfo->results->ray[i].x.y;
				}
			}
			else if (std::holds_alternative<RunType3D>(params))
			{
				auto const& x = std::get<RunType3D>(params).second;
				if (x.rayinfo->results->ray->Amp > 1.0e-10 &&
					x.rayinfo->results->ray->NumBotBnc > -1)
				{
					ret.X = x.rayinfo->results->ray[i].x.x;
					ret.Y = x.rayinfo->results->ray[i].x.y;
					ret.Z = -x.rayinfo->results->ray[i].x.z;
				}
			}
		}
	}

	return ret;
}

/// <summary>
/// Return the source depth.
/// Creates and sets to 0 depth if it doesn't exist
/// Always safe to call.
/// </summary>
/// <param name="sourceID"></param>
/// <returns></returns>
float FbellhopModule::GetSourceDepth(const int& sourceID) {
	float ret = 0.0f;
	if (CheckSourceDepthExists(sourceID)) {
		std::visit([&](auto& x)
			{
				ret = x.first.Pos->Sz[sourceID];
			}, params);
	}
	return ret;
}

/// <summary>
/// Create or modify the source then recalculate bellhop.
/// May be strange results if something tries to access bellhop
/// data during the calcualtion.
/// Source horizontal position is not changed.
/// </summary>
/// <param name="sourceID">Which source. Creates new</param>
/// <param name="z">depth of the source (m)</param>
/// <param name="y"></param>
/// <param name="z"></param>
void FbellhopModule::SetSourceDepth(const int& sourceID, const float& z)
{
	if (CheckSourceDepthExists(sourceID)) {
		std::visit([&](auto& x)
			{
				x.first.Pos->Sz[sourceID] = z;
			}, params);
	}
}

/// <summary>
/// Return the source position.
/// Creates and sets to 0 depth if the source doesn't exist
/// Always safe to call.
/// Sets x, y components to 0 for 2d data (source is below origin)
/// </summary>
/// <param name="sourceID"></param>
/// <returns></returns>
FVector FbellhopModule::GetSourcePosition(const int& sourceID)
{
	FVector ret;

	if (!CheckSourceDepthExists(sourceID))
	{
		LogBellhop("FbellhopModule::GetSourcePosition: "
			"Could not create source. Probably bad data ... stopping.\n");
		return ret;
	}

	if (std::holds_alternative<RunType3D>(params))
	{
		auto const& x = std::get<RunType3D>(params);
		ret.X = x.first.Pos->Sx[sourceID];
		ret.Y = x.first.Pos->Sy[sourceID];
		ret.Z = x.first.Pos->Sz[sourceID];
	}
	else {
		std::visit([&](auto const& x)
			{
				ret.X = 0;
				ret.Y = 0;
				ret.Z = x.first.Pos->Sz[sourceID];
			}, params);
	}

	return ret;
}

/// <summary>
/// Create or modify the source then recalculate bellhop.
/// May be strange results if something tries to access bellhop
/// data during the calcualtion.
/// </summary>
/// <param name="sourceID">Which source. Creates new</param>
/// <param name="position">position of the source (m)</param>
void
FbellhopModule::SetSourcePosition(const int& sourceID, const FVector& position)
{
	if (!CheckSourceDepthExists(sourceID)) {
		LogBellhop("Error: could not create source. Probably bad ... continuing\n");
	}

	if (std::holds_alternative<RunType3D>(params))
	{
		auto & x = std::get<RunType3D>(params);
		x.first.Pos->Sx[sourceID] = position.X;
		x.first.Pos->Sy[sourceID] = position.Y;
		x.first.Pos->Sz[sourceID] = position.Z;
		x.first.Pos->SxSyInKm = false;
	}
	else {
		std::visit([&](auto& x)
			{
				x.first.Pos->Sz[sourceID] = position.Z;
			}, params);
	}
}

/// <summary>
/// Get the sound speed profile.
/// Only the real part.
/// </summary>
/// <returns>array of depth versus real part of sound speed</returns>
TArray<FVector2D> FbellhopModule::GetSoundSpeedProfile()
{
	TArray<FVector2D> ret;
	std::visit([&](auto& x)
		{
			for (int i = 0; i < x.first.ssp->NPts; ++i)
			{
				FVector2D insert(x.first.ssp->z[i], x.first.ssp->c[i].real());
				ret.Add(insert);
			}
		}, params);
	return ret;
}

/// <summary>
/// All the sound speed parameters.
/// Only appends to the arrays.
/// </summary>
/// <param name="Depth">meters</param>
/// <param name="SoundSpeed">meters/sec</param>
/// <param name="NumberX">may be 0</param>
/// <param name="NumberY">may be 0</param>
/// <param name="NumberZ">Number of depths</param>
void FbellhopModule::GetSoundSpeedProfile(TArray<double>& Depth,
	TArray<double>& SoundSpeed,
	int& NumberX, int& NumberY, int& NumberZ)
{
	std::visit([&](auto& x)
		{
			for (int i = 0; i < x.first.ssp->NPts; ++i)
			{
				Depth.Add(x.first.ssp->z[i]);
				SoundSpeed.Add(x.first.ssp->c[i].real());
			}
			NumberX = x.first.ssp->Nx;
			NumberY = x.first.ssp->Ny;
			NumberZ = x.first.ssp->Nz;
		}, params);
}

/// <summary>
/// Set the sound speed profile.
/// Not much checking.
/// TODO: This is probably not useful.
/// Strongly prefer calling Set1DSoundSpeedProfile then 
/// </summary>
/// <param name="soundSpeedProfile">Depth and Sound Speed</param>
void FbellhopModule::SetSoundSpeedProfile(const TArray<FVector2D>& InSoundSpeedProfile)
{
	std::visit([&](auto& x)
		{
			x.first.ssp->NPts = InSoundSpeedProfile.Num();
			x.first.ssp->Nz = InSoundSpeedProfile.Num();
			for (int i = 0; i < InSoundSpeedProfile.Num(); ++i) {
				x.first.ssp->z[i] = InSoundSpeedProfile[i].X;
				x.first.ssp->alphaR[i] = InSoundSpeedProfile[i].Y;
				x.first.ssp->alphaI[i] = 0.0;
			}
			x.first.ssp->dirty = true;
		}, params);
}

/// <summary>
/// Set sound speed profile to be 1D and use the incoming data.
/// Puts it into 1D mode (safely), but you have to make sure the top/bottom match.
/// Negative is down in Unreal coordinates.
/// Depths should be sorted, e.g., {-1, -2, -4, -8, ...}
/// </summary>
/// <param name="InSoundSpeedProfile">Depth (m) and Sound Speed (m/s)</param>
void FbellhopModule::Set1DSoundSpeedProfile(const TArray<FVector2D>& InSoundSpeedProfile)
{
	std::visit([&](auto& x)
		{
			x.first.ssp->Type = 'N';
			x.first.ssp->NPts = InSoundSpeedProfile.Num();
			x.first.ssp->Nz = InSoundSpeedProfile.Num();
			for (int i = 0; i < InSoundSpeedProfile.Num(); ++i) {
				x.first.ssp->z[i] = -InSoundSpeedProfile[i].X;
				x.first.ssp->alphaR[i] = InSoundSpeedProfile[i].Y;
				x.first.ssp->alphaI[i] = 0.0;
			}
			x.first.ssp->dirty = true;
		}, params);
}

/// <summary>
/// SSP with range dependence.
/// GridX, GridY, Depth are the sample points and
///    must be monotonically increasing.
/// SoundSpeeds goes in order x, y, z as (x*Ny+y)*Nz+z
/// </summary>
/// <param name="GridX">m</param>
/// <param name="GridY">m</param>
/// <param name="Depth">meters</param>
/// <param name="SoundSpeeds">m/s</param>
void FbellhopModule::SetHexahedralSpeedProfile(const TArray<double>& GridX,
	const TArray<double>& GridY, const TArray<double>& Depth,
	const TArray<double>& SoundSpeeds)
{
	//always 3D
	if (std::holds_alternative<RunType3D>(params))
	{
		auto& x = std::get<RunType3D>(params);
		bhc::extsetup_ssp_hexahedral(x.first, GridX.Num(), GridY.Num(), Depth.Num());
		x.first.ssp->NPts = Depth.Num();
		x.first.ssp->Nx = GridX.Num();
		x.first.ssp->Ny = GridY.Num();
		x.first.ssp->Nz = Depth.Num();
		int i = 0;
		for (int iz = 0; iz < Depth.Num(); ++iz) {
			for (int ix = 0; ix < GridX.Num(); ++ix) {
				for (int iy = 0; iy < GridY.Num(); ++iy) {
					x.first.ssp->Seg.x[ix] = GridX[ix];
					x.first.ssp->Seg.y[iy] = GridY[iy];
					x.first.ssp->Seg.z[iz] = Depth[iz];
					x.first.ssp->z[iz] = Depth[iz];
					x.first.ssp->cMat[(ix * GridY.Num() + iy) * Depth.Num() + iz] =
						//1500.0 * (iz - iy) * (iz - iy);
						//1500.0 * (iz - ix) * (iz - ix);
						SoundSpeeds[i];
					++i;
				}
			}
		}
		x.first.Bdry->Top.hs.Depth = x.first.ssp->z[0];
		x.first.Bdry->Bot.hs.Depth = x.first.ssp->z[Depth.Num() - 1];
		x.first.ssp->rangeInKm = false;
	}
	else if (std::holds_alternative<RunTypeNx2D>(params))
	{
		LogBellhop("Warning: Nx2D not supported yet ... continuing, but probably fatal.");
	}
	else {
		LogBellhop("Warning: Hexahedral speed only available in 3D ... continuing.");
	}
}

/// <summary>
/// Get the sound speed at the 3D point.
/// </summary>
/// <param name="x">Test point - rectangular coords</param>
/// <param name="sound_speed">modified to ss in m/s</param>
/// <returns>success</returns>
bool FbellhopModule::GetSoundSpeed(const FVector& Position, float& SoundSpeed)
{
	//always 3D
	if (std::holds_alternative<RunType3D>(params))
	{
		auto& x = std::get<RunType3D>(params);
		bhc::VEC23<true> pos;
		pos[0] = Position.X;
		pos[1] = Position.Y;
		pos[2] = Position.Z;
		return bhc::get_ssp<true, true>(x.first, pos, SoundSpeed);
	} else if (std::holds_alternative<RunTypeNx2D>(params))
	{
		LogBellhop("Warning: Nx2D not supported yet ... continuing, but probably fatal.");
	}
	else {
		LogBellhop("Warning: Sound speed only available in 3D ... continuing.");
	}

	return false;
}


/// <summary>
/// Sets the corners for the transmission loss profile at bearing number.
/// </summary>
/// <param name="bearing">angle number</param>
/// <param name="corners">must have room for 4 corners (modified)</param>
void FbellhopModule::GetCorners(const int& bearing, TArray<FVector>& corners)
{
	std::visit([&](auto& x)
		{
	float TopDepth = x.first.Pos->Rz[0];
	float BottomDepth = x.first.Pos->Rz[x.first.Pos->NRz_per_range-1];
	float RMin = x.first.Pos->Rr[0];
	float RMax = x.first.Pos->Rr[x.first.Pos->NRr - 1];
	float theta = FMath::DegreesToRadians(x.first.Pos->theta[bearing]);
	float TiltHack = 0.01;
	corners[0] = FVector(FMath::Cos(theta) * RMin, FMath::Sin(theta) * RMin, -TopDepth);
	corners[1] = FVector(FMath::Cos(theta+TiltHack) * RMax, FMath::Sin(theta+TiltHack) * RMax, -TopDepth);
	corners[2] = FVector(FMath::Cos(theta) * RMax, FMath::Sin(theta) * RMax, -BottomDepth);
	corners[3] = FVector(FMath::Cos(theta) * RMin, FMath::Sin(theta) * RMin, -BottomDepth);
		}, params);
}

/// <summary>
/// Set the coordinates for a cylinder of transmission loss profiles.
/// No checking.
/// </summary>
/// <param name="radial">constant radial to choose [0,Nr)</param>
/// <param name="vertices">locations of top and bottom of those radials (modified)</param>
void FbellhopModule::GetCylinder(const int& radial, TArray<FVector>& vertices)
{
	std::visit([&](auto& x)
		{
			float TopDepth = x.first.Pos->Rz[0];
	float BottomDepth = x.first.Pos->Rz[x.first.Pos->NRz_per_range - 1];
	float R = x.first.Pos->Rr[radial];
	for (int i = 0; i < x.first.Pos->Ntheta; ++i)
	{
		float theta = FMath::DegreesToRadians(x.first.Pos->theta[i]);
		float CosTheta = FMath::Cos(theta);
		float SinTheta = FMath::Sin(theta);
		vertices.Add(FVector(R * CosTheta, R * SinTheta, -TopDepth));
		vertices.Add(FVector(R * CosTheta, R * SinTheta, -BottomDepth));
	}
		}, params);
}

/// <summary>
/// Get corners of the z-slice such that the circular z-slice is just contained
/// in the quad.
/// Quad encompassing one of the stack of pancakes.
/// </summary>
/// <param name="panckake">which z slice to take (bottom is 0)</param>
/// <param name="corners">Appends the corners of a quad just containing the pancake</param>
void FbellhopModule::GetHorizontal(const int& pancake, TArray<FVector>& corners)
{
	std::visit([&](auto& x)
		{
			float Depth = x.first.Pos->Rz[pancake];
	//TODO: this may cause issues in 2D. Are radials always all the same?
	float RMin = x.first.Pos->Rr[0];
	float RMax = x.first.Pos->Rr[x.first.Pos->NRr - 1];
	corners.Add(FVector(RMin + RMax, RMin + RMax, -Depth));
	corners.Add(FVector(RMin - RMax, RMin + RMax, -Depth));
	corners.Add(FVector(RMin + RMax, RMin - RMax, -Depth));
	corners.Add(FVector(RMin - RMax, RMin - RMax, -Depth));
		}, params);
}

/// <summary>
/// Get the data for the transmission loss.
/// A fan of 2D transmission losses (depth x range at various bearings).
/// Width, Height, and Bearings are set to zero if the transmission loss is not available.
/// May call for 2D data with just one Bearing at 0.
/// Sums up the transmission loss from all sources.
/// Modifies all inputs.
/// </summary>
/// <param name="TransmissionLoss">Total TL from all sources (complex). 
/// 3D data of size Width by Height by Bearings.
/// Iterates through depth then range then bearing.
/// Access for integer depth, range, and bearing like 
///       tl[bearing*Width*Height + depth*Height + range]</param>
/// <param name="Width">texture width</param>
/// <param name="Height">texture height</param>
/// <param name="Bearing">number of bearings</param>
void FbellhopModule::GetTransmissionLoss(TArray<bhc::cpxf>& TransmissionLoss,
	int32_t& Width, int32_t& Height, int32_t& Bearings)
{
	if (bTransmissionLossUpdated) {
		TransmissionLoss = _TransmissionLoss;
		Width = _Width;
		Height = _Height;
		Bearings = _Bearings;
	}

	std::visit([&](auto& x)
		{
			Width = x.first.Pos->NRz_per_range;
	Height = x.first.Pos->NRr;
	Bearings = x.first.Pos->Ntheta;

	if (Width == 0 || x.second.uAllSources == nullptr) {
		LogBellhop("Warning: no data in the transmission loss 3D. Check the file name ... continuing.");
		return;
	}

	TransmissionLoss.SetNumZeroed(Width * Height * Bearings, true);

	for (int32_t isx = 0; isx < x.first.Pos->NSx; ++isx) {
		for (int32_t isy = 0; isy < x.first.Pos->NSy; ++isy) {
			for (int32_t itheta = 0; itheta < x.first.Pos->Ntheta; ++itheta) {
				for (int32_t isz = 0; isz < x.first.Pos->NSz; ++isz) {
					for (int32_t Irz1 = 0; Irz1 < x.first.Pos->NRz_per_range; ++Irz1) {
						for (int32_t r = 0; r < x.first.Pos->NRr; ++r) {
							size_t ind = GetFieldAddr(isx, isy, isz, itheta, Irz1, r, x.first.Pos);
							bhc::cpxf v = x.second.uAllSources[GetFieldAddr(
								isx, isy, isz, itheta, Irz1, r, x.first.Pos)];
							TransmissionLoss[itheta * Width * Height + Irz1 * Height + r] += v;
						}
					}
				}
			}
		}
	}
		}, params);
}

/// <summary>
/// For compatibility. Replace with the 3D compatible version
/// </summary>
/// <param name="TransmissionLoss"></param>
/// <param name="Width"></param>
/// <param name="Height"></param>
/// <param name="DepthFactor"></param>
/// <param name="RangeFactor"></param>
void FbellhopModule::GetTransmissionLoss(TArray<bhc::cpxf>& TransmissionLoss,
	int32_t& Width, int32_t& Height,
	double& DepthFactor, double& RangeFactor)
{
	LogBellhop("Warning: deprecated call to 2D only TL function ... continuing.");
	if (!std::holds_alternative<RunType2D>(params))
	{
		LogBellhop("Error: only 2D supported ... continuing.");
		return;
	}

	RunType2D& p = std::get<RunType2D>(params);

	Width = 0;
	Height = 0;
	DepthFactor = 0;
	RangeFactor = 0;
	if (p.second.uAllSources) {
		Width = p.first.Pos->NRz_per_range;
		Height = p.first.Pos->NRr;
		DepthFactor = p.first.Beam->Box.y / double(p.first.Pos->NRz_per_range);
		RangeFactor = p.first.Beam->Box.r / double(p.first.Pos->NRr);
		TransmissionLoss.SetNumZeroed(Width*Height, true);

		for (int32_t isrc = 0; isrc < p.first.Pos->NSz; ++isrc) {
			for (int32_t Irz1 = 0; Irz1 < p.first.Pos->NRz_per_range; ++Irz1) {
				for (int32_t r = 0; r < p.first.Pos->NRr; ++r) {
					TransmissionLoss[Irz1 * Height + r] +=
						p.second.uAllSources[
							(isrc * p.first.Pos->NRz_per_range + Irz1) * p.first.Pos->NRr + r];
				}
			}
		}
	}
}

FString FbellhopModule::GetCurrentMode()
{
	FString ret;

	std::visit([&](auto& x)
		{
			// the length is hacked in in structs.hpp
			ret.AppendChars(x.first.Beam->RunType, 7);
		}, params);

	return ret;
}


/// <summary>
/// Set ray mode from bellhop options.
/// Rerun if the type has changed.
/// May modify ray data.
/// </summary>
void FbellhopModule::SetRayMode()
{
	std::visit([&](auto& x)
		{
			MarkBellhopRun(x.first.Beam->RunType[0] == 'R');
			x.first.Beam->RunType[0] = 'R';
		}, params);
}

bool FbellhopModule::IsRayMode()
{
	bool ret = false;
	std::visit([&](auto& x)
		{
			ret = (x.first.Beam != NULL) && x.first.Beam->RunType[0] == 'R';
		},
		params);
	return ret;
}

/// <summary>
/// Transmission loss mode from bellhop.
/// Rerun if the type has changed.
/// May modify transmission loss data.
/// </summary>
/// <param name="tl">Coherent, Incoherent, or Semicoherent</param>
void FbellhopModule::SetTransmissionLossMode(const TransmissionLossMode& tl)
{
	std::visit([&](auto& x)
		{
			MarkBellhopRun(x.first.Beam->RunType[0] == static_cast<char>(tl));
			x.first.Beam->RunType[0] = static_cast<char>(tl);
		}, params);
}

/// <summary>
/// Beam type from bellhop.
/// May modify transmission loss data.
/// Geometric hat beam (Hat) is the default.
/// </summary>
/// <param name="b"></param>
void FbellhopModule::SetBeamMode(const BeamMode& b)
{
	std::visit([&](auto& x)
		{
			x.first.Beam->RunType[1] = static_cast<char>(b);
		}, params);
}

bool FbellhopModule::IsTransmissionLossMode()
{
	bool ret = false;
	std::visit([&](auto& x)
		{
			ret = (x.first.Beam != NULL) &&
			(x.first.Beam->RunType[0] == static_cast<char>(TransmissionLossMode::Coherent)
				|| x.first.Beam->RunType[0] == static_cast<char>(TransmissionLossMode::Incoherent)
				|| x.first.Beam->RunType[0] == static_cast<char>(TransmissionLossMode::SemiCoherent));
		},
		params);
	return ret;
}

/// <summary>
/// Check if the sourceID is ok and/or create more sources.
/// Note that the it will create blank sources if you send in a 
///   large number. May run out of memory.
/// </summary>
/// <param name="sourceID">array access. Will create sources less than this.</param>
/// <returns>did an error occur?</returns>
bool FbellhopModule::CheckSourceDepthExists(const int& sourceID) {
	if (!IsBellhopSetup()) {
		FString m("FbellhopModule::CheckSourceDepthExists: "
			"No bellhop data available ... ignoring");
		UE_LOG(LogTemp, Error, TEXT("%s"), *m);
		return false;
	}

	if (sourceID < 0) {
		FString m("FbellhopModule::CheckSourceDepthExists: "
			"Negative source ID entered ... ignoring");
		UE_LOG(LogTemp, Error, TEXT("%s"), *m);
		return false;
	}

	if (sourceID > 100) {
		FString m("FbellhopModule::CheckSourceDepthExists: "
			"Hacked in maximum number of sources (100) exceeded ... ignoring");
		UE_LOG(LogTemp, Error, TEXT("%s"), *m);
		return false;
	}

	int current = 0;
	std::visit([&](auto& x)
		{
			current = x.first.Pos->NSz;
		}, params);

	if (sourceID >= current) {
		std::visit([=](auto& x)
			{
				std::vector<double> oldValues;
				int oldSize = x.first.Pos->NSz;
				for (int i = 0; i < oldSize; ++i) {
					oldValues.push_back(x.first.Pos->Sz[i]);
				}
				bhc::extsetup_sz(x.first, sourceID);
				for (int i = 0; i < oldSize; ++i) {
					x.first.Pos->Sz[i] = oldValues[i];
				}
			},
			params);
	}

	if (std::holds_alternative<RunType3D>(params))
	{
		auto& x = std::get<RunType3D>(params);
		if (sourceID >= x.first.Pos->NSx) {
			std::vector<double> oldXValues, oldYValues;
			int oldXSize = x.first.Pos->NSx;
			int oldYSize = x.first.Pos->NSy;
			for (int i = 0; i < oldXSize; ++i) {
				oldXValues.push_back(x.first.Pos->Sx[i]);
			}
			for (int i = 0; i < oldYSize; ++i) {
				oldYValues.push_back(x.first.Pos->Sy[i]);
			}
			bhc::extsetup_sxsy(x.first, sourceID, sourceID);
			for (int i = 0; i < oldXSize; ++i) {
				x.first.Pos->Sx[i] = oldXValues[i];
			}
			for (int i = 0; i < oldYSize; ++i) {
				x.first.Pos->Sy[i] = oldYValues[i];
			}
		}
	}

	return true;
}

/// <summary>
/// Field order from bellhopcuda for the transmission loss array.
/// Source is specified by the first 3 indices and the output location, the last 3.
/// Note that these are indices, not the actual values. Dereference output.uAllSources
///   for the values.
/// Copied from jobs.hpp - be careful if it changes in the dll.
/// No size checking.
/// </summary>
/// <param name="isx">source x</param>
/// <param name="isy">source y</param>
/// <param name="isz">source z</param>
/// <param name="itheta">receiver theta</param>
/// <param name="id">receiver depth</param>
/// <param name="ir">receiver range</param>
/// <param name="Pos">position struct from param</param>
/// <returns>index of output.uAllSources (TL data)</returns>
std::size_t FbellhopModule::GetFieldAddr(
	int32_t isx, int32_t isy, int32_t isz, int32_t itheta, int32_t id, int32_t ir,
	const bhc::Position* Pos)
{
	return (((((size_t)isz * (size_t)Pos->NSx + (size_t)isx) * (size_t)Pos->NSy
		+ (size_t)isy)
		* (size_t)Pos->Ntheta
		+ (size_t)itheta)
		* (size_t)Pos->NRz_per_range
		+ (size_t)id)
		* (size_t)Pos->NRr
		+ (size_t)ir;
}

/// <summary>
/// Reset the bhc initializer for defaults.
/// </summary>
void FbellhopModule::ResetBellhopInitialization()
{
	BellhopInitializaiton.FileRoot = nullptr;
	BellhopInitializaiton.gpuIndex = 0;
	BellhopInitializaiton.maxMemory = 4ull * 1024ull * 1024ull * 1024ull; // 4 GiB
	BellhopInitializaiton.numThreads = -1;
	BellhopInitializaiton.outputCallback = &LogBellhopOutput;
	BellhopInitializaiton.prtCallback = &LogBellhopPRT;
	BellhopInitializaiton.useRayCopyMode = false;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FbellhopModule, bellhop)
