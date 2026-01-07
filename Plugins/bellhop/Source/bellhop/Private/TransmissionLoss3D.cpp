// Fill out your copyright notice in the Description page of Project Settings.


#include "TransmissionLoss3D.h"

// Sets default values
ATransmissionLoss3D::ATransmissionLoss3D()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ColormapNames["Parula"] = tinycolormap::ColormapType::Parula;
	ColormapNames["Heat"] = tinycolormap::ColormapType::Heat;
	ColormapNames["Jet"] = tinycolormap::ColormapType::Jet;
	ColormapNames["Turbo"] = tinycolormap::ColormapType::Turbo;
	ColormapNames["Hot"] = tinycolormap::ColormapType::Hot;
	ColormapNames["Gray"] = tinycolormap::ColormapType::Gray;
	ColormapNames["Magma"] = tinycolormap::ColormapType::Magma;
	ColormapNames["Inferno"] = tinycolormap::ColormapType::Inferno;
	ColormapNames["Plasma"] = tinycolormap::ColormapType::Plasma;
	ColormapNames["Viridis"] = tinycolormap::ColormapType::Viridis;
	ColormapNames["Cividis"] = tinycolormap::ColormapType::Cividis;
	ColormapNames["Github"] = tinycolormap::ColormapType::Github;
	ColormapNames["Cubehelix"] = tinycolormap::ColormapType::Cubehelix;

}

// Called when the game starts or when spawned
void ATransmissionLoss3D::BeginPlay()
{
	Super::BeginPlay();

	Bellhop = FModuleManager::GetModulePtr<FbellhopModule>("bellhop");	
}

// Called every frame
void ATransmissionLoss3D::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATransmissionLoss3D::UpdateTransmissionLoss()
{
	Bellhop->GetTransmissionLoss(TransmissionLoss, AllWidth, AllHeight, AllBearings);
	Width = AllWidth.Num();
	Height = AllHeight.Num();
	Bearings = AllBearings.Num();

	UpdateLimits();
}

/// <summary>
/// Update the min and max over all the transmission loss data.
/// </summary>
void ATransmissionLoss3D::UpdateLimits()
{
	if (Width * Height * Bearings < 1) {
		UE_LOG(LogTemp, Warning, TEXT("Warning: Attempted to update limits with no TL data ... ignoring"));
		return;
	}

	MinValue = ToReal(TransmissionLoss[0]);
	MaxValue = ToReal(TransmissionLoss[0]);
	for (int i = 0; i < Width * Height * Bearings; ++i)
	{
		float v = ToReal(TransmissionLoss[i]);
		MinValue = std::min(MinValue, v);
		MaxValue = std::max(MaxValue, v);
	}

	UE_LOG(LogTemp, Warning, TEXT("Found TL range %g %g"), MinValue, MaxValue);
}

/// <summary>
/// The number of bearings.
/// Returns 0 if there's no bellhop model run.
/// </summary>
/// <returns>number of bearings for a loop</returns>
int ATransmissionLoss3D::GetNumBearings()
{
	return Bearings;
}

/// <summary>
/// The width of each sub texture
/// Returns 0 if there's no bellhop model run.
/// </summary>
/// <returns>width</returns>
int ATransmissionLoss3D::GetWidth()
{
	return Width;
}

/// <summary>
/// The height of each sub texture.
/// Returns 0 if there's no bellhop model run.
/// </summary>
/// <returns>height</returns>
int ATransmissionLoss3D::GetHeight()
{
	return Height;
}

/// <summary>
/// Return the source position.
/// Not well behaved if the source doesn't exist.
/// </summary>
/// <param name="SourceID">flips to same coordinates as mesh functions</param>
/// <returns></returns>
FVector ATransmissionLoss3D::GetSourcePosition(const int& SourceID) const {
	FVector ret = Bellhop->GetSourcePosition(SourceID);
	ret[2] *= -1.0f;
	return ret;
}

/// <summary>
/// Get the real space corners of the transmission loss texture.
/// Intended to replace the vector parameter in a 2x2 SplitMesh,
/// with the same triangle and UV.
/// Must be called after UpdateTransmissionLoss.
/// </summary>
/// <param name="Bearing">bearing number</param>
/// <returns>Four corners</returns>
TArray<FVector> ATransmissionLoss3D::GetCorners(const int& Bearing)
{
	TArray<FVector> ret;
	ret.SetNumZeroed(4);

	Bellhop->GetCorners(Bearing, ret);

	return ret;
}

/// <summary>
/// Get the real space vertices of a cylinder project of the transmission loss.
/// Intended to replace the vector parameter in 2xBearings SplitMeshWelded,
/// with the same trangle and UV assignment.
/// Easier than doing the logic here.
/// Must be called after UpdateTransmissionLoss.
/// </summary>
/// <param name="Radial">radial number</param>
/// <returns>2xBearings vertices in order for a cylinder from SplitMesh</returns>
TArray<FVector> ATransmissionLoss3D::GetCylinder(const int& Radial)
{
	TArray<FVector> ret;
	Bellhop->GetCylinder(Radial, ret);
	return ret;
}

/// <summary>
/// Get the real space vertices of a quad parallel to horizontal pancakes (constant z).
/// Intended to replace the vector parameter in 2xBearings (a quad) SplitMesh, with the 
/// same triangle and UV assignment, to skip doing the UV logic here.
/// Must be called after UpdateTransmissionLoss.
/// </summary>
/// <param name="Pancake">the z-slice to take</param>
/// <returns>quad corners</returns>
TArray<FVector> ATransmissionLoss3D::GetHorizontal(const int& Pancake)
{
	TArray<FVector> ret;
	Bellhop->GetHorizontal(Pancake, ret);
	return ret;
}

