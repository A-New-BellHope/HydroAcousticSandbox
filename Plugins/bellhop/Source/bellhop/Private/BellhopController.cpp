// Fill out your copyright notice in the Description page of Project Settings.


#include "BellhopController.h"

// Sets default values
ABellhopController::ABellhopController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	EnvfileRunning = false;
}

// Called when the game starts or when spawned
void ABellhopController::BeginPlay()
{
	Super::BeginPlay();

	Bellhop = FModuleManager::GetModulePtr<FbellhopModule>("bellhop");

	Init();
}

// Called every frame
void ABellhopController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/// <summary>
/// Check the background file status.
/// </summary>
/// <returns>Is file writing?</returns>
bool ABellhopController::IsEnvfileRunning() const {
	return EnvfileRunning;
}

/// <summary>
/// Run the acoustic model with the current parameters.
/// </summary>
void ABellhopController::RunBellhop() const
{
	Bellhop->RunBellhop();
}

void ABellhopController::ReadFile(const FString& BellhopRoot, const bool& O3D, const bool& R3D)
{
	UE_LOG(LogTemp, Warning, TEXT("Reading file %s"), *BellhopRoot);
	FString BHRoot = BellhopRoot;

	//try the default directory
	if (!CheckFileExists(BHRoot + ".env"))
	{
		UE_LOG(LogTemp, Warning, TEXT("Input file not found ... looking in default directory ..."));
		BHRoot = Bellhop->GetBellhopEnvironmentDirectory() +
			BellhopRoot;
	}

	//read from default
	if (!CheckFileExists(BHRoot + ".env"))
	{
		UE_LOG(LogTemp, Warning, TEXT("Input file not found or in default directory ... using generic default"));
		BHRoot = Bellhop->GetBellhopEnvironmentDirectory() +
			"/san_diego";
	}

	UE_LOG(LogTemp, Warning, TEXT("Using Bellhop env file %s"), *BHRoot);
	Bellhop->recalculateRays = !Bellhop->IsRayMode();
	Bellhop->RunBellhopFile(BHRoot, O3D, R3D);
	// make sure we're in ray mode (may recalculate)
	Bellhop->SetRayMode();
}

/// <summary>
/// Setup for a run without an environment file.
/// </summary>
/// <param name="O3D"></param>
/// <param name="R3D"></param>
void  ABellhopController::SetupDefaults(const bool& O3D, const bool& R3D)
{
	Bellhop->SetupDefaults(O3D, R3D);
}


/// <summary>
/// Returns the directory path of Bellhop
/// </summary>
/// <returns name="Bellhop Directory">string directory path</returns>
FString ABellhopController::GetPluginDefaultDirectory()
{
	return Bellhop->GetBellhopDirectory();
}

/// <summary>
/// Get the number of points given a specific ray ID
/// </summary>
/// <param name="InRayID">The id of the ray</param>
/// <returns>Number of points in given ray</returns>
int ABellhopController::GetNPoints(const int& InRayID) const
{
	return Bellhop->GetNPoints(InRayID);
}

/// <summary>
/// Get the total number of rays
/// </summary>
/// <returns>Total number of rays</returns>
int ABellhopController::GetNRays() const
{
	return Bellhop->GetNRays();
}
/// <summary>
/// Move a single point on the sound speed profile.
/// Messages the user if the point doesn't exist.
/// </summary>
/// <param name="Depth">index of the depth, not the value</param>
/// <param name="SoundSpeed">Speed at that depth (units?)</param>
void ABellhopController::AdjustSoundSpeedProfile(const int& Depth, const float& SoundSpeed)
{
	TArray<FVector2D> SSP = Bellhop->GetSoundSpeedProfile();
	if (Depth >= 0 && Depth < SSP.Num())
	{
		SSP[Depth].Y = SoundSpeed;
		Bellhop->SetSoundSpeedProfile(SSP);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to set depth at index"));
	}
}
/// <summary>
/// Really just testing only, set the sound speed to be a constant value (1500)
/// </summary>
void ABellhopController::TestingFlattenSoundSpeedProfile()
{
	TArray<FVector2D> SSP = Bellhop->GetSoundSpeedProfile();
	for (int i = 0; i < SSP.Num(); ++i) {
		SSP[i].Y = 1500.0f;
	}
	Bellhop->SetSoundSpeedProfile(SSP);
}
/// <summary>
/// Move the specified source.
/// Will create course if it's not there.
/// </summary>
/// <param name="Source"></param>
/// <param name="X"></param>
void ABellhopController::MoveSource(const int& Source, const float& X)
{
	UE_LOG(LogTemp, Warning, TEXT("Moving source ..."));
	Bellhop->SetSourceDepth(Source, Bellhop->GetSourceDepth(Source) + X);
	UE_LOG(LogTemp, Warning, TEXT("Done moving source."));
}

