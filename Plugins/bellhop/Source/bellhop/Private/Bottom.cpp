// Fill out your copyright notice in the Description page of Project Settings.


#include "Bottom.h"

// Sets default values
ABottom::ABottom()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABottom::BeginPlay()
{
	Super::BeginPlay();
	
	Bellhop = FModuleManager::GetModulePtr<FbellhopModule>("bellhop");
}

// Called every frame
void ABottom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/// <summary>
/// Read the data from Bellhop.
/// TODO: only really need the bottom mesh.
/// TODO: Should really wrap this (code copied).
/// </summary>
/// <param name="BellhopRoot"></param>
/// <param name="O3D"></param>
/// <param name="R3D"></param>
void ABottom::ReadFile(const FString& BellhopRoot,
	const bool& O3D = true, const bool& R3D = true)
{
	UE_LOG(LogTemp, Warning, TEXT("ABottom: Reading file %s"), *BellhopRoot);
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
	UE_LOG(LogTemp, Warning, TEXT("Reading file %hs"), TCHAR_TO_ANSI(*BellhopRoot));
	Bellhop->RunBellhopFile(BHRoot, O3D, R3D);
	bBottomDirty = true;
}

/// <summary>
/// Get the bottom vertices formatted for vertices in Unreal's
///    ProceduralMesh library.
/// Intended to be vertices for BluePrint to CreateGridMesh*
/// </summary>
/// <returns>bottom vertices in order (p1, p2, ...)
/// May be empty.</returns>
TArray<FVector> ABottom::GetBottomMeshVertices()
{
	UpdateBottomVertices();
	return BottomVertices;
}

/// <summary>
/// Get the UV values for sampling a height map texture.
/// </summary>
/// <returns>UV array for CreateMesh</returns>
TArray<FVector2d> ABottom::GetHeightMapUV()
{
	UpdateHeightMapUV();
	return HeightMapUV;
}

/// <summary>
/// Low and High bounds of the bottom mesh.
/// Most likely useful for creating a surface.
/// </summary>
/// <param name="Low"></param>
/// <param name="High"></param>
void ABottom::GetBottomBoundary(FVector& Low, FVector& High)
{
	UpdateBottomVertices();

	if (BottomVertices.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("No bottom data ... continuing"));
	}

	Low = BottomVertices[0];
	High = BottomVertices.Last();
}

/// <summary>
/// Number of points in x direction of the grid.
/// Intended for use with CreateGridMesh* blueprints.
/// </summary>
/// <returns>count</returns>
int ABottom::GetNPointsX()
{
	int ret = 0;
	if (Bellhop->IsBellhopReady())
	{
		ret = Bellhop->GetBoundarySizeX();
	}
	return ret;
}

/// <summary>
/// Number of points in y direction of the grid.
/// Intended for use with CreateGridMesh* blueprints.
/// Safe to call in any dimension and just returns 0 in 2d.
/// </summary>
/// <returns>count</returns>
int ABottom::GetNPointsY()
{
	int ret = 0;
	if (Bellhop->IsBellhopReady())
	{
		ret = Bellhop->GetBoundarySizeY();
	}
	return ret;
}

/// <summary>
/// Check if there's a file at FileName.
/// Could be put in a general utility file, if we ever make one.
/// </summary>
/// <param name="FileName">a filename</param>
/// <returns>file exists</returns>
bool
ABottom::CheckFileExists(const FString& FileName)
{
	return FPlatformFileManager::Get()
		.GetPlatformFile()
		.FileExists(*FileName);
}

/// <summary>
/// Update the bottom vertices if needed.
/// Should be called after the dll has some bottom data.
/// </summary>
void ABottom::UpdateBottomVertices()
{
	if (!bBottomDirty)
	{
		return;
	}

	BottomVertices = Bellhop->GetBoundaryPoints();
	UE_LOG(LogTemp, Warning, TEXT("Mesh Z range: Min=%.2f Max=%.2f"), ZMin, ZMax);
	bBottomDirty = false;
}

/// <summary>
/// Return the shallowest depth (or land height) in depth units.
/// Positive down. Above the sea surface is negative.
/// </summary>
/// <returns>Shallowest depth</returns>
float ABottom::GetZMin() const
{
	return -ZMin;
}

/// <summary>
/// Return the deepest depth (or land height) in depth units.
/// Positive down. Above the sea surface is negative.
/// </summary>
/// <returns>Deepest depth</returns>
float ABottom::GetZMax() const
{
	return -ZMax;
}

/// <summary>
/// Send bottom data to bellhop and update.
/// The grid should be regularly spaced.
/// </summary>
/// <param name="Depths">meters</param>
/// <param name="GridX">meters</param>
/// <param name="GridY">meters, not used in 2D</param>
/// <param name="O3D">ocean 3D</param>
/// <returns>success</returns>
bool ABottom::UpdateBottom(const TArray<double>& Depths,
	const TArray<double>& GridX, const TArray<double>& GridY,
	const bool& O3D)
{
	UE_LOG(LogTemp, Warning, TEXT("TIFF -> Bellhop: GridX=%d GridY=%d Depth=%d"),
		GridX.Num(), GridY.Num(), Depths.Num());
	if (O3D) {
		Bellhop->SetBottom(Depths, GridX, GridY, true);
	}
	else {
		Bellhop->SetBottom(Depths, GridX);
	}

	bBottomDirty = true;
	return true;
}

/// <summary>
/// Calculate UV linearly spread between the min/max height.
/// U is always 0.5, V varies (vertical gradient will work).
/// Coded so that V in (0, 0.5) maps to above surface,
///   and (0.5,1) maps to below the surface (ocean colors).
/// Will check the bottom vertices.
/// </summary>
void ABottom::UpdateHeightMapUV()
{
	UpdateBottomVertices();

	if (BottomVertices.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("No bottom data to make UV ... continuing"));
		return;
	}

	// TODO - might get slow, just want robust max,min
	TArray<double> depths;
	for (auto const& v : BottomVertices)
	{
		depths.Add(v.Z);
	}
	depths.Sort();

	int SkipExtreme = 10;
	int N = depths.Num();
	ZMin = depths[ (SkipExtreme < N) ? SkipExtreme : 0];
	ZMax = depths[(SkipExtreme < N) ? N - SkipExtreme - 1 : N - 1];

	if (FMath::IsNearlyEqual(ZMin, ZMax) || ZMin > ZMax)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bottom data is flat so assigning U,V=0.5,0.5 ... continuing"));
		ZMin = 0.5;
		ZMax = 0.5;
	}

	HeightMapUV.Empty(BottomVertices.Num());
	HeightMapUV.SetNum(BottomVertices.Num());
	for (int i = 0; i < BottomVertices.Num(); ++i)
	{
		double v = FMath::Clamp(BottomVertices[i].Z, ZMin, ZMax);
		if (v > 0)
		{
			HeightMapUV[i] = FVector2D(0.5,
				FMath::Clamp(0.5 * (1.0 - v / ZMax), 0.01, 0.5));
		}
		else
		{
			HeightMapUV[i] = FVector2D(0.5,
				FMath::Clamp(0.5 * (1.0 + v / ZMin), 0.5, 0.99));
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Mesh Z range: Min=%.2f Max=%.2f"), ZMin, ZMax);
}