/// <summary>
/// Set the material to use a dynamic texture of the data at Bearing.
/// </summary>
/// <param name="Material">modified</param>
/// <param name="Bearing">[0,Bearings)</param>
void ATransmissionLoss3D::UpdateTextureBearing(UMaterialInstanceDynamic* Material, const int& Bearing)
{
	if (Width == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No transmission loss data available from Bellhop. Check the parameters."));
		return;
	}

	int offset = Bearing * Width * Height;

	TArray<unsigned int> Pixels;
	for (int i = 0; i < Width*Height; ++i) {
		Pixels.Add(ColorMap(ToReal(TransmissionLoss[i+offset]), MinValue, MaxValue));
	}

	UTexture2D* tex = CreateTexture(Pixels, Width, Height);
	if (Material)
	{
		Material->SetTextureParameterValue(FName("TransmissionLoss"), tex);
	}

}

/// <summary>
/// Set material to use dynamic texture at constant radial.
/// </summary>
/// <param name="Material">modified</param>
/// <param name="Radial">[0,Width)</param>
void ATransmissionLoss3D::
UpdateTextureCylinder(UMaterialInstanceDynamic* Material, const int& Radial)
{
	if (Width == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No transmission loss data available from Bellhop. Check the parameters."));
		return;
	}

	TArray<unsigned int> Pixels;
	for (int i = 0; i < Bearings; ++i) {
		for (int j = 0; j < Width; ++j) {
			int ind = i * Width * Height + j * Height + Radial;
			Pixels.Add(ColorMap(ToReal(TransmissionLoss[ind]), MinValue, MaxValue));
		}
	}

	UTexture2D* tex = CreateTexture(Pixels, Bearings, Width);
	if (Material)
	{
		Material->SetTextureParameterValue(FName("TransmissionLoss"), tex);
		UE_LOG(LogTemp, Warning, TEXT("updated texture"));
	}
}

/// <summary>
/// Set material to use dynamic texture at constant height.
/// Radials are stacked here and projected using UV in the resulting texture.
/// </summary>
/// <param name="Material">modified</param>
/// <param name="TextureHeight">[0,Width)</param>
void ATransmissionLoss3D::
UpdateTextureHorizontal(UMaterialInstanceDynamic* Material, const int& Horizontal)
{
	if (Width == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No transmission loss data available from Bellhop. Check the parameters."));
		return;
	}

	TArray<unsigned int> Pixels;
	for (int i = 0; i < Bearings; ++i) {
		for (int j = 0; j < Height; ++j) {
			int ind = i * Width * Height + Horizontal * Height + j;
			Pixels.Add(ColorMap(ToReal(TransmissionLoss[ind]), MinValue, MaxValue));
		}
	}

	UTexture2D* tex = CreateTexture(Pixels, Height, Bearings);
	if (Material)
	{
		Material->SetTextureParameterValue(FName("TransmissionLoss"), tex);
		UE_LOG(LogTemp, Warning, TEXT("updated texture"));
	}
}

UTexture2D*
ATransmissionLoss3D::CreateTexture(const TArray<unsigned int>& Data,
	const int& TexWidth, const int& TexHeight) const
{
	UTexture2D* Texture;
	Texture = UTexture2D::CreateTransient(TexHeight, TexWidth, PF_R8G8B8A8);
	if (!Texture) return nullptr;
#if WITH_EDITORONLY_DATA
	Texture->MipGenSettings = TMGS_NoMipmaps;
#endif
	Texture->NeverStream = true;
	Texture->SRGB = 0;
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(RawData, Data.GetData(), TexWidth * TexHeight * 4);
	Mip.BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

/// <summary>
/// Map from complex TL to float (e.g. decibels)
/// </summary>
/// <param name="Value">value to convert</param>
/// <returns>float to render</returns>
float
ATransmissionLoss3D::ToReal(const bhc::cpxf& x) const
{
	float minValue = 1.0e-6f;
	return 20.0*log10(abs(x) > minValue ? abs(x) : minValue);
}

/// <summary>
/// Set the color map. Format is 0x00BBGGRR where RR, GG, and BB
/// are hex values (0x00 to 0xff).
/// TODO: make this changeable. Why is alpha not working?
/// </summary>
/// <param name="Value">input color</param>
/// <param name="Min">interpolate on this range</param>
/// <param name="Max"></param>
/// <returns>ABGR encoded color</returns>
unsigned int
ATransmissionLoss3D::ColorMap(const float& Value, const float& Min, const float& Max) const
{
	//find is safe as long as Colormap is only changed with Get and Set
	auto c = tinycolormap::GetColor((Value - Min) / (Max - Min),
		ColormapNames.find(Colormap)->second);
	unsigned int blue = c.bi();
	unsigned int red = c.ri();
	unsigned int green = c.gi();
	unsigned int alpha = 200;
	return (alpha << 24) | (blue << 16) | (green << 8) | red;
}

/// <summary>
/// Check if there's a file at FileName.
/// Could be put in a general utility file, if we ever make one.
/// </summary>
/// <param name="FileName">a filename</param>
/// <returns>file exists</returns>
bool 
ATransmissionLoss3D::CheckFileExists(const FString& FileName)
{
	return FPlatformFileManager::Get()
		.GetPlatformFile()
		.FileExists(*FileName);
}