/// <summary>
/// Move the specified source.
/// Will create source if it's not there.
/// </summary>
/// <param name="Source"></param>
/// <param name="position"></param>
void ABellhopController::MoveSource3D(const int& Source, const FVector& position)
{
	//UE_LOG(LogTemp, Warning, TEXT("Disabled moving 3D source for testing ..."));
	UE_LOG(LogTemp, Warning, TEXT("Moving 3D source ..."));
	Bellhop->SetSourcePosition(Source, FVector(position.X, position.Y, FMath::Abs(position.Z)));
	UE_LOG(LogTemp, Warning, TEXT("Done moving 3D source."));
}

/// <summary>
/// Read the ray azimuths.
/// 0 is east, -90 is north (bellhop convention). 
/// Not needed in 2d, but ok to call (will warn).
/// </summary>
/// <param name="low">degrees south of east</param>
/// <param name="high">degrees south of east</param>
/// <param name="n">numebr of samples</param>
void ABellhopController::GetRayAzimuths(float& low, float& high, int& n) const
{
	Bellhop->GetRayAzimuths(low, high, n);
}

/// <summary>
/// Read the ray altitudes.
/// 0 is horizontal, -90 is up, 90 is down (bellhop convention). 
/// </summary>
/// <param name="low">degrees below horizontal</param>
/// <param name="high">degrees below horizontal</param>
/// <param name="n">numebr of samples</param>
void ABellhopController::GetRayAltitudes(float& low, float& high, int& n) const
{
	Bellhop->GetRayAltitudes(low, high, n);
}

/// <summary>
/// Set the ray azimuths.
/// 0 is east, -90 is north (bellhop convention). 
/// Not needed in 2d, but ok to call (will warn).
/// </summary>
/// <param name="low">degrees south of east</param>
/// <param name="high">degrees south of east</param>
/// <param name="n">numebr of samples</param>
void ABellhopController::SetRayAzimuths(const float& low, const float& high, const int& n)
{
	Bellhop->SetRayAzimuths(low, high, n);
}

/// <summary>
/// Set the ray altitudes.
/// 0 is horizontal, -90 is up, 90 is down (bellhop convention). 
/// </summary>
/// <param name="low">degrees below horizontal</param>
/// <param name="high">degrees below horizontal</param>
/// <param name="n">numebr of samples</param>
void ABellhopController::SetRayAltitudes(const float& low, const float& high, const int& n)
{
	Bellhop->SetRayAltitudes(low, high, n);
}
/// <summary>
/// Return the max time, may be -1 if it is not valid.
/// </summary>
/// <returns>max times in seconds</returns>
float ABellhopController::GetMaxArrivalTime()
{
	if (Bellhop->GetMaxArrivalTime() < 1e-6) {
		UE_LOG(LogTemp, Warning, TEXT("Warning: hackish update of arrival times.\n"));
		for (int i = 0; i < Bellhop->GetNRays(); ++i)
			Bellhop->GetAllRayPointArrival(i);
	}
	return (float)Bellhop->GetMaxArrivalTime();
}
/// <summary>
/// Source location in world coordinates.
/// No Checking.
/// </summary>
/// <param name="SourceID">source number, 0 for one source</param>
/// <returns>World coordinate</returns>
FVector ABellhopController::GetSourceLocation(const int& SourceID)
{
	return Bellhop->GetSourcePosition(SourceID);
}

/// <summary>
/// Return the sound speed profile formatted for Unreal.
/// All input vectors are modified.
/// </summary>
/// <param name="Depth">meters</param>
/// <param name="X">meters, may be empty</param>
/// <param name="Y">meters, may be empty</param>
/// <param name="SoundSpeed">meters/second</param>
/// <returns>success</returns>
bool ABellhopController::GetSoundSpeedProfile(TArray<double>& Depth,
	TArray<double>& SoundSpeed,
	int& NumberX, int& NumberY, int& NumberDepth) const
{
	//TODO: other failure modes?
	if (!Bellhop->IsBellhopReady()) { return false; }

	Depth.Empty();
	SoundSpeed.Empty();
	Bellhop->GetSoundSpeedProfile(Depth, SoundSpeed, NumberX, NumberY, NumberDepth);

	return true;
}

/// <summary>
/// Set a single sound speed. Disables any multiple ssp mode.
/// TODO: probably bad if it changes modes.
/// </summary>
/// <param name="Depth"></param>
/// <param name="SoundSpeed"></param>
/// <returns>success</returns>
bool ABellhopController::SetSingleSoundSpeed(const TArray<double>& Depth,
	const TArray<double>& SoundSpeed)
{
	if (Depth.Num() != SoundSpeed.Num()) {
		UE_LOG(LogTemp, Warning, TEXT("Warning: Depth and sound speed counts don't match ... continuing without changes\n"));
		return false;
	}

	if (Depth.Num() < 2) {
		UE_LOG(LogTemp, Warning, TEXT("Warning: need at least two depth/sound speed pairs ... continuing without changes\n"));
		return false;
	}

	TArray<FVector2D> SSP;
	for(int i = 0; i < Depth.Num(); ++i) {
		SSP.Add({ Depth[i], SoundSpeed[i] });
	}
	Bellhop->SetSoundSpeedProfile(SSP);

	return true;
}

/// <summary>
/// Set hexahedral sound speed.
/// Intended for HYCOM data from netcdf.
/// </summary>
/// <param name="GridX">m</param>
/// <param name="GridY">m</param>
/// <param name="Depth">meters</param>
/// <param name="SoundSpeed">m/s</param>
/// <returns>success</returns>
bool ABellhopController::SetHexahedralSoundSpeed(
	const TArray<double>& GridX, const TArray<double>& GridY,
	const TArray<double>& Depth, const TArray<double>& SoundSpeed)
{
	Bellhop->SetHexahedralSpeedProfile(GridX, GridY, Depth, SoundSpeed);
	return true;
}


/// <summary>
/// Get the sound speed at a single point.
/// 3D only
/// </summary>
/// <param name="Position">rectangular coord</param>
/// <param name="SoundSpeed">m/s</param>
/// <returns>success</returns>
bool ABellhopController::GetSingleSoundSpeed(const FVector& Position, float& SoundSpeed)
{
	return Bellhop->GetSoundSpeed(Position, SoundSpeed);
}

void ABellhopController::SetRayMode()
{
	Bellhop->SetRayMode();
}

/// <summary>
/// Check that the source location is inside the SSP
///   and over the bathymetry.
/// </summary>
/// <param name="Location"></param>
/// <returns>ok</returns>
bool ABellhopController::CheckSource(const FVector& Location)
{
	return true;
}

/// <summary>
/// Write the env files and any supporting files for external bellhop.
/// TODO: should have more checking, but relying on the directory picker.
/// TODO: this writes on the background, but there will be problems if
///    someone tries to access Bellhop during the write.
/// </summary>
/// <param name="BaseName">usually short, e.g. Munk</param>
/// <param name="Directory">directory to write.</param>
void ABellhopController::WriteBellhopEnvironment(FString BaseName,
	FString Directory)
{
	if (IsEnvfileRunning()) {
		UE_LOG(LogTemp, Warning, TEXT("Already saving file ... returning"));
		return;
	}
	EnvfileRunning = true;

	if (Directory.IsEmpty()) {
		UE_LOG(LogTemp, Warning, TEXT("Empty save directory. Will save in c:\\bellhop\\."));
		Directory += TEXT("c:/bellhop/");
	}

	if (BaseName.IsEmpty()) {
		UE_LOG(LogTemp, Warning, TEXT("Empty base name. Defaulting to temp"));
		BaseName += TEXT("temp");
	}

	EnvFileName = Directory + BaseName;

	auto envWriteTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [&]
		{
			UE_LOG(LogTemp, Warning, TEXT("Starting env write to %s"), *EnvFileName);
			Bellhop->WriteEnvironment(EnvFileName);
			UE_LOG(LogTemp, Warning, TEXT("Done env write"));
			EnvfileRunning = false;
			OnEnvfileDoneEvent.Broadcast();
		}
	);
}

/// <summary>
/// Any extra initialization.
/// </summary>
void ABellhopController::Init()
{}

/// <summary>
/// Check if there's a file at FileName.
/// Could be put in a general utility file, if we ever make one.
/// </summary>
/// <param name="FileName">a filename</param>
/// <returns>file exists</returns>
bool ABellhopController::CheckFileExists(const FString & FileName)
{
	return FPlatformFileManager::Get()
		.GetPlatformFile()
		.FileExists(*FileName);
}

